# TradeBot Repository Remediation Program

## Purpose And Authority

- Purpose: define the sole current remediation focus, its nine work packages,
  their dependencies, entry and exit gates, evidence requirements, and safety
  boundaries.
- Status: focus lock and foundational governance approved; WP-0 merged and
  accepted; WP-1 approved as the current package;
  WP-2 through WP-8 Planned / NO-GO.
- Owner and review authority: Wade.
- Program plan: `PLAN-20260813-repository-cohesion-remediation` in
  `../PLANS.md`.
- Diagnostic baseline: repository-wide inspection of commit
  `7f89c597d3874e5c8782a49a31d42335c8bf1e17` on 2026-08-13.
- Authority level: program execution contract below `AGENTS.md`, accepted
  ADRs, `RISK_POLICY.md`, and `ARCHITECTURE.md`; implemented through the active
  plan. It overlays but does not rewrite historical phase or ADR records.

This program converts the repository-wide cohesion diagnosis into an exact
execution queue. Wade's 2026-08-14 approval authorizes offline WP-0 work and
conditional offline WP-1 work within their canonical scopes. It does not
authorize credential access, provider traffic, order activity, financial-limit
changes, workflow dispatch, release, deployment, or live trading. Wade's
follow-up separately authorizes staging, commit, push, PR, and merge for WP-0
and then dependency-gated WP-1.

## Focus Lock

Until Wade explicitly changes this lock:

1. The only implementation work is WP-0 through WP-8 in this document.
2. No new feature, strategy, provider gate, workstream, phase, optimization,
   platform selection, or deployment lane may pre-empt these packages.
3. A newly discovered defect must be mapped to an existing package. If it does
   not fit, work stops until Wade approves a scope revision; no tenth package
   is created implicitly.
4. Historical phase and provider-gate records remain evidence, not current
   execution authority.
5. A package title alone does not authorize implementation. WP-0 and WP-1 have
   exact operator approval recorded here; WP-2 through WP-8 require separate
   operator GO after their entry conditions are reviewed.
6. Live trading, real or demo orders, provider execution, credential-value
   access, risk-limit changes, release, and deployment remain separate NO-GO
   actions even after all nine packages are complete.

## Evidence Classification

The diagnosis established observed facts and strongly supported inferences.
Package implementation must re-check the affected code and tests at its own
starting commit; this baseline must not be treated as immutable source truth.

Observed baseline evidence includes:

- default, deep-sanitizer, Gate 6, Gate 7, and Gate 7 sanitizer suites passing
  at the inspected state;
- `throughput_bench 100000` consuming 100,000 ticks but reporting only 260
  fills, with the same 260-fill ceiling at 261 ticks;
- a live-capable runtime surface that is not an operationally credible live
  system;
- inconsistent execution, persistence, accounting, risk, runtime-mode,
  timestamp, generated-artifact, transport, CI, and authority contracts.

Passing tests are evidence only for their covered behavior. They do not
override the cross-module defects that define these packages.

## Approved Program Foundation

The objective is to strengthen the infrastructure foundation. It does not
define a separate preliminary gate, phase, package, artifact, or approval
checkpoint. The foundational governance structure was merged in PR #34 and
explicitly approved by Wade on 2026-08-14.

The approved foundation establishes:

- `AGENTS.md`, `PLANS.md`, `PROJECT_STATE.md`, `ROADMAP.md`, workflow, risk,
  testing, security, release, contributor, and index documents point to this
  program without conflicting current-next-action claims.
- Exactly nine packages are named consistently, with one canonical definition
  here and concise references elsewhere.
- Every package has a problem statement, scope, exclusions, dependencies,
  acceptance criteria, tests, approval boundary, and rollback rule.
- Historical phase, ADR, and cTrader evidence remains historically accurate
  but is not represented as active work.
- Governance work does not conceal source, test, CMake, workflow, credential,
  generated-artifact, financial-control, runtime-mode, or provider-state
  changes.
- Documentation validation, automation-policy validation, scope review, and
  Git diff hygiene pass, with unavailable checks reported.

## Package Register

