#include "ExecutionEngine.hpp"
#include "AnalyticsEngine.hpp"
#include "BrokerGateway.hpp"
#include "L2OrderBook.hpp"
#include "SmaCrossStrategy.hpp"
#include "TriggerOrderManager.hpp"
#include "MarketCandle.hpp"
#include <chrono>
#include <cmath>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <limits>
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
    m_orderPool.resize(ORDER_POOL_SIZE);
    m_freeOrderNodes.reserve(ORDER_POOL_SIZE);
    for (auto& node : m_orderPool) {
        m_freeOrderNodes.push_back(&node);
    }
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
    m_gateway->addAcknowledgementCallback(
        [this](const OrderAcknowledgement& acknowledgement) {
            onBrokerAcknowledgement(acknowledgement);
        });
    m_gateway->addExecutionCallback([this](const ExecutionEvent& execution) {
        onBrokerExecution(execution);
    });
}

ExecutionEngine::OrderNode* ExecutionEngine::acquireOrderNode() noexcept
{
    std::lock_guard<std::mutex> lock(m_poolMutex);
    if (m_freeOrderNodes.empty()) {
        return nullptr;
    }
    OrderNode* node = m_freeOrderNodes.back();
    m_freeOrderNodes.pop_back();
    node->inUse = true;
    return node;
}

void ExecutionEngine::releaseOrderNode(OrderNode* node) noexcept
{
    if (!node) { return; }
    std::lock_guard<std::mutex> lock(m_poolMutex);
    node->inUse = false;
    m_freeOrderNodes.push_back(node);
}

void ExecutionEngine::copyToFixed(std::array<char, 24>& dst,
                                  const std::string& src) noexcept
{
    dst.fill('\0');
    const std::size_t count = std::min(dst.size() - 1, src.size());
    std::memcpy(dst.data(), src.data(), count);
}

std::string ExecutionEngine::fromFixed(const std::array<char, 24>& src)
{
    return std::string(src.data());
}

void ExecutionEngine::queueOrderEvent(const OrderBusEvent& event) noexcept
{
    OrderNode* node = acquireOrderNode();
    if (!node) {
        m_droppedBusEvents.fetch_add(1);
        return;
    }
    node->event = event;
    if (!m_orderBus.push(node)) {
        m_droppedBusEvents.fetch_add(1);
        releaseOrderNode(node);
    }
}

