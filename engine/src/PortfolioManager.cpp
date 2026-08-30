#include "PortfolioManager.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace {

template <typename T>
T requireValue(const std::optional<T>& value, const char* message)
{
    if (!value.has_value()) {
        throw std::overflow_error(message);
    }
    return *value;
}

void requireCounterCapacity(int value, const char* message)
{
    if (value == std::numeric_limits<int>::max()) {
        throw std::overflow_error(message);
    }
}

} // namespace

PortfolioManager::Valuation PortfolioManager::valueState(
    Financial::Money cash,
    const std::unordered_map<std::string, PositionState>& positions) const
{
    Financial::Money markedValue{};
    Financial::Money unrealized{};
    for (const auto& [symbol, state] : positions) {
        (void)symbol;
        const auto marked = requireValue(
            Financial::notional(state.m_lastMarkPrice, state.m_quantity),
            "PortfolioManager: marked notional overflow");
        markedValue = requireValue(
            Financial::add(markedValue, marked),
            "PortfolioManager: aggregate marked value overflow");
        const auto positionPnl = requireValue(
            Financial::subtract(marked, state.m_costBasis),
            "PortfolioManager: unrealized P&L overflow");
        unrealized = requireValue(
            Financial::add(unrealized, positionPnl),
            "PortfolioManager: aggregate unrealized P&L overflow");
    }

    const auto equity = requireValue(
        Financial::add(cash, markedValue),
        "PortfolioManager: total equity overflow");
    const Financial::Money maxEquity = equity.units > m_maxEquity.units
        ? equity : m_maxEquity;
    const double drawdown = maxEquity.isPositive()
        ? (maxEquity.toDouble() - equity.toDouble()) / maxEquity.toDouble()
        : 0.0;

    return Valuation{
        unrealized,
        equity,
        maxEquity,
        drawdown,
        std::max(m_maxDrawdown, drawdown)
    };
}

void PortfolioManager::commitValuation(const Valuation& valuation) noexcept
{
    m_unrealizedPnL = valuation.unrealizedPnL;
    m_totalEquity = valuation.totalEquity;
    m_maxEquity = valuation.maxEquity;
    m_currentDrawdown = valuation.currentDrawdown;
    m_maxDrawdown = valuation.maxDrawdown;
}

void PortfolioManager::openLong(const std::string& symbol, double netPrice,
                                uint64_t timestamp, double fee,
                                double capitalToCommit,
                                double explicitUnits,
                                const std::string& strategyId)
{
    if (symbol.empty()) {
        throw std::invalid_argument("PortfolioManager: symbol must not be empty");
    }
    if (m_positions.contains(symbol)) {
        throw std::logic_error(
            "PortfolioManager: cannot open a new position for '" + symbol
            + "' while one is already open");
    }
    if (explicitUnits < 0.0 || capitalToCommit < 0.0 || fee < 0.0) {
        throw std::invalid_argument("PortfolioManager: negative accounting input");
    }

    const auto price = Financial::price(netPrice);
    const auto entryFee = Financial::money(fee);
    if (!price.has_value() || !price->isPositive()
        || !entryFee.has_value() || entryFee->isNegative()) {
        throw std::invalid_argument("PortfolioManager: invalid price or fee");
    }

    Financial::Money budget = m_cash;
    if (capitalToCommit > 0.0) {
        const auto normalizedBudget = Financial::money(capitalToCommit);
        if (!normalizedBudget.has_value() || !normalizedBudget->isPositive()
            || normalizedBudget->units > m_cash.units) {
            throw std::invalid_argument("PortfolioManager: invalid capital budget");
        }
        budget = *normalizedBudget;
    }

    Financial::Quantity quantity{};
    Financial::Money costBasis{};
    Financial::Money debit{};
    if (explicitUnits > 0.0) {
        const auto normalizedQuantity = Financial::quantity(explicitUnits);
        if (!normalizedQuantity.has_value() || !normalizedQuantity->isPositive()) {
            throw std::invalid_argument("PortfolioManager: invalid explicit quantity");
        }
        quantity = *normalizedQuantity;
        costBasis = requireValue(
            Financial::notional(*price, quantity),
            "PortfolioManager: entry notional overflow");
        debit = requireValue(
            Financial::add(costBasis, *entryFee),
            "PortfolioManager: entry debit overflow");
        if (debit.units > budget.units) {
            throw std::logic_error("PortfolioManager: explicit quantity exceeds capital budget");
        }
    } else {
        const auto notionalBudget = Financial::subtract(budget, *entryFee);
        if (!notionalBudget.has_value() || !notionalBudget->isPositive()) {
            throw std::logic_error("PortfolioManager: fee exhausts capital budget");
        }
        const auto affordable = Financial::quantityForNotional(*notionalBudget, *price);
        if (!affordable.has_value() || !affordable->isPositive()) {
            throw std::logic_error("PortfolioManager: capital normalizes to zero quantity");
        }
        quantity = *affordable;
        costBasis = requireValue(
            Financial::notional(*price, quantity),
            "PortfolioManager: entry notional overflow");
        debit = requireValue(
            Financial::add(costBasis, *entryFee),
            "PortfolioManager: entry debit overflow");
    }

    if (debit.units > m_cash.units) {
        throw std::logic_error("PortfolioManager: insufficient cash for entry fill");
    }
    if (!costBasis.isPositive()) {
        throw std::logic_error("PortfolioManager: entry notional normalizes to zero");
    }
    const auto averageEntry = requireValue(
        Financial::averagePrice(costBasis, quantity),
        "PortfolioManager: average entry price overflow");
    const auto nextCash = requireValue(
        Financial::subtract(m_cash, debit),
        "PortfolioManager: cash debit overflow");
    const auto nextFees = requireValue(
        Financial::add(m_totalFeesPaid, *entryFee),
        "PortfolioManager: total fee overflow");

    auto nextPositions = m_positions;
    nextPositions.emplace(symbol, PositionState{
        quantity,
        averageEntry,
        costBasis,
        *price,
        timestamp,
        *entryFee,
        strategyId
    });
    const Valuation valuation = valueState(nextCash, nextPositions);
    requireCounterCapacity(m_tradeCount, "PortfolioManager: trade counter overflow");

    m_cash = nextCash;
    m_positions = std::move(nextPositions);
    m_totalFeesPaid = nextFees;
    ++m_tradeCount;
    commitValuation(valuation);
}