| Package | Name | Baseline severity | Status | Primary dependencies |
| --- | --- | --- | --- | --- |
| WP-0 | Live containment | Critical | Complete — merged and accepted | Approved program foundation |
| WP-1 | Persistence and generated-state containment | Critical | Approved — current | Accepted WP-0 evidence |
| WP-2 | Accounting and quantity correctness | High | Planned / NO-GO | WP-1 |
| WP-3 | Risk-state repair | High | Planned / NO-GO | WP-2 |
| WP-4 | Unified order lifecycle | High | Planned / NO-GO | WP-0 through WP-3 |
| WP-5 | Runtime-mode and data contracts | High | Planned / NO-GO | WP-0 through WP-4 |
| WP-6 | Transport and provider integration | High | Planned / NO-GO | WP-0 through WP-5; WP-7 controls as needed |
| WP-7 | CI and observability | High | Planned / NO-GO except approved WP-0/WP-1 evidence slices | Integrates with every package |
| WP-8 | Authority synchronization | High | Planned / NO-GO except approved WP-0/WP-1 synchronization slices | Integrates with every package; closes after WP-0 through WP-7 |

`Approved — current` authorizes the named package's offline planning and
implementation within its canonical scope. `Approved — queued` carries the
same bounded authority but cannot activate until its dependency is accepted.
`Planned / NO-GO` means that problem definition and sequencing are accepted as
the focal queue, but implementation has not been authorized.

## Sequencing And Parallelism

- Safety spine: WP-0 → WP-1 → WP-2 → WP-3 → WP-4 → WP-5 → WP-6.
- WP-7 may establish package-specific test and observability scaffolding only
  through the currently approved WP-0 or WP-1 slice. It must add
  evidence before or with the behavior it is intended to protect.
- WP-8 runs as a documentation/authority closeout slice after each accepted
  package and performs final program reconciliation last.
- Multiple packages may not modify the same ownership, state, order, risk,
  timestamp, or provider contract concurrently unless an approved plan proves
  isolation and names the merge order.
- Full CTest suites from different build trees run sequentially unless all
  shared resources are proven isolated.

## WP-0 — Live Containment

### Problem

The nominal `LIVE` surface is present but its startup, provider selection,
network transport, broker state, and fill behavior do not form a fail-closed,
operationally credible path. Existing policy alone is not a runtime barrier.

### Outcome

No normal build or invocation can enter a provider-capable or real-order path
without an explicit compile-time and runtime unlock that fails closed before
credentials, network access, or order construction.

### In Scope

- `SystemConfig`, CLI/startup mode selection, live-capable adapter binding,
  broker-gateway readiness, local-fill fallback semantics, provider fallback
  removal, and offline enforcement tests.
- Default build and CI assertions that provider-capable options remain off.
- A clear distinction between BACKTEST, locally simulated PAPER, external
  sandbox, and LIVE.

### Out Of Scope

- Provider implementation, credential use, network calls, demo or real orders,
  changing risk limits, or claiming live readiness.

### Acceptance And Tests

- Default and documented invocations cannot contact a provider or submit an
  order.
- A requested unavailable/unauthorized live-capable mode terminates before
  credential lookup and network startup; it does not downgrade to simulation.
- A disconnected or unready gateway cannot create a local fill that appears
  externally executed.
- A protective trigger rejected by an unavailable gateway remains active for
  reevaluation; it is removed only after execution accepts the triggered exit.
- BACKTEST and simulated PAPER behavior remain explicit and regression-tested.
- Full default CTest, targeted mode/gateway tests, sanitizer coverage, CI policy
  checks, and a no-provider side-effect review pass.

The approved implementation uses `TRADEBOT_ENABLE_LIVE_RUNTIME=OFF` as the
normal build state plus `--unlock-live-runtime` as the second technical gate.
Both must pass before the legacy LIVE adapter may start its network client. The
WP-0 design also removes credential ownership and provider-specific REST
requests from that data adapter; those belong to a future separately approved provider
adapter. These gates never grant operator or provider authority.

### Approval And Rollback

Requires exact operator approval for source implementation plus network/live,
execution-pipeline, and risk review. Roll back by reverting only the approved
WP-0 change; if rollback would reopen an unsafe live surface, keep the runtime
disabled and stop for operator direction.

## WP-1 — Persistence And Generated-State Containment

### Problem

Snapshot behavior does not reliably preserve all open positions or the risk
and lifecycle state needed for safe restart. Checkpoint failure can be ignored.
Generated-state policy describes directories as ignored although current Git
rules mainly ignore selected file extensions.

### Outcome

Restart state is complete, versioned, validated, atomic, and fail-closed; every
generated output has an explicit non-source location and source-control rule.

### In Scope

- `StateSerializer`, event-loop checkpoint handling, portfolio/risk/order-
  lifecycle state, deduplication identity, snapshot version/migration rules,
  atomic writes, generated result paths, `.gitignore`, and provenance policy.

### Out Of Scope

- Changing trading logic, risk thresholds, provider state, credentials, or
  treating a snapshot as external reconciliation truth.

