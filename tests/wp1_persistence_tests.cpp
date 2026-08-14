#include "EventLoop.hpp"
#include "ExecutionEngine.hpp"
#include "PortfolioAllocator.hpp"
#include "PortfolioManager.hpp"
#include "RegimeDetector.hpp"
#include "RiskEngine.hpp"
#include "StateSerializer.hpp"
#include "SystemConfig.hpp"

#include <cmath>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {
void require(bool condition, const char* message) {
    if (!condition) { throw std::runtime_error(message); }
}

std::filesystem::path testRoot() {
    const auto path = std::filesystem::temp_directory_path() /
        ("tradebot-wp1-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(path);
    return path;
}

void seedPortfolio(PortfolioManager& portfolio, int positions) {
    portfolio.openLong("CLOSED", 25.0, 11, 0.25, 250.0, 10.0, "MR_01");
    portfolio.closePosition("CLOSED", 27.5, 12, 0.30, "MR_01");
    if (positions >= 1) {
        portfolio.openLong("FIRST", 100.25, 101, 1.25, 10'000.0, 10.0, "SMA_01");
        portfolio.updatePnL("FIRST", 105.5);
    }
    if (positions >= 2) {
        portfolio.openLong("SECOND", 50.5, 202, 0.75, 5'000.0, 20.0, "MR_01");
        portfolio.updatePnL("SECOND", 48.0);
    }
    OrderRecord order;
    order.symbol = "FIRST"; order.orderType = OrderType::LIMIT; order.isBuy = true;
    order.limitPrice = 90.0; order.quantity = 3.0; order.placedTimestamp = 303;
    portfolio.placePendingOrder(order);
}

void seedRisk(RiskEngine& risk, SystemConfig& config) {
    risk.setSystemConfig(&config);
    risk.setTotalDrawdown(0.031);
    risk.setDailyDrawdown(0.012);
    risk.pushEquityReturn(100'000.0); risk.pushEquityReturn(99'000.0); risk.pushEquityReturn(99'500.0);
    risk.pushAssetReturn("FIRST", 100.0, 1'000.0);
    risk.pushAssetReturn("FIRST", 101.0, 1'010.0);
    risk.pushAssetReturn("FIRST", 99.0, 990.0);
    risk.reportLatency(config.latencyMaxMs + 1);
    risk.setHaltTrading(true);
    risk.syncPosition("FIRST", 10.0);
    risk.updateLiveVolatility(3.0, 1.0);
}

void requirePortfolioEqual(const PortfolioManager::Snapshot& a,
                           const PortfolioManager::Snapshot& b) {
    require(a.cash == b.cash && a.unrealizedPnL == b.unrealizedPnL
            && a.totalEquity == b.totalEquity && a.maxEquity == b.maxEquity
            && a.currentDrawdown == b.currentDrawdown && a.maxDrawdown == b.maxDrawdown
            && a.tradeCount == b.tradeCount && a.totalFeesPaid == b.totalFeesPaid
            && a.roundTripCount == b.roundTripCount && a.nextOrderId == b.nextOrderId,
            "portfolio accounting did not round-trip exactly");
    require(a.positions.size() == b.positions.size(), "open-position count changed");
    for (std::size_t i = 0; i < a.positions.size(); ++i) {
        const auto& x = a.positions[i]; const auto& y = b.positions[i];
        require(x.position.symbol == y.position.symbol && x.position.quantity == y.position.quantity
                && x.position.entryPrice == y.position.entryPrice && x.position.isLong == y.position.isLong
                && x.entryTimestamp == y.entryTimestamp && x.entryFee == y.entryFee
                && x.strategyId == y.strategyId, "open-position metadata changed");
    }
    require(a.pendingOrders.size() == b.pendingOrders.size(), "pending orders changed");
    auto leftOrder = a.pendingOrders.begin(), rightOrder = b.pendingOrders.begin();
    for (; leftOrder != a.pendingOrders.end(); ++leftOrder, ++rightOrder) {
        require(leftOrder->symbol == rightOrder->symbol
                && leftOrder->orderType == rightOrder->orderType
                && leftOrder->isBuy == rightOrder->isBuy
                && leftOrder->limitPrice == rightOrder->limitPrice
                && leftOrder->trailOffset == rightOrder->trailOffset
                && leftOrder->trailBest == rightOrder->trailBest
                && leftOrder->quantity == rightOrder->quantity
                && leftOrder->capitalToCommit == rightOrder->capitalToCommit
                && leftOrder->placedTimestamp == rightOrder->placedTimestamp
                && leftOrder->orderId == rightOrder->orderId,
                "pending-order state changed");
    }
    require(a.tradeLog.size() == b.tradeLog.size(), "trade log changed");
    for (std::size_t i = 0; i < a.tradeLog.size(); ++i) {
        const auto& x = a.tradeLog[i]; const auto& y = b.tradeLog[i];
        require(x.symbol == y.symbol && x.openTimestamp == y.openTimestamp
                && x.closeTimestamp == y.closeTimestamp && x.entryPrice == y.entryPrice
                && x.exitPrice == y.exitPrice && x.quantity == y.quantity
                && x.totalFees == y.totalFees && x.realizedPnL == y.realizedPnL
                && x.grossPnL == y.grossPnL && x.strategy_id == y.strategy_id,
                "trade-log state changed");
    }
}

void testRoundTrips(const std::filesystem::path& root) {
    for (int count : {0, 1, 2}) {
        PortfolioManager source; seedPortfolio(source, count);
        SystemConfig config; RiskEngine sourceRisk(source, 4, 0.08, 16); seedRisk(sourceRisk, config);
        RegimeDetector sourceRegime; sourceRegime.update(101, 99, 100); sourceRegime.update(102, 100, 101);
        PortfolioAllocator sourceAllocator; sourceAllocator.onTradeClosed("SMA_01", 42.0);
        StateSerializer serializer;
        const auto path = root / ("roundtrip-" + std::to_string(count) + ".json");
        require(serializer.saveSnapshot(source, sourceRisk, sourceRegime, sourceAllocator, 777, path.string()),
                "snapshot save failed");

        PortfolioManager restored; RiskEngine restoredRisk(restored, 4, 0.08, 16);
        restoredRisk.setSystemConfig(&config); RegimeDetector restoredRegime; PortfolioAllocator restoredAllocator;
        std::uint64_t timestamp = 0;
        require(serializer.loadSnapshot(restored, restoredRisk, restoredRegime, restoredAllocator,
                                        timestamp, path.string()), "snapshot load failed");
        require(timestamp == 777, "checkpoint timestamp changed");
        requirePortfolioEqual(source.snapshotState(), restored.snapshotState());
        const auto riskA = sourceRisk.snapshotState(), riskB = restoredRisk.snapshotState();
        require(riskA.totalDrawdown == riskB.totalDrawdown && riskA.dailyDrawdown == riskB.dailyDrawdown
                && riskA.configuredMaxPositions == riskB.configuredMaxPositions
                && riskA.configuredVarLimit == riskB.configuredVarLimit
                && riskA.configuredVarWindow == riskB.configuredVarWindow
                && riskA.prevEquity == riskB.prevEquity
                && riskA.prevEquityValid == riskB.prevEquityValid
                && riskA.returnWindow == riskB.returnWindow && riskA.currentVaR95 == riskB.currentVaR95
                && riskA.returnStdDev == riskB.returnStdDev
                && riskA.covarianceMatrix == riskB.covarianceMatrix
                && riskA.assetOrder == riskB.assetOrder
                && riskA.multiAssetMode == riskB.multiAssetMode
                && riskA.closeOnly == riskB.closeOnly && riskA.halted == riskB.halted
                && riskA.lastLatencyMs == riskB.lastLatencyMs
                && riskA.syncedPositions == riskB.syncedPositions
                && riskA.effectiveMaxPositions == riskB.effectiveMaxPositions
                && riskA.effectiveVarLimit == riskB.effectiveVarLimit
                && riskA.volatilityScaled == riskB.volatilityScaled,
                "risk state did not round-trip exactly");
        require(riskA.assetStates.size() == riskB.assetStates.size(), "risk asset count changed");
        for (const auto& [symbol, x] : riskA.assetStates) {
            const auto it = riskB.assetStates.find(symbol);
            require(it != riskB.assetStates.end(), "risk asset disappeared");
            const auto& y = it->second;
            require(x.returnWindow == y.returnWindow && x.prevPrice == y.prevPrice
                    && x.prevPriceValid == y.prevPriceValid
                    && x.positionValue == y.positionValue, "risk asset state changed");
        }
        require(sourceRegime.getState().returnWindow == restoredRegime.getState().returnWindow,
                "regime state changed");
        require(sourceAllocator.getPhi("SMA_01") == restoredAllocator.getPhi("SMA_01"),
                "allocator state changed");

        OrderRecord next; next.symbol = "NEXT"; next.orderType = OrderType::LIMIT;
        const auto priorNext = source.placePendingOrder(next);
        const auto restoredNext = restored.placePendingOrder(next);
        require(priorNext == restoredNext, "pending-order identity/dedup state changed");
    }
}

std::string readFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
void writeFile(const std::filesystem::path& path, const std::string& text) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc); out << text;
}

void testRejectsWithoutMutation(const std::filesystem::path& root) {
    PortfolioManager source; seedPortfolio(source, 1); RiskEngine sourceRisk(source);
    StateSerializer serializer; const auto valid = root / "valid.json";
    require(serializer.saveSnapshot(source, sourceRisk, 44, valid.string()), "baseline save failed");

    const std::string original = readFile(valid);
    for (const auto& [name, contents] : {
            std::pair<std::string, std::string>{"malformed.json", "{not-json"},
            {"legacy.json", [&] { auto value = original; const auto pos = value.find("\"version\": 12");
                value.replace(pos, std::string("\"version\": 12").size(), "\"version\": 11"); return value; }()},
            {"corrupt.json", [&] { auto value = original; const auto pos = value.find("\"payload\": \"") + 12;
                value[pos] = value[pos] == '0' ? '1' : '0'; return value; }()},
            {"trailing.json", original + "garbage"}}) {
        const auto path = root / name; writeFile(path, contents);
        PortfolioManager target; target.openLong("SENTINEL", 10.0, 1, 0, 100.0, 1.0, "KEEP");
        RiskEngine targetRisk(target); targetRisk.setHaltTrading(true);
        const auto beforePortfolio = target.snapshotState(); const auto beforeRisk = targetRisk.snapshotState();
        std::uint64_t timestamp = 999;
        require(!serializer.loadSnapshot(target, targetRisk, timestamp, path.string()),
                "invalid snapshot was accepted");
        requirePortfolioEqual(beforePortfolio, target.snapshotState());
        require(beforeRisk.halted == targetRisk.snapshotState().halted && timestamp == 999,
                "rejected snapshot mutated target state");
    }

    const auto blocker = root / "blocker"; writeFile(blocker, "not-a-directory");
    require(!serializer.saveSnapshot(source, sourceRisk, 45, (blocker / "state.json").string()),
            "failed checkpoint reported success");
    require(!serializer.lastError().empty(), "failed checkpoint was not actionable");
    require(readFile(valid) == original, "failed write damaged last valid snapshot");

    sourceRisk.setTotalDrawdown(std::numeric_limits<double>::quiet_NaN());
    require(!serializer.saveSnapshot(source, sourceRisk, 45, valid.string()),
            "non-finite state replaced the last valid checkpoint");
    require(readFile(valid) == original, "non-finite state damaged last valid snapshot");

    PortfolioManager mismatchPortfolio;
    mismatchPortfolio.openLong("SENTINEL", 10.0, 1, 0, 100.0, 1.0, "KEEP");
    RiskEngine mismatchRisk(mismatchPortfolio, 1);
    const auto mismatchBefore = mismatchPortfolio.snapshotState();
    std::uint64_t mismatchTs = 999;
    require(!serializer.loadSnapshot(mismatchPortfolio, mismatchRisk, mismatchTs, valid.string()),
            "snapshot widened a stricter runtime risk configuration");
    requirePortfolioEqual(mismatchBefore, mismatchPortfolio.snapshotState());
    require(mismatchTs == 999, "risk configuration rejection mutated checkpoint time");

    PortfolioManager transientPortfolio;
    RiskEngine transientRisk(transientPortfolio);
    transientRisk.reportApiError();
    require(!serializer.saveSnapshot(transientPortfolio, transientRisk, 46,
                                     (root / "transient.json").string()),
            "transient API-error window was persisted without its time source");
}

class NoSignal final : public IStrategy {
public:
    AlphaSignal generateSignal(const MarketCandle& candle) override {
        return {candle.symbol, "NONE", 0.0};
    }
};

void testEventLoopStopsOnCheckpointFailure(const std::filesystem::path& root) {
    PortfolioManager portfolio; RiskEngine risk(portfolio); ExecutionEngine execution(portfolio, risk, "X");
    NoSignal strategy; EventLoop loop(strategy, risk, execution, portfolio); StateSerializer serializer;
    const auto blocker = root / "event-blocker"; writeFile(blocker, "not-a-directory");
    loop.setStateSerializer(&serializer, 1, (blocker / "state.json").string());
    MarketCandle candle{"2026.01.01", "00:00", "X", 10, 1, 1, 1, 1, 1};
    loop.processCandle(candle); candle.epochTimestamp = 11;
    bool threw = false;
    try { loop.processCandle(candle); } catch (const std::runtime_error& error) {
        threw = std::string(error.what()).find("checkpoint failed") != std::string::npos;
    }
    require(threw, "EventLoop continued after checkpoint failure");
}
} // namespace

int main() {
    const auto root = testRoot();
    try {
        testRoundTrips(root); testRejectsWithoutMutation(root); testEventLoopStopsOnCheckpointFailure(root);
        std::filesystem::remove_all(root);
        std::cout << "WP-1 persistence tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::filesystem::remove_all(root);
        std::cerr << "WP-1 persistence test failed: " << error.what() << "\n";
        return 1;
    }
}
