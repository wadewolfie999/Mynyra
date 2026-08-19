#include "BrokerGateway.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace {

std::optional<Decimal64> rescaleTowardZero(Decimal64 value,
                                           std::uint8_t targetScale) noexcept
{
    if (value.scale > Decimal64::MAX_SCALE || targetScale > Decimal64::MAX_SCALE) {
        return std::nullopt;
    }
    std::int64_t units = value.units;
    while (value.scale < targetScale) {
        if (units > std::numeric_limits<std::int64_t>::max() / 10
            || units < std::numeric_limits<std::int64_t>::min() / 10) {
            return std::nullopt;
        }
        units *= 10;
        ++value.scale;
    }
    while (value.scale > targetScale) {
        units /= 10;
        --value.scale;
    }
    return Decimal64{units, targetScale};
}

std::optional<Decimal64> normalizeToStep(Decimal64 value,
                                         Decimal64 step) noexcept
{
    if (!value.isPositive() || !step.isPositive()) {
        return std::nullopt;
    }
    const auto scaled = rescaleTowardZero(value, step.scale);
    if (!scaled.has_value()) {
        return std::nullopt;
    }
    return Decimal64{
        (scaled->units / step.units) * step.units,
        step.scale
    };
}

InstrumentSpec defaultInstrument(const OrderRequest& request)
{
    const auto scale = request.quantity.scale;
    InstrumentSpec spec;
    spec.version = 1;
    spec.canonicalSymbol = request.canonicalSymbol;
    spec.executionAlias = request.canonicalSymbol;
    spec.tickSize = Decimal64{1, 8};
    spec.contractSize = Decimal64{1, 0};
    spec.minimumQuantity = Decimal64{1, scale};
    spec.maximumQuantity = Decimal64{std::numeric_limits<std::int64_t>::max(), scale};
    spec.quantityStep = Decimal64{1, scale};
    spec.complete = true;
    return spec;
}

} // namespace

BrokerGateway::BrokerGateway(const SystemConfig& config,
                             PortfolioManager& portfolio)
    : m_config(config)
    , m_portfolio(portfolio)
{
    if (m_config.isPaperMode()) {
        m_adapter = std::make_unique<DeterministicBrokerAdapter>();
    }
    bindAdapterCallbacks();
}

BrokerGateway::BrokerGateway(const SystemConfig& config,
                             PortfolioManager& portfolio,
                             std::unique_ptr<IBrokerAdapter> adapter,
                             bool liveAdapterApproved)
    : m_config(config)
    , m_portfolio(portfolio)
    , m_adapter(std::move(adapter))
    , m_liveAdapterApproved(liveAdapterApproved)
{
    bindAdapterCallbacks();
}

BrokerGateway::~BrokerGateway()
{
    disconnect();
}

void BrokerGateway::connect()
{
    if (!adapterAuthorized() || !m_adapter) {
        handleHealth(AdapterHealthEvent{
            1, AdapterHealthState::Disconnected, 0, 0, 0,
            FailureCategory::Validation,
            m_config.isLiveMode()
                ? "LIVE adapter is not separately approved"
                : "adapter unavailable in BACKTEST",
            "gateway-fail-closed"});
        return;
    }
    if (!m_adapter->connect()) {
        handleHealth(AdapterHealthEvent{
            1, AdapterHealthState::Disconnected, 0, 0, 0,
            FailureCategory::Transport, "adapter connection failed",
            "gateway-connect-failed"});
    }
}

void BrokerGateway::disconnect() noexcept
{
    if (m_adapter) {
        m_adapter->disconnect();
    }
}

bool BrokerGateway::isConnected() const noexcept
{
    return adapterAuthorized() && m_adapter && m_adapter->isConnected();
}

void BrokerGateway::setSymbolAlias(std::string canonicalSymbol,
                                   std::string executionAlias)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_symbolAliases[std::move(canonicalSymbol)] = std::move(executionAlias);
}

void BrokerGateway::setInstrumentSpec(const InstrumentSpec& spec)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_instrumentSpecs[spec.canonicalSymbol] = spec;
    if (auto* deterministic = deterministicAdapter()) {
        deterministic->setInstrumentSpec(spec);
    }
}