### Acceptance And Tests

- Zero, one, and multiple open positions round-trip exactly, including the
  first element.
- Risk halt/close-only sources, required accounting state, lifecycle identity,
  and deduplication state either restore exactly or cause a documented
  fail-closed incompatibility.
- Partial/corrupt/version-incompatible snapshots are rejected without mutating
  live in-memory state; failed checkpoint writes are observable and actionable.
- Generated JSON, CSV, binary, logs, archives, and evidence paths are covered
  by exact ignore/provenance tests without hiding intentional fixtures.
- Migration, restart-deduplication, malformed-input, and source-control policy
  tests pass.

### Approval And Rollback

Requires exact operator approval for persistence/schema implementation and
data-policy review. Rollback must preserve snapshot compatibility or supply a
versioned migration/rejection path; never silently load a newer unsafe format.

## WP-2 — Accounting And Quantity Correctness

### Problem

Price, quantity, fee, cash, position, and P&L calculations cross fixed-point,
integer, and floating representations without one enforced unit contract.
Multi-asset marking and long-run fill/accounting behavior are not reliable.

### Outcome

Every financial value has one named unit, rounding rule, overflow rule, and
owner, and portfolio accounting remains correct across long sequences and
multiple symbols.

### In Scope

- Canonical price/quantity/notional/fee contracts; normalization boundaries;
  cash, position, realized/unrealized P&L, fee and slippage accounting;
  multi-symbol marking; serialization compatibility; benchmark correctness
  assertions.

### Out Of Scope

- Strategy tuning, profitability claims, risk-limit value changes, provider
  connectivity, or performance optimization before correctness.

### Acceptance And Tests

- Golden integer/fixed-point vectors define units, signedness, rounding, and
  overflow at every execution/accounting boundary.
- Buy, sell, reduce, close, reversal, partial fill, fee, and slippage cases
  reconcile cash, quantity, average price, and realized/unrealized P&L.
- Marking one symbol cannot reset or reprice another symbol from its entry.
- The 261-tick/260-fill regression is explained and corrected or converted into
  an explicit, tested design rule; long-run conservation/property tests pass.
- Existing replay and snapshot compatibility is migrated or rejected
  explicitly.

### Approval And Rollback

Requires exact operator approval because accounting and sizing are financial-
sensitive. Any change to actual limit values requires a separate approval.
Rollback must be paired with snapshot/schema compatibility and accounting
reconciliation evidence.

## WP-3 — Risk-State Repair

### Problem

Daily drawdown, VaR aggregation, halt/close-only composition, health events,
and final normalized-quantity evaluation do not form one persistent,
monotonic, fail-closed risk decision.

### Outcome

Risk state has explicit sources and lifecycle rules; independent hazards cannot
clear each other, and every new-exposure decision is made on the final
normalized order quantity.

### In Scope

- Daily loss/drawdown activation, VaR aggregation, position exposure,
  final-quantity decision API, halt/close-only source composition, adapter and
  data-health propagation, reconciliation mismatch policy, persistence, and
  operator-clear semantics.

### Out Of Scope

- Raising or lowering configured financial limits, provider traffic, automatic
  halt clearing, or live authorization.

### Acceptance And Tests

- Existing configured limits are applied with documented units and no silent
  overwrite or disabled branch.
- Each halt/close-only source is independently set, latched, persisted, and
  cleared only by its defined authority; clearing one source cannot clear
  another.
- Missing/stale market, account, or reconciliation data blocks new exposure.
- Risk evaluates the exact final normalized quantity immediately before order
  submission; compatibility routes cannot inject an always-allow result.
- Boundary, recovery, restart, stale-data, health, reconciliation, and
  multi-source state-machine tests pass under sanitizers.

### Approval And Rollback

Requires exact operator approval for the named risk behavior and values,
followed by risk and execution review. Rollback must fail closed and preserve
the operator's ability to halt; it must not reinstate an always-allow route.

## WP-4 — Unified Order Lifecycle

### Problem

Order construction, risk, gateway routing, acknowledgements, fills,
cancellation, pending/trigger state, portfolio mutation, and reconciliation
have compatibility paths and premature state transitions that can disagree.

### Outcome

One broker-neutral lifecycle is authoritative from intent through terminal
reconciliation. Portfolio state changes only from confirmed executions or an
explicitly approved reconciliation action.

### In Scope

- Strategy/allocation intent conversion, order identity, normalized risk
  decision, gateway submission, acknowledgement/rejection, partial/full fills,
  cancellation/races/timeouts, trigger/pending retention, idempotency,
  reconciliation, and audit events.

