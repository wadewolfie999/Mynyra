#include "providers/ctrader/CTraderDemoRuntime.hpp"

#include "BrokerGateway.hpp"
#include "ExecutionEngine.hpp"
#include "IPortfolioView.hpp"
#include "MeanReversionStrategy.hpp"
#include "MynyraDemoCommissioning.hpp"
#include "MynyraEventSink.hpp"
#include "PortfolioAllocator.hpp"
#include "PortfolioManager.hpp"
#include "RegimeDetector.hpp"
#include "RiskEngine.hpp"
#include "SmaCrossStrategy.hpp"
#include "StrategyPipeline.hpp"
#include "providers/ctrader/CTraderProviderAdapter.hpp"

#include <chrono>
#include <atomic>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <unordered_set>

namespace tradebot::ctrader {
namespace {

std::uint64_t nowNs() noexcept
{
    const auto value = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return value > 0 ? static_cast<std::uint64_t>(value) : 0;
}

std::string newSessionId()
{
    std::random_device source;
    std::ostringstream text;
    text << std::hex << nowNs() << '-'
         << static_cast<std::uint64_t>(source()) << source();
    return text.str();
}

bool validWarmup(const std::vector<MarketCandle>& candles) noexcept
{
    if (candles.size() != 100) return false;
    std::unordered_set<std::uint64_t> timestamps;
    std::uint64_t previous = 0;
    for (const auto& candle : candles) {
        if (candle.symbol != "XAUUSD" || candle.epochTimestamp == 0
            || candle.epochTimestamp <= previous
            || candle.open <= 0.0 || candle.high <= 0.0
            || candle.low <= 0.0 || candle.close <= 0.0
            || candle.low > candle.high
            || !timestamps.insert(candle.epochTimestamp).second) {
            return false;
        }
        previous = candle.epochTimestamp;
    }
    return true;
}

bool validArmingEvidence(const OrderRiskContext& context) noexcept
{
    constexpr std::uint64_t maximumBboAgeNs = 5'000'000'000ULL;
    const auto freshAtEvaluation = [&context](std::uint64_t timestamp) {
        return timestamp > 0
            && context.evaluationTimestampNs >= timestamp
            && context.evaluationTimestampNs - timestamp
                   <= maximumBboAgeNs;
    };
    return context.account.complete && context.instrument.complete
        && context.instrument.canonicalSymbol == "XAUUSD"
        && context.instrument.tradingEnabled
        && context.instrument.minimumQuantity.isPositive()
        && context.instrument.maximumQuantity.isPositive()
        && context.instrument.quantityStep.isPositive()
        && context.reconciliation.complete
        && context.reconciliation.status == ReconciliationStatus::Matched
        && context.reconciliation.positions.empty()
        && context.reconciliation.pendingOrderCount == 0
        && context.sameGeneration && context.connectionGeneration > 0
        && context.instrumentVersion == context.instrument.version
        && context.reconciliation.connectionGeneration
               == context.connectionGeneration
        && freshAtEvaluation(context.account.sourceTimestampNs)
        && freshAtEvaluation(context.account.ingestionTimestampNs)
        && freshAtEvaluation(context.reconciliation.timestampNs)
        && context.bboComplete && context.bid.isPositive()
        && context.ask.isPositive()
        && context.ask.toDouble() > context.bid.toDouble()
        && context.evaluationTimestampNs >= context.bboSourceTimestampNs
        && context.bboSourceTimestampNs > 0
        && context.evaluationTimestampNs - context.bboSourceTimestampNs
               <= maximumBboAgeNs;
}

bool emitRuntime(IEventSink& sink, const std::string& sessionId,
                 const std::shared_ptr<std::atomic<std::uint64_t>>& sequence,
                 std::string eventType,
                 FailureCategory failure = FailureCategory::None)
{
    MynyraEvent event;
    event.sessionId = sessionId;
    event.localSequence = sequence->fetch_add(1) + 1;
    event.emittedTimestampNs = nowNs();
    event.canonicalSymbol = "XAUUSD";
    event.eventType = std::move(eventType);
    event.failure = failure;
    return sink.emit(event, EventFlush::LifecycleBoundary);
}

} // namespace

int runCTraderDemoRuntime(const SystemConfig& config)
{
    if (!config.isDemoMode() || !config.canEnterCTraderDemoRuntime()
        || config.isLiveMode()) {
        return EXIT_FAILURE;
    }

    const std::string sessionId = newSessionId();
    const std::filesystem::path evidencePath =
        std::filesystem::path("output/mynyra-demo")
        / (sessionId + ".ndjson");
    auto console = std::make_shared<ConsoleEventSink>(std::cout);
    auto ndjson = std::make_shared<NdjsonEventSink>(evidencePath);
    CompositeEventSink sink;
    sink.add(console);
    sink.add(ndjson);
    auto runtimeSequence =
        std::make_shared<std::atomic<std::uint64_t>>(0);
    if (!ndjson->isOpen()
        || !emitRuntime(sink, sessionId, runtimeSequence,
                        "mynyra_demo_session_started")) {
        return EXIT_FAILURE;
    }

    PortfolioManager compatibilityPortfolio;
    BrokerPortfolioMirror mirror;
    RiskEngine risk(compatibilityPortfolio, 1);
    risk.setSystemConfig(&config);

    auto adapter = std::make_unique<CTraderProviderAdapter>(config.freshOAuth);
    CTraderProviderAdapter* provider = adapter.get();
    BrokerGateway gateway(config, compatibilityPortfolio,
                          std::move(adapter), true);
    ExecutionEngine execution(compatibilityPortfolio, risk, "XAUUSD",
                              0.0, 0.0, 0.0);
    execution.bindBrokerGateway(&gateway);
    MynyraDemoCommissioningController controller(
        MynyraDemoCommissioningConfig{config.commissionDemoOrder,
                                      std::chrono::seconds(45)},
        sessionId, execution, gateway, mirror, sink, runtimeSequence);

    gateway.connect();
    if (!gateway.isConnected()) {
        emitRuntime(sink, sessionId, runtimeSequence,
                    provider->lastDiagnostic(), provider->lastFailure());
        emitRuntime(sink, sessionId, runtimeSequence,
                    "mynyra_demo_connection_failed", provider->lastFailure());
        return EXIT_FAILURE;
    }
    emitRuntime(sink, sessionId, runtimeSequence,
                "mynyra_demo_authenticated");

    const auto history = provider->historicalCandles();
    if (!validWarmup(history)) {
        emitRuntime(sink, sessionId, runtimeSequence,
                    "mynyra_demo_warmup_rejected",
                    FailureCategory::MalformedEvent);
        gateway.disconnect();
        return EXIT_FAILURE;
    }

    SmaCrossStrategy sma(12, 26, "SMA_01");
    MeanReversionStrategy meanReversion(20, 2.0, 14, "MR_01");
    PortfolioAllocator allocator(0.3, 0.3);
    allocator.setWeight("SMA_01", 0.6);
    allocator.setWeight("MR_01", 0.4);
    RegimeDetector regime;
    allocator.setRegimeDetector(&regime);
    StrategyPipeline pipeline({&sma, &meanReversion}, allocator, &regime);
    for (const auto& candle : history) {
        (void)pipeline.advance(candle, false);
    }
    emitRuntime(sink, sessionId, runtimeSequence,
                "mynyra_demo_historical_warmup_complete");

    // One completed live bar is an explicit arming prerequisite and cannot
    // itself commission an order.
    const auto firstLive = provider->waitForCompletedCandle(
        std::chrono::minutes(2));
    if (!firstLive.has_value()) {
        emitRuntime(sink, sessionId, runtimeSequence,
                    "mynyra_demo_live_bar_timeout",
                    FailureCategory::Timeout);
        gateway.disconnect();
        return EXIT_FAILURE;
    }
    (void)pipeline.advance(*firstLive, false);
    const auto armingEvidence = provider->riskContext(PositionSide::Long);
    if (!armingEvidence.has_value()
        || !validArmingEvidence(*armingEvidence)) {
        emitRuntime(sink, sessionId, runtimeSequence,
                    provider->lastDiagnostic(), provider->lastFailure());
        emitRuntime(sink, sessionId, runtimeSequence,
                    "mynyra_demo_arming_rejected",
                    provider->lastFailure() == FailureCategory::None
                        ? FailureCategory::ReconciliationMismatch
                        : provider->lastFailure());
        gateway.disconnect();
        return EXIT_FAILURE;
    }
    emitRuntime(sink, sessionId, runtimeSequence,
                "mynyra_demo_signal_observation_armed");

    const auto signalDeadline = std::chrono::steady_clock::now()
                              + std::chrono::hours(4);
    while (std::chrono::steady_clock::now() < signalDeadline) {
        const auto candle = provider->waitForCompletedCandle(
            std::chrono::seconds(75));
        if (!candle.has_value()) {
            if (!gateway.isConnected()) break;
            continue;
        }
        const StrategyDecision decision = pipeline.advance(*candle, true);
        if (decision.action == Signal::NONE) continue;
        const PositionSide direction = decision.action == Signal::BUY
            ? PositionSide::Long : PositionSide::Short;
        auto context = provider->riskContext(direction);
        if (!context.has_value()) {
            emitRuntime(sink, sessionId, runtimeSequence,
                        provider->lastDiagnostic(), provider->lastFailure());
            emitRuntime(sink, sessionId, runtimeSequence,
                        "mynyra_demo_risk_context_unavailable",
                        provider->lastFailure());
            gateway.disconnect();
            return EXIT_FAILURE;
        }
        const MynyraDemoOutcome outcome = controller.commission(
            decision, *context);
        gateway.disconnect();
        if (outcome == MynyraDemoOutcome::Succeeded
            || outcome == MynyraDemoOutcome::ReadOnly) {
            return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
    }

    emitRuntime(sink, sessionId, runtimeSequence,
                "mynyra_demo_signal_timeout", FailureCategory::Timeout);
    gateway.disconnect();
    return EXIT_FAILURE;
}

} // namespace tradebot::ctrader
