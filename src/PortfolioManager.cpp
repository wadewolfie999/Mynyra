#include "PortfolioManager.hpp"
#include <algorithm>
#include <stdexcept>
#include <cmath>

void PortfolioManager::openLong(const std::string& symbol, double netPrice,
                                uint64_t timestamp, double fee,
                                double capitalToCommit,
                                double explicitUnits,
                                const std::string& strategyId)
{
    if (m_positions.count(symbol) && m_positions.at(symbol).m_hasPosition) {
        throw std::logic_error(
            "PortfolioManager: cannot open a new position for '" + symbol +
            "' while one is already open");
    }
    if (netPrice <= 0.0) {
        throw std::invalid_argument("PortfolioManager: price must be positive");
    }

    m_totalFeesPaid += fee;

    PositionState& state = m_positions[symbol];
    state.m_entryFee       = fee;
    state.m_entryTimestamp = timestamp;
    state.m_hasPosition    = true;
    state.m_strategyId     = strategyId;

    state.m_position.symbol     = symbol;
    state.m_position.isLong     = true;
    state.m_position.entryPrice = netPrice;
    // Capital slice: 0.0 means "use all remaining cash" (single-asset behaviour).
    const double capital = (capitalToCommit > 0.0 && capitalToCommit <= m_cash)
                           ? capitalToCommit
                           : m_cash;
    // ATR-based sizing: use explicit units when provided; otherwise derive from capital.
    state.m_position.quantity   = (explicitUnits > 0.0)
                                  ? explicitUnits
                                  : (capital - fee) / netPrice;
    m_cash                     -= capital;

    ++m_tradeCount;
}

void PortfolioManager::addToLong(const std::string& symbol, double netPrice,
                                 double quantity, double fee,
                                 double capitalToCommit,
                                 const std::string& strategyId)
{
    auto it = m_positions.find(symbol);
    if (it == m_positions.end() || !it->second.m_hasPosition) {
        // If there is no existing long, promote this call to an openLong.
        const double capital = (capitalToCommit > 0.0)
                               ? capitalToCommit
                               : (quantity * netPrice + fee);
        openLong(symbol, netPrice, /*timestamp=*/0, fee, capital, quantity, strategyId);
        return;
    }
    if (netPrice <= 0.0 || quantity <= 0.0) {
        throw std::invalid_argument("PortfolioManager::addToLong requires positive price and quantity");
    }

    PositionState& state = it->second;
    const double currentQty = state.m_position.quantity;
    const double newQty     = currentQty + quantity;
    if (newQty <= 0.0) {
        throw std::logic_error("PortfolioManager::addToLong invalid resulting quantity");
    }

    const double capital = (capitalToCommit > 0.0)
                           ? capitalToCommit
                           : (quantity * netPrice + fee);
    if (capital > m_cash) {
        throw std::logic_error("PortfolioManager::addToLong insufficient cash for add-on fill");
    }

    const double weightedEntry = ((state.m_position.entryPrice * currentQty)
                                + (netPrice * quantity)) / newQty;

    m_totalFeesPaid += fee;
    state.m_position.entryPrice = weightedEntry;
    state.m_position.quantity   = newQty;
    if (!strategyId.empty()) {
        state.m_strategyId = strategyId;
    }
    m_cash -= capital;
    ++m_tradeCount;
}

void PortfolioManager::closePosition(const std::string& symbol, double netPrice,
                                     uint64_t timestamp, double fee,
                                     const std::string& strategyId,
                                     double explicitQuantity)
{
    auto it = m_positions.find(symbol);
    if (it == m_positions.end() || !it->second.m_hasPosition) {
        return;
    }
    if (netPrice <= 0.0) {
        throw std::invalid_argument("PortfolioManager: price must be positive");
    }

    m_totalFeesPaid += fee;

    PositionState& state = it->second;
    const double currentQty    = state.m_position.quantity;
    const double closeQty = (explicitQuantity > 0.0 && explicitQuantity < currentQty)
                            ? explicitQuantity
                            : currentQty;
    const double entryFeeAllocated = (currentQty > 0.0)
                                     ? state.m_entryFee * (closeQty / currentQty)
                                     : state.m_entryFee;

    const double grossProceeds = closeQty * netPrice;
    const double netProceeds   = grossProceeds - fee;
    const double costBasis     = closeQty * state.m_position.entryPrice;
    const double grossPnL      = grossProceeds - costBasis;
    const double realizedPnL   = netProceeds - costBasis - entryFeeAllocated;

    TradeRecord rec;
    rec.symbol         = symbol;
    rec.openTimestamp  = state.m_entryTimestamp;
    rec.closeTimestamp = timestamp;
    rec.entryPrice     = state.m_position.entryPrice;
    rec.exitPrice      = netPrice;
    rec.quantity       = closeQty;
    rec.totalFees      = fee + entryFeeAllocated;
    rec.grossPnL       = grossPnL;
    rec.realizedPnL    = realizedPnL;
    // Attribute to the strategy that opened the position (or override if provided).
    rec.strategy_id    = !strategyId.empty() ? strategyId : state.m_strategyId;
    m_tradeLog.push_back(rec);

    m_cash             += netProceeds;
    state.m_position.quantity -= closeQty;
    state.m_entryFee          -= entryFeeAllocated;

    const bool fullyClosed = (state.m_position.quantity <= 1e-12);
    if (fullyClosed) {
        state.m_hasPosition = false;
        state.m_position    = {};
        state.m_entryFee    = 0.0;
        state.m_entryTimestamp = 0;
        ++m_roundTripCount;
    }

    m_unrealizedPnL = 0.0;
    ++m_tradeCount;

    // Recompute total equity: cash + MTM of all still-open positions (at cost basis,
    // since we don't have current prices here — updatePnL will refine on next candle).
    double mtmSum = 0.0;
    for (const auto& [sym, st] : m_positions) {
        if (!st.m_hasPosition) { continue; }
        mtmSum += st.m_position.quantity * st.m_position.entryPrice;
    }
    m_totalEquity = m_cash + mtmSum;

    if (m_totalEquity > m_maxEquity) {
        m_maxEquity = m_totalEquity;
    }
    if (m_maxEquity > 0.0) {
        m_currentDrawdown = (m_maxEquity - m_totalEquity) / m_maxEquity;
    }
    m_maxDrawdown = std::max(m_maxDrawdown, m_currentDrawdown);
}

