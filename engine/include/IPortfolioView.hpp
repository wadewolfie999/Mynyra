#pragma once

#include "BrokerAdapterContracts.hpp"

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class PortfolioManager;

// Read-only economic state consumed by risk and reconciliation. Provider
// adapters cannot mutate a view directly.
class IPortfolioView {
public:
    virtual ~IPortfolioView() = default;
    virtual AccountSnapshot account() const = 0;
    virtual std::vector<PositionSnapshot> positions() const = 0;
    virtual std::size_t pendingOrderCount() const noexcept = 0;
    virtual std::uint64_t generation() const noexcept = 0;
};

// BACKTEST/PAPER view over the existing spot-cash PortfolioManager ledger.
class LocalPortfolioView final : public IPortfolioView {
public:
    explicit LocalPortfolioView(const PortfolioManager& portfolio) noexcept;

    AccountSnapshot account() const override;
    std::vector<PositionSnapshot> positions() const override;
    std::size_t pendingOrderCount() const noexcept override;
    std::uint64_t generation() const noexcept override { return 0; }

private:
    const PortfolioManager& m_portfolio;
};

enum class MirrorApplyResult : std::uint8_t {
    Applied,
    Duplicate,
    UnknownOrder,
    InvalidEvent,
    InvalidTransition
};

// DEMO's broker-authoritative leveraged-CFD ledger. It intentionally does not
// reuse PortfolioManager's full-notional spot-cash accounting.
class BrokerPortfolioMirror final : public IPortfolioView {
public:
    bool registerIntent(std::uint64_t localOrderId, const OrderIntent& intent);
    MirrorApplyResult applyExecution(const ExecutionEvent& event);
    bool applyReconciliation(const ReconciliationSnapshot& snapshot);

    AccountSnapshot account() const override;
    std::vector<PositionSnapshot> positions() const override;
    std::size_t pendingOrderCount() const noexcept override;
    std::uint64_t generation() const noexcept override;

    std::optional<OrderIntent> intent(std::uint64_t localOrderId) const;
    bool appliedEvent(std::string_view eventKey) const;
    bool isFlat() const noexcept;

private:
    struct IntentState {
        OrderIntent intent;
        Decimal64 cumulativeFilled;
        std::uint64_t lastTimestampNs{0};
        std::uint64_t lastSequence{0};
    };

    static std::string provisionalPositionId(std::uint64_t localOrderId);

    mutable std::mutex m_mutex;
    AccountSnapshot m_account;
    std::unordered_map<std::string, PositionSnapshot> m_positions;
    std::unordered_map<std::uint64_t, IntentState> m_intents;
    std::unordered_set<std::string> m_eventKeys;
    std::size_t m_pendingOrders{0};
    std::uint64_t m_generation{0};
};