std::optional<NormalizedOrder> BrokerGateway::normalizeOrder(
    const OrderRequest& request,
    std::string* rejectionReason) const
{
    const auto reject = [rejectionReason](const std::string& reason)
        -> std::optional<NormalizedOrder> {
        if (rejectionReason) {
            *rejectionReason = reason;
        }
        return std::nullopt;
    };

    if (request.localOrderId == 0 || request.canonicalSymbol.empty()
        || !request.quantity.isPositive() || request.idempotencyKey.empty()) {
        return reject("invalid broker-neutral order request");
    }
    if (request.type == BrokerOrderType::Unknown
        || (request.type == BrokerOrderType::Market
            && !request.referencePrice.has_value())
        || (request.type == BrokerOrderType::Limit
            && !request.limitPrice.has_value())
        || (request.type == BrokerOrderType::Stop
            && !request.stopPrice.has_value())) {
        return reject("order type lacks required price evidence");
    }

    InstrumentSpec spec;
    std::string alias;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const auto specIt = m_instrumentSpecs.find(request.canonicalSymbol);
        spec = specIt == m_instrumentSpecs.end()
            ? defaultInstrument(request) : specIt->second;
        const auto aliasIt = m_symbolAliases.find(request.canonicalSymbol);
        alias = aliasIt == m_symbolAliases.end()
            ? spec.executionAlias : aliasIt->second;
    }

    if (!spec.complete || !spec.quantityStep.isPositive()) {
        return reject("instrument metadata is incomplete");
    }
    const auto normalizedQuantity = normalizeToStep(request.quantity,
                                                    spec.quantityStep);
    if (!normalizedQuantity.has_value() || !normalizedQuantity->isPositive()) {
        return reject("quantity normalizes to zero or is not representable");
    }
    const auto minimum = rescaleTowardZero(spec.minimumQuantity,
                                           normalizedQuantity->scale);
    const auto maximum = rescaleTowardZero(spec.maximumQuantity,
                                           normalizedQuantity->scale);
    if (!minimum.has_value() || !maximum.has_value()
        || normalizedQuantity->units < minimum->units
        || normalizedQuantity->units > maximum->units) {
        return reject("normalized quantity violates instrument bounds");
    }

    NormalizedOrder normalized;
    normalized.request = request;
    normalized.executionSymbol = alias.empty() ? request.canonicalSymbol : alias;
    normalized.normalizedQuantity = *normalizedQuantity;
    normalized.instrumentVersion = spec.version;

    const auto normalizePrice = [&spec](const std::optional<Decimal64>& price)
        -> std::optional<Decimal64> {
        if (!price.has_value()) {
            return std::nullopt;
        }
        return normalizeToStep(*price, spec.tickSize);
    };
    normalized.normalizedLimitPrice = normalizePrice(request.limitPrice);
    normalized.normalizedStopPrice = normalizePrice(request.stopPrice);
    normalized.normalizedReferencePrice = normalizePrice(request.referencePrice);
    if ((request.limitPrice.has_value() && !normalized.normalizedLimitPrice.has_value())
        || (request.stopPrice.has_value() && !normalized.normalizedStopPrice.has_value())
        || (request.referencePrice.has_value()
            && !normalized.normalizedReferencePrice.has_value())) {
        return reject("price is not representable for instrument tick size");
    }
    return normalized;
}

GatewayDispatchResult BrokerGateway::dispatchOrder(
    const NormalizedOrder& order,
    const RiskDecision& finalRiskDecision)
{
    GatewayDispatchResult result;
    result.localOrderId = order.request.localOrderId;
    if (!finalRiskDecision.allowed) {
        result.failure = finalRiskDecision.failure == FailureCategory::None
            ? FailureCategory::Validation : finalRiskDecision.failure;
        result.reason = finalRiskDecision.reason.empty()
            ? "final quantity-aware risk decision rejected dispatch"
            : finalRiskDecision.reason;
        return result;
    }
    if (!isConnected()) {
        result.failure = FailureCategory::Transport;
        result.reason = m_config.isLiveMode()
            ? "LIVE dispatch blocked: adapter not approved or connected"
            : "adapter not connected";
        ++m_totalApiErrors;
        return result;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        OrderRequest lifecycleRequest = order.request;
        lifecycleRequest.quantity = order.normalizedQuantity;
        lifecycleRequest.limitPrice = order.normalizedLimitPrice;
        lifecycleRequest.stopPrice = order.normalizedStopPrice;
        lifecycleRequest.referencePrice = order.normalizedReferencePrice;
        if (!m_lifecycle.create(lifecycleRequest)) {
            result.failure = FailureCategory::Validation;
            result.reason = "duplicate or invalid order identity";
            ++m_totalApiErrors;
            return result;
        }
        const auto baseSequence = order.request.sequence;
        if (m_lifecycle.markRiskChecked(order.request.localOrderId,
                                        "gateway-risk-" + std::to_string(order.request.localOrderId),
                                        order.request.timestampNs,
                                        baseSequence + 1)
                != LifecycleApplyResult::Applied
            || m_lifecycle.markSubmitted(order.request.localOrderId,
                                         "gateway-submit-" + std::to_string(order.request.localOrderId),
                                         order.request.timestampNs,
                                         baseSequence + 2)
                != LifecycleApplyResult::Applied) {
            result.failure = FailureCategory::Validation;
            result.reason = "lifecycle dispatch transition failed";
            ++m_totalApiErrors;
            return result;
        }
    }

    ++m_totalOrdersSubmitted;
    if (!m_adapter->submit(order)) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_lifecycle.markUnknown(order.request.localOrderId,
                                "gateway-dispatch-failed-"
                                    + std::to_string(order.request.localOrderId),
                                FailureCategory::Transport,
                                "adapter refused dispatch",
                                order.request.timestampNs,
                                order.request.sequence + 3);
        result.failure = FailureCategory::Transport;
        result.reason = "adapter refused dispatch";
        ++m_totalApiErrors;
        return result;
    }

    result.dispatched = true;
    return result;
}