void PortfolioManager::updatePnL(const std::string& symbol, double currentPrice)
{
    auto it = m_positions.find(symbol);
    if (it != m_positions.end() && it->second.m_hasPosition && currentPrice > 0.0) {
        const PositionState& state = it->second;
        m_unrealizedPnL = (currentPrice - state.m_position.entryPrice)
                          * state.m_position.quantity;
    } else {
        m_unrealizedPnL = 0.0;
    }
    // Re-compute total equity as cash + MTM of ALL open positions.
    // For the symbol being updated, use the supplied currentPrice.
    // For all other open positions, use their last known entry price as a
    // conservative cost-basis proxy (exact live prices arrive on their own candles).
    double mtmSum = 0.0;
    for (const auto& [sym, state] : m_positions) {
        if (!state.m_hasPosition) { continue; }
        double price = (sym == symbol && currentPrice > 0.0)
                       ? currentPrice
                       : state.m_position.entryPrice;
        mtmSum += state.m_position.quantity * price;
    }
    m_totalEquity = m_cash + mtmSum;

    if (m_totalEquity > m_maxEquity) {
        m_maxEquity = m_totalEquity;
    }
    if (m_maxEquity > 0.0) {
        m_currentDrawdown = (m_maxEquity - m_totalEquity) / m_maxEquity;
    }
    m_maxDrawdown = std::max(m_maxDrawdown, m_currentDrawdown);
}

bool PortfolioManager::hasPosition(const std::string& symbol) const noexcept
{
    auto it = m_positions.find(symbol);
    return it != m_positions.end() && it->second.m_hasPosition;
}

std::size_t PortfolioManager::getOpenPositionCount() const noexcept
{
    std::size_t count = 0;
    for (const auto& [sym, state] : m_positions) {
        if (state.m_hasPosition) { ++count; }
    }
    return count;
}

double PortfolioManager::getCashBalance()      const noexcept { return m_cash; }
double PortfolioManager::getTotalEquity()      const noexcept { return m_totalEquity; }
double PortfolioManager::getCurrentDrawdown()  const noexcept { return m_currentDrawdown; }

double PortfolioManager::getEntryPrice(const std::string& symbol) const noexcept
{
    auto it = m_positions.find(symbol);
    if (it != m_positions.end() && it->second.m_hasPosition) {
        return it->second.m_position.entryPrice;
    }
    return 0.0;
}

double PortfolioManager::getPositionQuantity(const std::string& symbol) const noexcept
{
    auto it = m_positions.find(symbol);
    if (it != m_positions.end() && it->second.m_hasPosition) {
        return it->second.m_position.quantity;
    }
    return 0.0;
}

double PortfolioManager::getTotalFeesPaid()    const noexcept { return m_totalFeesPaid; }
double PortfolioManager::getMaxDrawdown()      const noexcept { return m_maxDrawdown; }
int    PortfolioManager::getTradeCount()       const noexcept { return m_tradeCount; }
int    PortfolioManager::getRoundTripCount()   const noexcept { return m_roundTripCount; }

const std::vector<TradeRecord>& PortfolioManager::getTradeLog() const noexcept
{
    return m_tradeLog;
}

