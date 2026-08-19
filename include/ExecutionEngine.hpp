#pragma once
#include "FinancialMath.hpp"
#include "IStrategy.hpp"
#include "PortfolioManager.hpp"
#include "RiskEngine.hpp"
#include "MarketCandle.hpp"
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

// Forward declaration — AnalyticsEngine is injected optionally.
class AnalyticsEngine;
class SmaCrossStrategy;
class BrokerGateway;
struct ExecutionEvent;
class TriggerOrderManager;
class L2OrderBook;

// ExecutionEngine simulates order fills against a PortfolioManager.
// It respects the RiskEngine gatekeeper: new BUY entries are blocked when
// canTrade() returns false, but existing positions may still be closed.
//
// Realism parameters:
//   feeRate     — fractional fee per trade (e.g. 0.001 = 0.1 %).
//   slippageBps — one-way slippage in basis points (e.g. 5 bps = 0.0005).
//   riskPct     — fraction of total equity risked per trade for ATR sizing
//                 (e.g. 0.01 = 1 %).  When a SmaCrossStrategy is injected and
//                 its ATR is valid, dynamic sizing is used:
//                   Units = (E_total * riskPct) / ATR(t)
//                 Otherwise, the equal-slice capital-allocation fallback is used.
class ExecutionEngine {
public:
    ExecutionEngine(PortfolioManager& portfolio,
                    RiskEngine&       riskEngine,
                    std::string       symbol      = "BTCUSDT",
                    double            feeRate     = 0.001,
                    double            slippageBps = 5.0,
                    double            riskPct     = 0.01);

    // Inject analytics recorder (optional — pass nullptr to disable).
    void setAnalyticsEngine(AnalyticsEngine* analytics) noexcept;

    // Inject the strategy pointer so the engine can query live ATR values.
    // Pass nullptr to disable ATR-based sizing (falls back to equal-slice).
    void setStrategy(SmaCrossStrategy* strategy) noexcept;

    // Bind optional broker gateway. Once bound, orders route only through the
    // gateway; an unavailable gateway fails closed and never falls back to a
    // local simulated fill.
    void bindBrokerGateway(BrokerGateway* gateway) noexcept;

    // Attempt to execute the given signal at `marketPrice`.
    // Returns true when an order was actually filled.
    // `timestamp` is the candle epoch passed to PortfolioManager.
    bool execute(Signal signal, double marketPrice, uint64_t timestamp = 0,
                 const std::string& strategyId = "");

    // Phase 9: Evaluate pending limit/stop orders against the current candle
    // and route any triggered fills through the RiskEngine before executing.
    // Returns the number of pending orders that were triggered and filled.
    int processPendingOrders(const MarketCandle& candle);

    // Phase 9: Place a pending LIMIT, STOP_LOSS, or TRAILING_STOP order.
    // The order is registered in PortfolioManager's queue and evaluated on
    // each subsequent candle.  Returns the assigned orderId.
    uint64_t placePendingOrder(const OrderRecord& order) noexcept;

    // Phase 18: bind a trigger-order manager that evaluates STOP_LOSS / OCO
    // orders against an L2 order book BBO stream.
    void bindTriggerOrderManager(TriggerOrderManager* manager) noexcept;

    // Phase 18: place STOP_LOSS and OCO trigger orders for this engine symbol.
    // `quantity=0.0` means "close the currently open position quantity".
    uint64_t placeStopLossTrigger(double stopPrice,
                                  double quantity = 0.0,
                                  const std::string& strategyId = "") noexcept;
    uint64_t placeOcoTrigger(double stopPrice,
                             double takeProfitPrice,
                             double quantity = 0.0,
                             const std::string& strategyId = "") noexcept;

    // Phase 18: evaluate trigger orders against L2 best bid/ask and route
    // triggered fills through execute() for the current symbol.
    int processTriggerOrders(const L2OrderBook& orderBook,
                             uint64_t timestamp);

    int getSignalCount()  const noexcept;
    int getFilledCount()  const noexcept;
    int getBlockedCount() const noexcept;
    double getLastRouteLatencyMs() const noexcept;
    double getMaxRouteLatencyMs() const noexcept;
    uint64_t getLatencyBreachCount() const noexcept;
    uint64_t lastBrokerOrderId() const noexcept;
    double pendingBrokerQuantity(uint64_t orderId) const noexcept;

private:
    bool dispatchBrokerOrder(bool isBuy,
                             Financial::Quantity quantity,
                             Financial::Price requestedPrice,
                             uint64_t timestamp,
                             const std::string& strategyId);
    void onBrokerExecution(const ExecutionEvent& execution);

    PortfolioManager& m_portfolio;
    RiskEngine&       m_riskEngine;
    std::string       m_symbol;
    Financial::Fraction m_feeRate;
    Financial::Fraction m_slippage;
    double            m_riskPct;           // fraction of equity risked per trade
    AnalyticsEngine*  m_analytics{nullptr};
    SmaCrossStrategy* m_strategy{nullptr};
    BrokerGateway*    m_gateway{nullptr};
    TriggerOrderManager* m_triggerOrders{nullptr};

    int m_signalCount{0};
    int m_filledCount{0};
    int m_blockedCount{0};

    std::atomic<uint64_t> m_nextLocalOrderId{1};

    // Routing latency instrumentation.
    std::atomic<double> m_lastRouteLatencyMs{0.0};
    std::atomic<double> m_maxRouteLatencyMs{0.0};
    std::atomic<uint64_t> m_latencyBreaches{0};

    // Partial-fill tracking.
    std::unordered_map<uint64_t, Financial::Quantity> m_pendingBrokerQty;
    std::unordered_map<uint64_t, Financial::Quantity> m_appliedBrokerQty;
    mutable std::mutex m_fillMutex;
    std::atomic<uint64_t> m_lastBrokerOrderId{0};
};
