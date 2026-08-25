#include "MynyraDemoCommissioning.hpp"

#include <algorithm>
#include <limits>
#include <thread>

MynyraDemoCommissioningController::MynyraDemoCommissioningController(
    MynyraDemoCommissioningConfig config,
    std::string sessionId,
    ExecutionEngine& execution,
    BrokerGateway& gateway,
    BrokerPortfolioMirror& mirror,
    IEventSink& sink,
    std::shared_ptr<std::atomic<std::uint64_t>> eventSequence)
    : m_config(config)
    , m_execution(execution)
    , m_gateway(gateway)
    , m_mirror(mirror)
    , m_state(std::make_shared<SharedState>())
{
    m_state->mirror = &mirror;
    m_state->sink = &sink;
    m_state->sessionId = std::move(sessionId);
    m_state->eventSequence = eventSequence
        ? std::move(eventSequence)
        : std::make_shared<std::atomic<std::uint64_t>>(0);
    const auto shared = m_state;
    m_gateway.addAcknowledgementCallback(
        [shared](const OrderAcknowledgement& event) {
            onAcknowledgement(shared, event);
        });
    m_gateway.addExecutionCallback(
        [shared](const ExecutionEvent& event) {
            onExecution(shared, event);
        });
}

MynyraDemoCommissioningController::~MynyraDemoCommissioningController()
{
    m_state->active.store(false);
    m_state->changed.notify_all();
}

std::uint64_t MynyraDemoCommissioningController::nowNs() noexcept
{
    const auto count = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return count > 0 ? static_cast<std::uint64_t>(count) : 0;
}

const char* MynyraDemoCommissioningController::legName(Leg leg) noexcept
{
    switch (leg) {
        case Leg::Entry: return "entry";
        case Leg::Close: return "close";
        case Leg::ResidualClose: return "residual_close";
    }
    return "unknown";
}

bool MynyraDemoCommissioningController::emit(
    const std::shared_ptr<SharedState>& state,
    MynyraEvent event,
    EventFlush flush) noexcept
{
    if (!state->active.load() || state->sink == nullptr) return false;
    event.sessionId = state->sessionId;
    event.localSequence = state->eventSequence->fetch_add(1) + 1;
    event.emittedTimestampNs = nowNs();
    const bool ok = state->sink->emit(event, flush);
    if (!ok) state->sinkHealthy.store(false);
    return ok;
}

void MynyraDemoCommissioningController::onAcknowledgement(
    const std::shared_ptr<SharedState>& state,
    const OrderAcknowledgement& event)
{
    if (!state->active.load()) return;
    Leg leg = Leg::Entry;
    {
        std::lock_guard<std::mutex> lock(state->mutex);
        const auto found = state->orders.find(event.localOrderId);
        if (found == state->orders.end()) return;
        leg = found->second.leg;
        found->second.acceptanceEvidence = event.accepted;
        found->second.rejected = !event.accepted;
        found->second.failure = event.failure;
    }
    MynyraEvent output;
    output.sourceTimestampNs = event.timestampNs;
    output.canonicalSymbol = "XAUUSD";
    output.eventType = std::string("mynyra_demo_") + legName(leg)
                     + (event.accepted ? "_accepted" : "_rejected");
    output.localOrderId = event.localOrderId;
    output.lifecycleState = event.accepted
        ? OrderLifecycleState::Accepted : OrderLifecycleState::Rejected;
    output.failure = event.failure;
    emit(state, std::move(output), EventFlush::LifecycleBoundary);
    state->changed.notify_all();
}