PortfolioManager::Snapshot PortfolioManager::snapshotState() const
{
    Snapshot snapshot;
    snapshot.cash = m_cash;
    snapshot.unrealizedPnL = m_unrealizedPnL;
    snapshot.totalEquity = m_totalEquity;
    snapshot.maxEquity = m_maxEquity;
    snapshot.currentDrawdown = m_currentDrawdown;
    snapshot.maxDrawdown = m_maxDrawdown;
    snapshot.tradeCount = m_tradeCount;
    snapshot.totalFeesPaid = m_totalFeesPaid;
    snapshot.tradeLog = m_tradeLog;
    snapshot.roundTripCount = m_roundTripCount;
    snapshot.pendingOrders = m_pendingOrders;
    snapshot.nextOrderId = m_nextOrderId;
    snapshot.positions.reserve(m_positions.size());
    for (const auto& [symbol, state] : m_positions) {
        if (!state.m_hasPosition) { continue; }
        snapshot.positions.push_back({state.m_position, state.m_entryTimestamp,
                                      state.m_entryFee, state.m_strategyId});
    }
    std::sort(snapshot.positions.begin(), snapshot.positions.end(),
              [](const PositionSnapshot& lhs, const PositionSnapshot& rhs) {
                  return lhs.position.symbol < rhs.position.symbol;
              });
    return snapshot;
}

void PortfolioManager::restoreState(const Snapshot& snapshot)
{
    std::unordered_map<std::string, PositionState> positions;
    positions.reserve(snapshot.positions.size());
    for (const auto& persisted : snapshot.positions) {
        PositionState state;
        state.m_position = persisted.position;
        state.m_hasPosition = true;
        state.m_entryTimestamp = persisted.entryTimestamp;
        state.m_entryFee = persisted.entryFee;
        state.m_strategyId = persisted.strategyId;
        positions.emplace(persisted.position.symbol, std::move(state));
    }

    m_cash = snapshot.cash;
    m_positions = std::move(positions);
    m_unrealizedPnL = snapshot.unrealizedPnL;
    m_totalEquity = snapshot.totalEquity;
    m_maxEquity = snapshot.maxEquity;
    m_currentDrawdown = snapshot.currentDrawdown;
    m_maxDrawdown = snapshot.maxDrawdown;
    m_tradeCount = snapshot.tradeCount;
    m_totalFeesPaid = snapshot.totalFeesPaid;
    m_tradeLog = snapshot.tradeLog;
    m_roundTripCount = snapshot.roundTripCount;
    m_pendingOrders = snapshot.pendingOrders;
    m_nextOrderId = snapshot.nextOrderId;
}

// ── Phase 9: Pending Order Queue ─────────────────────────────────────────────

uint64_t PortfolioManager::placePendingOrder(const OrderRecord& order) noexcept
{
    OrderRecord rec   = order;
    rec.orderId       = m_nextOrderId++;
    // Initialise trailing-stop best price to limitPrice at placement.
    if (rec.orderType == OrderType::TRAILING_STOP) {
        rec.trailBest = rec.limitPrice;
    }
    m_pendingOrders.push_back(rec);
    return rec.orderId;
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
        OrderRecord& ord = *it;
        if (ord.symbol != symbol) {
            ++it;
            continue;
        }

        bool triggered = false;
        double fillPrice = 0.0;

        switch (ord.orderType) {
            case OrderType::LIMIT: {
                if (ord.isBuy) {
                    // Buy limit: triggers when the candle low touches the limit.
                    if (low <= ord.limitPrice) {
                        triggered = true;
                        fillPrice = ord.limitPrice; // assume fill at limit
                    }
                } else {
                    // Sell limit: triggers when the candle high reaches the limit.
                    if (high >= ord.limitPrice) {
                        triggered = true;
                        fillPrice = ord.limitPrice;
                    }
                }
                break;
            }
            case OrderType::STOP_LOSS: {
                // Stop-loss is always a sell: triggers when low drops to/below stop.
                if (low <= ord.limitPrice) {
                    triggered = true;
                    // Worst-case fill: candle may gap below; use min(stop, low).
                    fillPrice = std::min(ord.limitPrice, low);
                }
                break;
            }
            case OrderType::TRAILING_STOP: {
                // Advance the trail best on new highs.
                if (close > ord.trailBest) {
                    ord.trailBest  = close;
                    ord.limitPrice = ord.trailBest - ord.trailOffset;
                }
                // Trigger when low drops to/below the dynamic stop.
                if (low <= ord.limitPrice) {
                    triggered = true;
                    fillPrice = std::min(ord.limitPrice, low);
                }
                break;
            }
            case OrderType::MARKET:
                // Market orders should not be in the pending queue.
                triggered = true;
                fillPrice = close;
                break;
        }

        if (triggered) {
            OrderFillResult res;
            res.orderId         = ord.orderId;
            res.filled          = true;
            res.symbol          = ord.symbol;
            res.orderType       = ord.orderType;
            res.isBuy           = ord.isBuy;
            res.fillPrice       = fillPrice;
            res.quantity        = ord.quantity;
            res.capitalToCommit = ord.capitalToCommit;
            res.fillTimestamp   = timestamp;
            fills.push_back(res);
            it = m_pendingOrders.erase(it);
        } else {
            ++it;
        }
    }
    return fills;
}