void PortfolioManager::addToLong(const std::string& symbol, double netPrice,
                                 double quantity, double fee,
                                 double capitalToCommit,
                                 const std::string& strategyId)
{
    auto current = m_positions.find(symbol);
    if (current == m_positions.end()) {
        const double budget = capitalToCommit > 0.0
            ? capitalToCommit : (quantity * netPrice + fee);
        openLong(symbol, netPrice, 0, fee, budget, quantity, strategyId);
        return;
    }
    if (quantity <= 0.0 || fee < 0.0 || capitalToCommit < 0.0) {
        throw std::invalid_argument("PortfolioManager::addToLong invalid accounting input");
    }

    const auto price = Financial::price(netPrice);
    const auto addedQuantity = Financial::quantity(quantity);
    const auto addedFee = Financial::money(fee);
    if (!price.has_value() || !price->isPositive()
        || !addedQuantity.has_value() || !addedQuantity->isPositive()
        || !addedFee.has_value() || addedFee->isNegative()) {
        throw std::invalid_argument("PortfolioManager::addToLong unrepresentable input");
    }

    const auto addedCost = requireValue(
        Financial::notional(*price, *addedQuantity),
        "PortfolioManager: add-on notional overflow");
    const auto debit = requireValue(
        Financial::add(addedCost, *addedFee),
        "PortfolioManager: add-on debit overflow");
    if (!addedCost.isPositive()) {
        throw std::logic_error("PortfolioManager: add-on notional normalizes to zero");
    }
    if (debit.units > m_cash.units) {
        throw std::logic_error("PortfolioManager::addToLong insufficient cash");
    }
    if (capitalToCommit > 0.0) {
        const auto budget = Financial::money(capitalToCommit);
        if (!budget.has_value() || budget->isNegative() || debit.units > budget->units) {
            throw std::logic_error("PortfolioManager::addToLong exceeds capital budget");
        }
    }

    auto nextPositions = m_positions;
    PositionState& state = nextPositions.at(symbol);
    state.m_quantity = requireValue(
        Financial::add(state.m_quantity, *addedQuantity),
        "PortfolioManager: position quantity overflow");
    state.m_costBasis = requireValue(
        Financial::add(state.m_costBasis, addedCost),
        "PortfolioManager: cost-basis overflow");
    state.m_entryFee = requireValue(
        Financial::add(state.m_entryFee, *addedFee),
        "PortfolioManager: entry-fee overflow");
    state.m_averageEntryPrice = requireValue(
        Financial::averagePrice(state.m_costBasis, state.m_quantity),
        "PortfolioManager: average-price overflow");
    state.m_lastMarkPrice = *price;
    if (!strategyId.empty()) {
        state.m_strategyId = strategyId;
    }

    const auto nextCash = requireValue(
        Financial::subtract(m_cash, debit),
        "PortfolioManager: cash debit overflow");
    const auto nextFees = requireValue(
        Financial::add(m_totalFeesPaid, *addedFee),
        "PortfolioManager: total fee overflow");
    const Valuation valuation = valueState(nextCash, nextPositions);
    requireCounterCapacity(m_tradeCount, "PortfolioManager: trade counter overflow");

    m_cash = nextCash;
    m_positions = std::move(nextPositions);
    m_totalFeesPaid = nextFees;
    ++m_tradeCount;
    commitValuation(valuation);
}

