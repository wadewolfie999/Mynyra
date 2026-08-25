#include "IPortfolioView.hpp"

#include "PortfolioManager.hpp"

#include <algorithm>
#include <limits>

namespace {

Decimal64 decimalOrZero(double value, std::uint8_t scale) noexcept
{
    return Decimal64::fromDouble(value, scale,
                                 DecimalRounding::NearestTiesAwayFromZero)
        .value_or(Decimal64{0, scale});
}

bool quantitiesValid(const OrderIntent& intent,
                     Decimal64 previousFilled,
                     const ExecutionEvent& event) noexcept
{
    return event.cumulativeFilledQuantity.scale
               == intent.exactQuantity.scale
        && event.remainingQuantity.scale == intent.exactQuantity.scale
        && event.cumulativeFilledQuantity.units >= 0
        && event.remainingQuantity.units >= 0
        && event.cumulativeFilledQuantity.units
               <= intent.exactQuantity.units
        && event.remainingQuantity.units
               <= intent.exactQuantity.units
        && event.cumulativeFilledQuantity.units
               <= std::numeric_limits<std::int64_t>::max()
                    - event.remainingQuantity.units
        && event.cumulativeFilledQuantity.units
               + event.remainingQuantity.units
               == intent.exactQuantity.units
        && event.cumulativeFilledQuantity.units
               > previousFilled.units;
}

} // namespace

LocalPortfolioView::LocalPortfolioView(
    const PortfolioManager& portfolio) noexcept
    : m_portfolio(portfolio)
{}

AccountSnapshot LocalPortfolioView::account() const
{
    AccountSnapshot result;
    result.snapshotVersion = 0;
    result.balance = decimalOrZero(m_portfolio.getCashBalance(), 8);
    result.equity = decimalOrZero(m_portfolio.getTotalEquity(), 8);
    result.freeMargin = result.balance;
    result.currency = "LOCAL";
    result.complete = true;
    return result;
}

std::vector<PositionSnapshot> LocalPortfolioView::positions() const
{
    const auto state = m_portfolio.snapshotState();
    std::vector<PositionSnapshot> result;
    result.reserve(state.positions.size());
    for (const auto& local : state.positions) {
        PositionSnapshot position;
        position.canonicalSymbol = local.position.symbol;
        position.quantity = decimalOrZero(local.position.quantity, 8);
        position.averagePrice = decimalOrZero(local.position.entryPrice, 8);
        position.logicalPositionId = "local-" + local.position.symbol;
        position.side = local.position.isLong
            ? PositionSide::Long : PositionSide::Short;
        result.push_back(std::move(position));
    }
    return result;
}

std::size_t LocalPortfolioView::pendingOrderCount() const noexcept
{
    try {
        return m_portfolio.snapshotState().pendingOrders.size();
    } catch (...) {
        return std::numeric_limits<std::size_t>::max();
    }
}

