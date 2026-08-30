#include "BrokerGateway.hpp"
#include "ExecutionEngine.hpp"
#include "IPortfolioView.hpp"
#include "MynyraDemoCommissioning.hpp"
#include "MynyraEventSink.hpp"
#include "MeanReversionStrategy.hpp"
#include "PortfolioAllocator.hpp"
#include "PortfolioManager.hpp"
#include "RiskEngine.hpp"
#include "SmaCrossStrategy.hpp"
#include "StrategyPipeline.hpp"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

void require(bool condition, const std::string& message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

std::uint64_t nowNs()
{
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
}

InstrumentSpec instrument()
{
    InstrumentSpec spec;
    spec.version = 7;
    spec.canonicalSymbol = "XAUUSD";
    spec.executionAlias = "XAU/USD";
    spec.tickSize = Decimal64{1, 2};
    spec.contractSize = Decimal64{10000, 2};
    spec.minimumQuantity = Decimal64{100, 2};
    spec.maximumQuantity = Decimal64{100000, 2};
    spec.quantityStep = Decimal64{100, 2};
    spec.effectiveTimestampNs = nowNs();
    spec.complete = true;
    spec.tradingEnabled = true;
    spec.supportsLong = true;
    spec.supportsShort = true;
    return spec;
}

AccountSnapshot account(std::uint64_t version)
{
    AccountSnapshot value;
    value.snapshotVersion = version;
    value.balance = Decimal64{1000000, 2};
    value.equity = Decimal64{1000000, 2};
    value.realizedPnl = Decimal64{0, 2};
    value.unrealizedPnl = Decimal64{0, 2};
    value.marginUsed = Decimal64{0, 2};
    value.freeMargin = Decimal64{1000000, 2};
    value.currency = "USD";
    value.sourceTimestampNs = nowNs();
    value.ingestionTimestampNs = value.sourceTimestampNs;
    value.complete = true;
    return value;
}

OrderRiskContext emptyRiskContext(
    PositionSide expectedMarginSide = PositionSide::Long)
{
    OrderRiskContext context;
    context.account = account(1);
    context.instrument = instrument();
    context.reconciliation.snapshotVersion = 1;
    context.reconciliation.timestampNs = nowNs();
    context.reconciliation.account = context.account;
    context.reconciliation.status = ReconciliationStatus::Matched;
    context.reconciliation.connectionGeneration = 1;
    context.reconciliation.complete = true;
    context.expectedMargin = Decimal64{1000, 2};
    context.expectedMarginSide = expectedMarginSide;
    context.grossExposure = Decimal64{0, 2};
    context.bid = Decimal64{230000, 2};
    context.ask = Decimal64{230010, 2};
    context.instrumentVersion = context.instrument.version;
    context.bboSourceTimestampNs = nowNs() - 1'000'000;
    context.evaluationTimestampNs = nowNs();
    context.connectionGeneration = 1;
    context.bboComplete = true;
    context.sameGeneration = true;
    return context;
}

class FixedStrategy final : public IStrategy {
public:
    FixedStrategy(std::string id, double conviction)
        : m_id(std::move(id)), m_conviction(conviction) {}
    AlphaSignal generateSignal(const MarketCandle& candle) override
    {
        return AlphaSignal{candle.symbol, m_id, m_conviction};
    }
private:
    std::string m_id;
    double m_conviction;
};

class VectorSink final : public IEventSink {
public:
    bool emit(const MynyraEvent& event, EventFlush) noexcept override
    {
        events.push_back(event);
        lines.push_back(serializeMynyraEvent(event));
        return true;
    }
    std::vector<MynyraEvent> events;
    std::vector<std::string> lines;
};

class FakeDemoAdapter final : public IBrokerAdapter {
public:
    explicit FakeDemoAdapter(bool directFill,
                             bool suppressEntryEvents = false,
                             bool partialFirstClose = false,
                             bool ambiguousPartialClose = false,
                             bool suppressedEntryCreatesExposure = true)
        : m_directFill(directFill)
        , m_suppressEntryEvents(suppressEntryEvents)
        , m_partialFirstClose(partialFirstClose)
        , m_ambiguousPartialClose(ambiguousPartialClose)
        , m_suppressedEntryCreatesExposure(suppressedEntryCreatesExposure)
    {}

    void setAcknowledgementCallback(AcknowledgementCallback callback) override
        { m_ack = std::move(callback); }
    void setExecutionCallback(ExecutionCallback callback) override
        { m_execution = std::move(callback); }
    void setCancelCallback(CancelCallback callback) override
        { m_cancel = std::move(callback); }
    void setHealthCallback(HealthCallback callback) override
        { m_healthCallback = std::move(callback); }
    bool connect() override { m_connected = true; return true; }
    void disconnect() noexcept override { m_connected = false; }
    bool isConnected() const noexcept override { return m_connected; }

    bool submit(const NormalizedOrder& order) override
    {
        if (!m_connected) return false;
        ++submitCount;
        submitted.push_back(order);
        const bool opening = order.request.positionEffect == PositionEffect::Open;
        const bool ambiguousClose = !opening && m_ambiguousPartialClose
            && !m_partialCloseUsed;
        const bool failedCloseSubmit = !opening && m_failFirstCloseSubmit
            && !m_failedCloseSubmitUsed;
        if (!m_directFill && !(opening && m_suppressEntryEvents)
            && !ambiguousClose && !failedCloseSubmit && m_ack) {
            OrderAcknowledgement ack;
            ack.localOrderId = order.request.localOrderId;
            ack.externalOrderId = "fake-order-"
                + std::to_string(order.request.localOrderId);
            ack.accepted = true;
            ack.timestampNs = nowNs();
            ack.sequence = 10 + submitCount * 10;
            ack.eventKey = "fake-ack-"
                + std::to_string(order.request.localOrderId);
            m_ack(ack);
        }

        if (opening) {
            if (!m_suppressEntryEvents || m_suppressedEntryCreatesExposure) {
                m_position = PositionSnapshot{
                    "XAUUSD", order.normalizedQuantity,
                    order.normalizedReferencePrice.value(), "logical-position-1",
                    order.request.positionSide};
                if (m_partialSuppressedEntry) {
                    m_position->quantity.units /= 2;
                }
            }
        } else {
            require(order.request.logicalPositionId.has_value()
                    && *order.request.logicalPositionId == "logical-position-1",
                    "close must use reconciled logical position identity");
            require(order.normalizedQuantity == m_position->quantity,
                    "close must use reconciled exact quantity");
        }

        if (opening && m_failFirstEntrySubmit) {
            m_failFirstEntrySubmit = false;
            return false;
        }
        if (failedCloseSubmit) {
            m_failedCloseSubmitUsed = true;
            return false;
        }
        if (opening && m_suppressEntryEvents) return true;

        Decimal64 filledQuantity = order.normalizedQuantity;
        if (!opening && (m_partialFirstClose || m_ambiguousPartialClose)
            && !m_partialCloseUsed) {
            m_partialCloseUsed = true;
            filledQuantity.units /= 2;
        }

        if (m_execution && !ambiguousClose) {
            ExecutionEvent fill;
            fill.localOrderId = order.request.localOrderId;
            fill.externalOrderId = "fake-order-"
                + std::to_string(order.request.localOrderId);
            fill.cumulativeFilledQuantity = filledQuantity;
            fill.remainingQuantity = Decimal64{
                order.normalizedQuantity.units - filledQuantity.units,
                order.normalizedQuantity.scale};
            fill.fillPrice = order.normalizedReferencePrice.value();
            fill.fee = Decimal64{0, 8};
            fill.timestampNs = nowNs();
            fill.sequence = 11 + submitCount * 10;
            fill.eventKey = "fake-fill-"
                + std::to_string(order.request.localOrderId);
            fill.positionSide = order.request.positionSide;
            fill.positionEffect = order.request.positionEffect;
            m_execution(fill);
        }
        if (opening && m_reconcileHalfAfterFullEntryFill) {
            m_position->quantity.units /= 2;
        }
        if (!opening) {
            const bool preserveResidual =
                m_reportFullCloseWithoutReduction
                && !m_reportFullCloseWithoutReductionUsed;
            m_reportFullCloseWithoutReductionUsed =
                m_reportFullCloseWithoutReductionUsed || preserveResidual;
            if (!preserveResidual) {
                m_position->quantity.units -= filledQuantity.units;
                if (m_position->quantity.isZero()) m_position.reset();
            }
        }
        return true;
    }

    bool cancel(const CancelRequest&) override { return false; }
    ReconciliationSnapshot reconcile(std::uint64_t timestampNs) override
    {
        ReconciliationSnapshot snapshot;
        snapshot.snapshotVersion = ++m_snapshotVersion;
        snapshot.timestampNs = timestampNs;
        snapshot.account = account(snapshot.snapshotVersion);
        if (m_position.has_value()) snapshot.positions.push_back(*m_position);
        snapshot.status = ReconciliationStatus::Matched;
        snapshot.pendingOrderCount = 0;
        snapshot.connectionGeneration = 1;
        snapshot.complete = true;
        return snapshot;
    }
    AdapterHealthEvent health() const override
    {
        AdapterHealthEvent event;
        event.state = m_connected ? AdapterHealthState::Connected
                                  : AdapterHealthState::Disconnected;
        return event;
    }
    std::optional<AccountSnapshot> accountSnapshot() const override
        { return account(m_snapshotVersion); }
    std::optional<InstrumentSpec> instrumentSpec(const std::string& symbol) const override
        { return symbol == "XAUUSD" ? std::optional<InstrumentSpec>(instrument())
                                    : std::nullopt; }

    void usePartialSuppressedEntry() noexcept
        { m_partialSuppressedEntry = true; }
    void failFirstEntrySubmitAfterExposure() noexcept
        { m_failFirstEntrySubmit = true; }
    void failFirstCloseSubmitWithoutFill() noexcept
        { m_failFirstCloseSubmit = true; }
    void reconcileHalfAfterFullEntryFill() noexcept
        { m_reconcileHalfAfterFullEntryFill = true; }
    void reportFullCloseWithoutExposureReduction() noexcept
        { m_reportFullCloseWithoutReduction = true; }

    int submitCount{0};
    std::vector<NormalizedOrder> submitted;

private:
    bool m_directFill{false};
    bool m_suppressEntryEvents{false};
    bool m_partialFirstClose{false};
    bool m_ambiguousPartialClose{false};
    bool m_suppressedEntryCreatesExposure{true};
    bool m_partialSuppressedEntry{false};
    bool m_failFirstEntrySubmit{false};
    bool m_failFirstCloseSubmit{false};
    bool m_failedCloseSubmitUsed{false};
    bool m_reconcileHalfAfterFullEntryFill{false};
    bool m_reportFullCloseWithoutReduction{false};
    bool m_reportFullCloseWithoutReductionUsed{false};
    bool m_partialCloseUsed{false};
    bool m_connected{false};
    std::uint64_t m_snapshotVersion{1};
    std::optional<PositionSnapshot> m_position;
    AcknowledgementCallback m_ack;
    ExecutionCallback m_execution;
    CancelCallback m_cancel;
    HealthCallback m_healthCallback;
};

void testStrategyPipelineEligibility()
{
    FixedStrategy sma("SMA_01", 1.0);
    FixedStrategy mr("MR_01", 1.0);
    PortfolioAllocator allocator(0.3, 0.3);
    allocator.setWeight("SMA_01", 0.6);
    allocator.setWeight("MR_01", 0.4);
    StrategyPipeline pipeline({&sma, &mr}, allocator);
    MarketCandle candle;
    candle.symbol = "XAUUSD";
    candle.epochTimestamp = 1;
    candle.open = 2300.0;
    candle.high = 2301.0;
    candle.low = 2299.0;
    candle.close = 2300.5;
    candle.volume = 10.0;
    const auto warmup = pipeline.advance(candle, false);
    const auto live = pipeline.advance(candle, true);
    require(warmup.action == Signal::BUY && live.action == Signal::BUY,
            "pipeline extraction must preserve ensemble action");
    require(!warmup.executionEligible && live.executionEligible,
            "warmup decision must remain ineligible for execution");
}

void testStrategyPipelineParity()
{
    SmaCrossStrategy pipelineSma(12, 26, "SMA_01");
    MeanReversionStrategy pipelineMr(20, 2.0, 14, "MR_01");
    RegimeDetector pipelineRegime;
    PortfolioAllocator pipelineAllocator(0.3, 0.3);
    pipelineAllocator.setWeight("SMA_01", 0.6);
    pipelineAllocator.setWeight("MR_01", 0.4);
    pipelineAllocator.setRegimeDetector(&pipelineRegime);
    StrategyPipeline pipeline(
        {&pipelineSma, &pipelineMr}, pipelineAllocator, &pipelineRegime);

    SmaCrossStrategy referenceSma(12, 26, "SMA_01");
    MeanReversionStrategy referenceMr(20, 2.0, 14, "MR_01");
    RegimeDetector referenceRegime;
    PortfolioAllocator referenceAllocator(0.3, 0.3);
    referenceAllocator.setWeight("SMA_01", 0.6);
    referenceAllocator.setWeight("MR_01", 0.4);
    referenceAllocator.setRegimeDetector(&referenceRegime);

    for (std::uint64_t index = 0; index < 140; ++index) {
        const double close = 2300.0
            + std::sin(static_cast<double>(index) / 5.0) * 8.0
            + static_cast<double>(index) * 0.05;
        MarketCandle input;
        input.symbol = "XAUUSD";
        input.epochTimestamp = 60 * (index + 1);
        input.open = close - 0.2;
        input.high = close + 0.8;
        input.low = close - 0.8;
        input.close = close;
        input.volume = 10.0 + static_cast<double>(index);

        const StrategyDecision actual = pipeline.advance(input, index >= 100);
        referenceRegime.update(input.high, input.low, input.close);
        std::vector<AlphaSignal> signals{
            referenceSma.generateSignal(input),
            referenceMr.generateSignal(input)};
        const AllocationResult expected = referenceAllocator.ensemble(signals);
        require(actual.action == expected.action
                    && std::abs(actual.totalConviction
                                - expected.totalConviction) < 1e-12
                    && actual.strategyAttribution
                        == (expected.dominantStrategyId.empty()
                                ? "ENSEMBLE" : expected.dominantStrategyId)
                    && actual.regime == expected.regime,
                "StrategyPipeline extraction changed ensemble behavior");
        require(actual.executionEligible == (index >= 100),
                "historical warmup eligibility changed during extraction");
    }
}

void testExactRiskLongShortAndClose()
{
    PortfolioManager portfolio;
    RiskEngine risk(portfolio, 1);
    auto context = emptyRiskContext();
    for (const PositionSide side : {PositionSide::Long, PositionSide::Short}) {
        auto sideContext = context;
        sideContext.expectedMarginSide = side;
        OrderIntent open;
        open.side = side;
        open.effect = PositionEffect::Open;
        open.exactQuantity = context.instrument.minimumQuantity;
        open.referencePrice = side == PositionSide::Long
            ? context.ask : context.bid;
        open.canonicalSymbol = "XAUUSD";
        require(risk.evaluateOrder(open, sideContext).allowed,
                "minimum-volume long and short must pass complete Demo risk evidence");
    }
    auto stale = context;
    stale.bboSourceTimestampNs = stale.evaluationTimestampNs - 6'000'000'000ULL;
    OrderIntent staleOpen;
    staleOpen.side = PositionSide::Long;
    staleOpen.effect = PositionEffect::Open;
    staleOpen.exactQuantity = context.instrument.minimumQuantity;
    staleOpen.referencePrice = context.ask;
    staleOpen.canonicalSymbol = "XAUUSD";
    require(!risk.evaluateOrder(staleOpen, stale).allowed,
            "stale BBO must reject opening exposure");

    auto staleAccount = context;
    staleAccount.account.sourceTimestampNs =
        staleAccount.evaluationTimestampNs - 6'000'000'000ULL;
    require(!risk.evaluateOrder(staleOpen, staleAccount).allowed,
            "stale account snapshot must reject opening exposure");
    auto staleReconciliation = context;
    staleReconciliation.reconciliation.timestampNs =
        staleReconciliation.evaluationTimestampNs - 6'000'000'000ULL;
    require(!risk.evaluateOrder(staleOpen, staleReconciliation).allowed,
            "stale reconciliation must reject opening exposure");
    auto inconsistentExposure = context;
    inconsistentExposure.grossExposure = Decimal64{1, 2};
    require(!risk.evaluateOrder(staleOpen, inconsistentExposure).allowed,
            "nonzero exposure with empty reconciliation must fail closed");
    auto wrongDirectionMargin = context;
    wrongDirectionMargin.expectedMarginSide = PositionSide::Short;
    require(!risk.evaluateOrder(staleOpen, wrongDirectionMargin).allowed,
            "expected margin for the opposite direction must fail closed");
    auto mismatchedReference = staleOpen;
    mismatchedReference.referencePrice = context.bid;
    require(!risk.evaluateOrder(mismatchedReference, context).allowed,
            "opening reference price must match the direction-specific BBO");

    context.reconciliation.positions.push_back(PositionSnapshot{
        "XAUUSD", Decimal64{100, 2}, Decimal64{230000, 2},
        "logical-1", PositionSide::Short});
    OrderIntent close;
    close.side = PositionSide::Short;
    close.effect = PositionEffect::Close;
    close.exactQuantity = Decimal64{100, 2};
    close.referencePrice = context.ask;
    close.canonicalSymbol = "XAUUSD";
    close.logicalPositionId = "logical-1";
    risk.setHaltTrading(true);
    require(risk.evaluateOrder(close, context).allowed,
            "reconciled close must remain allowed during halt");
}

void testMirrorRejectsDuplicateAndMismatch()
{
    BrokerPortfolioMirror mirror;
    OrderIntent open;
    open.side = PositionSide::Short;
    open.effect = PositionEffect::Open;
    open.exactQuantity = Decimal64{100, 2};
    open.referencePrice = Decimal64{230000, 2};
    open.canonicalSymbol = "XAUUSD";
    require(mirror.registerIntent(1, open), "mirror must register exact intent");
    ExecutionEvent fill;
    fill.localOrderId = 1;
    fill.externalOrderId = "internal";
    fill.cumulativeFilledQuantity = Decimal64{100, 2};
    fill.remainingQuantity = Decimal64{0, 2};
    fill.fillPrice = Decimal64{230000, 2};
    fill.fee = Decimal64{0, 2};
    fill.timestampNs = nowNs();
    fill.sequence = 1;
    fill.eventKey = "event-1";
    fill.positionSide = PositionSide::Short;
    fill.positionEffect = PositionEffect::Open;
    require(mirror.applyExecution(fill) == MirrorApplyResult::Applied,
            "first valid mirror fill must apply");
    require(mirror.applyExecution(fill) == MirrorApplyResult::Duplicate,
            "duplicate mirror fill must be rejected");

    auto reconciliation = emptyRiskContext().reconciliation;
    reconciliation.snapshotVersion = 2;
    reconciliation.account = account(2);
    reconciliation.positions.push_back(PositionSnapshot{
        "XAUUSD", Decimal64{100, 2}, Decimal64{230000, 2},
        "local-order-1", PositionSide::Short});
    require(mirror.applyReconciliation(reconciliation),
            "new authoritative reconciliation must apply");
    require(!mirror.applyReconciliation(reconciliation),
            "duplicate reconciliation version must be rejected");
    auto staleReconciliation = reconciliation;
    staleReconciliation.snapshotVersion = 1;
    staleReconciliation.account = account(1);
    require(!mirror.applyReconciliation(staleReconciliation),
            "stale reconciliation version must be rejected");
}

#if TRADEBOT_ENABLE_CTRADER_DEMO
void testReadOnlyDecisionCannotSubmit()
{
    SystemConfig config;
    config.mode = SystemMode::DEMO;
    PortfolioManager portfolio;
    RiskEngine risk(portfolio, 1);
    risk.setSystemConfig(&config);
    auto adapter = std::make_unique<FakeDemoAdapter>(false);
    auto* fake = adapter.get();
    BrokerGateway gateway(config, portfolio, std::move(adapter), true);
    ExecutionEngine execution(portfolio, risk, "XAUUSD", 0.0, 0.0, 0.0);
    execution.bindBrokerGateway(&gateway);
    BrokerPortfolioMirror mirror;
    VectorSink sink;
    MynyraDemoCommissioningController controller(
        MynyraDemoCommissioningConfig{false, std::chrono::milliseconds(20)},
        "read-only-session", execution, gateway, mirror, sink);
    gateway.connect();
    StrategyDecision decision;
    decision.canonicalSymbol = "XAUUSD";
    decision.action = Signal::BUY;
    decision.strategyAttribution = "SMA_01";
    decision.executionEligible = true;
    decision.candleTimestamp = nowNs();
    require(controller.commission(decision, emptyRiskContext())
                == MynyraDemoOutcome::ReadOnly,
            "uncommissioned DEMO decision must stay read-only");
    require(fake->submitCount == 0 && !controller.entryAttempted(),
            "read-only DEMO path submitted an order");
}

void testClosedLoop(PositionSide side, bool directFill)
{
    SystemConfig config;
    config.mode = SystemMode::DEMO;
    PortfolioManager portfolio;
    RiskEngine risk(portfolio, 1);
    risk.setSystemConfig(&config);
    auto adapter = std::make_unique<FakeDemoAdapter>(directFill);
    auto* fake = adapter.get();
    BrokerGateway gateway(config, portfolio, std::move(adapter), true);
    ExecutionEngine execution(portfolio, risk, "XAUUSD", 0.0, 0.0, 0.0);
    execution.bindBrokerGateway(&gateway);
    BrokerPortfolioMirror mirror;
    VectorSink sink;
    MynyraDemoCommissioningController controller(
        MynyraDemoCommissioningConfig{true, std::chrono::milliseconds(100)},
        "test-session", execution, gateway, mirror, sink);
    gateway.connect();

    StrategyDecision decision;
    decision.canonicalSymbol = "XAUUSD";
    decision.action = side == PositionSide::Long ? Signal::BUY : Signal::SELL;
    decision.totalConviction = side == PositionSide::Long ? 0.8 : -0.8;
    decision.referencePrice = 2300.0;
    decision.strategyAttribution = "SMA_01";
    decision.candleTimestamp = nowNs();
    decision.executionEligible = true;
    require(controller.commission(decision, emptyRiskContext(side))
                == MynyraDemoOutcome::Succeeded,
            "fake Demo closed loop must succeed");
    require(fake->submitCount == 2,
            "closed loop must submit exactly one entry and one close");
    require(fake->submitted[0].request.positionEffect == PositionEffect::Open
            && fake->submitted[0].request.positionSide == side,
            "entry must preserve side/effect");
    require(fake->submitted[1].request.positionEffect == PositionEffect::Close
            && fake->submitted[1].request.positionSide == side,
            "close must preserve reconciled side/effect");
    require(mirror.isFlat(), "final mirror must be flat");
    require(!sink.events.empty()
            && sink.events.back().eventType == "mynyra_demo_m1_succeeded",
            "success marker must be the terminal event");
}


void testAmbiguousEntryIsClosedWithoutRetry()
{
    SystemConfig config;
    config.mode = SystemMode::DEMO;
    PortfolioManager portfolio;
    RiskEngine risk(portfolio, 1);
    risk.setSystemConfig(&config);
    auto adapter = std::make_unique<FakeDemoAdapter>(false, true, false);
    auto* fake = adapter.get();
    BrokerGateway gateway(config, portfolio, std::move(adapter), true);
    ExecutionEngine execution(portfolio, risk, "XAUUSD", 0.0, 0.0, 0.0);
    execution.bindBrokerGateway(&gateway);
    BrokerPortfolioMirror mirror;
    VectorSink sink;
    MynyraDemoCommissioningController controller(
        MynyraDemoCommissioningConfig{true, std::chrono::milliseconds(20)},
        "ambiguous-session", execution, gateway, mirror, sink);
    gateway.connect();
    StrategyDecision decision;
    decision.canonicalSymbol = "XAUUSD";
    decision.action = Signal::BUY;
    decision.strategyAttribution = "SMA_01";
    decision.executionEligible = true;
    decision.candleTimestamp = nowNs();
    require(controller.commission(decision, emptyRiskContext())
                == MynyraDemoOutcome::RecoveryRequired,
            "ambiguous entry evidence must never produce success");
    require(fake->submitCount == 2,
            "reconciled ambiguous exposure must be closed without an entry retry");
    require(mirror.isFlat(), "ambiguous confirmed exposure must be flattened");
}

void testAmbiguousEntryReconciledFlatDoesNotDemandRecovery()
{
    SystemConfig config;
    config.mode = SystemMode::DEMO;
    PortfolioManager portfolio;
    RiskEngine risk(portfolio, 1);
    risk.setSystemConfig(&config);
    auto adapter = std::make_unique<FakeDemoAdapter>(
        false, true, false, false, false);
    auto* fake = adapter.get();
    BrokerGateway gateway(config, portfolio, std::move(adapter), true);
    ExecutionEngine execution(portfolio, risk, "XAUUSD", 0.0, 0.0, 0.0);
    execution.bindBrokerGateway(&gateway);
    BrokerPortfolioMirror mirror;
    VectorSink sink;
    MynyraDemoCommissioningController controller(
        MynyraDemoCommissioningConfig{true, std::chrono::milliseconds(20)},
        "ambiguous-flat-session", execution, gateway, mirror, sink);
    gateway.connect();
    StrategyDecision decision;
    decision.canonicalSymbol = "XAUUSD";
    decision.action = Signal::BUY;
    decision.strategyAttribution = "SMA_01";
    decision.executionEligible = true;
    decision.candleTimestamp = nowNs();
    require(controller.commission(decision, emptyRiskContext())
                == MynyraDemoOutcome::Failed,
            "authoritative flat reconciliation must resolve entry ambiguity");
    require(fake->submitCount == 1 && mirror.isFlat(),
            "flat ambiguous entry must not retry or submit a close");
}

void testTransportAmbiguousEntryIsReconciledWithoutRetry()
{
    SystemConfig config;
    config.mode = SystemMode::DEMO;
    PortfolioManager portfolio;
    RiskEngine risk(portfolio, 1);
    risk.setSystemConfig(&config);
    auto adapter = std::make_unique<FakeDemoAdapter>(false, true, false);
    auto* fake = adapter.get();
    fake->failFirstEntrySubmitAfterExposure();
    BrokerGateway gateway(config, portfolio, std::move(adapter), true);
    ExecutionEngine execution(portfolio, risk, "XAUUSD", 0.0, 0.0, 0.0);
    execution.bindBrokerGateway(&gateway);
    BrokerPortfolioMirror mirror;
    VectorSink sink;
    MynyraDemoCommissioningController controller(
        MynyraDemoCommissioningConfig{true, std::chrono::milliseconds(20)},
        "transport-ambiguous-entry", execution, gateway, mirror, sink);
    gateway.connect();
    StrategyDecision decision;
    decision.canonicalSymbol = "XAUUSD";
    decision.action = Signal::BUY;
    decision.strategyAttribution = "SMA_01";
    decision.executionEligible = true;
    decision.candleTimestamp = nowNs();
    require(controller.commission(decision, emptyRiskContext())
                == MynyraDemoOutcome::RecoveryRequired,
            "transport-ambiguous entry must never produce success");
    require(fake->submitCount == 2 && mirror.isFlat(),
            "transport-ambiguous entry must reconcile and close without retry");
}

void testConfirmedPartialEntryIsClosedWithoutRetry()
{
    SystemConfig config;
    config.mode = SystemMode::DEMO;
    PortfolioManager portfolio;
    RiskEngine risk(portfolio, 1);
    risk.setSystemConfig(&config);
    auto adapter = std::make_unique<FakeDemoAdapter>(false, true, false);
    auto* fake = adapter.get();
    fake->usePartialSuppressedEntry();
    BrokerGateway gateway(config, portfolio, std::move(adapter), true);
    ExecutionEngine execution(portfolio, risk, "XAUUSD", 0.0, 0.0, 0.0);
    execution.bindBrokerGateway(&gateway);
    BrokerPortfolioMirror mirror;
    VectorSink sink;
    MynyraDemoCommissioningController controller(
        MynyraDemoCommissioningConfig{true, std::chrono::milliseconds(20)},
        "partial-entry", execution, gateway, mirror, sink);
    gateway.connect();
    StrategyDecision decision;
    decision.canonicalSymbol = "XAUUSD";
    decision.action = Signal::SELL;
    decision.strategyAttribution = "MR_01";
    decision.executionEligible = true;
    decision.candleTimestamp = nowNs();
    require(controller.commission(
                decision, emptyRiskContext(PositionSide::Short))
                == MynyraDemoOutcome::RecoveryRequired,
            "partial entry evidence must never produce success");
    require(fake->submitCount == 2 && mirror.isFlat(),
            "confirmed partial entry must be closed without an entry retry");
    require(fake->submitted[1].normalizedQuantity.units
                == fake->submitted[0].normalizedQuantity.units / 2,
            "partial-entry close must use the exact reconciled residual");
}

void testEntryFillReconciliationMismatchFlattensWithoutSuccess()
{
    SystemConfig config;
    config.mode = SystemMode::DEMO;
    PortfolioManager portfolio;
    RiskEngine risk(portfolio, 1);
    risk.setSystemConfig(&config);
    auto adapter = std::make_unique<FakeDemoAdapter>(false);
    auto* fake = adapter.get();
    fake->reconcileHalfAfterFullEntryFill();
    BrokerGateway gateway(config, portfolio, std::move(adapter), true);
    ExecutionEngine execution(portfolio, risk, "XAUUSD", 0.0, 0.0, 0.0);
    execution.bindBrokerGateway(&gateway);
    BrokerPortfolioMirror mirror;
    VectorSink sink;
    MynyraDemoCommissioningController controller(
        MynyraDemoCommissioningConfig{true, std::chrono::milliseconds(20)},
        "entry-reconciliation-mismatch", execution, gateway, mirror, sink);
    gateway.connect();
    StrategyDecision decision;
    decision.canonicalSymbol = "XAUUSD";
    decision.action = Signal::BUY;
    decision.strategyAttribution = "SMA_01";
    decision.executionEligible = true;
    decision.candleTimestamp = nowNs();
    require(controller.commission(decision, emptyRiskContext())
                == MynyraDemoOutcome::RecoveryRequired,
            "entry fill/reconciliation mismatch must not produce success");
    require(fake->submitCount == 2 && mirror.isFlat(),
            "confirmed mismatched entry exposure must still be flattened");
}

void testResidualCloseRecovery()
{
    SystemConfig config;
    config.mode = SystemMode::DEMO;
    PortfolioManager portfolio;
    RiskEngine risk(portfolio, 1);
    risk.setSystemConfig(&config);
    auto adapter = std::make_unique<FakeDemoAdapter>(false, false, true);
    auto* fake = adapter.get();
    BrokerGateway gateway(config, portfolio, std::move(adapter), true);
    ExecutionEngine execution(portfolio, risk, "XAUUSD", 0.0, 0.0, 0.0);
    execution.bindBrokerGateway(&gateway);
    BrokerPortfolioMirror mirror;
    VectorSink sink;
    MynyraDemoCommissioningController controller(
        MynyraDemoCommissioningConfig{true, std::chrono::milliseconds(20)},
        "residual-session", execution, gateway, mirror, sink);
    gateway.connect();
    StrategyDecision decision;
    decision.canonicalSymbol = "XAUUSD";
    decision.action = Signal::SELL;
    decision.strategyAttribution = "MR_01";
    decision.executionEligible = true;
    decision.candleTimestamp = nowNs();
    const auto outcome = controller.commission(
        decision, emptyRiskContext(PositionSide::Short));
    if (outcome != MynyraDemoOutcome::Succeeded) {
        for (const auto& event : sink.events) {
            std::cerr << "EVENT: " << event.eventType
                      << " failure=" << static_cast<int>(event.failure) << '\n';
        }
        for (const auto& order : fake->submitted) {
            std::cerr << "ORDER: " << order.request.localOrderId
                      << " effect=" << static_cast<int>(order.request.positionEffect)
                      << " quantity=" << order.normalizedQuantity.units << '\n';
        }
    }
    require(outcome == MynyraDemoOutcome::Succeeded,
            "one exact residual close must complete the lifecycle");
    require(fake->submitCount == 3,
            "residual recovery must be one entry plus two close requests");
    require(mirror.isFlat(), "residual recovery must reconcile flat");
}

void testAmbiguousCloseFlattensWithoutFalseSuccess()
{
    SystemConfig config;
    config.mode = SystemMode::DEMO;
    PortfolioManager portfolio;
    RiskEngine risk(portfolio, 1);
    risk.setSystemConfig(&config);
    auto adapter = std::make_unique<FakeDemoAdapter>(
        false, false, false, true);
    auto* fake = adapter.get();
    BrokerGateway gateway(config, portfolio, std::move(adapter), true);
    ExecutionEngine execution(portfolio, risk, "XAUUSD", 0.0, 0.0, 0.0);
    execution.bindBrokerGateway(&gateway);
    BrokerPortfolioMirror mirror;
    VectorSink sink;
    MynyraDemoCommissioningController controller(
        MynyraDemoCommissioningConfig{true, std::chrono::milliseconds(20)},
        "ambiguous-close-session", execution, gateway, mirror, sink);
    gateway.connect();
    StrategyDecision decision;
    decision.canonicalSymbol = "XAUUSD";
    decision.action = Signal::BUY;
    decision.strategyAttribution = "SMA_01";
    decision.executionEligible = true;
    decision.candleTimestamp = nowNs();
    require(controller.commission(decision, emptyRiskContext())
                == MynyraDemoOutcome::RecoveryRequired,
            "reconciliation-only close evidence must never produce success");
    require(fake->submitCount == 3 && mirror.isFlat(),
            "ambiguous partial close must use one exact residual close and flatten");
    require(!sink.events.empty()
            && sink.events.back().eventType == "mynyra_demo_recovery_required",
            "ambiguous close must terminate with recovery-required evidence");
}

void testZeroFillAmbiguousCloseGetsOneExactRetry()
{
    SystemConfig config;
    config.mode = SystemMode::DEMO;
    PortfolioManager portfolio;
    RiskEngine risk(portfolio, 1);
    risk.setSystemConfig(&config);
    auto adapter = std::make_unique<FakeDemoAdapter>(false);
    auto* fake = adapter.get();
    fake->failFirstCloseSubmitWithoutFill();
    BrokerGateway gateway(config, portfolio, std::move(adapter), true);
    ExecutionEngine execution(portfolio, risk, "XAUUSD", 0.0, 0.0, 0.0);
    execution.bindBrokerGateway(&gateway);
    BrokerPortfolioMirror mirror;
    VectorSink sink;
    MynyraDemoCommissioningController controller(
        MynyraDemoCommissioningConfig{true, std::chrono::milliseconds(20)},
        "zero-fill-close", execution, gateway, mirror, sink);
    gateway.connect();
    StrategyDecision decision;
    decision.canonicalSymbol = "XAUUSD";
    decision.action = Signal::BUY;
    decision.strategyAttribution = "SMA_01";
    decision.executionEligible = true;
    decision.candleTimestamp = nowNs();
    require(controller.commission(decision, emptyRiskContext())
                == MynyraDemoOutcome::RecoveryRequired,
            "ambiguous close retry must not manufacture success evidence");
    require(fake->submitCount == 3 && mirror.isFlat(),
            "zero-fill close ambiguity must receive one exact retry and flatten");
    require(fake->submitted[1].normalizedQuantity
                == fake->submitted[2].normalizedQuantity,
            "zero-fill close retry must preserve the reconciled residual");
}

void testFilledCloseWithBrokerResidualGetsSafetyClose()
{
    SystemConfig config;
    config.mode = SystemMode::DEMO;
    PortfolioManager portfolio;
    RiskEngine risk(portfolio, 1);
    risk.setSystemConfig(&config);
    auto adapter = std::make_unique<FakeDemoAdapter>(false);
    auto* fake = adapter.get();
    fake->reportFullCloseWithoutExposureReduction();
    BrokerGateway gateway(config, portfolio, std::move(adapter), true);
    ExecutionEngine execution(portfolio, risk, "XAUUSD", 0.0, 0.0, 0.0);
    execution.bindBrokerGateway(&gateway);
    BrokerPortfolioMirror mirror;
    VectorSink sink;
    MynyraDemoCommissioningController controller(
        MynyraDemoCommissioningConfig{true, std::chrono::milliseconds(20)},
        "filled-close-residual", execution, gateway, mirror, sink);
    gateway.connect();
    StrategyDecision decision;
    decision.canonicalSymbol = "XAUUSD";
    decision.action = Signal::SELL;
    decision.strategyAttribution = "MR_01";
    decision.executionEligible = true;
    decision.candleTimestamp = nowNs();
    require(controller.commission(
                decision, emptyRiskContext(PositionSide::Short))
                == MynyraDemoOutcome::RecoveryRequired,
            "filled close with broker residual must not produce success");
    require(fake->submitCount == 3 && mirror.isFlat(),
            "broker residual after a filled close must receive one safety close");
}
#endif

void testEventSchemaCannotCarryProviderSecrets()
{
    MynyraEvent event;
    event.sessionId = "safe-session";
    event.localSequence = 1;
    event.mode = SystemMode::DEMO;
    event.canonicalSymbol = "XAUUSD";
    event.eventType = "mynyra_demo_test";
    event.failure = FailureCategory::Authentication;
    const std::string encoded = serializeMynyraEvent(event);
    require(encoded.find("accessToken") == std::string::npos
            && encoded.find("provider_order_id") == std::string::npos
            && encoded.find("raw_error") == std::string::npos,
            "event schema must exclude provider secrets and native identifiers");
}

} // namespace

int main()
{
    testStrategyPipelineEligibility();
    testStrategyPipelineParity();
    testExactRiskLongShortAndClose();
    testMirrorRejectsDuplicateAndMismatch();
    testEventSchemaCannotCarryProviderSecrets();
#if TRADEBOT_ENABLE_CTRADER_DEMO
    testReadOnlyDecisionCannotSubmit();
    testClosedLoop(PositionSide::Long, false);
    testClosedLoop(PositionSide::Short, true);
    testAmbiguousEntryIsClosedWithoutRetry();
    testAmbiguousEntryReconciledFlatDoesNotDemandRecovery();
    testTransportAmbiguousEntryIsReconciledWithoutRetry();
    testConfirmedPartialEntryIsClosedWithoutRetry();
    testEntryFillReconciliationMismatchFlattensWithoutSuccess();
    testResidualCloseRecovery();
    testAmbiguousCloseFlattensWithoutFalseSuccess();
    testZeroFillAmbiguousCloseGetsOneExactRetry();
    testFilledCloseWithBrokerResidualGetsSafetyClose();
#endif
    std::cout << "mynyra_demo_core_tests: PASS\n";
    return 0;
}