void PortfolioManager::closePosition(const std::string& symbol, double netPrice,
                                     uint64_t timestamp, double fee,
                                     const std::string& strategyId,
                                     double explicitQuantity)
{
    const auto current = m_positions.find(symbol);
    if (current == m_positions.end()) {
        return;
    }
    if (explicitQuantity < 0.0 || fee < 0.0) {
        throw std::invalid_argument("PortfolioManager: negative close input");
    }
    const auto price = Financial::price(netPrice);
    const auto exitFee = Financial::money(fee);
    if (!price.has_value() || !price->isPositive()
        || !exitFee.has_value() || exitFee->isNegative()) {
        throw std::invalid_argument("PortfolioManager: invalid close price or fee");
    }

    Financial::Quantity closeQuantity = current->second.m_quantity;
    if (explicitQuantity > 0.0) {
        // Confirmed quantities have already crossed the scale-8 boundary.
        // Re-normalizing them toward zero can lose one unit after a
        // decimal -> double -> decimal round trip on some standard libraries,
        // leaving a phantom residual position after a full close.
        const auto normalizedQuantity = Financial::quantity(
            explicitQuantity, Financial::Rounding::RejectUnaligned);
        if (!normalizedQuantity.has_value() || !normalizedQuantity->isPositive()) {
            throw std::invalid_argument("PortfolioManager: invalid close quantity");
        }
        if (normalizedQuantity->units > current->second.m_quantity.units) {
            throw std::logic_error(
                "PortfolioManager: reversal/over-close is unsupported");
        }
        closeQuantity = *normalizedQuantity;
    }

    const PositionState& prior = current->second;
    const auto grossProceeds = requireValue(
        Financial::notional(*price, closeQuantity),
        "PortfolioManager: close notional overflow");
    const auto netProceeds = Financial::subtract(grossProceeds, *exitFee);
    if (!netProceeds.has_value() || netProceeds->isNegative()) {
        throw std::logic_error("PortfolioManager: close fee exceeds proceeds");
    }
    const auto allocatedCost = requireValue(
        Financial::proportional(prior.m_costBasis, closeQuantity, prior.m_quantity),
        "PortfolioManager: allocated cost-basis overflow");
    const auto allocatedEntryFee = requireValue(
        Financial::proportional(prior.m_entryFee, closeQuantity, prior.m_quantity),
        "PortfolioManager: allocated entry-fee overflow");
    const auto grossPnl = requireValue(
        Financial::subtract(grossProceeds, allocatedCost),
        "PortfolioManager: gross P&L overflow");
    const auto afterExitFee = requireValue(
        Financial::subtract(grossPnl, *exitFee),
        "PortfolioManager: exit-fee P&L overflow");
    const auto realizedPnl = requireValue(
        Financial::subtract(afterExitFee, allocatedEntryFee),
        "PortfolioManager: realized P&L overflow");
    const auto tradeFees = requireValue(
        Financial::add(*exitFee, allocatedEntryFee),
        "PortfolioManager: trade-fee overflow");

    TradeRecord record;
    record.symbol = symbol;
    record.openTimestamp = prior.m_entryTimestamp;
    record.closeTimestamp = timestamp;
    record.entryPrice = prior.m_averageEntryPrice.toDouble();
    record.exitPrice = price->toDouble();
    record.quantity = closeQuantity.toDouble();
    record.totalFees = tradeFees.toDouble();
    record.realizedPnL = realizedPnl.toDouble();
    record.grossPnL = grossPnl.toDouble();
    record.strategy_id = !strategyId.empty() ? strategyId : prior.m_strategyId;

    auto nextPositions = m_positions;
    PositionState& next = nextPositions.at(symbol);
    const auto remainingQuantity = requireValue(
        Financial::subtract(next.m_quantity, closeQuantity),
        "PortfolioManager: remaining quantity overflow");
    if (remainingQuantity.isZero()) {
        nextPositions.erase(symbol);
    } else {
        next.m_quantity = remainingQuantity;
        next.m_costBasis = requireValue(
            Financial::subtract(next.m_costBasis, allocatedCost),
            "PortfolioManager: remaining cost-basis overflow");
        next.m_entryFee = requireValue(
            Financial::subtract(next.m_entryFee, allocatedEntryFee),
            "PortfolioManager: remaining entry-fee overflow");
        next.m_averageEntryPrice = requireValue(
            Financial::averagePrice(next.m_costBasis, next.m_quantity),
            "PortfolioManager: remaining average-price overflow");
        next.m_lastMarkPrice = *price;
    }

    const auto nextCash = requireValue(
        Financial::add(m_cash, *netProceeds),
        "PortfolioManager: sale cash overflow");
    const auto nextFees = requireValue(
        Financial::add(m_totalFeesPaid, *exitFee),
        "PortfolioManager: total fee overflow");
    auto nextTradeLog = m_tradeLog;
    nextTradeLog.push_back(record);
    const Valuation valuation = valueState(nextCash, nextPositions);
    requireCounterCapacity(m_tradeCount, "PortfolioManager: trade counter overflow");
    if (remainingQuantity.isZero()) {
        requireCounterCapacity(m_roundTripCount,
                               "PortfolioManager: round-trip counter overflow");
    }

    m_cash = nextCash;
    m_positions = std::move(nextPositions);
    m_totalFeesPaid = nextFees;
    m_tradeLog = std::move(nextTradeLog);
    ++m_tradeCount;
    if (remainingQuantity.isZero()) {
        ++m_roundTripCount;
    }
    commitValuation(valuation);
}

