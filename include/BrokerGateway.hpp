#pragma once

#include "BrokerAdapterContracts.hpp"
#include "DeterministicBrokerAdapter.hpp"
#include "IBrokerAdapter.hpp"
#include "OrderLifecycleStore.hpp"
#include "PortfolioManager.hpp"
#include "SystemConfig.hpp"

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct GatewayDispatchResult {
    bool dispatched{false};
    std::uint64_t localOrderId{0};
    FailureCategory failure{FailureCategory::None};
    std::string reason;
};

class BrokerGateway {
public:
    using FaultInjectorConfig = DeterministicBrokerAdapter::FaultConfig;
    using AcknowledgementCallback = std::function<void(const OrderAcknowledgement&)>;
    using ExecutionCallback = std::function<void(const ExecutionEvent&)>;
    using CancelCallback = std::function<void(const CancelResult&)>;
    using HealthCallback = std::function<void(const AdapterHealthEvent&)>;

    BrokerGateway(const SystemConfig& config,
                  PortfolioManager& portfolio);
    BrokerGateway(const SystemConfig& config,
                  PortfolioManager& portfolio,
                  std::unique_ptr<IBrokerAdapter> adapter,
                  bool liveAdapterApproved = false);
    ~BrokerGateway();

    void connect();
    void disconnect() noexcept;
    bool isConnected() const noexcept;

    void setSymbolAlias(std::string canonicalSymbol, std::string executionAlias);
    void setInstrumentSpec(const InstrumentSpec& spec);
    std::optional<NormalizedOrder> normalizeOrder(
        const OrderRequest& request,
        std::string* rejectionReason = nullptr) const;
    GatewayDispatchResult dispatchOrder(const NormalizedOrder& order,
                                        const RiskDecision& finalRiskDecision);
    bool requestCancel(const CancelRequest& request);
    ReconciliationSnapshot reconciliationSnapshot(std::uint64_t timestampNs);

    std::optional<OrderLifecycleRecord> orderLifecycle(
        std::uint64_t localOrderId) const noexcept;
    AdapterHealthEvent adapterHealth() const;

    void setAcknowledgementCallback(AcknowledgementCallback callback) noexcept;
    void setExecutionCallback(ExecutionCallback callback) noexcept;
    void addExecutionCallback(ExecutionCallback callback) noexcept;
    void setCancelCallback(CancelCallback callback) noexcept;
    void setHealthCallback(HealthCallback callback) noexcept;

    void simulateExecutionEvent(const ExecutionEvent& execution);
    void injectNextOrderError(const std::string& errorMessage) noexcept;
    void injectNextPartialFill(double fillRatio) noexcept;
    bool setPaperSimulationCosts(double feeRate, double slippageBps) noexcept;
    void setFaultInjectorConfig(const FaultInjectorConfig& config) noexcept;
    FaultInjectorConfig faultInjectorConfig() const noexcept;

    std::uint64_t totalOrdersSubmitted() const noexcept;
    std::uint64_t totalFillsReceived() const noexcept;
    std::uint64_t totalApiErrors() const noexcept;
    std::uint64_t reconciliationCount() const noexcept;
    std::uint64_t faultDropsTriggered() const noexcept;
    std::uint64_t faultLatencySpikesTriggered() const noexcept;
    std::uint64_t faultDisconnectsTriggered() const noexcept;
    std::uint64_t connectLifecycleCount() const noexcept;
    std::uint64_t disconnectLifecycleCount() const noexcept;
    std::uint64_t reconnectLifecycleCount() const noexcept;

private:
    bool adapterAuthorized() const noexcept;
    DeterministicBrokerAdapter* deterministicAdapter() noexcept;
    const DeterministicBrokerAdapter* deterministicAdapter() const noexcept;
    void bindAdapterCallbacks();
    void handleAcknowledgement(const OrderAcknowledgement& acknowledgement);
    void handleExecution(const ExecutionEvent& execution);
    void handleCancel(const CancelResult& result);
    void handleHealth(const AdapterHealthEvent& health);

    const SystemConfig& m_config;
    PortfolioManager& m_portfolio;
    std::unique_ptr<IBrokerAdapter> m_adapter;
    bool m_liveAdapterApproved{false};

    mutable std::mutex m_mutex;
    OrderLifecycleStore m_lifecycle;
    std::unordered_map<std::string, std::string> m_symbolAliases;
    std::unordered_map<std::string, InstrumentSpec> m_instrumentSpecs;
    ReconciliationSnapshot m_lastReconciliation;
    AdapterHealthEvent m_health;

    AcknowledgementCallback m_acknowledgementCallback;
    ExecutionCallback m_executionCallback;
    std::vector<ExecutionCallback> m_executionCallbacks;
    CancelCallback m_cancelCallback;
    HealthCallback m_healthCallback;

    std::atomic<std::uint64_t> m_totalOrdersSubmitted{0};
    std::atomic<std::uint64_t> m_totalFillsReceived{0};
    std::atomic<std::uint64_t> m_totalApiErrors{0};
    std::atomic<std::uint64_t> m_reconciliationCount{0};
};