bool BrokerPortfolioMirror::registerIntent(
    std::uint64_t localOrderId, const OrderIntent& intent)
{
    if (localOrderId == 0 || intent.canonicalSymbol.empty()
        || !intent.exactQuantity.isPositive()
        || !intent.referencePrice.isPositive()
        || (intent.effect == PositionEffect::Close
            && (!intent.logicalPositionId.has_value()
                || intent.logicalPositionId->empty()))) {
        return false;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    IntentState state;
    state.intent = intent;
    state.cumulativeFilled = Decimal64{0, intent.exactQuantity.scale};
    return m_intents.emplace(localOrderId, std::move(state)).second;
}

MirrorApplyResult BrokerPortfolioMirror::applyExecution(
    const ExecutionEvent& event)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (event.eventKey.empty()) return MirrorApplyResult::InvalidEvent;
    if (m_eventKeys.contains(event.eventKey)) return MirrorApplyResult::Duplicate;
    const auto stateIt = m_intents.find(event.localOrderId);
    if (stateIt == m_intents.end()) return MirrorApplyResult::UnknownOrder;
    IntentState& state = stateIt->second;
    if (event.positionSide != state.intent.side
        || event.positionEffect != state.intent.effect
        || !event.fillPrice.isPositive()
        || (state.lastSequence > 0 && event.sequence > 0
            && event.sequence <= state.lastSequence)
        || (state.lastTimestampNs > 0 && event.timestampNs > 0
            && event.timestampNs < state.lastTimestampNs)
        || !quantitiesValid(state.intent, state.cumulativeFilled, event)) {
        return MirrorApplyResult::InvalidEvent;
    }

    const Decimal64 delta{
        event.cumulativeFilledQuantity.units - state.cumulativeFilled.units,
        state.intent.exactQuantity.scale};
    const std::string key = state.intent.logicalPositionId.value_or(
        provisionalPositionId(event.localOrderId));

    if (state.intent.effect == PositionEffect::Open) {
        auto& position = m_positions[key];
        if (!position.logicalPositionId.empty()
            && (position.canonicalSymbol != state.intent.canonicalSymbol
                || position.side != state.intent.side
                || position.quantity.scale != delta.scale)) {
            return MirrorApplyResult::InvalidTransition;
        }
        position.canonicalSymbol = state.intent.canonicalSymbol;
        position.logicalPositionId = key;
        position.side = state.intent.side;
        position.averagePrice = event.fillPrice;
        position.quantity.scale = delta.scale;
        if (position.quantity.units
            > std::numeric_limits<std::int64_t>::max() - delta.units) {
            return MirrorApplyResult::InvalidEvent;
        }
        position.quantity.units += delta.units;
    } else {
        const auto positionIt = m_positions.find(key);
        if (positionIt == m_positions.end()
            || positionIt->second.canonicalSymbol != state.intent.canonicalSymbol
            || positionIt->second.side != state.intent.side
            || positionIt->second.quantity.scale != delta.scale
            || delta.units > positionIt->second.quantity.units) {
            return MirrorApplyResult::InvalidTransition;
        }
        positionIt->second.quantity.units -= delta.units;
        if (positionIt->second.quantity.isZero()) m_positions.erase(positionIt);
    }

    state.cumulativeFilled = event.cumulativeFilledQuantity;
    state.lastTimestampNs = event.timestampNs;
    state.lastSequence = event.sequence;
    m_eventKeys.insert(event.eventKey);
    return MirrorApplyResult::Applied;
}

bool BrokerPortfolioMirror::applyReconciliation(
    const ReconciliationSnapshot& snapshot)
{
    if (!snapshot.complete || !snapshot.account.complete
        || snapshot.status != ReconciliationStatus::Matched
        || snapshot.connectionGeneration == 0
        || snapshot.snapshotVersion == 0
        || snapshot.account.snapshotVersion != snapshot.snapshotVersion) {
        return false;
    }
    std::unordered_map<std::string, PositionSnapshot> replacement;
    for (const auto& position : snapshot.positions) {
        if (position.canonicalSymbol.empty()
            || position.logicalPositionId.empty()
            || !position.quantity.isPositive()
            || !position.averagePrice.isPositive()
            || !replacement.emplace(position.logicalPositionId, position).second) {
            return false;
        }
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_generation > snapshot.connectionGeneration
        || (m_generation == snapshot.connectionGeneration
            && m_account.snapshotVersion > 0
            && snapshot.snapshotVersion <= m_account.snapshotVersion)) {
        return false;
    }
    m_account = snapshot.account;
    m_positions = std::move(replacement);
    m_pendingOrders = snapshot.pendingOrderCount;
    m_generation = snapshot.connectionGeneration;
    return true;
}

AccountSnapshot BrokerPortfolioMirror::account() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_account;
}

std::vector<PositionSnapshot> BrokerPortfolioMirror::positions() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<PositionSnapshot> result;
    result.reserve(m_positions.size());
    for (const auto& [_, position] : m_positions) result.push_back(position);
    std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) {
        return left.logicalPositionId < right.logicalPositionId;
    });
    return result;
}

std::size_t BrokerPortfolioMirror::pendingOrderCount() const noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pendingOrders;
}

std::uint64_t BrokerPortfolioMirror::generation() const noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_generation;
}

std::optional<OrderIntent> BrokerPortfolioMirror::intent(
    std::uint64_t localOrderId) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto found = m_intents.find(localOrderId);
    return found == m_intents.end()
        ? std::nullopt : std::optional<OrderIntent>(found->second.intent);
}

bool BrokerPortfolioMirror::appliedEvent(std::string_view eventKey) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_eventKeys.contains(std::string(eventKey));
}

bool BrokerPortfolioMirror::isFlat() const noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_positions.empty() && m_pendingOrders == 0;
}

std::string BrokerPortfolioMirror::provisionalPositionId(
    std::uint64_t localOrderId)
{
    return "local-order-" + std::to_string(localOrderId);
}