void PortfolioManager::updatePnL(const std::string& symbol, double currentPrice)
{
    const auto current = m_positions.find(symbol);
    if (current == m_positions.end()) {
        return;
    }
    const auto mark = Financial::price(currentPrice);
    if (!mark.has_value() || !mark->isPositive()) {
        throw std::invalid_argument("PortfolioManager: invalid mark price");
    }

    auto nextPositions = m_positions;
    nextPositions.at(symbol).m_lastMarkPrice = *mark;
    const Valuation valuation = valueState(m_cash, nextPositions);
    m_positions = std::move(nextPositions);
    commitValuation(valuation);
}

bool PortfolioManager::hasPosition(const std::string& symbol) const noexcept
{
    return m_positions.contains(symbol);
}

std::size_t PortfolioManager::getOpenPositionCount() const noexcept
{
    return m_positions.size();
}

double PortfolioManager::getCashBalance() const noexcept { return m_cash.toDouble(); }
double PortfolioManager::getTotalEquity() const noexcept { return m_totalEquity.toDouble(); }
double PortfolioManager::getCurrentDrawdown() const noexcept { return m_currentDrawdown; }

double PortfolioManager::getEntryPrice(const std::string& symbol) const noexcept
{
    const auto it = m_positions.find(symbol);
    return it == m_positions.end() ? 0.0 : it->second.m_averageEntryPrice.toDouble();
}

double PortfolioManager::getPositionQuantity(const std::string& symbol) const noexcept
{
    const auto it = m_positions.find(symbol);
    return it == m_positions.end() ? 0.0 : it->second.m_quantity.toDouble();
}

double PortfolioManager::getTotalFeesPaid() const noexcept
{
    return m_totalFeesPaid.toDouble();
}

double PortfolioManager::getMaxDrawdown() const noexcept { return m_maxDrawdown; }
int PortfolioManager::getTradeCount() const noexcept { return m_tradeCount; }
int PortfolioManager::getRoundTripCount() const noexcept { return m_roundTripCount; }

const std::vector<TradeRecord>& PortfolioManager::getTradeLog() const noexcept
{
    return m_tradeLog;
}