### Out Of Scope

- Provider-native adapter implementation, external orders, strategy changes,
  and risk-limit changes.

### Acceptance And Tests

- Every new-exposure path uses the same final risk and lifecycle boundary; no
  compatibility or disconnected-gateway path fabricates execution.
- Pending and trigger state survives rejection/timeout and is removed only by
  its documented confirmed terminal transition.
- Duplicate, out-of-order, partial-fill, cancel/fill race, disconnect, restart,
  and reconciliation events are deterministic and idempotent.
- End-to-end strategy → allocation → execution → risk → gateway → deterministic
  adapter → fill/reconcile tests assert portfolio, risk, metrics, and persisted
  lifecycle state.

### Approval And Rollback

Requires exact operator approval plus execution-pipeline and risk review.
Rollback must preserve lifecycle schema compatibility and block new exposure
when local and external state cannot be reconciled.

## WP-5 — Runtime-Mode And Data Contracts

### Problem

Startup mode, input source, adapter ownership, concurrency, timestamps, replay
formats, stale/out-of-order handling, and default input assumptions are not one
coherent contract across BACKTEST, PAPER, and the locked LIVE surface.

### Outcome

Each runtime mode has exactly one declared data and execution topology with
explicit ownership, shutdown, timestamp, schema, and validation rules.

### In Scope

- Startup/config validation, mode-specific orchestration, market-data source
  selection, event-loop/thread ownership, shutdown, candle/replay timestamp
  units, event ordering, stale/out-of-order behavior, input path requirements,
  replay schema/version/provenance, and deterministic fixtures.

### Out Of Scope

- Provider adapter implementation, external connectivity, new strategies,
  deployment, and unlocking LIVE.

### Acceptance And Tests

- BACKTEST is deterministic and requires an explicit valid input or an
  approved tracked synthetic fixture; PAPER is explicitly local simulation;
  LIVE remains rejected under WP-0 controls.
- No runtime simultaneously wires contradictory CSV/live data sources or
  detached shared state; lifecycle and thread ownership are deterministic.
- Every timestamp field has a unit, clock domain, ordering rule, and conversion
  boundary; malformed, stale, future, ambiguous, and out-of-order data fails
  as specified.
- CSV/binary replay round-trip, version compatibility, cursor/resume, shutdown,
  concurrency, and mode-topology tests pass.

### Approval And Rollback

Requires exact operator approval plus data/replay and network/live-boundary
review. Rollback must retain WP-0 containment and reject incompatible replay or
configuration rather than guessing.

## WP-6 — Transport And Provider Integration

### Problem

The generic network/live-data surface is not production credible, provider-
specific behavior leaks through legacy paths, and the accepted cTrader proof
is isolated rather than an integrated broker-neutral adapter.

### Outcome

An offline-verifiable, provider-specific adapter can attach only below
`BrokerGateway`, translates to the accepted neutral contracts, and has bounded
transport, authentication, health, backpressure, reconnect, and redaction
semantics without enabling provider execution by default.

### In Scope

- Transport framing/TLS/timeouts/backpressure, authentication ownership,
  provider adapter translation, symbol/account metadata, market-data health,
  lifecycle/reconciliation mapping, deterministic provider fixtures, and
  default-off build isolation.
- A separately gated external-evidence substage may be planned only after the
  offline adapter and WP-0 through WP-5 evidence are accepted.

### Out Of Scope

- Provider traffic under the base package approval, credential-value access,
  demo or real orders, live accounts, limit changes, deployment, and live use.

### Acceptance And Tests

- No provider-native type, unit, identifier, error, or retry semantic crosses
  the adapter boundary.
- Transport handles partial I/O, framing limits, TLS verification, timeouts,
  backpressure, disconnect, bounded reconnect policy, cancellation, and
  terminal clearing with fixed redacted diagnostics.
- Synthetic tests cover market data, order lifecycle mapping, partial fills,
  cancellation, reconciliation, rate limits, stale data, disconnects, and
  credential failure without network or secret access.
- Provider options remain default-off and normal CI remains offline.
- Any external sandbox/provider evidence requires a new exact artifact,
  presence-only preflight, bounded process/attempt, stop condition, and
  separate Wade approval. It still does not authorize orders or live use.

### Approval And Rollback

Offline implementation requires exact operator approval plus architecture,
network/live, security, risk, and dependency review. Every external process is
a separate approval. Rollback removes or disables the provider adapter while
preserving broker-neutral core state and reconciliation evidence.

## WP-7 — CI And Observability