void MynyraDemoCommissioningController::onExecution(
    const std::shared_ptr<SharedState>& state,
    const ExecutionEvent& event)
{
    if (!state->active.load()) return;
    Leg leg = Leg::Entry;
    OrderIntent intent;
    MirrorApplyResult mirrorResult = MirrorApplyResult::InvalidEvent;
    {
        std::lock_guard<std::mutex> lock(state->mutex);
        const auto found = state->orders.find(event.localOrderId);
        if (found == state->orders.end()) return;
        leg = found->second.leg;
        intent = found->second.intent;
        mirrorResult = state->mirror == nullptr
            ? MirrorApplyResult::InvalidEvent
            : state->mirror->applyExecution(event);
        if (mirrorResult == MirrorApplyResult::Applied) {
            found->second.completeFill = event.remainingQuantity.isZero();
            found->second.acceptanceEvidence =
                found->second.acceptanceEvidence
                || event.acceptanceImpliedByFill;
        } else if (mirrorResult == MirrorApplyResult::Duplicate) {
            found->second.duplicateApplied = true;
        } else {
            found->second.failure = FailureCategory::MalformedEvent;
        }
    }
    MynyraEvent output;
    output.sourceTimestampNs = event.timestampNs;
    output.canonicalSymbol = intent.canonicalSymbol;
    output.eventType = std::string("mynyra_demo_") + legName(leg)
        + (event.remainingQuantity.isZero() ? "_filled" : "_partial_fill");
    output.strategyAttribution = intent.strategyAttribution;
    output.localOrderId = event.localOrderId;
    output.logicalPositionId = intent.logicalPositionId;
    output.lifecycleState = event.remainingQuantity.isZero()
        ? OrderLifecycleState::Filled : OrderLifecycleState::PartiallyFilled;
    output.failure = mirrorResult == MirrorApplyResult::Applied
        ? FailureCategory::None : FailureCategory::MalformedEvent;
    output.acceptanceImpliedByFill = event.acceptanceImpliedByFill;
    emit(state, std::move(output), EventFlush::LifecycleBoundary);
    state->changed.notify_all();
}

bool MynyraDemoCommissioningController::track(
    std::uint64_t orderId, Leg leg, const OrderIntent& intent)
{
    if (!m_mirror.registerIntent(orderId, intent)) return false;
    std::lock_guard<std::mutex> lock(m_state->mutex);
    return m_state->orders.emplace(
        orderId, TrackedOrder{leg, intent}).second;
}

MynyraDemoCommissioningController::LegEvidence
MynyraDemoCommissioningController::waitForLeg(std::uint64_t orderId)
{
    std::unique_lock<std::mutex> lock(m_state->mutex);
    const auto deadline = std::chrono::steady_clock::now()
                        + m_config.lifecycleTimeout;
    m_state->changed.wait_until(lock, deadline, [&] {
        const auto found = m_state->orders.find(orderId);
        return found == m_state->orders.end()
            || found->second.completeFill || found->second.rejected
            || found->second.duplicateApplied
            || found->second.failure != FailureCategory::None
            || !m_state->active.load();
    });
    const auto found = m_state->orders.find(orderId);
    if (found == m_state->orders.end()) {
        return LegEvidence{false, false, false, false,
                           FailureCategory::Unknown};
    }
    LegEvidence evidence;
    evidence.acceptance = found->second.acceptanceEvidence;
    evidence.filled = found->second.completeFill;
    evidence.rejected = found->second.rejected;
    evidence.duplicateApplied = found->second.duplicateApplied;
    evidence.failure = found->second.failure;
    if (!evidence.filled && !evidence.rejected
        && evidence.failure == FailureCategory::None) {
        evidence.failure = FailureCategory::Timeout;
    }
    return evidence;
}

std::optional<std::uint64_t> MynyraDemoCommissioningController::dispatch(
    Leg leg, const OrderIntent& intent, const OrderRiskContext& context)
{
    const std::uint64_t orderId = m_execution.reserveBrokerOrderId();
    if (!track(orderId, leg, intent)) return std::nullopt;
    const GatewayDispatchResult result = m_execution.executeIntent(
        intent, context, orderId);
    if (!result.dispatched) {
        const auto lifecycle = m_gateway.orderLifecycle(orderId);
        const bool outcomeAmbiguous = lifecycle.has_value()
            && lifecycle->state == OrderLifecycleState::Unknown;
        MynyraEvent event;
        event.sourceTimestampNs = intent.decisionTimestampNs;
        event.canonicalSymbol = intent.canonicalSymbol;
        event.eventType = std::string("mynyra_demo_") + legName(leg)
                        + (outcomeAmbiguous
                               ? "_dispatch_ambiguous"
                               : "_dispatch_rejected");
        event.strategyAttribution = intent.strategyAttribution;
        event.localOrderId = orderId;
        event.logicalPositionId = intent.logicalPositionId;
        event.failure = result.failure;
        emit(m_state, std::move(event), EventFlush::LifecycleBoundary);
        if (outcomeAmbiguous) {
            {
                std::lock_guard<std::mutex> lock(m_state->mutex);
                const auto found = m_state->orders.find(orderId);
                if (found == m_state->orders.end()) return std::nullopt;
                found->second.failure = result.failure == FailureCategory::None
                    ? FailureCategory::Unknown : result.failure;
            }
            m_state->changed.notify_all();
            // The adapter may have written some or all of the request before
            // transport failure. Preserve the identity for reconciliation;
            // this is not permission to resubmit the order.
            return orderId;
        }
        return std::nullopt;
    }
    return orderId;
}