void ExecutionEngine::drainOrderBus()
{
    if (!m_gateway || !m_gateway->isConnected()) {
        return;
    }

    OrderNode* node = nullptr;
    while (m_orderBus.pop(node)) {
        if (!node) { continue; }
        const auto dispatchNs =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
        const double deltaMs = static_cast<double>(dispatchNs - node->event.signalNs) / 1'000'000.0;

        m_lastRouteLatencyMs.store(deltaMs);
        const double prevMax = m_maxRouteLatencyMs.load();
        if (deltaMs > prevMax) {
            m_maxRouteLatencyMs.store(deltaMs);
        }
        if (deltaMs > 5.0) {
            m_latencyBreaches.fetch_add(1);
            m_riskEngine.reportLatency(static_cast<uint32_t>(std::ceil(deltaMs)));
        }

        OrderRequest request;
        request.localOrderId = node->event.orderLocalId;
        request.canonicalSymbol = fromFixed(node->event.symbol);
        request.side = node->event.isBuy ? OrderSide::Buy : OrderSide::Sell;
        request.type = BrokerOrderType::Market;
        request.quantity = Decimal64::fromDouble(
            node->event.quantity, 8, DecimalRounding::TowardZero)
            .value_or(Decimal64{});
        request.referencePrice = Decimal64::fromDouble(
            node->event.requestedPrice, 8,
            DecimalRounding::NearestTiesAwayFromZero);
        request.sourceId = fromFixed(node->event.strategyId);
        request.timestampNs = node->event.timestamp;
        request.sequence = 1;
        request.idempotencyKey = "execution-"
                               + std::to_string(request.localOrderId);

        std::string rejection;
        const auto normalized = m_gateway->normalizeOrder(request, &rejection);
        if (!normalized.has_value()) {
            ++m_blockedCount;
            releaseOrderNode(node);
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(m_fillMutex);
            m_gatewayOrderContexts.emplace(request.localOrderId,
                GatewayOrderContext{*normalized, request.sourceId, Decimal64{},
                                    {}, 0, 0});
        }
        const auto dispatch = m_gateway->dispatchOrder(*normalized,
                                                        node->event.riskDecision);
        if (!dispatch.dispatched) {
            std::lock_guard<std::mutex> lock(m_fillMutex);
            m_gatewayOrderContexts.erase(request.localOrderId);
            ++m_blockedCount;
        }
        releaseOrderNode(node);
    }
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
        const RiskDecision riskDecision = m_riskEngine.evaluateOrder(OrderSide::Buy);
        if (!riskDecision.allowed) {
            ++m_blockedCount;
            std::cout << "[RISK]  BUY blocked by RiskEngine"
                      << " | Price: "  << std::fixed << std::setprecision(2) << marketPrice
                      << " | Equity: " << m_portfolio.getTotalEquity()
                      << " | DD: "     << std::setprecision(4)
                      << (m_portfolio.getCurrentDrawdown() * 100.0) << "%"
                      << "\n";
            return false;
        }
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

        if (m_gateway && m_gateway->isConnected()) {
            OrderBusEvent evt;
            evt.orderLocalId = m_nextLocalOrderId.fetch_add(1);
            copyToFixed(evt.symbol, m_symbol);
            copyToFixed(evt.strategyId, strategyId);
            evt.isBuy = true;
            evt.quantity = units.toDouble();
            evt.requestedPrice = normalizedMarketPrice->toDouble();
            evt.timestamp = timestamp;
            evt.signalNs = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count());
            evt.riskDecision = riskDecision;

            queueOrderEvent(evt);
            drainOrderBus();
            return true;
        } else {
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
        const RiskDecision riskDecision = m_riskEngine.evaluateOrder(OrderSide::Sell);
        if (!riskDecision.allowed) {
            ++m_blockedCount;
            return false;
        }
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

        if (m_gateway && m_gateway->isConnected()) {
            OrderBusEvent evt;
            evt.orderLocalId = m_nextLocalOrderId.fetch_add(1);
            copyToFixed(evt.symbol, m_symbol);
            copyToFixed(evt.strategyId, strategyId);
            evt.isBuy = false;
            evt.quantity = qty->toDouble();
            evt.requestedPrice = normalizedMarketPrice->toDouble();
            evt.timestamp = timestamp;
            evt.signalNs = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count());
            evt.riskDecision = riskDecision;
            queueOrderEvent(evt);
            drainOrderBus();
            return true;
        } else {
            m_portfolio.closePosition(m_symbol, fillPrice->toDouble(), timestamp,
                                      executionFee->toDouble(), strategyId,
                                      qty->toDouble());
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
uint64_t ExecutionEngine::getDroppedBusEvents() const noexcept { return m_droppedBusEvents.load(); }
uint64_t ExecutionEngine::lastBrokerOrderId() const noexcept { return m_lastBrokerOrderId.load(); }

double ExecutionEngine::pendingBrokerQuantity(uint64_t orderId) const noexcept
{
    std::lock_guard<std::mutex> lock(m_fillMutex);
    auto it = m_pendingBrokerQty.find(orderId);
    return (it == m_pendingBrokerQty.end()) ? 0.0 : it->second;
}

void ExecutionEngine::onBrokerAcknowledgement(
    const OrderAcknowledgement& acknowledgement)
{
    std::lock_guard<std::mutex> lock(m_fillMutex);
    const auto it = m_gatewayOrderContexts.find(acknowledgement.localOrderId);
    if (it == m_gatewayOrderContexts.end()) {
        return;
    }
    if (!acknowledgement.accepted) {
        m_gatewayOrderContexts.erase(it);
        m_pendingBrokerQty.erase(acknowledgement.localOrderId);
        m_riskEngine.reportApiError();
        return;
    }
    if (acknowledgement.externalOrderId.empty()) {
        m_gatewayOrderContexts.erase(it);
        m_pendingBrokerQty.erase(acknowledgement.localOrderId);
        m_riskEngine.reportApiError();
        return;
    }
    it->second.externalOrderId = acknowledgement.externalOrderId;
}

void ExecutionEngine::onBrokerExecution(const ExecutionEvent& execution)
{
    std::unique_lock<std::mutex> lock(m_fillMutex);
    const auto contextIt = m_gatewayOrderContexts.find(execution.localOrderId);
    if (contextIt == m_gatewayOrderContexts.end()) {
        return;
    }
    GatewayOrderContext& context = contextIt->second;
    const auto& request = context.order.request;
    const auto quantityScale = context.order.normalizedQuantity.scale;
    const bool sequenceStale = context.lastExecutionSequence > 0
        && execution.sequence > 0
        && execution.sequence <= context.lastExecutionSequence;
    const bool timestampStale = context.lastExecutionTimestamp > 0
        && execution.timestampNs > 0
        && execution.timestampNs < context.lastExecutionTimestamp;
    const bool cumulativeOverflow = execution.cumulativeFilledQuantity.units < 0
        || execution.cumulativeFilledQuantity.units
            > context.order.normalizedQuantity.units;
    const bool remainingOverflow = execution.remainingQuantity.units < 0
        || execution.remainingQuantity.units
            > context.order.normalizedQuantity.units;
    bool quantitiesDoNotReconcile = true;
    if (!cumulativeOverflow && !remainingOverflow
        && execution.cumulativeFilledQuantity.units
            <= std::numeric_limits<std::int64_t>::max()
                - execution.remainingQuantity.units) {
        quantitiesDoNotReconcile =
            execution.cumulativeFilledQuantity.units
                + execution.remainingQuantity.units
            != context.order.normalizedQuantity.units;
    }
    if (request.canonicalSymbol != m_symbol
        || execution.eventKey.empty()
        || execution.externalOrderId.empty()
        || (!context.externalOrderId.empty()
            && execution.externalOrderId != context.externalOrderId)
        || execution.cumulativeFilledQuantity.scale != quantityScale
        || execution.remainingQuantity.scale != quantityScale
        || sequenceStale || timestampStale
        || execution.cumulativeFilledQuantity.units
            <= context.cumulativeFilled.units
        || cumulativeOverflow || remainingOverflow
        || quantitiesDoNotReconcile) {
        m_riskEngine.reportApiError();
        return;
    }

    const Decimal64 filledDelta{
        execution.cumulativeFilledQuantity.units - context.cumulativeFilled.units,
        quantityScale};
    if (!filledDelta.isPositive()) {
        m_riskEngine.reportApiError();
        return;
    }

    const auto fillPrice = Financial::price(execution.fillPrice.toDouble(),
                                            Financial::Rounding::RejectUnaligned);
    const auto executedQuantity = Financial::quantity(filledDelta.toDouble(),
                                                       Financial::Rounding::RejectUnaligned);
    const auto fillFee = Financial::money(execution.fee.toDouble(),
                                          Financial::Rounding::RejectUnaligned);
    if (!fillPrice.has_value() || !fillPrice->isPositive()
        || !executedQuantity.has_value() || !executedQuantity->isPositive()
        || !fillFee.has_value() || fillFee->isNegative()) {
        m_riskEngine.reportApiError();
        return;
    }

    const std::string& strategyId = context.strategyId;

    try {
        if (request.side == OrderSide::Buy) {
            const auto cost = Financial::notional(*fillPrice, *executedQuantity);
            const auto debit = cost.has_value()
                ? Financial::add(*cost, *fillFee) : std::nullopt;
            if (!cost.has_value() || !debit.has_value()) {
                m_riskEngine.reportApiError();
                return;
            }
            if (!m_portfolio.hasPosition(request.canonicalSymbol)) {
                m_portfolio.openLong(request.canonicalSymbol,
                                     fillPrice->toDouble(),
                                     execution.timestampNs,
                                     fillFee->toDouble(),
                                     debit->toDouble(),
                                     executedQuantity->toDouble(),
                                     strategyId);
            } else {
                m_portfolio.addToLong(request.canonicalSymbol,
                                      fillPrice->toDouble(),
                                      executedQuantity->toDouble(),
                                      fillFee->toDouble(),
                                      debit->toDouble(),
                                      strategyId);
            }
            if (m_analytics) {
                m_analytics->recordTrade(execution.timestampNs, 'B',
                                         fillPrice->toDouble(),
                                         executedQuantity->toDouble(),
                                         fillFee->toDouble(), 0.0, 0.0,
                                         strategyId);
                m_analytics->recordSlippage(execution.timestampNs,
                                            request.canonicalSymbol,
                                            request.referencePrice.has_value()
                                                ? request.referencePrice->toDouble()
                                                : 0.0,
                                            fillPrice->toDouble(),
                                            executedQuantity->toDouble());
            }
        } else {
            const auto held = Financial::quantity(
                m_portfolio.getPositionQuantity(request.canonicalSymbol),
                Financial::Rounding::RejectUnaligned);
            if (!held.has_value() || !held->isPositive()
                || executedQuantity->units > held->units) {
                m_riskEngine.reportApiError();
                return;
            }
            m_portfolio.closePosition(request.canonicalSymbol,
                                      fillPrice->toDouble(),
                                      execution.timestampNs,
                                      fillFee->toDouble(),
                                      strategyId,
                                      executedQuantity->toDouble());
            double accuratePnL = 0.0;
            double accurateGross = 0.0;
            const auto& log = m_portfolio.getTradeLog();
            if (!log.empty()) {
                accuratePnL = log.back().realizedPnL;
                accurateGross = log.back().grossPnL;
            }
            if (m_analytics) {
                m_analytics->recordTrade(execution.timestampNs, 'S',
                                         fillPrice->toDouble(),
                                         executedQuantity->toDouble(),
                                         fillFee->toDouble(),
                                         accuratePnL, accurateGross, strategyId);
                m_analytics->recordSlippage(execution.timestampNs,
                                            request.canonicalSymbol,
                                            request.referencePrice.has_value()
                                                ? request.referencePrice->toDouble()
                                                : 0.0,
                                            fillPrice->toDouble(),
                                            executedQuantity->toDouble());
            }
        }
    } catch (const std::exception&) {
        m_riskEngine.reportApiError();
        return;
    }

    context.cumulativeFilled = execution.cumulativeFilledQuantity;
    context.lastExecutionTimestamp = execution.timestampNs;
    context.lastExecutionSequence = execution.sequence;
    m_riskEngine.syncPosition(request.canonicalSymbol,
                              m_portfolio.getPositionQuantity(request.canonicalSymbol));
    m_lastBrokerOrderId.store(execution.localOrderId);
    ++m_filledCount;

    if (!execution.remainingQuantity.isZero()) {
        m_pendingBrokerQty[execution.localOrderId] =
            execution.remainingQuantity.toDouble();
    } else {
        m_pendingBrokerQty.erase(execution.localOrderId);
        m_gatewayOrderContexts.erase(contextIt);
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
                                      res.fillTimestamp, executionFee->toDouble(),
                                      "", qty->toDouble());
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