### Problem

Current CI passes without exercising important end-to-end accounting, risk,
restart, runtime-mode, and provider-neutral lifecycle failures. Benchmark
correctness, warnings, static analysis, and operational health signals are
incomplete.

### Outcome

Each package has a required offline evidence matrix, and runtime behavior emits
bounded, actionable, non-sensitive state needed to detect and diagnose failure.

### In Scope

- Package-specific test targets and CI matrix, sanitizer/static-analysis and
  warning policy, benchmark correctness assertions, fixture/provenance checks,
  state/risk/order/adapter health metrics, redaction, artifact manifests, and
  failure-preserving diagnostics.

### Out Of Scope

- Disabling checks to obtain a pass, uploading executables, provider traffic,
  deployment, secrets, account data, or claims of production/live readiness.

### Acceptance And Tests

- CI maps every WP acceptance criterion to a named offline check and cannot
  silently skip default-off or safety assertions.
- The 261-tick benchmark regression has a correctness gate independent of
  throughput measurement.
- Warnings and analyzer findings have an explicit zero/new-baseline policy;
  flaky/shared-resource failures preserve first evidence and run sequentially
  where required.
- Metrics expose state transitions and correlation IDs without secrets,
  provider payloads, prices, account identifiers, or private state.
- Offline artifact delivery remains non-executable, exact-revision, and
  checksum-verified.

### Approval And Rollback

Requires exact operator approval for each CI/workflow or observability slice;
workflow dispatch and artifact upload remain separate actions. Rollback must
not weaken the minimum checks protecting already accepted packages.

## WP-8 — Authority Synchronization

### Problem

Implementation, tests, plans, ADR consequences, roadmap/status claims,
generated-artifact policy, and operational readiness statements have drifted.
Historical completion language can be mistaken for present correctness or
authorization.

### Outcome

Every authoritative claim matches accepted implementation evidence, each
tracked file has a clear role/disposition, and historical records are clearly
separated from current authority.

### In Scope

- Per-package documentation synchronization; project state, roadmap, plans,
  architecture, risk, testing, security, data, release, workflow, ADR indexes
  and consequences where warranted; tracked-file inventory/disposition;
  terminology and stale-claim cleanup; final program closeout.

### Out Of Scope

- Retrospective invention, changing an ADR status without explicit review,
  claiming operational/live readiness, source behavior changes hidden as docs,
  or deleting decision-relevant history.

### Acceptance And Tests

- After each package, source behavior, tests, current state, risk/architecture
  policy, and plan evidence agree at the same evidence epoch.
- Historical phase and provider-gate records are labelled as history and do
  not appear as the next authorized action.
- The complete tracked-file inventory has one actual role and disposition per
  file; obsolete or duplicate files are removed only through separate approved
  changes with preserved history.
- Documentation indexes, cross-references, automation validation, stale-claim
  scans, link checking when available, and Git diff hygiene pass.
- Final closeout lists unresolved questions and explicitly states whether the
  repository remains operationally unsafe or has reached a narrower accepted
  condition. Completion never grants live authorization.

### Approval And Rollback

Requires exact operator approval for each synchronization slice and any ADR
status/consequence change. Rollback reverts only the affected documentation
slice and must not reintroduce a claim contradicted by accepted code evidence.

## Package Entry Template

Before any WP starts, its plan slice must record:

- exact starting branch, full commit, tracked/index state, and relevant ignored
  artifacts;
- observed facts revalidated from current source/tests and hypotheses still
  unresolved;
- exact files/subsystems, implementation steps, invariants, acceptance tests,
  rollback, and stop condition;
- required specialists and review authority;
- whether staging, commit, push, workflow dispatch, credential access,
  provider traffic, orders, risk-limit changes, deployment, or live use are
  authorized; absent items are prohibited;
- a bounded correction budget and handoff rule.

## Package Exit Template

A WP may be accepted only when:

- all in-scope criteria pass at the named evidence epoch;
- failures, warnings, skips, generated outputs, migrations, and residual risks
  are reported;
- rollback is tested or concretely reviewable;
- WP-7 evidence and WP-8 authority synchronization for the package are
  complete;
- Wade explicitly accepts the result; and
- the next package remains NO-GO until separately authorized.

## Program Completion Boundary

Program completion means the nine-package remediation evidence has been
accepted and authority documents are synchronized. It does not mean profitable,
production-ready, provider-approved, deployable, or safe for live trading.
Those claims require their own evidence and explicit operator decisions under
`RISK_POLICY.md` and `LIVE_TRADING_READINESS.md`.