PortfolioManager::Snapshot PortfolioManager::snapshotState() const
{
    Snapshot snapshot;
    snapshot.cash = m_cash.toDouble();
    snapshot.unrealizedPnL = m_unrealizedPnL.toDouble();
    snapshot.totalEquity = m_totalEquity.toDouble();
    snapshot.maxEquity = m_maxEquity.toDouble();
    snapshot.currentDrawdown = m_currentDrawdown;
    snapshot.maxDrawdown = m_maxDrawdown;
    snapshot.tradeCount = m_tradeCount;
    snapshot.totalFeesPaid = m_totalFeesPaid.toDouble();
    snapshot.tradeLog = m_tradeLog;
    snapshot.roundTripCount = m_roundTripCount;
    snapshot.pendingOrders = m_pendingOrders;
    snapshot.nextOrderId = m_nextOrderId;
    snapshot.positions.reserve(m_positions.size());
    for (const auto& [symbol, state] : m_positions) {
        snapshot.positions.push_back({
            Position{symbol, state.m_quantity.toDouble(),
                     state.m_averageEntryPrice.toDouble(), true},
            state.m_entryTimestamp,
            state.m_entryFee.toDouble(),
            state.m_costBasis.toDouble(),
            state.m_lastMarkPrice.toDouble(),
            state.m_strategyId
        });
    }
    std::sort(snapshot.positions.begin(), snapshot.positions.end(),
              [](const PositionSnapshot& lhs, const PositionSnapshot& rhs) {
                  return lhs.position.symbol < rhs.position.symbol;
              });
    return snapshot;
}

void PortfolioManager::restoreState(const Snapshot& snapshot)
{
    const auto cash = Financial::money(snapshot.cash, Financial::Rounding::RejectUnaligned);
    const auto unrealized = Financial::money(
        snapshot.unrealizedPnL, Financial::Rounding::RejectUnaligned);
    const auto equity = Financial::money(
        snapshot.totalEquity, Financial::Rounding::RejectUnaligned);
    const auto maxEquity = Financial::money(
        snapshot.maxEquity, Financial::Rounding::RejectUnaligned);
    const auto fees = Financial::money(
        snapshot.totalFeesPaid, Financial::Rounding::RejectUnaligned);
    if (!cash.has_value() || cash->isNegative() || !unrealized.has_value()
        || !equity.has_value() || equity->isNegative()
        || !maxEquity.has_value() || maxEquity->isNegative()
        || !fees.has_value() || fees->isNegative()) {
        throw std::invalid_argument("PortfolioManager: incompatible accounting snapshot");
    }

    std::unordered_map<std::string, PositionState> positions;
    positions.reserve(snapshot.positions.size());
    for (const auto& persisted : snapshot.positions) {
        const auto quantity = Financial::quantity(
            persisted.position.quantity, Financial::Rounding::RejectUnaligned);
        const auto average = Financial::price(
            persisted.position.entryPrice, Financial::Rounding::RejectUnaligned);
        const auto entryFee = Financial::money(
            persisted.entryFee, Financial::Rounding::RejectUnaligned);
        const auto costBasis = Financial::money(
            persisted.costBasis, Financial::Rounding::RejectUnaligned);
        const auto lastMark = Financial::price(
            persisted.lastMarkPrice, Financial::Rounding::RejectUnaligned);
        const auto derivedAverage = costBasis.has_value() && quantity.has_value()
            ? Financial::averagePrice(*costBasis, *quantity)
            : std::nullopt;
        if (persisted.position.symbol.empty() || !persisted.position.isLong
            || !quantity.has_value() || !quantity->isPositive()
            || !average.has_value() || !average->isPositive()
            || !entryFee.has_value() || entryFee->isNegative()
            || !costBasis.has_value() || !costBasis->isPositive()
            || !lastMark.has_value() || !lastMark->isPositive()
            || !derivedAverage.has_value() || *derivedAverage != *average
            || !positions.emplace(persisted.position.symbol, PositionState{
                    *quantity, *average, *costBasis, *lastMark,
                    persisted.entryTimestamp, *entryFee, persisted.strategyId
                }).second) {
            throw std::invalid_argument("PortfolioManager: invalid position snapshot");
        }
    }

    PortfolioManager candidate;
    candidate.m_maxEquity = *maxEquity;
    candidate.m_maxDrawdown = snapshot.maxDrawdown;
    const Valuation valuation = candidate.valueState(*cash, positions);
    if (valuation.unrealizedPnL != *unrealized
        || valuation.totalEquity != *equity
        || valuation.maxEquity != *maxEquity
        || std::fabs(valuation.currentDrawdown - snapshot.currentDrawdown) > 1e-12
        || std::fabs(valuation.maxDrawdown - snapshot.maxDrawdown) > 1e-12) {
        throw std::invalid_argument("PortfolioManager: inconsistent accounting snapshot");
    }

    m_cash = *cash;
    m_positions = std::move(positions);
    m_unrealizedPnL = *unrealized;
    m_totalEquity = *equity;
    m_maxEquity = *maxEquity;
    m_currentDrawdown = snapshot.currentDrawdown;
    m_maxDrawdown = snapshot.maxDrawdown;
    m_tradeCount = snapshot.tradeCount;
    m_totalFeesPaid = *fees;
    m_tradeLog = snapshot.tradeLog;
    m_roundTripCount = snapshot.roundTripCount;
    m_pendingOrders = snapshot.pendingOrders;
    m_nextOrderId = snapshot.nextOrderId;
}

