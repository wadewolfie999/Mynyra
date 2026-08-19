#include "ExecutionEngine.hpp"
#include "AnalyticsEngine.hpp"
#include "BrokerGateway.hpp"
#include "L2OrderBook.hpp"
#include "SmaCrossStrategy.hpp"
#include "TriggerOrderManager.hpp"
#include "MarketCandle.hpp"
#include <chrono>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <stdexcept>

ExecutionEngine::ExecutionEngine(PortfolioManager& portfolio,
                                 RiskEngine&       riskEngine,
                                 std::string       symbol,
                                 double            feeRate,
                                 double            slippageBps,
                                 double            riskPct)
    : m_portfolio(portfolio)
    , m_riskEngine(riskEngine)
    , m_symbol(std::move(symbol))
    , m_riskPct(riskPct)
{
    const auto normalizedFeeRate = Financial::fraction(feeRate);
    const auto normalizedSlippage = Financial::fraction(slippageBps / 10'000.0);
    if (!normalizedFeeRate.has_value() || normalizedFeeRate->isNegative()
        || normalizedFeeRate->units >= Financial::SCALE_FACTOR
        || !normalizedSlippage.has_value() || normalizedSlippage->isNegative()
        || normalizedSlippage->units >= Financial::SCALE_FACTOR
        || !std::isfinite(riskPct) || riskPct < 0.0) {
        throw std::invalid_argument("ExecutionEngine: invalid financial configuration");
    }
    m_feeRate = *normalizedFeeRate;
    m_slippage = *normalizedSlippage;
}

void ExecutionEngine::setAnalyticsEngine(AnalyticsEngine* analytics) noexcept
{
    m_analytics = analytics;
}

void ExecutionEngine::setStrategy(SmaCrossStrategy* strategy) noexcept
{
    m_strategy = strategy;
}

void ExecutionEngine::bindBrokerGateway(BrokerGateway* gateway) noexcept
{
    m_gateway = gateway;
    if (!m_gateway) { return; }
    (void)m_gateway->setPaperSimulationCosts(
        m_feeRate.toDouble(), m_slippage.toDouble() * 10'000.0);
    m_gateway->addExecutionCallback([this](const ExecutionEvent& execution) {
        onBrokerExecution(execution);
    });
}

bool ExecutionEngine::dispatchBrokerOrder(
    bool isBuy, Financial::Quantity quantity, Financial::Price requestedPrice,
    uint64_t timestamp, const std::string& strategyId)
{
    const auto signalNs = std::chrono::steady_clock::now();
    if (!m_gateway || !m_gateway->isConnected()) {
        ++m_blockedCount;
        return false;
    }

    OrderRequest request;
    request.localOrderId = m_nextLocalOrderId.fetch_add(1);
    request.canonicalSymbol = m_symbol;
    request.side = isBuy ? OrderSide::Buy : OrderSide::Sell;
    request.type = BrokerOrderType::Market;
    request.quantity = Decimal64{quantity.units, Financial::SCALE};
    request.referencePrice = Decimal64{requestedPrice.units, Financial::SCALE};
    request.sourceId = strategyId;
    request.timestampNs = timestamp;
    request.sequence = 1;
    request.idempotencyKey = "execution-" + std::to_string(request.localOrderId);

    std::string rejectionReason;
    const auto normalized = m_gateway->normalizeOrder(request, &rejectionReason);
    if (!normalized.has_value()) {
        ++m_blockedCount;
        return false;
    }

    RiskDecision finalRisk;
    finalRisk.riskIncreasing = isBuy;
    finalRisk.allowed = !isBuy || m_riskEngine.canTrade();
    if (!finalRisk.allowed) {
        finalRisk.failure = FailureCategory::Validation;
        finalRisk.action = RuleBreachAction::Halt;
        finalRisk.reason = "RiskEngine rejected risk-increasing entry";
        ++m_blockedCount;
    }

    const auto elapsed = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - signalNs).count();
    m_lastRouteLatencyMs.store(elapsed);
    double observedMax = m_maxRouteLatencyMs.load();
    while (elapsed > observedMax
           && !m_maxRouteLatencyMs.compare_exchange_weak(observedMax, elapsed)) {}
    if (elapsed > 5.0) {
        m_latencyBreaches.fetch_add(1);
        m_riskEngine.reportLatency(static_cast<uint32_t>(std::ceil(elapsed)));
    }

    const auto dispatch = m_gateway->dispatchOrder(*normalized, finalRisk);
    if (!dispatch.dispatched) {
        if (finalRisk.allowed && dispatch.failure != FailureCategory::None) {
            m_riskEngine.reportApiError();
            ++m_blockedCount;
        }
        return false;
    }

    m_lastBrokerOrderId.store(dispatch.localOrderId);
    return true;
}

bool ExecutionEngine::execute(Signal signal, double marketPrice, uint64_t timestamp,
                              const std::string& strategyId)
{
    if (signal == Signal::NONE) {
        return false;
    }

    ++m_signalCount;

    const auto normalizedMarketPrice = Financial::price(marketPrice);
    if (!normalizedMarketPrice.has_value() || !normalizedMarketPrice->isPositive()) {
        ++m_blockedCount;
        return false;
    }

    // Once a gateway is bound it is the only execution boundary. Never turn
    // transport/readiness failure into a local fill that could be mistaken for
    // externally executed state.
    if (m_gateway && !m_gateway->isConnected()) {
        ++m_blockedCount;
        std::cout << "[EXECUTION] " << (signal == Signal::BUY ? "BUY" : "SELL")
                  << " blocked: bound BrokerGateway is unavailable\n";
        return false;
    }

    if (signal == Signal::BUY) {
        if (m_portfolio.hasPosition(m_symbol)) {
            return false;
        }

        const auto fillPrice = Financial::applySlippage(
            *normalizedMarketPrice, m_slippage, true);
        const auto availableCash = Financial::money(
            m_portfolio.getCashBalance(), Financial::Rounding::RejectUnaligned);
        if (!fillPrice.has_value() || !availableCash.has_value()
            || !availableCash->isPositive()) {
            ++m_blockedCount;
            return false;
        }

        // ── Dynamic ATR-based position sizing ────────────────────────────
        const std::size_t maxPos  = m_riskEngine.getMaxConcurrentPositions();
        const std::size_t openPos = m_portfolio.getOpenPositionCount();

        Financial::Money totalBudget = *availableCash;
        Financial::Quantity units{};

        if (m_strategy != nullptr && m_strategy->isATRValid()
            && m_strategy->getATR() > 0.0)
        {
            // ATR parity sizing: Units = (E_total * riskPct) / ATR(t)
            const double equity = m_portfolio.getTotalEquity();
            const double atr    = m_strategy->getATR();
            const auto desired = Financial::quantity(
                (equity * m_riskPct) / atr, Financial::Rounding::TowardZero);
            if (!desired.has_value() || !desired->isPositive()) {
                ++m_blockedCount;
                return false;
            }
            units = *desired;
            std::cout << "[SIZE]  ATR=" << std::fixed << std::setprecision(4) << atr
                      << " | Units=" << std::setprecision(4) << units.toDouble()
                      << "\n";
        } else {
            // Fallback: equal-slice capital allocation
            const std::size_t remaining = (maxPos > openPos) ? (maxPos - openPos) : 1;
            totalBudget = (maxPos == 0)
                ? *availableCash
                : Financial::Money{
                    availableCash->units / static_cast<std::int64_t>(remaining)};
            const auto notionalBudget = Financial::notionalBeforeFee(
                totalBudget, m_feeRate);
            const auto affordable = notionalBudget.has_value()
                ? Financial::quantityForNotional(*notionalBudget, *fillPrice)
                : std::nullopt;
            if (!affordable.has_value() || !affordable->isPositive()) {
                ++m_blockedCount;
                return false;
            }
            units = *affordable;
        }

        auto cost = Financial::notional(*fillPrice, units);
        auto executionFee = cost.has_value()
            ? Financial::fee(*cost, m_feeRate) : std::nullopt;
        auto debit = (cost.has_value() && executionFee.has_value())
            ? Financial::add(*cost, *executionFee) : std::nullopt;
        if (!debit.has_value() || debit->units > totalBudget.units) {
            const auto notionalBudget = Financial::notionalBeforeFee(
                totalBudget, m_feeRate);
            const auto affordable = notionalBudget.has_value()
                ? Financial::quantityForNotional(*notionalBudget, *fillPrice)
                : std::nullopt;
            if (!affordable.has_value() || !affordable->isPositive()) {
                ++m_blockedCount;
                return false;
            }
            units = *affordable;
            cost = Financial::notional(*fillPrice, units);
            executionFee = cost.has_value()
                ? Financial::fee(*cost, m_feeRate) : std::nullopt;
            debit = (cost.has_value() && executionFee.has_value())
                ? Financial::add(*cost, *executionFee) : std::nullopt;
        }
        if (!cost.has_value() || !executionFee.has_value() || !debit.has_value()
            || debit->units > totalBudget.units) {
            ++m_blockedCount;
            return false;
        }

        if (m_gateway) {
            return dispatchBrokerOrder(true, units, *normalizedMarketPrice,
                                       timestamp, strategyId);
        } else {
            if (!m_riskEngine.canTrade()) {
                ++m_blockedCount;
                return false;
            }
            m_portfolio.openLong(m_symbol, fillPrice->toDouble(), timestamp,
                                 executionFee->toDouble(), debit->toDouble(),
                                 units.toDouble(), strategyId);
            ++m_filledCount;

            if (m_analytics) {
                m_analytics->recordTrade(timestamp, 'B', fillPrice->toDouble(),
                                         m_portfolio.getPositionQuantity(m_symbol),
                                         executionFee->toDouble(), 0.0, 0.0, strategyId);
                m_analytics->recordSlippage(timestamp, m_symbol,
                                            normalizedMarketPrice->toDouble(),
                                            fillPrice->toDouble(),
                                            m_portfolio.getPositionQuantity(m_symbol));
            }

            std::cout << "[TRADE] BUY"
                      << " | Symbol: "   << m_symbol
                      << " | Strategy: " << (strategyId.empty() ? "N/A" : strategyId)
                      << " | Fill: $"    << std::fixed << std::setprecision(2) << fillPrice->toDouble()
                      << " | Fee: $"     << std::setprecision(2) << executionFee->toDouble()
                      << " | Equity: "   << m_portfolio.getTotalEquity()
                      << " | DD: "       << std::setprecision(4)
                      << (m_portfolio.getCurrentDrawdown() * 100.0) << "%"
                      << "\n";
            return true;
        }
    }

    if (signal == Signal::SELL) {
        if (!m_portfolio.hasPosition(m_symbol)) {
            return false;
        }

        const auto fillPrice = Financial::applySlippage(
            *normalizedMarketPrice, m_slippage, false);
        const auto qty = Financial::quantity(
            m_portfolio.getPositionQuantity(m_symbol),
            Financial::Rounding::RejectUnaligned);
        const auto proceeds = (fillPrice.has_value() && qty.has_value())
            ? Financial::notional(*fillPrice, *qty) : std::nullopt;
        const auto executionFee = proceeds.has_value()
            ? Financial::fee(*proceeds, m_feeRate) : std::nullopt;
        if (!fillPrice.has_value() || !qty.has_value() || !qty->isPositive()
            || !proceeds.has_value() || !executionFee.has_value()) {
            ++m_blockedCount;
            return false;
        }

        if (m_gateway) {
            return dispatchBrokerOrder(false, *qty, *normalizedMarketPrice,
                                       timestamp, strategyId);
        } else {
            m_portfolio.closePosition(m_symbol, fillPrice->toDouble(), timestamp,
                                      executionFee->toDouble(), strategyId);
            ++m_filledCount;

            // Retrieve accurate PnL from the just-recorded TradeRecord.
            double accuratePnL   = 0.0;
            double accurateGross = 0.0;
            const auto& log = m_portfolio.getTradeLog();
            if (!log.empty()) {
                accuratePnL  = log.back().realizedPnL;
                accurateGross = log.back().grossPnL;
            }

            if (m_analytics) {
                m_analytics->recordTrade(timestamp, 'S', fillPrice->toDouble(),
                                         qty->toDouble(), executionFee->toDouble(),
                                         accuratePnL, accurateGross, strategyId);
                m_analytics->recordSlippage(timestamp, m_symbol,
                                            normalizedMarketPrice->toDouble(),
                                            fillPrice->toDouble(), qty->toDouble());
            }

            std::cout << "[TRADE] SELL"
                      << " | Symbol: "   << m_symbol
                      << " | Strategy: " << (strategyId.empty() ? "N/A" : strategyId)
                      << " | Fill: $"    << std::fixed << std::setprecision(2) << fillPrice->toDouble()
                      << " | Fee: $"     << std::setprecision(2) << executionFee->toDouble()
                      << " | PnL: $"     << std::setprecision(2) << accuratePnL
                      << " | Equity: "   << m_portfolio.getTotalEquity()
                      << " | DD: "       << std::setprecision(4)
                      << (m_portfolio.getCurrentDrawdown() * 100.0) << "%"
                      << "\n";
            return true;
        }
    }

    return false;
}

int ExecutionEngine::getSignalCount()  const noexcept { return m_signalCount; }
int ExecutionEngine::getFilledCount()  const noexcept { return m_filledCount; }
int ExecutionEngine::getBlockedCount() const noexcept { return m_blockedCount; }
double ExecutionEngine::getLastRouteLatencyMs() const noexcept { return m_lastRouteLatencyMs.load(); }
double ExecutionEngine::getMaxRouteLatencyMs() const noexcept { return m_maxRouteLatencyMs.load(); }
uint64_t ExecutionEngine::getLatencyBreachCount() const noexcept { return m_latencyBreaches.load(); }
uint64_t ExecutionEngine::lastBrokerOrderId() const noexcept { return m_lastBrokerOrderId.load(); }

double ExecutionEngine::pendingBrokerQuantity(uint64_t orderId) const noexcept
{
    std::lock_guard<std::mutex> lock(m_fillMutex);
    const auto it = m_pendingBrokerQty.find(orderId);
    return (it == m_pendingBrokerQty.end()) ? 0.0 : it->second.toDouble();
}

void ExecutionEngine::onBrokerExecution(const ExecutionEvent& execution)
{
    if (!m_gateway) {
        return;
    }

    const auto lifecycle = m_gateway->orderLifecycle(execution.localOrderId);
    if (!lifecycle.has_value() || lifecycle->request.canonicalSymbol != m_symbol) {
        m_riskEngine.reportApiError();
        return;
    }

    const auto cumulative = Financial::quantity(
        execution.cumulativeFilledQuantity.toDouble(),
        Financial::Rounding::RejectUnaligned);
    const auto remaining = Financial::quantity(
        execution.remainingQuantity.toDouble(),
        Financial::Rounding::RejectUnaligned);
    const auto fillPrice = Financial::price(
        execution.fillPrice.toDouble(), Financial::Rounding::RejectUnaligned);
    const auto fillFee = Financial::money(
        execution.fee.toDouble(), Financial::Rounding::RejectUnaligned);
    if (!cumulative.has_value() || !cumulative->isPositive()
        || !remaining.has_value() || remaining->isNegative()
        || !fillPrice.has_value() || !fillPrice->isPositive()
        || !fillFee.has_value() || fillFee->isNegative()) {
        m_riskEngine.reportApiError();
        return;
    }

    Financial::Quantity previous{};
    {
        std::lock_guard<std::mutex> lock(m_fillMutex);
        const auto it = m_appliedBrokerQty.find(execution.localOrderId);
        if (it != m_appliedBrokerQty.end()) {
            previous = it->second;
        }
    }
    const auto incremental = Financial::subtract(*cumulative, previous);
    if (!incremental.has_value() || !incremental->isPositive()) {
        m_riskEngine.reportApiError();
        return;
    }

    const bool isBuy = lifecycle->request.side == OrderSide::Buy;
    const std::string& strategyId = lifecycle->request.sourceId;
    const double requestedPrice = lifecycle->request.referencePrice.has_value()
        ? lifecycle->request.referencePrice->toDouble()
        : fillPrice->toDouble();

    try {
        if (isBuy) {
            const auto cost = Financial::notional(*fillPrice, *incremental);
            const auto debit = cost.has_value()
                ? Financial::add(*cost, *fillFee) : std::nullopt;
            if (!cost.has_value() || !debit.has_value()) {
                m_riskEngine.reportApiError();
                return;
            }
            if (!m_portfolio.hasPosition(m_symbol)) {
                m_portfolio.openLong(m_symbol, fillPrice->toDouble(),
                                     execution.timestampNs, fillFee->toDouble(),
                                     debit->toDouble(), incremental->toDouble(),
                                     strategyId);
            } else {
                m_portfolio.addToLong(m_symbol, fillPrice->toDouble(),
                                      incremental->toDouble(), fillFee->toDouble(),
                                      debit->toDouble(), strategyId);
            }
            if (m_analytics) {
                m_analytics->recordTrade(execution.timestampNs, 'B',
                                         fillPrice->toDouble(), incremental->toDouble(),
                                         fillFee->toDouble(), 0.0, 0.0, strategyId);
                m_analytics->recordSlippage(execution.timestampNs, m_symbol,
                                            requestedPrice, fillPrice->toDouble(),
                                            incremental->toDouble());
            }
        } else {
            const auto held = Financial::quantity(
                m_portfolio.getPositionQuantity(m_symbol),
                Financial::Rounding::RejectUnaligned);
            if (!held.has_value() || !held->isPositive()
                || incremental->units > held->units) {
                m_riskEngine.reportApiError();
                return;
            }
            m_portfolio.closePosition(m_symbol, fillPrice->toDouble(),
                                      execution.timestampNs, fillFee->toDouble(),
                                      strategyId, incremental->toDouble());

            double accuratePnL = 0.0;
            double accurateGross = 0.0;
            const auto& log = m_portfolio.getTradeLog();
            if (!log.empty()) {
                accuratePnL = log.back().realizedPnL;
                accurateGross = log.back().grossPnL;
            }
            if (m_analytics) {
                m_analytics->recordTrade(execution.timestampNs, 'S',
                                         fillPrice->toDouble(), incremental->toDouble(),
                                         fillFee->toDouble(), accuratePnL, accurateGross,
                                         strategyId);
                m_analytics->recordSlippage(execution.timestampNs, m_symbol,
                                            requestedPrice, fillPrice->toDouble(),
                                            incremental->toDouble());
            }
        }
    } catch (const std::exception&) {
        m_riskEngine.reportApiError();
        return;
    }

    m_riskEngine.syncPosition(m_symbol, m_portfolio.getPositionQuantity(m_symbol));
    ++m_filledCount;
    m_lastBrokerOrderId.store(execution.localOrderId);

    {
        std::lock_guard<std::mutex> lock(m_fillMutex);
        m_appliedBrokerQty[execution.localOrderId] = *cumulative;
        if (remaining->isPositive()) {
            m_pendingBrokerQty[execution.localOrderId] = *remaining;
        } else {
            m_pendingBrokerQty.erase(execution.localOrderId);
            m_appliedBrokerQty.erase(execution.localOrderId);
        }
    }
}

// ── Phase 9: Pending Order Lifecycle ─────────────────────────────────────────

uint64_t ExecutionEngine::placePendingOrder(const OrderRecord& order) noexcept
{
    return m_portfolio.placePendingOrder(order);
}

void ExecutionEngine::bindTriggerOrderManager(TriggerOrderManager* manager) noexcept
{
    m_triggerOrders = manager;
}

uint64_t ExecutionEngine::placeStopLossTrigger(double stopPrice,
                                               double quantity,
                                               const std::string& strategyId) noexcept
{
    if (!m_triggerOrders) {
        return 0;
    }
    const double qty = (quantity > 0.0) ? quantity : m_portfolio.getPositionQuantity(m_symbol);
    return m_triggerOrders->placeStopLoss(m_symbol,
                                          /*isBuy=*/false,
                                          stopPrice,
                                          qty,
                                          strategyId);
}

uint64_t ExecutionEngine::placeOcoTrigger(double stopPrice,
                                          double takeProfitPrice,
                                          double quantity,
                                          const std::string& strategyId) noexcept
{
    if (!m_triggerOrders) {
        return 0;
    }
    const double qty = (quantity > 0.0) ? quantity : m_portfolio.getPositionQuantity(m_symbol);
    return m_triggerOrders->placeOco(m_symbol,
                                     /*isBuy=*/false,
                                     stopPrice,
                                     takeProfitPrice,
                                     qty,
                                     strategyId);
}

int ExecutionEngine::processTriggerOrders(const L2OrderBook& orderBook,
                                          uint64_t timestamp)
{
    if (!m_triggerOrders) {
        return 0;
    }

    const auto bbo = orderBook.bbo();
    if (!bbo.valid) {
        return 0;
    }

    std::vector<TriggerFillEvent> triggers;
    triggers.reserve(16);
    const std::size_t triggeredCount =
        m_triggerOrders->evaluate(m_symbol, bbo, timestamp, triggers);

    int executed = 0;
    for (const auto& ev : triggers) {
        const Signal signal = ev.isBuy ? Signal::BUY : Signal::SELL;
        const std::string sid = ev.strategyId.empty() ? "TRIGGER" : ev.strategyId;
        if (execute(signal, ev.executionPrice, ev.timestamp, sid)) {
            (void)m_triggerOrders->cancel(ev.orderId);
            ++executed;
        }
    }
    return static_cast<int>(std::min<std::size_t>(static_cast<std::size_t>(executed),
                                                  triggeredCount));
}

int ExecutionEngine::processPendingOrders(const MarketCandle& candle)
{
    std::vector<OrderFillResult> fills =
        m_portfolio.evaluatePendingOrders(candle.symbol,
                                          candle.high,
                                          candle.low,
                                          candle.close,
                                          candle.epochTimestamp);
    int filledCount = 0;
    for (const auto& res : fills) {
        if (!res.filled) { continue; }

        const auto fillPrice = Financial::price(res.fillPrice);
        if (!fillPrice.has_value() || !fillPrice->isPositive()) {
            ++m_blockedCount;
            continue;
        }

        if (res.isBuy) {
            if (!m_riskEngine.canTrade()) {
                ++m_blockedCount;
                std::cout << "[RISK]  Pending BUY (id=" << res.orderId
                          << ") blocked by RiskEngine at $"
                          << std::fixed << std::setprecision(2)
                          << fillPrice->toDouble() << "\n";
                continue;
            }
            if (m_portfolio.hasPosition(res.symbol)) {
                continue;
            }

            const auto slippedPrice = Financial::applySlippage(
                *fillPrice, m_slippage, true);
            const auto cash = Financial::money(
                m_portfolio.getCashBalance(), Financial::Rounding::RejectUnaligned);
            if (!slippedPrice.has_value() || !cash.has_value() || !cash->isPositive()) {
                ++m_blockedCount;
                continue;
            }
            Financial::Money budget = *cash;
            if (res.capitalToCommit > 0.0) {
                const auto requestedBudget = Financial::money(res.capitalToCommit);
                if (!requestedBudget.has_value() || !requestedBudget->isPositive()) {
                    ++m_blockedCount;
                    continue;
                }
                budget = requestedBudget->units < cash->units ? *requestedBudget : *cash;
            }

            auto units = res.quantity > 0.0
                ? Financial::quantity(res.quantity)
                : std::optional<Financial::Quantity>{};
            if (!units.has_value() || !units->isPositive()) {
                const auto notionalBudget = Financial::notionalBeforeFee(budget, m_feeRate);
                units = notionalBudget.has_value()
                    ? Financial::quantityForNotional(*notionalBudget, *slippedPrice)
                    : std::nullopt;
            }
            auto cost = units.has_value()
                ? Financial::notional(*slippedPrice, *units) : std::nullopt;
            auto executionFee = cost.has_value()
                ? Financial::fee(*cost, m_feeRate) : std::nullopt;
            auto debit = (cost.has_value() && executionFee.has_value())
                ? Financial::add(*cost, *executionFee) : std::nullopt;
            if (!debit.has_value() || debit->units > budget.units) {
                const auto notionalBudget = Financial::notionalBeforeFee(budget, m_feeRate);
                units = notionalBudget.has_value()
                    ? Financial::quantityForNotional(*notionalBudget, *slippedPrice)
                    : std::nullopt;
                cost = units.has_value()
                    ? Financial::notional(*slippedPrice, *units) : std::nullopt;
                executionFee = cost.has_value()
                    ? Financial::fee(*cost, m_feeRate) : std::nullopt;
                debit = (cost.has_value() && executionFee.has_value())
                    ? Financial::add(*cost, *executionFee) : std::nullopt;
            }
            if (!units.has_value() || !units->isPositive() || !executionFee.has_value()
                || !debit.has_value() || debit->units > budget.units) {
                ++m_blockedCount;
                continue;
            }
            m_portfolio.openLong(res.symbol, slippedPrice->toDouble(),
                                 res.fillTimestamp, executionFee->toDouble(),
                                 debit->toDouble(), units->toDouble());
            ++m_filledCount;
            ++filledCount;

            if (m_analytics) {
                m_analytics->recordTrade(res.fillTimestamp, 'B',
                                         slippedPrice->toDouble(),
                                         m_portfolio.getPositionQuantity(res.symbol),
                                         executionFee->toDouble(), 0.0);
            }
            std::cout << "[PENDING] BUY FILL id=" << res.orderId
                      << " sym=" << res.symbol
                      << " price=$" << std::fixed << std::setprecision(2)
                      << slippedPrice->toDouble()
                      << "\n";
        } else {
            if (!m_portfolio.hasPosition(res.symbol)) { continue; }

            const auto slippedPrice = Financial::applySlippage(
                *fillPrice, m_slippage, false);
            const auto qty = Financial::quantity(
                m_portfolio.getPositionQuantity(res.symbol),
                Financial::Rounding::RejectUnaligned);
            const auto proceeds = (slippedPrice.has_value() && qty.has_value())
                ? Financial::notional(*slippedPrice, *qty) : std::nullopt;
            const auto executionFee = proceeds.has_value()
                ? Financial::fee(*proceeds, m_feeRate) : std::nullopt;
            if (!slippedPrice.has_value() || !qty.has_value() || !qty->isPositive()
                || !proceeds.has_value() || !executionFee.has_value()) {
                ++m_blockedCount;
                continue;
            }

            m_portfolio.closePosition(res.symbol, slippedPrice->toDouble(),
                                      res.fillTimestamp, executionFee->toDouble());
            ++m_filledCount;
            ++filledCount;

            double accuratePnL   = 0.0;
            double accurateGross = 0.0;
            const auto& log = m_portfolio.getTradeLog();
            if (!log.empty()) {
                accuratePnL  = log.back().realizedPnL;
                accurateGross = log.back().grossPnL;
            }
            if (m_analytics) {
                m_analytics->recordTrade(res.fillTimestamp, 'S',
                                         slippedPrice->toDouble(), qty->toDouble(),
                                         executionFee->toDouble(),
                                         accuratePnL, accurateGross);
            }
            std::cout << "[PENDING] SELL FILL id=" << res.orderId
                      << " sym=" << res.symbol
                      << " price=$" << std::fixed << std::setprecision(2)
                      << slippedPrice->toDouble()
                      << " PnL=$" << std::setprecision(2) << accuratePnL << "\n";
        }
    }
    return filledCount;
}