bool MynyraDemoCommissioningController::reconcileEntry(
    const OrderIntent& intent, ReconciliationSnapshot& snapshot)
{
    snapshot = m_gateway.reconciliationSnapshot(nowNs());
    const bool valid = snapshot.complete && snapshot.account.complete
        && snapshot.status == ReconciliationStatus::Matched
        && snapshot.pendingOrderCount == 0 && snapshot.positions.size() == 1
        && snapshot.positions.front().canonicalSymbol == intent.canonicalSymbol
        && snapshot.positions.front().side == intent.side
        && snapshot.positions.front().quantity.scale == intent.exactQuantity.scale
        && snapshot.positions.front().quantity.isPositive()
        && snapshot.positions.front().quantity.units
               <= intent.exactQuantity.units
        && !snapshot.positions.front().logicalPositionId.empty();
    MynyraEvent event;
    event.sourceTimestampNs = snapshot.timestampNs;
    event.canonicalSymbol = intent.canonicalSymbol;
    event.eventType = valid
        ? "mynyra_demo_entry_reconciled"
        : "mynyra_demo_entry_reconciliation_failed";
    event.lifecycleState = OrderLifecycleState::Reconciled;
    event.failure = valid ? FailureCategory::None
                          : FailureCategory::ReconciliationMismatch;
    emit(m_state, std::move(event), EventFlush::LifecycleBoundary);
    return valid && m_mirror.applyReconciliation(snapshot);
}

bool MynyraDemoCommissioningController::reconcileFlat(
    ReconciliationSnapshot& snapshot)
{
    snapshot = m_gateway.reconciliationSnapshot(nowNs());
    const bool valid = snapshot.complete && snapshot.account.complete
        && snapshot.status == ReconciliationStatus::Matched
        && snapshot.positions.empty() && snapshot.pendingOrderCount == 0;
    MynyraEvent event;
    event.sourceTimestampNs = snapshot.timestampNs;
    event.canonicalSymbol = "XAUUSD";
    event.eventType = valid
        ? "mynyra_demo_flat_reconciled"
        : "mynyra_demo_flat_reconciliation_failed";
    event.lifecycleState = OrderLifecycleState::Reconciled;
    event.failure = valid ? FailureCategory::None
                          : FailureCategory::ReconciliationMismatch;
    emit(m_state, std::move(event), EventFlush::LifecycleBoundary);
    return valid && m_mirror.applyReconciliation(snapshot)
        && m_mirror.isFlat();
}

MynyraDemoOutcome MynyraDemoCommissioningController::fail(
    FailureCategory failure, bool recoveryRequired) noexcept
{
    MynyraEvent event;
    event.canonicalSymbol = "XAUUSD";
    event.eventType = recoveryRequired
        ? "mynyra_demo_recovery_required" : "mynyra_demo_m1_failed";
    event.failure = failure;
    emit(m_state, std::move(event), EventFlush::LifecycleBoundary);
    return recoveryRequired
        ? MynyraDemoOutcome::RecoveryRequired : MynyraDemoOutcome::Failed;
}