bool BrokerGateway::requestCancel(const CancelRequest& request)
{
    if (!isConnected()) {
        return false;
    }
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_lifecycle.requestCancel(request) != LifecycleApplyResult::Applied) {
            return false;
        }
    }
    if (m_adapter->cancel(request)) {
        return true;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lifecycle.markUnknown(
        request.localOrderId,
        "gateway-cancel-dispatch-failed-" + std::to_string(request.localOrderId),
        FailureCategory::Transport,
        "adapter refused cancel dispatch",
        request.timestampNs,
        request.sequence + 1);
    return false;
}

ReconciliationSnapshot BrokerGateway::reconciliationSnapshot(
    std::uint64_t timestampNs)
{
    ++m_reconciliationCount;
    if (!isConnected()) {
        ReconciliationSnapshot unavailable;
        unavailable.timestampNs = timestampNs;
        unavailable.status = ReconciliationStatus::Unsupported;
        return unavailable;
    }
    auto snapshot = m_adapter->reconcile(timestampNs);
    if (snapshot.account.complete) {
        const double cashDelta = std::fabs(
            m_portfolio.getCashBalance() - snapshot.account.balance.toDouble());
        const double equityDelta = std::fabs(
            m_portfolio.getTotalEquity() - snapshot.account.equity.toDouble());
        if (cashDelta > 0.01 || equityDelta > 0.01) {
            snapshot.status = ReconciliationStatus::PriceOrCostMismatch;
        }
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lastReconciliation = snapshot;
    return snapshot;
}

std::optional<OrderLifecycleRecord> BrokerGateway::orderLifecycle(
    std::uint64_t localOrderId) const noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto* record = m_lifecycle.find(localOrderId);
    return record ? std::optional<OrderLifecycleRecord>(*record) : std::nullopt;
}

AdapterHealthEvent BrokerGateway::adapterHealth() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_health;
}

void BrokerGateway::setAcknowledgementCallback(
    AcknowledgementCallback callback) noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_acknowledgementCallback = std::move(callback);
}

void BrokerGateway::setExecutionCallback(ExecutionCallback callback) noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_executionCallback = std::move(callback);
}

void BrokerGateway::addExecutionCallback(ExecutionCallback callback) noexcept
{
    if (!callback) {
        return;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    m_executionCallbacks.push_back(std::move(callback));
}

void BrokerGateway::setCancelCallback(CancelCallback callback) noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cancelCallback = std::move(callback);
}

void BrokerGateway::setHealthCallback(HealthCallback callback) noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_healthCallback = std::move(callback);
}

void BrokerGateway::simulateExecutionEvent(const ExecutionEvent& execution)
{
    handleExecution(execution);
}

void BrokerGateway::injectNextOrderError(const std::string& errorMessage) noexcept
{
    if (auto* deterministic = deterministicAdapter()) {
        deterministic->injectNextError(errorMessage);
    }
}

void BrokerGateway::injectNextPartialFill(double fillRatio) noexcept
{
    if (auto* deterministic = deterministicAdapter()) {
        deterministic->injectNextPartialFill(fillRatio);
    }
}

bool BrokerGateway::setPaperSimulationCosts(double feeRate,
                                            double slippageBps) noexcept
{
    auto* deterministic = deterministicAdapter();
    return deterministic != nullptr
        && deterministic->setSimulationCosts(feeRate, slippageBps);
}

void BrokerGateway::setFaultInjectorConfig(
    const FaultInjectorConfig& config) noexcept
{
    if (auto* deterministic = deterministicAdapter()) {
        deterministic->setFaultConfig(config);
    }
}

BrokerGateway::FaultInjectorConfig BrokerGateway::faultInjectorConfig() const noexcept
{
    const auto* deterministic = deterministicAdapter();
    return deterministic ? deterministic->faultConfig() : FaultInjectorConfig{};
}

