#pragma once
#include "FinancialMath.hpp"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <list>

// ── Phase 9: Advanced Order Types ────────────────────────────────────────────

// Strongly-typed order classification.
enum class OrderType {
    MARKET,         // Execute immediately at next available price.
    LIMIT,          // Buy (or sell) only when price falls to (or rises to) the limit.
    STOP_LOSS,      // Close long position when price drops to (or below) stop level.
    TRAILING_STOP   // Stop level trails the best-seen price by a fixed offset.
};

// A pending order record placed in the order queue.
struct OrderRecord {
    std::string symbol;
    OrderType   orderType{OrderType::LIMIT};
    bool        isBuy{true};          // true = buy-side, false = sell-side
    double      limitPrice{0.0};      // trigger price for LIMIT / STOP_LOSS
    double      trailOffset{0.0};     // fixed offset for TRAILING_STOP
    double      trailBest{0.0};       // best price seen since order was placed
    double      quantity{0.0};        // units to trade (0.0 = size at fill time)
    double      capitalToCommit{0.0}; // capital slice for buy orders
    uint64_t    placedTimestamp{0};   // epoch when order was placed
    uint64_t    orderId{0};           // monotone identifier
};

// Result of evaluating a pending order against a new candle.
struct OrderFillResult {
    uint64_t    orderId{0};
    bool        filled{false};
    std::string symbol;
    OrderType   orderType{OrderType::LIMIT};
    bool        isBuy{true};
    double      fillPrice{0.0};   // exact price at which the order triggered
    double      quantity{0.0};
    double      capitalToCommit{0.0};
    uint64_t    fillTimestamp{0};
};

// Represents a single open position.
struct Position {
    std::string symbol;
    double      quantity{0.0};
    double      entryPrice{0.0};
    bool        isLong{true};  // true = long (BUY), false = short
};

// Record of a completed round-trip trade (open + close).
struct TradeRecord {
    std::string symbol;
    uint64_t  openTimestamp{0};
    uint64_t  closeTimestamp{0};
    double    entryPrice{0.0};
    double    exitPrice{0.0};
    double    quantity{0.0};
    double    totalFees{0.0};   // fees paid on both legs combined
    double    realizedPnL{0.0}; // net profit after fees
    double    grossPnL{0.0};    // profit before fees (exit proceeds - cost basis)
    std::string strategy_id;    // originating strategy ("SMA_01", "MR_01", "ENSEMBLE")
};

// PortfolioManager tracks cash, multiple open positions keyed by symbol,
// aggregate equity, and drawdown.
// Starting balance: 100,000 USD.
class PortfolioManager {
public:
    static constexpr double STARTING_CASH = 100'000.0;

    PortfolioManager() = default;

    struct PositionSnapshot {
        Position position;
        uint64_t entryTimestamp{0};
        double entryFee{0.0};
        double costBasis{0.0};
        double lastMarkPrice{0.0};
        std::string strategyId;
    };

    struct Snapshot {
        double cash{STARTING_CASH};
        std::vector<PositionSnapshot> positions;
        double unrealizedPnL{0.0};
        double totalEquity{STARTING_CASH};
        double maxEquity{STARTING_CASH};
        double currentDrawdown{0.0};
        double maxDrawdown{0.0};
        int tradeCount{0};
        double totalFeesPaid{0.0};
        std::vector<TradeRecord> tradeLog;
        int roundTripCount{0};
        std::list<OrderRecord> pendingOrders;
        uint64_t nextOrderId{1};
    };

    // Open a long position for `symbol` at the confirmed fill price.
    // `capitalToCommit` is the maximum total debit (notional plus fee).
    // Pass 0.0 (default) to use all remaining cash as that debit budget.
    // `explicitUnits` > 0.0 overrides unit calculation (ATR-based sizing path);
    // the exact notional plus `fee` must still fit the debit budget.
    // `timestamp` is the candle epoch; `fee` is deducted exactly once here.
    void openLong(const std::string& symbol, double netPrice,
                  uint64_t timestamp = 0, double fee = 0.0,
                  double capitalToCommit = 0.0,
                  double explicitUnits   = 0.0,
                  const std::string& strategyId = "");

