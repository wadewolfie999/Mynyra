#pragma once

#include "BrokerGateway.hpp"
#include "ExecutionEngine.hpp"
#include "IPortfolioView.hpp"
#include "MynyraEventSink.hpp"
#include "StrategyPipeline.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

enum class MynyraDemoOutcome : std::uint8_t {
    ReadOnly,
    NoEligibleSignal,
    EntryDispatched,
    Succeeded,
    Failed,
    RecoveryRequired
};

struct MynyraDemoCommissioningConfig {
    bool commissionOrder{false};
    std::chrono::milliseconds lifecycleTimeout{std::chrono::seconds(30)};
};

class MynyraDemoCommissioningController {
public:
    MynyraDemoCommissioningController(
        MynyraDemoCommissioningConfig config,
        std::string sessionId,
        ExecutionEngine& execution,
        BrokerGateway& gateway,
        BrokerPortfolioMirror& mirror,
        IEventSink& sink,
        std::shared_ptr<std::atomic<std::uint64_t>> eventSequence = {});
    ~MynyraDemoCommissioningController();

    MynyraDemoCommissioningController(
        const MynyraDemoCommissioningController&) = delete;
    MynyraDemoCommissioningController& operator=(
        const MynyraDemoCommissioningController&) = delete;

    MynyraDemoOutcome commission(const StrategyDecision& decision,
                                 const OrderRiskContext& openingContext);
    bool entryAttempted() const noexcept;
    bool succeeded() const noexcept;

private:
    enum class Leg : std::uint8_t { Entry, Close, ResidualClose };

    struct TrackedOrder {
        Leg leg{Leg::Entry};
        OrderIntent intent;
        bool acceptanceEvidence{false};
        bool completeFill{false};
        bool rejected{false};
        bool duplicateApplied{false};
        FailureCategory failure{FailureCategory::None};
    };

    struct SharedState {
        std::mutex mutex;
        std::condition_variable changed;
        std::unordered_map<std::uint64_t, TrackedOrder> orders;
        BrokerPortfolioMirror* mirror{nullptr};
        IEventSink* sink{nullptr};
        std::string sessionId;
        std::shared_ptr<std::atomic<std::uint64_t>> eventSequence;
        std::atomic<bool> sinkHealthy{true};
        std::atomic<bool> active{true};
    };

    struct LegEvidence {
        bool acceptance{false};
        bool filled{false};
        bool rejected{false};
        bool duplicateApplied{false};
        FailureCategory failure{FailureCategory::None};
    };

    static std::uint64_t nowNs() noexcept;
    static const char* legName(Leg leg) noexcept;
    static bool emit(const std::shared_ptr<SharedState>& state,
                     MynyraEvent event,
                     EventFlush flush) noexcept;
    static void onAcknowledgement(const std::shared_ptr<SharedState>& state,
                                  const OrderAcknowledgement& event);
    static void onExecution(const std::shared_ptr<SharedState>& state,
                            const ExecutionEvent& event);

    bool track(std::uint64_t orderId, Leg leg, const OrderIntent& intent);
    LegEvidence waitForLeg(std::uint64_t orderId);
    std::optional<std::uint64_t> dispatch(
        Leg leg, const OrderIntent& intent, const OrderRiskContext& context);
    bool reconcileEntry(const OrderIntent& intent,
                        ReconciliationSnapshot& snapshot);
    bool reconcileFlat(ReconciliationSnapshot& snapshot);
    MynyraDemoOutcome fail(FailureCategory failure,
                           bool recoveryRequired) noexcept;

    MynyraDemoCommissioningConfig m_config;
    ExecutionEngine& m_execution;
    BrokerGateway& m_gateway;
    BrokerPortfolioMirror& m_mirror;
    std::shared_ptr<SharedState> m_state;
    std::atomic<bool> m_entryAttempted{false};
    std::atomic<bool> m_succeeded{false};
};