uint64_t PortfolioManager::placePendingOrder(const OrderRecord& order) noexcept
{
    if (order.symbol.empty() || order.quantity < 0.0 || order.capitalToCommit < 0.0
        || order.limitPrice < 0.0 || order.trailOffset < 0.0
        || !Financial::quantity(order.quantity).has_value()
        || !Financial::money(order.capitalToCommit).has_value()
        || !Financial::price(order.limitPrice).has_value()
        || !Financial::price(order.trailOffset).has_value()
        || !Financial::price(order.trailBest).has_value()
        || m_nextOrderId == std::numeric_limits<uint64_t>::max()) {
        return 0;
    }

    OrderRecord record = order;
    record.quantity = Financial::quantity(order.quantity)->toDouble();
    record.capitalToCommit = Financial::money(order.capitalToCommit)->toDouble();
    record.limitPrice = Financial::price(order.limitPrice)->toDouble();
    record.trailOffset = Financial::price(order.trailOffset)->toDouble();
    record.trailBest = Financial::price(order.trailBest)->toDouble();
    record.orderId = m_nextOrderId++;
    if (record.orderType == OrderType::TRAILING_STOP) {
        record.trailBest = record.limitPrice;
    }
    m_pendingOrders.push_back(record);
    return record.orderId;
}

bool PortfolioManager::cancelPendingOrder(uint64_t orderId) noexcept
{
    for (auto it = m_pendingOrders.begin(); it != m_pendingOrders.end(); ++it) {
        if (it->orderId == orderId) {
            m_pendingOrders.erase(it);
            return true;
        }
    }
    return false;
}

std::vector<OrderFillResult> PortfolioManager::evaluatePendingOrders(
    const std::string& symbol,
    double high, double low, double close,
    uint64_t timestamp) noexcept
{
    std::vector<OrderFillResult> fills;

    for (auto it = m_pendingOrders.begin(); it != m_pendingOrders.end(); ) {
        OrderRecord& order = *it;
        if (order.symbol != symbol) {
            ++it;
            continue;
        }

        bool triggered = false;
        double fillPrice = 0.0;
        switch (order.orderType) {
            case OrderType::LIMIT:
                if ((order.isBuy && low <= order.limitPrice)
                    || (!order.isBuy && high >= order.limitPrice)) {
                    triggered = true;
                    fillPrice = order.limitPrice;
                }
                break;
            case OrderType::STOP_LOSS:
                if (low <= order.limitPrice) {
                    triggered = true;
                    fillPrice = std::min(order.limitPrice, low);
                }
                break;
            case OrderType::TRAILING_STOP:
                if (close > order.trailBest) {
                    const auto best = Financial::price(close);
                    const auto stop = best.has_value()
                        ? Financial::price(best->toDouble() - order.trailOffset)
                        : std::nullopt;
                    if (best.has_value() && best->isPositive()
                        && stop.has_value() && stop->isPositive()) {
                        order.trailBest = best->toDouble();
                        order.limitPrice = stop->toDouble();
                    }
                }
                if (low <= order.limitPrice) {
                    triggered = true;
                    fillPrice = std::min(order.limitPrice, low);
                }
                break;
            case OrderType::MARKET:
                triggered = true;
                fillPrice = close;
                break;
        }

        const auto normalizedFill = Financial::price(fillPrice);
        if (triggered && normalizedFill.has_value() && normalizedFill->isPositive()) {
            fills.push_back(OrderFillResult{
                order.orderId,
                true,
                order.symbol,
                order.orderType,
                order.isBuy,
                normalizedFill->toDouble(),
                order.quantity,
                order.capitalToCommit,
                timestamp
            });
            it = m_pendingOrders.erase(it);
        } else {
            ++it;
        }
    }
    return fills;
}