std::uint64_t BrokerGateway::totalOrdersSubmitted() const noexcept
{
    return m_totalOrdersSubmitted.load();
}

std::uint64_t BrokerGateway::totalFillsReceived() const noexcept
{
    return m_totalFillsReceived.load();
}

std::uint64_t BrokerGateway::totalApiErrors() const noexcept
{
    return m_totalApiErrors.load();
}

std::uint64_t BrokerGateway::reconciliationCount() const noexcept
{
    return m_reconciliationCount.load();
}

std::uint64_t BrokerGateway::faultDropsTriggered() const noexcept
{
    const auto* deterministic = deterministicAdapter();
    return deterministic ? deterministic->faultDropsTriggered() : 0;
}

std::uint64_t BrokerGateway::faultLatencySpikesTriggered() const noexcept
{
    const auto* deterministic = deterministicAdapter();
    return deterministic ? deterministic->faultLatencySpikesTriggered() : 0;
}

std::uint64_t BrokerGateway::faultDisconnectsTriggered() const noexcept
{
    const auto* deterministic = deterministicAdapter();
    return deterministic ? deterministic->faultDisconnectsTriggered() : 0;
}

std::uint64_t BrokerGateway::connectLifecycleCount() const noexcept
{
    const auto* deterministic = deterministicAdapter();
    return deterministic ? deterministic->connectCount() : 0;
}

std::uint64_t BrokerGateway::disconnectLifecycleCount() const noexcept
{
    const auto* deterministic = deterministicAdapter();
    return deterministic ? deterministic->disconnectCount() : 0;
}

std::uint64_t BrokerGateway::reconnectLifecycleCount() const noexcept
{
    const auto* deterministic = deterministicAdapter();
    return deterministic ? deterministic->reconnectCount() : 0;
}

bool BrokerGateway::adapterAuthorized() const noexcept
{
    return m_config.isPaperMode()
        || (m_config.canEnterLiveRuntime()
            && m_liveAdapterApproved
            && deterministicAdapter() == nullptr);
}

DeterministicBrokerAdapter* BrokerGateway::deterministicAdapter() noexcept
{
    return dynamic_cast<DeterministicBrokerAdapter*>(m_adapter.get());
}

const DeterministicBrokerAdapter* BrokerGateway::deterministicAdapter() const noexcept
{
    return dynamic_cast<const DeterministicBrokerAdapter*>(m_adapter.get());
}

void BrokerGateway::bindAdapterCallbacks()
{
    if (!m_adapter) {
        return;
    }
    m_adapter->setAcknowledgementCallback(
        [this](const OrderAcknowledgement& acknowledgement) {
            handleAcknowledgement(acknowledgement);
        });
    m_adapter->setExecutionCallback(
        [this](const ExecutionEvent& execution) { handleExecution(execution); });
    m_adapter->setCancelCallback(
        [this](const CancelResult& result) { handleCancel(result); });
    m_adapter->setHealthCallback(
        [this](const AdapterHealthEvent& health) { handleHealth(health); });
}

void BrokerGateway::handleAcknowledgement(
    const OrderAcknowledgement& acknowledgement)
{
    AcknowledgementCallback callback;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const auto applied = m_lifecycle.applyAcknowledgement(acknowledgement);
        if (applied != LifecycleApplyResult::Applied) {
            ++m_totalApiErrors;
            return;
        }
        if (!acknowledgement.accepted) {
            ++m_totalApiErrors;
        }
        callback = m_acknowledgementCallback;
    }
    if (callback) {
        callback(acknowledgement);
    }
}

void BrokerGateway::handleExecution(const ExecutionEvent& execution)
{
    ExecutionCallback callback;
    std::vector<ExecutionCallback> callbacks;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const auto applied = m_lifecycle.applyExecution(execution);
        if (applied != LifecycleApplyResult::Applied) {
            if (applied != LifecycleApplyResult::Duplicate) {
                ++m_totalApiErrors;
            }
            return;
        }
        ++m_totalFillsReceived;
        callback = m_executionCallback;
        callbacks = m_executionCallbacks;
    }
    if (callback) {
        callback(execution);
    }
    for (const auto& observer : callbacks) {
        if (observer) {
            observer(execution);
        }
    }
}

void BrokerGateway::handleCancel(const CancelResult& result)
{
    CancelCallback callback;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_lifecycle.applyCancelResult(result)
            != LifecycleApplyResult::Applied) {
            ++m_totalApiErrors;
            return;
        }
        callback = m_cancelCallback;
    }
    if (callback) {
        callback(result);
    }
}

void BrokerGateway::handleHealth(const AdapterHealthEvent& health)
{
    HealthCallback callback;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_health = health;
        callback = m_healthCallback;
    }
    if (callback) {
        callback(health);
    }
}