MynyraDemoOutcome MynyraDemoCommissioningController::commission(
    const StrategyDecision& decision,
    const OrderRiskContext& openingContext)
{
    const std::uint64_t decisionTimestampNs =
        decision.candleTimestamp
                <= std::numeric_limits<std::uint64_t>::max() / 1'000'000'000ULL
            ? decision.candleTimestamp * 1'000'000'000ULL
            : 0;
    MynyraEvent decisionEvent;
    decisionEvent.sourceTimestampNs = decisionTimestampNs;
    decisionEvent.canonicalSymbol = decision.canonicalSymbol;
    decisionEvent.eventType = "mynyra_demo_strategy_decision";
    decisionEvent.strategyAction = decision.action;
    decisionEvent.strategyConviction = decision.totalConviction;
    decisionEvent.strategyAttribution = decision.strategyAttribution;
    emit(m_state, std::move(decisionEvent), EventFlush::Buffered);

    if (!decision.executionEligible || decision.action == Signal::NONE) {
        return MynyraDemoOutcome::NoEligibleSignal;
    }
    if (!m_config.commissionOrder) {
        MynyraEvent event;
        event.sourceTimestampNs = decisionTimestampNs;
        event.canonicalSymbol = decision.canonicalSymbol;
        event.eventType = "mynyra_demo_read_only_signal_observed";
        event.strategyAction = decision.action;
        event.strategyConviction = decision.totalConviction;
        event.strategyAttribution = decision.strategyAttribution;
        emit(m_state, std::move(event), EventFlush::LifecycleBoundary);
        return MynyraDemoOutcome::ReadOnly;
    }
    bool expected = false;
    if (!m_entryAttempted.compare_exchange_strong(expected, true)) {
        return fail(FailureCategory::Validation, false);
    }

    OrderIntent entry;
    entry.side = decision.action == Signal::BUY
        ? PositionSide::Long : PositionSide::Short;
    entry.effect = PositionEffect::Open;
    entry.exactQuantity = openingContext.instrument.minimumQuantity;
    entry.referencePrice = entry.side == PositionSide::Long
        ? openingContext.ask : openingContext.bid;
    entry.canonicalSymbol = decision.canonicalSymbol;
    entry.strategyAttribution = decision.strategyAttribution;
    entry.decisionTimestampNs = nowNs();

    const auto entryOrder = dispatch(Leg::Entry, entry, openingContext);
    if (!entryOrder.has_value()) {
        return fail(FailureCategory::Validation, false);
    }
    LegEvidence entryEvidence = waitForLeg(*entryOrder);

    ReconciliationSnapshot entryReconciliation;
    if (!entryEvidence.filled) {
        // Never resubmit an entry. Reconciliation may prove the exposure and
        // permit only the close path.
        if (!reconcileEntry(entry, entryReconciliation)) {
            const bool conclusivelyFlat = entryReconciliation.complete
                && entryReconciliation.account.complete
                && entryReconciliation.status == ReconciliationStatus::Matched
                && entryReconciliation.pendingOrderCount == 0
                && entryReconciliation.positions.empty()
                && m_mirror.applyReconciliation(entryReconciliation)
                && m_mirror.isFlat();
            if (conclusivelyFlat) {
                return fail(entryEvidence.failure, false);
            }
            return fail(entryEvidence.failure, true);
        }
    } else if (!reconcileEntry(entry, entryReconciliation)) {
        return fail(FailureCategory::ReconciliationMismatch, true);
    }
    const bool entryQuantityMatchesLifecycle =
        entryReconciliation.positions.front().quantity.scale
            == entry.exactQuantity.scale
        && entryReconciliation.positions.front().quantity.units
            == entry.exactQuantity.units;
    const bool entryProofComplete = entryEvidence.acceptance
                                 && entryEvidence.filled
                                 && !entryEvidence.duplicateApplied
                                 && entryQuantityMatchesLifecycle;
    const auto& reconciledPosition = entryReconciliation.positions.front();
    OrderIntent close;
    close.side = reconciledPosition.side;
    close.effect = PositionEffect::Close;
    close.exactQuantity = reconciledPosition.quantity;
    // Native position-close does not consume a quote. Use the authoritative
    // reconciled entry price as structural price evidence so a reconnect does
    // not carry a stale pre-entry BBO into the risk-reducing intent.
    close.referencePrice = reconciledPosition.averagePrice;
    close.canonicalSymbol = reconciledPosition.canonicalSymbol;
    close.strategyAttribution = entry.strategyAttribution;
    close.logicalPositionId = reconciledPosition.logicalPositionId;
    close.decisionTimestampNs = nowNs();

    OrderRiskContext closeContext = openingContext;
    const auto closeInstrument = m_gateway.instrumentSpec(
        reconciledPosition.canonicalSymbol);
    if (!closeInstrument.has_value() || !closeInstrument->complete) {
        return fail(FailureCategory::ReconciliationMismatch, true);
    }
    closeContext.instrument = *closeInstrument;
    closeContext.instrumentVersion = closeInstrument->version;
    closeContext.account = entryReconciliation.account;
    closeContext.reconciliation = entryReconciliation;
    closeContext.expectedMargin = Decimal64{0, openingContext.expectedMargin.scale};
    closeContext.connectionGeneration = entryReconciliation.connectionGeneration;
    closeContext.sameGeneration = true;
    closeContext.evaluationTimestampNs = nowNs();

    const auto closeOrder = dispatch(Leg::Close, close, closeContext);
    if (!closeOrder.has_value()) {
        return fail(FailureCategory::Validation, true);
    }
    LegEvidence closeEvidence = waitForLeg(*closeOrder);
    std::optional<std::uint64_t> residualOrderId;
    bool initialClosePartialProof = false;
    bool residualProofComplete = false;

    ReconciliationSnapshot finalReconciliation;
    if (!reconcileFlat(finalReconciliation)) {
        // One residual close is allowed only when authoritative reconciliation
        // proves exactly one residual commissioning position and no pending order.
        if (!finalReconciliation.complete
            || finalReconciliation.status != ReconciliationStatus::Matched
            || finalReconciliation.pendingOrderCount != 0
            || finalReconciliation.positions.size() != 1) {
            return fail(FailureCategory::ReconciliationMismatch, true);
        }
        const auto& residual = finalReconciliation.positions.front();
        if (residual.canonicalSymbol != close.canonicalSymbol
            || residual.side != close.side
            || residual.logicalPositionId != *close.logicalPositionId
            || !residual.quantity.isPositive()
            || residual.quantity.scale != close.exactQuantity.scale
            || residual.quantity.units > close.exactQuantity.units) {
            return fail(FailureCategory::ReconciliationMismatch, true);
        }
        if (!m_mirror.applyReconciliation(finalReconciliation)) {
            return fail(FailureCategory::ReconciliationMismatch, true);
        }
        auto partialLifecycle = m_gateway.orderLifecycle(*closeOrder);
        if (!partialLifecycle.has_value()) {
            return fail(FailureCategory::ReconciliationMismatch, true);
        }
        const Decimal64 reconciledFilled{
            close.exactQuantity.units - residual.quantity.units,
            close.exactQuantity.scale};
        initialClosePartialProof =
            reconciledFilled.isPositive()
            && partialLifecycle->state == OrderLifecycleState::PartiallyFilled
            && closeEvidence.acceptance
            && !closeEvidence.duplicateApplied
            && partialLifecycle->filledQuantity == reconciledFilled
            && partialLifecycle->remainingQuantity == residual.quantity;
        bool initialLifecycleReconciled = false;
        if (partialLifecycle->state != OrderLifecycleState::PartiallyFilled
            && partialLifecycle->state != OrderLifecycleState::Unknown
            && partialLifecycle->state != OrderLifecycleState::Filled) {
            // The provider outcome was ambiguous, but reconciliation proved a
            // strict reduction and no pending close. Mark that uncertainty
            // before the one permitted exact-residual close; it can flatten
            // exposure but can never manufacture milestone success evidence.
            if (!m_gateway.markOrderUnknown(
                    *closeOrder, FailureCategory::Timeout,
                    "close outcome resolved only by reconciliation",
                    finalReconciliation.timestampNs)) {
                return fail(FailureCategory::ReconciliationMismatch, true);
            }
            partialLifecycle = m_gateway.orderLifecycle(*closeOrder);
        }
        if (!partialLifecycle.has_value()) {
            return fail(FailureCategory::ReconciliationMismatch, true);
        }
        if (partialLifecycle->state == OrderLifecycleState::PartiallyFilled
            || partialLifecycle->state == OrderLifecycleState::Unknown) {
            initialLifecycleReconciled = m_gateway.applyOrderReconciliation(
                *closeOrder, OrderLifecycleState::Canceled,
                reconciledFilled, residual.quantity,
                finalReconciliation.timestampNs);
            if (!initialLifecycleReconciled) {
                return fail(FailureCategory::ReconciliationMismatch, true);
            }
        } else if (partialLifecycle->state != OrderLifecycleState::Filled) {
            return fail(FailureCategory::ReconciliationMismatch, true);
        }
        // A locally complete close that still has a broker residual is a
        // lifecycle mismatch. It may use the one safety close below, but the
        // mismatch can never contribute to milestone success.
        m_execution.markBrokerOrderInactive(*closeOrder);
        OrderIntent residualClose = close;
        residualClose.exactQuantity = residual.quantity;
        residualClose.decisionTimestampNs = nowNs();
        OrderRiskContext residualContext = closeContext;
        const auto residualInstrument = m_gateway.instrumentSpec(
            residual.canonicalSymbol);
        if (!residualInstrument.has_value() || !residualInstrument->complete) {
            return fail(FailureCategory::ReconciliationMismatch, true);
        }
        residualContext.instrument = *residualInstrument;
        residualContext.instrumentVersion = residualInstrument->version;
        residualContext.account = finalReconciliation.account;
        residualContext.reconciliation = finalReconciliation;
        residualContext.connectionGeneration = finalReconciliation.connectionGeneration;
        residualContext.evaluationTimestampNs = nowNs();
        const auto residualOrder = dispatch(
            Leg::ResidualClose, residualClose, residualContext);
        if (!residualOrder.has_value()) {
            return fail(FailureCategory::Validation, true);
        }
        residualOrderId = *residualOrder;
        const LegEvidence residualEvidence = waitForLeg(*residualOrder);
        const bool residualFlat = reconcileFlat(finalReconciliation);
        if (!residualFlat || !residualEvidence.acceptance
            || !residualEvidence.filled || residualEvidence.duplicateApplied) {
            return fail(residualEvidence.failure, true);
        }
        residualProofComplete = initialLifecycleReconciled;
    }

    const auto entryLifecycle = m_gateway.orderLifecycle(*entryOrder);
    const auto closeLifecycle = m_gateway.orderLifecycle(*closeOrder);
    const auto residualLifecycle = residualOrderId.has_value()
        ? m_gateway.orderLifecycle(*residualOrderId) : std::nullopt;
    const bool closeLifecycleComplete = closeLifecycle.has_value()
        && (closeLifecycle->state == OrderLifecycleState::Filled
            || (residualOrderId.has_value()
                && closeLifecycle->state == OrderLifecycleState::Reconciled
                && closeLifecycle->reconciledState.has_value()
                && *closeLifecycle->reconciledState
                    == OrderLifecycleState::Canceled));
    const bool closeProofComplete = residualOrderId.has_value()
        ? initialClosePartialProof && residualProofComplete
        : closeEvidence.acceptance && closeEvidence.filled
            && !closeEvidence.duplicateApplied;
    if (!entryProofComplete || !closeProofComplete
        || !entryLifecycle.has_value() || !closeLifecycle.has_value()
        || entryLifecycle->state != OrderLifecycleState::Filled
        || !closeLifecycleComplete
        || (residualOrderId.has_value()
            && (!residualLifecycle.has_value()
                || residualLifecycle->state != OrderLifecycleState::Filled))
        || m_execution.pendingBrokerQuantity(*entryOrder) != 0.0
        || m_execution.pendingBrokerQuantity(*closeOrder) != 0.0
        || (residualOrderId.has_value()
            && m_execution.pendingBrokerQuantity(*residualOrderId) != 0.0)
        || !m_mirror.isFlat() || !m_state->sinkHealthy.load()) {
        return fail(FailureCategory::ReconciliationMismatch, true);
    }

    MynyraEvent success;
    success.canonicalSymbol = "XAUUSD";
    success.eventType = "mynyra_demo_m1_succeeded";
    success.failure = FailureCategory::None;
    if (!emit(m_state, std::move(success), EventFlush::LifecycleBoundary)) {
        return MynyraDemoOutcome::Failed;
    }
    m_succeeded.store(true);
    return MynyraDemoOutcome::Succeeded;
}

bool MynyraDemoCommissioningController::entryAttempted() const noexcept
{
    return m_entryAttempted.load();
}

bool MynyraDemoCommissioningController::succeeded() const noexcept
{
    return m_succeeded.load();
}