    // Increase an existing long position by adding `quantity` at `netPrice`.
    // Updates weighted-average entry price and deducts capital from cash.
    void addToLong(const std::string& symbol, double netPrice,
                   double quantity, double fee = 0.0,
                   double capitalToCommit = 0.0,
                   const std::string& strategyId = "");

    // Close all or `explicitQuantity` of the open long at the confirmed fill
    // price. `fee` is deducted exactly once from sale proceeds here.
    void closePosition(const std::string& symbol, double netPrice,
                       uint64_t timestamp = 0, double fee = 0.0,
                       const std::string& strategyId = "",
                       double explicitQuantity = 0.0);

    // Refresh unrealized PnL, totalEquity, and currentDrawdown for `symbol`
    // using the latest market price.
    void updatePnL(const std::string& symbol, double currentPrice);

    // Returns true when a position is currently open for `symbol`.
    bool hasPosition(const std::string& symbol) const noexcept;

    double getCashBalance()      const noexcept;
    double getTotalEquity()      const noexcept;
    double getCurrentDrawdown()  const noexcept; // fraction, e.g. 0.05 == 5 %
    double getEntryPrice(const std::string& symbol)       const noexcept;
    double getPositionQuantity(const std::string& symbol) const noexcept;
    double getTotalFeesPaid()    const noexcept;
    double getMaxDrawdown()      const noexcept; // peak-to-valley, fractional

    // Number of symbols with an open position right now.
    std::size_t getOpenPositionCount() const noexcept;

    // ── Phase 9: Pending Order Queue ──────────────────────────────────────

    // Place a pending (non-MARKET) order into the queue.
    // Returns the assigned orderId.
    uint64_t placePendingOrder(const OrderRecord& order) noexcept;

    // Cancel a pending order by orderId.  Returns true if found and removed.
    bool cancelPendingOrder(uint64_t orderId) noexcept;

    // Evaluate all pending orders against the supplied candle extremes.
    // Filled orders are removed from the queue and returned in the result vector.
    // The caller (ExecutionEngine) is responsible for routing fills through
    // the RiskEngine before calling openLong() / closePosition().
    std::vector<OrderFillResult> evaluatePendingOrders(
        const std::string& symbol,
        double high, double low, double close,
        uint64_t timestamp) noexcept;

    int    getTradeCount()       const noexcept;
    int    getRoundTripCount()   const noexcept; // completed buy+sell pairs

    // Access the full closed-trade log.
    const std::vector<TradeRecord>& getTradeLog() const noexcept;

    // Complete, single persistence contract. Supersedes the incomplete Phase-9
    // tuple and pending-order restore helpers.
    Snapshot snapshotState() const;
    void restoreState(const Snapshot& snapshot);

private:
    // Per-symbol open-position state (entry metadata lives alongside Position).
    struct PositionState {
        Financial::Quantity m_quantity{};
        Financial::Price m_averageEntryPrice{};
        Financial::Money m_costBasis{};
        Financial::Price m_lastMarkPrice{};
        uint64_t m_entryTimestamp{0};
        Financial::Money m_entryFee{};
        std::string m_strategyId;
    };

    struct Valuation {
        Financial::Money unrealizedPnL{};
        Financial::Money totalEquity{};
        Financial::Money maxEquity{};
        double currentDrawdown{0.0};
        double maxDrawdown{0.0};
    };

    Valuation valueState(
        Financial::Money cash,
        const std::unordered_map<std::string, PositionState>& positions) const;
    void commitValuation(const Valuation& valuation) noexcept;

    Financial::Money m_cash{10'000'000'000'000LL};

    // Multi-asset position map: symbol -> PositionState.
    std::unordered_map<std::string, PositionState> m_positions;

    Financial::Money m_unrealizedPnL{};
    Financial::Money m_totalEquity{10'000'000'000'000LL};
    Financial::Money m_maxEquity{10'000'000'000'000LL};
    double m_currentDrawdown{0.0};
    double m_maxDrawdown{0.0};
    int    m_tradeCount{0};
    Financial::Money m_totalFeesPaid{};

    std::vector<TradeRecord> m_tradeLog;
    int m_roundTripCount{0};

    // Phase 9: pending order queue and monotone counter.
    std::list<OrderRecord> m_pendingOrders;
    uint64_t               m_nextOrderId{1};
};
