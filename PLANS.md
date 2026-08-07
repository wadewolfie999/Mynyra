# TradeBot Planning System

## Purpose And Authority

- Purpose: define when and how implementation plans are authored, approved, executed, resumed, closed, abandoned, or superseded.
- Authority level: active approved plans rank below `docs/ARCHITECTURE.md` and above `docs/PROJECT_STATE.md`.
- Audience: operator, maintainers, Codex, contributors, reviewers, and research agents.
- Related documents: `AGENTS.md`, `docs/WORKFLOW.md`, `docs/HANDOFF.md`, `docs/ROADMAP.md`, and `docs/decisions/`.

TradeBot uses plans to prevent scope drift, preserve architectural continuity, and make safety-sensitive work resumable across sessions and actors.

## When A Plan Is Required

A plan is required before implementation when work involves any of the following:

- Source behavior changes.
- Architecture or subsystem-boundary changes.
- Financial, order-execution, risk-limit, credential, or live-capable behavior.
- Data schema, replay semantics, timestamp handling, or generated-output semantics.
- New dependencies or toolchain changes.
- Performance claims or benchmark-driven optimization.
- Multi-file changes with unclear sequencing.
- Work that may span sessions or require handoff.
- Any task where rollback or acceptance criteria are nontrivial.

## When A Direct Patch Is Acceptable

A direct patch is acceptable when all are true:

- Scope is small and clear.
- Risk classification is low.
- The change is documentation-only or a narrow correction.
- No public interface, runtime mode, financial behavior, credential handling, or architecture boundary changes.
- Verification is obvious and local.
- No active plan or ADR is contradicted.

Even direct patches must inspect state before mutation and report verification.

## Plan Identifiers

Use stable IDs:

```text
PLAN-YYYYMMDD-short-topic
```

Examples of valid shapes:

- `PLAN-20260606-phase19-revalidation-docs`
- `PLAN-20260606-l2-applybbo-benchmark`

Do not reuse IDs. If a plan is superseded, create a new ID and cross-reference the prior plan.

## Ownership And Review

- Owner: the operator or maintainer accountable for outcome and approvals.
- Implementer: the human or agent making changes.
- Review authority: the operator or designated reviewer who accepts evidence.
- Codex may draft plans and implement approved plans, but does not self-approve financial, architectural, live-trading, commit, push, or release decisions.

## Lifecycle States

- Draft: being written or refined.
- Proposed: ready for operator or review-authority approval.
- Approved: the plan is accepted for its stated purpose; implementation may begin only when the related roadmap phase is authorized and all required operator, risk, and phase gates are satisfied.
- In Progress: implementation has started.
- Blocked: progress requires unavailable evidence, approval, access, or dependency.
- Verifying: implementation is complete and checks are running.
- Complete: acceptance criteria and closure evidence are satisfied.
- Superseded: replaced by another plan.
- Abandoned: intentionally stopped without completion.

## Planning Rules

Every plan must state:

- Objective and success criteria.
- Scope and out-of-scope boundaries.
- Dependencies and preconditions.
- Assumptions and invariants.
- Expected files or subsystems to change.
- Implementation steps.
- Verification strategy.
- Rollback strategy.
- Risks and decision points.
- Progress log and deviations.
- Completion evidence.

Plans must not invent repository facts. If evidence is missing, record the uncertainty and define the inspection needed to resolve it.

Plan approval, phase approval, and ADR acceptance are distinct. None independently authorizes a blocked phase, source implementation, broker selection, documentation-platform selection, credentials, or live trading.

## Workstream Architecture Relationship

`docs/WORKSTREAM_ARCHITECTURE.md` is the current project-level domain and coordination map. Plans must identify the affected workstream and the governing roadmap phase, if any, without treating a workstream label or strategic activity as implementation approval.

Workstream II completed Phase 23 selection with FIBO Group through cTrader as
the demo-only XAUUSD target. ADR 0004 makes official cTrader Open API the sole
integration path, classifies the Algo Bridge as abandoned/non-controlling/out
of scope, and permanently cancels OANDA. Gate 1 and Gate 3 are revalidated;
Gate 2 and Gate 5 were accepted by Wade on 2026-08-07. Gate 5.1 execution,
Gate 6A discovery,
the mandatory Wade account checkpoint, Gate 6B authentication, market data,
orders, and live actions remain separately blocked.

Strategy 3 gate labels may be added later as a governance overlay. Do not infer or implement a full Kanban system from this planning rule.

## Deviation Handling

If implementation discovers a material mismatch with the plan:

- Stop changing affected areas.
- Record the deviation.
- Inspect enough context to understand impact.
- If the deviation affects scope, architecture, financial safety, credentials, tests, or data semantics, request operator approval before proceeding.
- If the deviation is minor and still inside scope, document it in the progress log and final report.

## Session Resumption

Before resuming an active plan:

```sh
git status --short
git branch --show-current
git log --oneline --decorate -n 5
```

Then compare the plan with:

- Current `docs/PROJECT_STATE.md`.
- Relevant ADRs.
- Changed files and untracked files.
- Previous handoff in `docs/HANDOFF.md` format, if present.

Resume only from a verified state. If state is ambiguous, create a handoff-style assessment before further mutation.

## Closure Requirements

A plan may be closed only after:

- Scope is implemented or explicitly reduced by approval.
- Acceptance criteria are satisfied.
- Verification commands and outputs are recorded.
- Deviations and unresolved risks are recorded.
- Files changed are listed.
- Documentation updates are complete.
- Rollback path remains understandable.
- Final outcome states whether the plan is complete, superseded, or abandoned.

## Abandoned Or Superseded Plans

Do not delete abandoned or superseded plans if they contain decision-relevant history. Mark status clearly and link the replacement plan or reason for abandonment. Execution history belongs primarily in Git commits, pull requests, issues, and handoffs, not in a growing diary inside `docs/PROJECT_STATE.md`.

## Plan Schema

```markdown
# Plan: <title>

- Plan ID:
- Status:
- Owner:
- Implementer:
- Review authority:
- Related roadmap phase:
- Related issue or decision:
- Created:
- Updated:

## Objective

## Context

## Scope

## Out of Scope

## Preconditions

## Assumptions

## Invariants

## Files Expected to Change

## Implementation Steps

## Verification

## Risks

## Rollback

## Progress Log

## Deviations

## Completion Evidence

## Final Outcome
```

# Plan: Rebaseline cTrader Open API Gates 1-5

- Plan ID: `PLAN-20260806-ctrader-open-api-gate5`
- Status: Complete — Gate 1/Gate 3 revalidated and Gate 2/Gate 5 accepted by
  Wade; Gate 5.1 and the Gate 6 umbrella remain unauthorized
- Owner: Wade
- Implementer: Codex
- Review authority: Wade
- Related roadmap phase: Workstream II, Open API Gate 5 design only
- Related issue or decision: Wade rebaseline directive dated 2026-08-07; ADR
  0004
- Created: 2026-08-06
- Updated: 2026-08-07

## Objective

Preserve and prune the active Gate 5 review surface, revalidate protocol fit,
numeric mapping, and pre-implementation baseline integrity, correct the final
Gate 2/Gate 5 defects, and produce self-contained acceptance evidence without
executing Gate 5.1, Gate 6A, or Gate 6B.

## Context

Wade reported `TradeBot Demo Integration` as `Active`. Official Open API is the
sole integration path and OANDA is permanently cancelled. The Algo Bridge is
abandoned, non-controlling, and out of scope. The active Open API worktree is
branch `codex/ctrader-open-api-gate5` at accepted baseline `400b486`; its Gate 5
changes are unstaged and uncommitted. Wade superseded the previous corrective
handoff, authorized bounded Gates 1-3 revalidation, and kept the Gate 6
umbrella (`Gate 6A → mandatory Wade checkpoint → Gate 6B`) blocked.

## Scope

- Reconcile the active Open API worktree and detect Bridge contamination by
  changed path/content only.
- Audit every active tracked and untracked Gate 5 file without general cleanup.
- Revalidate Gates 1-3 from official cTrader documentation, pinned official
  proto2 sources, accepted broker-neutral contracts, and the active baseline.
- Correct Gate 2 with deterministic integer-only price rounding and an exact
  zero-anchored volume predicate.
- Correct only the controlling Gate 5/authority documents required for OAuth
  correlation, Gate 5.1, staged Gate 6A/checkpoint/Gate 6B account selection,
  demo-only/secret boundaries, and gate status.
- Create pre-pruning and post-pruning deterministic snapshots plus one
  self-contained acceptance-evidence bundle outside the repository.

## Out of Scope

- OAuth authorization, callback execution, token exchange or refresh.
- cTrader connection, account discovery/authentication, symbols, XAUUSD data,
  quotes, orders, modifications, cancellations, or any live account action.
- Source, test, dependency, runtime-mode, endpoint-client, or risk-limit changes.
- Any Bridge investigation, recovery, preservation, validation, implementation,
  merge, deletion, or historical reconstruction.
- Gate 5.1 execution; Gate 6A discovery; Gate 6B authentication; staging,
  commit, push, reset, clean, stash, or relocation.

## Preconditions

- Explicit Wade directive is controlling.
- Repository/worktree identity and dirty-change ownership are unambiguous.
- No credential or token value is inspected, created, copied, or logged.

## Assumptions

- Application status and target details are operator-supplied controlling facts.
- Official cTrader documentation and the official proto-message repository are
  controlling protocol sources.
- Actual FIBO account/symbol values are unavailable by design because this
  directive prohibits operational account and market requests.

## Invariants

- `BACKTEST` remains default and no runtime behavior changes.
- `accounts` is the only initial scope; `trading` remains blocked.
- `demo.ctraderapi.com:5035` with Protobuf over strict TLS/TCP is the only
  future message endpoint and has no fallback.
- Visible account/login numbers are never used as `ctidTraderAccountId`.
- Live trading and every order operation remain prohibited.

## Files Expected to Change

- `.gitignore`, `.env.example`, `PLANS.md`.
- Gates 1-3 evidence, Gate 5 design, ADR/index, architecture, security,
  configuration, project state, roadmap, workstream, risk, backlog, and
  documentation index files.

## Implementation Steps

1. Reconcile the active worktree, baseline, upstream, status, and diff scope.
2. Detect Bridge contamination by active changed paths only; stop if found.
3. Prune the ten excessive skill edits and historical Phase 22 rewrite proven
   to be prior Gate 5 scope expansion.
4. Revalidate Gate 1 protocol fit from official documentation and a pinned
   official schema revision; stop if any essential decision remains open.
5. Revalidate Gate 2 conversions against `Decimal64` and provider field
   types/presence; stop if any required factor is invented or unresolved.
6. Revalidate Gate 3 accepted-base, contamination, production-behavior, and
   smallest future source-surface integrity.
7. Reconcile Gate 5 correlation, demo selection, secrets, and endpoint rules.
8. Run offline Git, documentation, schema-reference, and secret-safety checks.
9. Create post-pruning and self-contained external evidence and stop before
   Gate 6A and therefore before the complete Gate 6 umbrella.

## Verification

```sh
git worktree list --porcelain
git diff --check
git status --short
git diff --name-status
grep -RInE 'TO''DO|TB''D|FIX''ME|PLACE''HOLDER|example ''only' AGENTS.md PLANS.md CONTRIBUTING.md docs .agents 2>/dev/null || true
grep -RInE 'live trading|live-trading|real order|API key|credential|secret' AGENTS.md PLANS.md CONTRIBUTING.md docs .agents 2>/dev/null || true
```

Review `.env.example` and the diff with a secret-pattern scan that reports only
whether a potential value exists.

## Risks

- cTrader does not document OAuth `state`; without verified correlation the
  loopback callback remains exposed to local injection/login-CSRF and OAuth is
  blocked.
- A configurable endpoint could accidentally reach a live host.
- Long-lived refresh tokens or account IDs could leak through logs or local
  files if the Keychain/redaction boundary is bypassed.
- `protoc`, the Protobuf runtime, and a dedicated strict TLS implementation are
  not installed or dependency-approved; they remain Gate 6A prerequisites.
- The exact FIBO `brokerTitleShort` literal and intended demo account must be
  observed during a separately authorized Gate 6A and confirmed by Wade before
  Gate 6B; neither may be guessed or required circularly before discovery. The
  spot timestamp unit also requires later authorized evidence.

## Rollback

With operator approval, reverse only the active Gate 5 documentation diff. Do
not inspect or alter the abandoned Bridge, delete branches/worktrees, or rotate
any secret; Gate 5 creates no external or credential state.

## Progress Log

- 2026-08-06: Established the active Gate 5 branch at baseline `400b486` and
  drafted the initial design; Wade did not accept that report.
- 2026-08-06: Confirmed no existing local/remote Open API branch or recoverable
  Open API diff, then created the scoped branch from exact baseline `400b486`.
- 2026-08-06: Verified official cTrader OAuth, account discovery, endpoint,
  refresh, error, heartbeat, and account-ID semantics and authored Gate 5.
- 2026-08-07: Reconciled the active Open API worktree without inspecting Bridge
  history; no Bridge paths contaminate the active diff.
- 2026-08-07: Found no controlling Gates 1-3 Open API artifact or generated
  cTrader definitions on the active branch/baseline; bounded revalidation is
  awaiting Wade authorization.
- 2026-08-07: Corrected the undocumented OAuth `state` assumption, account-field
  presence/identity rules, sole-path disposition, and Gate 6 umbrella blockers.
- 2026-08-07: Preserved the complete pre-pruning state and hashes outside the
  repository, then removed only ten proven excessive skill edits and the
  historical Phase 22 rewrite.
- 2026-08-07: Revalidated Gate 1 with Protobuf/TLS-TCP on the fixed demo port,
  Gate 2 with exact checked field conversions, and Gate 3 as the
  pre-implementation accepted-base/local-diff integrity boundary.
- 2026-08-07: Corrected demo selection so other live entries are excluded but
  do not invalidate one uniquely eligible FIBO demo account.
- 2026-08-07: Final correction replaced inbound price `RejectUnaligned` with
  exact integer `NearestTiesAwayFromZero`, fixed the zero-anchored volume
  predicate, defined Gate 5.1, and split account proof into Gate 6A, a mandatory
  Wade checkpoint, and Gate 6B.
- 2026-08-07: Wade accepted Gate 2 and Gate 5. Acceptance records and the
  canonical evidence package were finalized without authorizing Gate 5.1 or
  any part of the Gate 6 umbrella.

## Deviations

No prior controlling Gates 1-3 artifact existed, so Wade-authorized bounded
revalidation created new evidence. No Bridge history or operational cTrader
state was used. Runtime FIBO metadata remains assigned to its later gate.

## Completion Evidence

Pre/post snapshots, Gates 1-3 documents, corrected Gate 5 design, scope audit,
checks, hashes, and a self-contained external bundle are required for closure.
No prohibited operational or Git-history action is performed.

## Final Outcome

Gate 1 and Gate 3 are revalidated. Gate 2 and Gate 5 were accepted by Wade on
2026-08-07. Gate 5.1 and the Gate 6 umbrella (`Gate 6A → mandatory Wade
checkpoint → Gate 6B`) remain blocked and separately authorized; acceptance
does not imply execution authority.

# Active Approved Research Plan: Workstream II Iran-Compatible Provider Search

- Plan ID: `PLAN-20260729-iran-compatible-provider-search`
- Status: Superseded — FIBO Group/cTrader selected; OANDA permanently cancelled
- Owner: Wade
- Implementer: Codex and approved research actors
- Review authority: Wade
- Related workstream/phase: Workstream II; bounded pre-Phase-23 evidence preparation
- Created: 2026-07-29
- Updated: 2026-07-29

## Objective

Record the OANDA critical blocker and hold, then build a current evidence base for a lawful Iran-compatible replacement capable of a demo-only XAUUSD automation milestone.

Success requires a provider comparison with explicit Iran-resident eligibility, demo, instrument, automation, deterministic-replay, security, and risk gates, followed by a Wade checkpoint. It does not require or permit provider selection or account action.

## Context

OANDA's available registration path did not provide a lawful route for an Iranian citizen residing in Tehran. OANDA Support may clarify that state later. The operator has prohibited VPN-based residence substitution, false details, or third-party identity use and has authorized a pivot to Iran-compatible foundations.

## Scope

- Record OANDA as ON HOLD.
- Research current first-party eligibility, demo, XAUUSD, and automation evidence.
- Define mandatory written-support questions and candidate gates.
- Rank candidates only for further validation, not selection.
- Synchronize current-state, roadmap, backlog, and documentation-index wording.

## Out Of Scope

- Provider selection or formal Phase 23 activation.
- Account creation, login, identity-document submission, or funding.
- Credentials, tokens, endpoints, connectivity, provider code, or dependencies.
- Demo, sandbox, or real orders.
- Core-engine, risk-limit, order-routing, or live-trading changes.

## Preconditions

- Wade's OANDA hold and Iran-compatible pivot directive is controlling.
- The operator explicitly authorized research and repository documentation.
- `main` was inspected at `6bdc548`; no open PRs or issues were returned.
- Phase 23 remains formally Not Started and Phase 24 remains Blocked.

## Assumptions

- Provider country rules, legal entities, platform availability, and sanctions posture can change.
- A Persian-language site, accessible registration form, or omission from a restriction list is not conclusive eligibility evidence.
- Written provider confirmation is required before any account action.

## Invariants

- Use true Iranian citizenship and Tehran residence.
- No VPN-location substitution, false address, third-party identity, or concealed documents.
- Demo-only research never implies live readiness.
- Provider-native data remains below `BrokerGateway`.
- Deterministic audit/replay, fail-closed behavior, and existing risk gates remain intact.

## Files Expected To Change

- `PLANS.md`
- `docs/PROJECT_STATE.md`
- `docs/ROADMAP.md`
- `docs/RESIDUAL_GAPS_BACKLOG.md`
- `docs/README.md`
- `docs/WORKSTREAM_II_IRAN_COMPATIBLE_PROVIDER_PIVOT.md`

No source, test, build, configuration, credential, account, data, or generated file is in scope.

## Implementation Steps

1. Reconcile current `main` authority and the prior conditional OANDA lane.
2. Screen candidates using current first-party eligibility, demo, XAUUSD, and automation evidence.
3. Record verified facts, inferences, unresolved items, exclusions, and support questions.
4. Synchronize authority-facing documentation without opening Phase 23 or Phase 24.
5. Run connector-backed content, scope, and non-authorization checks.
6. Stop at the written-support and Wade-selection checkpoint.

## Verification

- Confirm only the six expected Markdown files change.
- Confirm OANDA is ON HOLD in all active state references.
- Confirm Iran-resident lawful eligibility is a mandatory evidence gate.
- Confirm no provider is described as selected.
- Confirm Phase 23 remains formally Not Started and Phase 24 remains Blocked.
- Confirm no account, credential, connectivity, order, provider-code, risk, or live authorization appears.
- Local CMake/CTest and `git diff --check` are not required for source behavior; connector-backed content checks must be reported because this execution has no local checkout.

## Risks

- Provider marketing or translated pages may conflict with the controlling legal entity's current terms.
- Iran eligibility may change quickly or differ between demo, live, API, nationality, residence, and payment access.
- Offshore regulation and operational recourse may be materially weaker than OANDA's.
- Platform/API availability may not include the candidate's demo account or exact XAUUSD symbol.
- Research ranking could be misread as selection unless gates remain explicit.

## Rollback

Revert only this documentation package. OANDA remains blocked by real-world eligibility evidence regardless of rollback. No external account, credential, connection, order, or runtime state exists to unwind.

## Progress Log

- 2026-07-29: Wade placed OANDA integration ON HOLD and directed an Iran-compatible replacement search.
- 2026-07-29: Operator authorized both research and repository documentation.
- 2026-07-29: Initial first-party screen identified FIBO Group cTrader as the strongest provisional validation candidate; no provider was selected.
- 2026-07-29: Current-state, roadmap, backlog, planning, and documentation-index updates prepared.

## Deviations

No provider was contacted and no account path was exercised. Written Iran-resident and API confirmation remains unresolved.

## Completion Evidence

The initial evidence package is complete when merged and cross-document checks pass. The plan remains In Progress until written provider evidence is reviewed at a Wade checkpoint.

## Final Outcome

Pending. Provider selection, Phase 24, account creation, credentials, connectivity, implementation, orders, and live trading remain Blocked / NO-GO.

# Completed Plan: Workstream I Broker-Neutral Completion

- Plan ID: `PLAN-20260624-workstream-i-broker-neutral-completion`
- Status: Complete — Accepted
- Owner: Operator
- Implementer: Codex
- Review authority: Operator
- Related roadmap phase: Phase 22: Broker-Neutral Execution Adapter Alignment and MT5/Prop-Account Readiness
- Related issue or decision: ADR 0003 and explicit operator implementation directive dated 2026-06-24
- Created: 2026-06-24
- Updated: 2026-06-25

## Objective

Finish the broker-neutral Workstream I implementation while preserving deterministic `BACKTEST`, locally simulated `PAPER`, existing financial risk limits, and live-trading NO-GO. Provider selection and concrete broker integration remain Workstream II Phases 23-24.

Success requires explicit lifecycle contracts, a provider-neutral adapter boundary below `BrokerGateway`, deterministic simulation/replay, quantity-aware risk evaluation, versioned snapshots and lifecycle persistence, full regression evidence, synchronized documentation, and no provider schema leakage into the core.

## Context

Phase 21 is Complete — Approved and ADR 0003 is Accepted. The Phase 22 research artifact and Workstream I contracts defined the required broker-neutral behavior. The operator explicitly authorized this bounded broker-neutral implementation on 2026-06-24, accepted the Operator Acceptance Candidate on 2026-06-25, and PR #15 merged the accepted adapter-gateway implementation into `main`.

## Scope

- Broker-neutral fixed-point values, order lifecycle, failure, health, account, instrument, reconciliation, rule-profile, and risk-decision contracts.
- `OrderLifecycleStore` with legal transitions, stable internal IDs, idempotency, duplicate prevention, partial/unknown state, and audit history.
- `IBrokerAdapter` below `BrokerGateway` and a deterministic local adapter for PAPER/tests.
- Gateway normalization, ID correlation, adapter health, explicit cancel/reconciliation events, and fail-closed mode behavior.
- `ExecutionEngine` submission semantics and final normalized-quantity `RiskEngine` evaluation.
- Provider-neutral synthetic rule evaluation with no provider thresholds.
- Versioned lifecycle/snapshot persistence, deterministic replay fixtures, metrics, tests, measurements, documentation, and closure evidence.

## Out of Scope

- Broker, prop firm, program, account, instrument universe, or adapter-topology selection.
- Provider APIs, MT5 terminal integration, external endpoints, network sessions, credentials, certificates, account access, or captured private data.
- Sandbox or real orders, live trading, Phase 23 or Phase 24 activation.
- Changes to existing risk limits, drawdown thresholds, position sizing, fees, slippage, kill-switch behavior, or live-readiness requirements.
- Generated artifact publication or profitability/production-readiness claims.

## Preconditions

- Operator implementation GO is recorded by the 2026-06-24 directive.
- ADR 0003 and the approved Phase 21 artifacts remain authoritative.
- Work begins from current clean `main` and proceeds as sequential reviewed PRs.
- Any provider-dependent assumption stops the affected package and is deferred to Phases 23-24.

## Assumptions

- No external consumer compatibility guarantee exists beyond repository call sites and documented behavior.
- Existing `BACKTEST` results and Phase 13-18 tests remain regression authority.
- Synthetic adapter events and rule profiles are deterministic, provider-neutral, and safe to version as test code or small labelled fixtures.

## Invariants

- Strategy and allocation code never submit directly to an adapter.
- New exposure passes preliminary and final normalized-quantity risk checks.
- Acknowledgement never implies fill; only validated deduplicated execution or approved reconciliation mutates portfolio/risk state.
- `BACKTEST` performs no network action; `PAPER` stays locally simulated; `LIVE` without a separately approved concrete adapter fails closed.
- No adapter event, reconnect, or snapshot clears halt or close-only automatically.
- No secret values, provider-native schemas, account data, or generated outputs enter Git.

## Files Expected to Change

- Broker-neutral contract, lifecycle, adapter, gateway, execution, risk, metrics, and serialization headers/sources under `include/` and `src/`.
- `tests/phase22_tests.cpp`, deterministic test helpers, `CMakeLists.txt`, and a broker-neutral benchmark only after correctness tests exist.
- Architecture, testing, configuration, data, project-state, roadmap, Workstream I, and plan documentation.

## Implementation Steps

1. Activate authority and record the bounded GO before source edits.
2. Add broker-neutral contracts, fixed-point arithmetic, lifecycle storage, and initial Phase 22 tests.
3. Add `IBrokerAdapter`, deterministic PAPER adapter, and gateway normalization/event handling.
4. Integrate explicit submission/lifecycle behavior with final quantity-aware risk checks and provider-neutral rule evaluation.
5. Add versioned persistence, deterministic replay/reconciliation, metrics, and failure/restart coverage.
6. Add reproducible measurements after correctness, synchronize documentation, and close only on reviewed evidence.

## Verification

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build -R phase22_tests --output-on-failure
ctest --test-dir build -R 'phase13_tests|phase17_tests|phase18_tests' --output-on-failure
ctest --test-dir build --output-on-failure
git diff --check
```

Required scenarios include acknowledgement without fill, rejection, partial/final fill, cancel rejection/confirmation, cancel-fill race, timeout, expiry, unknown state, reconciliation, duplicate/stale/out-of-order events, halt/close-only, stale/missing snapshots, fixed-point normalization, version migration, restart deduplication, deterministic replay, network-free `BACKTEST`, repeatable `PAPER`, and fail-closed `LIVE` without an approved adapter.

## Risks

- Duplicate economic mutation, invalid lifecycle transitions, unsafe rounding, stale snapshots, restart deduplication loss, risk-gate bypass, provider leakage, mode confusion, and unbounded lifecycle memory.
- Broad interface changes may create regression risk across execution, benchmarks, and main wiring; changes therefore remain sequential and independently revertible.

## Rollback

- Revert each merged implementation PR independently in reverse order with operator approval.
- Retain version-1 snapshot reading until version-2 migration evidence is accepted.
- If a package requires provider-specific behavior or ADR boundary changes, stop before merging it and return Phase 22 to Blocked pending review.

## Progress Log

- 2026-06-24: Operator explicitly directed implementation of this bounded broker-neutral plan.
- 2026-06-24: Local `main` fast-forwarded to `1a32f7f`; tracked worktree clean; local-only handoff excluded through `.git/info/exclude`.
- 2026-06-24: Authority-activation PR started; no source changes made before activation.
- 2026-06-25: Adapter-gateway slice advanced to Operator Acceptance Candidate with `IBrokerAdapter`, deterministic PAPER adapter, gateway normalization/lifecycle dispatch, fail-closed mode tests, and local configure/build/CTest validation.
- 2026-06-25: Operator declared the candidate accepted; PR #15 merged `feat: advance Workstream I to acceptance candidate` into `main` as merge commit `ba8643c69bd14e2ffa3822f79e5bf44c58c539fc`.

## Deviations

- None.

## Completion Evidence

- PR #14 merged the broker-neutral lifecycle contracts and Phase 22 test foundation.
- PR #15 merged `IBrokerAdapter`, deterministic PAPER adapter behavior, `BrokerGateway` normalization/lifecycle dispatch, fail-closed mode coverage, and synchronized architecture/project-state/plan documentation.
- Local validation for the accepted candidate passed on 2026-06-25:
  - `cmake -S . -B build`
  - `cmake --build build`
  - `ctest --test-dir build -R phase22_tests --output-on-failure`
  - `ctest --test-dir build --output-on-failure`
- GitHub validation for PR #15 passed before merge.
- Operator acceptance was declared on 2026-06-25 before this authority-doc closure.

## Final Outcome

Complete — Accepted. Phase 22 broker-neutral implementation is closed at the approved Workstream I boundary. Broker selection, MT5 connectivity, credentials or account access, sandbox or real orders, Phase 23 activation, broker-dependent implementation, and live trading remain unauthorized until separate explicit operator approval.

# Completed Plan: Phase 22 Execution Adapter Re-Scope

- Plan ID: `PLAN-20260621-phase22-execution-adapter-rescope`
- Status: Complete — Approved for Documentation/Scoping Only
- Owner: Operator
- Implementer: Codex
- Review authority: Operator
- Related roadmap phase: Phase 22: Broker-Neutral Execution Adapter Alignment and MT5/Prop-Account Readiness
- Related issue or decision: ADR 0003 and the operator directive dated 2026-06-21
- Created: 2026-06-21
- Updated: 2026-06-21

## Objective

Replace the obsolete Phase 22 venue target with broker-neutral execution-adapter alignment and deterministic MT5/prop-account readiness contracts. Preserve Phase 22 implementation as Blocked / NO-GO and preserve live trading as unauthorized.

## Context

Phase 21 is Complete — Approved and ADR 0003 is Accepted. Those approvals establish broker-neutral architecture but do not authorize Phase 22 implementation. The operator approved this documentation/scoping re-scope and explicitly withheld authorization for source changes, MT5 connectivity, account access, credentials, broker-specific implementation, and live trading.

## Scope

- Synchronize the Phase 22 title, objective, boundaries, and gate in `docs/ROADMAP.md` and `docs/PROJECT_STATE.md`.
- Define broker-neutral contracts for order lifecycle, account/equity snapshots, position reconciliation, symbol metadata, lot sizing, execution-result mapping, and failure handling.
- Preserve deterministic replay and simulation requirements for all future contract validation.
- Define configurable prop-account risk-rule modeling requirements without selecting a prop firm or inventing account-specific thresholds.
- Permit offline MT5/prop-account compatibility research only.

## Out of Scope

- C++ source, tests, CMake, ADR 0003, broker code, credentials, risk limits, live configuration, or implementation files.
- Broker selection, prop-firm selection, or Workstream II Phase 23 decisions.
- MT5 terminal bridges, platform APIs, broker APIs, network connectivity, account access, or real order routing.
- Live trading, account mutation, secret inspection, or risk-control bypass.

## Preconditions

- Work begins from a clean `main` branch on a dedicated documentation branch.
- Phase 21 remains Complete — Approved.
- ADR 0003 remains Accepted.
- Phase 22 implementation and live trading remain Blocked / NO-GO.

## Assumptions

- MT5/prop-account readiness is a downstream compatibility target, not a broker or prop-firm selection.
- Offline research may identify contract requirements but cannot authorize connectivity or implementation.
- No firm-specific risk threshold is authoritative without a later explicit operator decision and evidence.

## Invariants

- Strategy and allocation logic remain independent of broker and platform schemas.
- New exposure remains gated through `ExecutionEngine` and `RiskEngine` before `BrokerGateway` submission.
- Future adapters attach below `BrokerGateway`; they do not bypass it.
- Portfolio and risk state change only from confirmed fill or reconciliation evidence.
- `BACKTEST` remains deterministic and independent of network, terminal, account, credential, wall-clock, and live endpoint state.
- Prop-account constraints may tighten existing risk controls but must never weaken them.

## Files Expected to Change

- `PLANS.md`
- `docs/ROADMAP.md`
- `docs/PROJECT_STATE.md`
- `docs/README.md` only if an index or authority cross-reference requires synchronization

## Implementation Steps

1. Verify Git state and current Phase 21, ADR 0003, Phase 22, and live-trading authority.
2. Replace obsolete Phase 22 venue wording with the approved broker-neutral title and scope.
3. Record MT5/prop-account readiness as a downstream compatibility target without selecting a broker, prop firm, connection method, or account.
4. Record the required broker-neutral execution, reconciliation, symbol, sizing, result, failure, replay, and prop-risk contracts.
5. Preserve explicit implementation, connectivity, credential, account-access, and live-trading NO-GO gates.
6. Run documentation consistency, obsolete-reference, phase-state, ADR-status, and live-status validation.

## Verification

Documentation-only verification:

```sh
git status --short
git diff --name-status
git diff --stat
git diff --check
rg -n -i 'Nobi''tex' docs/ROADMAP.md docs/PROJECT_STATE.md PLANS.md docs/README.md
rg -n "Phase 21|Phase 22|ADR[- ]?0003|ADR 0003|Accepted|Blocked / NO-GO|live trading|MT5|prop-account" docs/ROADMAP.md docs/PROJECT_STATE.md PLANS.md docs/README.md docs/decisions/README.md docs/decisions/0003-workstream-i-integration-architecture.md docs/RISK_POLICY.md docs/LIVE_TRADING_READINESS.md
```

CMake and CTest are not required because this plan changes documentation only and does not alter source behavior or verified build/test commands.

## Risks

- MT5 readiness language could be misread as connectivity or implementation authorization.
- Prop-account readiness could be misread as selecting a prop firm or accepting unverified risk thresholds.
- A platform-specific contract could leak into core strategy, allocation, replay, analytics, portfolio, execution, or risk code.
- Phase 22 scoping could be mistaken for phase activation.

## Rollback

Revert only this documentation re-scope with operator approval. ADR 0003, Phase 21 artifacts, source, tests, CMake, credentials, broker code, risk limits, and live configuration remain unchanged.

## Progress Log

- 2026-06-21: Operator approved the Phase 22 target re-scope for documentation and scoping only.
- 2026-06-21: Created `docs/phase22-execution-adapter-rescope` from clean `main` at `c1df5ee`.
- 2026-06-21: Synchronized roadmap, project-state, and planning authority; `docs/README.md` required no index change.

## Deviations

None currently recorded.

## Completion Evidence

Completed on 2026-06-21 with documentation-only validation:

- `git diff --check` passed with no whitespace errors.
- The obsolete Phase 22 venue-reference scan returned no matches.
- Phase 21 remains Complete — Approved and ADR 0003 remains Accepted.
- Phase 22 implementation remains Blocked / NO-GO.
- MT5 connectivity, account access, credentials, real order routing, and live trading remain unauthorized.
- Placeholder audit returned no hits.
- Documentation and skill inventories completed.
- CMake and CTest were skipped because source, tests, CMake, runtime behavior, and verified commands were unchanged.

## Final Outcome

Phase 22 is re-scoped to broker-neutral execution-adapter alignment with MT5/prop-account readiness as the downstream compatibility target. Documentation/scoping and offline research are authorized; implementation, MT5 connectivity, account access, credentials, real order routing, and live trading remain Blocked / NO-GO.

# Historical Completed Plan: Workstream I Integration Alignment

- Plan ID: `PLAN-20260618-workstream-i-integration-alignment`
- Status: Complete — Approved
- Owner: Operator
- Implementer: Codex
- Review authority: Operator
- Related roadmap phase: Phase 21 Infrastructure Alignment
- Related issue or decision: `docs/decisions/0003-workstream-i-integration-architecture.md`
- Created: 2026-06-18
- Updated: 2026-06-19

## Objective

Construct the missing Phase 21 plan layer for Workstream I: broker-neutral integration architecture between the deterministic TradeBot core and future broker or exchange interfaces.

## Context

Phase 18 and Phase 19 continuity around local replay, L2 order-book behavior, trigger-order BBO inputs, and `applyBbo` validation is treated as baseline evidence. No Phase 22 implementation is authorized by this plan.

## Scope

- Create the Workstream I integration architecture document.
- Create the Workstream I adapter contract document.
- Create the Workstream I risk matrix.
- Create the Workstream I replay compatibility checklist.
- Create an ADR for broker-neutral integration architecture.
- Update documentation indexes.

## Out of Scope

- C++ source edits.
- Broker-specific APIs, endpoints, schemas, or credentials.
- Live trading or real order routing.
- Risk-limit, position-sizing, drawdown, halt, close-only, fee, or slippage changes.
- Generated-data cleanup or benchmark-result publication.
- Phase 22 implementation.

## Preconditions

- Operator approved Phase 21 plan execution.
- Repository state was inspected before mutation.
- Work is documentation-only.

## Assumptions

- Phase 18/19 L2 validation is accepted as continuity evidence by the operator.
- Phase 21 artifacts are allowed to describe future Phase 22 constraints without implementing them.
- `BACKTEST` remains the deterministic default.
- `LIVE` remains prohibited without explicit future operator authorization.

## Invariants

- Strategies must not directly submit broker orders.
- New exposure must pass through `ExecutionEngine` and `RiskEngine`.
- Broker-facing behavior remains behind `BrokerGateway`.
- Credentials remain behind `AuthManager` and `SystemConfig`.
- Replay and backtest behavior must not depend on external network, broker, credential, or wall-clock state.

## Files Expected to Change

- `PLANS.md`
- `docs/README.md`
- `docs/WORKSTREAM_I_INTEGRATION_ARCHITECTURE.md`
- `docs/WORKSTREAM_I_ADAPTER_CONTRACT.md`
- `docs/WORKSTREAM_I_RISK_MATRIX.md`
- `docs/WORKSTREAM_I_REPLAY_COMPATIBILITY_CHECKLIST.md`
- `docs/decisions/README.md`
- `docs/decisions/0003-workstream-i-integration-architecture.md`

## Implementation Steps

1. Inspect repository state, documentation indexes, planning schema, and ADR template.
2. Draft Phase 21 integration architecture from verified repository boundaries only.
3. Draft broker-neutral adapter contract without external API assumptions.
4. Draft subsystem risk matrix and replay compatibility checklist.
5. Draft ADR 0003 for operator disposition.
6. Update documentation indexes and this active plan.
7. Run documentation validation checks.

## Verification

Documentation-only verification:

```sh
git diff --check
find docs -maxdepth 3 -type f -print | sort
find .agents/skills -name SKILL.md -print | sort
grep -RInE 'TO''DO|TB''D|FIX''ME|PLACE''HOLDER|example ''only' AGENTS.md PLANS.md CONTRIBUTING.md docs .agents 2>/dev/null || true
grep -RInE 'live trading|live-trading|real order|API key|credential|secret' AGENTS.md PLANS.md CONTRIBUTING.md docs .agents 2>/dev/null || true
```

CMake and CTest are not required because this plan changes documentation only and does not alter verified commands or source behavior.

## Risks

- Documentation can drift if Phase 22 implementation starts before Phase 21 artifacts are reviewed.
- Broker-neutral contracts may need future refinement when a specific broker or exchange is approved.
- At plan execution time, the checked-out branch was `main`, while project-state docs still referenced Phase 19 context.

## Rollback

Revert the documentation-only changes in Git or delete the newly added Workstream I artifacts and remove their index entries, with operator approval.

## Progress Log

- 2026-06-18: Operator approved Phase 21 plan execution and prohibited Phase 22 implementation.
- 2026-06-18: Phase 21 documentation artifacts and ADR were drafted.
- 2026-06-19: Operator accepted ADR 0003, approved Phase 21 Workstream I artifacts, and kept Phase 22 Blocked / NO-GO.

## Deviations

None currently recorded.

## Completion Evidence

Completed on 2026-06-18 with documentation-only validation:

```sh
git diff --check
find docs -maxdepth 3 -type f -print | sort
find .agents/skills -name SKILL.md -print | sort
grep -RInE 'TO''DO|TB''D|FIX''ME|PLACE''HOLDER|example ''only' AGENTS.md PLANS.md CONTRIBUTING.md docs .agents 2>/dev/null || true
grep -RInE 'live trading|live-trading|real order|API key|credential|secret' AGENTS.md PLANS.md CONTRIBUTING.md docs .agents 2>/dev/null || true
```

Results:

- `git diff --check` passed with no whitespace errors.
- Documentation and skill inventories completed.
- Placeholder audit returned no hits.
- Risk-term audit returned expected safety-policy and Workstream I references; no secret values were printed.
- CMake and CTest were skipped because this plan changed documentation only and did not alter source behavior or verified commands.

## Final Outcome

At Phase 21 closeout, the plan-layer artifacts were complete and approved, ADR 0003 was accepted, and Phase 22 implementation remained Blocked / NO-GO. The later research-only Phase 22 authorization is recorded in the plan below; it does not change the implementation gate.

# Approved Research Plan: Phase 22 Offline MT5/Prop-Account Research

- Plan ID: `PLAN-20260621-phase22-offline-mt5-prop-research`
- Status: Approved — Research-Only
- Owner: Operator
- Implementer: Codex and future approved research actors
- Review authority: Operator
- Related roadmap phase: Phase 22 Broker-Neutral Execution Adapter Alignment and MT5/Prop-Account Readiness
- Related issue or decision: ADR 0003 and the approved Workstream I Phase 21 artifacts
- Created: 2026-06-21
- Updated: 2026-06-21

## Objective

Create a broker-neutral, offline evidence base for future MT5 and synthetic prop-account modeling without implementing an adapter, choosing a broker or prop firm, accessing an account, using credentials, connecting to MT5, or authorizing real orders or live trading.

Success means:

- The active authority documents use the Phase 22 title `Broker-Neutral Execution Adapter Alignment and MT5/Prop-Account Readiness`.
- Nobitex is omitted from active Phase 22 scope.
- `docs/PHASE22_OFFLINE_MT5_PROP_RESEARCH.md` records the research surfaces, evidence structure, broker-neutral boundaries, deterministic fixture requirements, and blocked decisions.
- Phase 22 implementation remains `Blocked / NO-GO`.
- Phase 23 remains `Not Started`, with no broker or prop firm selected.

## Context

The operator authorized documentation-only offline research and supplied a completed deep-research memo as research input. Research input does not have roadmap, implementation, credential, connectivity, account-access, broker-selection, real-order, or live-trading authority. At task start, the repository contained obsolete Nobitex wording in `docs/ROADMAP.md`; this plan required that wording to be removed from active Phase 22 scope before research-only approval was recorded.

## Scope

- Reconcile `PLANS.md`, `docs/ROADMAP.md`, and `docs/PROJECT_STATE.md` around the current Phase 22 title and gates.
- Create the offline MT5/prop-account research artifact.
- Index the artifact in `docs/README.md`.
- Record MT5 integration surfaces as research classes rather than a selected topology.
- Define synthetic/configurable prop-account rule dimensions without provider thresholds.
- Define future broker-neutral contracts, subsystem boundaries, determinism constraints, fixtures, replay validation, and rollback/non-activation safeguards.

## Out of Scope

- C++ source, headers, CMake, tests, scripts, runtime configuration, data, or generated-output changes.
- MT5 terminal connectivity, terminal installation or automation, account login, account access, external API calls, or network sessions.
- Credentials, endpoints, certificates, account identifiers, or private account data.
- Broker, provider, prop firm, program, rulebook, bridge topology, account type, or instrument selection.
- Real orders, sandbox orders, live account mutation, live trading, Phase 22 software implementation, or Phase 23 activation.
- Financial-limit or risk-default changes.

## Preconditions

- The operator explicitly authorized this bounded documentation/research task.
- Phase 21 is Complete - Approved and ADR 0003 is Accepted.
- Phase 22 implementation remains Blocked / NO-GO.
- Repository state and Git safety are inspected before edits.
- Phase 23 broker selection has not started.

## Assumptions

- The findings enumerated in the operator task are the usable research input from the completed deep-research memo.
- A separately preserved memo artifact was not present in the supplied attachment directory during this task; provider-specific or version-sensitive claims therefore require future evidence-register entries before reliance.
- MT5 product surfaces and prop-program rules may differ by deployment, license, provider, jurisdiction, account, program, and rulebook version.
- Provider examples, if later added, are evidence for rule dimensions only and are not recommendations or readiness evidence.

## Invariants

- `BACKTEST` remains deterministic and offline.
- `PAPER` remains locally simulated unless separately approved work changes that boundary.
- `LIVE` and real orders remain unauthorized.
- New exposure must pass through `ExecutionEngine`, `RiskEngine`, and `BrokerGateway` boundaries.
- Portfolio state may change only from validated, deduplicated fill evidence or approved reconciliation evidence.
- MT5-native types, ticket semantics, status codes, fill policies, and schemas must not become TradeBot core authority contracts.
- Prop-account rules remain synthetic, configurable, versioned, and provider-neutral during Phase 22 research.
- Unknown, stale, malformed, or unreconciled external state fails closed for risk-increasing actions.

## Files Expected to Change

- `PLANS.md`
- `docs/ROADMAP.md`
- `docs/PROJECT_STATE.md`
- `docs/PHASE22_OFFLINE_MT5_PROP_RESEARCH.md`
- `docs/README.md`

No source, CMake, test, script, config, credential, data, runtime, or generated file is expected to change.

## Implementation Steps

These are documentation steps only; they are not Phase 22 software implementation:

1. Audit Git state and the current authority, risk, architecture, live-readiness, adapter, and replay contracts.
2. Replace obsolete active Phase 22 Nobitex wording with the operator-approved broker-neutral MT5/prop-account research scope.
3. Record the offline research GO separately from the Phase 22 implementation NO-GO.
4. Create the research artifact with explicit evidence, lifecycle, reconciliation, account, symbol, lot-sizing, failure, synthetic-rule, subsystem-boundary, determinism, fixture, and blocked-decision sections.
5. Add the artifact to the documentation index.
6. Run documentation, authority, scope, and non-activation audits.

## Verification

Required Git and documentation checks:

```sh
git status --short
git diff --name-status
git diff --stat
git diff --check
find docs -maxdepth 3 -type f -print | sort
grep -RInE 'TO''DO|TB''D|FIX''ME|PLACE''HOLDER|example ''only' AGENTS.md PLANS.md CONTRIBUTING.md docs .agents 2>/dev/null || true
grep -RInE 'live trading|live-trading|real order|API key|credential|secret' AGENTS.md PLANS.md CONTRIBUTING.md docs .agents 2>/dev/null || true
```

The review must also prove:

- Only the five expected Markdown files changed.
- No active authority document assigns Nobitex to Phase 22.
- No wording selects or authorizes a broker, prop firm, program, account, bridge topology, credentials, connectivity, real orders, sandbox use, or live trading.
- MT5-specific schema remains research evidence and does not leak into TradeBot core authority contracts.
- Prop-account rules remain synthetic/configurable unless clearly labeled as external evidence examples.

CMake and CTest are not required because this plan changes documentation only and does not change source, build definitions, tests, runtime behavior, or verified commands.

## Risks

- MT5 interfaces can differ by integration class, deployment, product license, terminal build, server, and broker configuration.
- Prop-account rules can change and can differ by provider, program, jurisdiction, rulebook version, and account state.
- Treating request acceptance as execution could create false portfolio state.
- Ticket reuse or mutation, delayed history, duplicate events, and out-of-order events could corrupt lifecycle state without stable internal identity and deduplication.
- Quantity normalization could increase exposure unless units, rounding policy, invalid-volume rejection, and post-normalization risk checks are explicit.
- Research wording could be misread as implementation or live authorization unless non-activation gates remain prominent.

## Rollback

With operator approval, revert only the five documentation changes listed above. Because this plan permits no source, configuration, credential, connection, account, or runtime mutation, rollback must require no service shutdown, account action, credential rotation, or external-state recovery.

## Progress Log

- 2026-06-21: The Phase 22 offline MT5/prop-account research plan was drafted as Proposed.
- 2026-06-21: The operator authorized documentation-only authority reconciliation and research artifact creation while preserving implementation NO-GO.
- 2026-06-21: Repository and authority preflight found a clean `main` worktree and obsolete Nobitex language only in the active roadmap.

## Deviations

The operator described the completed deep-research memo as available research input, but no separate memo file was present in the supplied attachment directory. This documentation uses only the memo findings enumerated in the operator objective and does not add provider-specific or version-sensitive claims.

## Completion Evidence

The research-only approval gate is the explicit operator decision plus reconciliation of `PLANS.md`, `docs/ROADMAP.md`, and `docs/PROJECT_STATE.md`.

Documentation verification completed on 2026-06-21:

- `git status --short` listed only the five expected documentation paths.
- `git diff --name-status` and `git diff --stat` showed only tracked authority/index documentation changes; the new research file remained intentionally untracked for operator review.
- `git diff --check` passed with no output.
- `git diff --no-index --check /dev/null docs/PHASE22_OFFLINE_MT5_PROP_RESEARCH.md` reported no whitespace errors; exit status `1` was the expected no-index difference status for a new file.
- The placeholder audit returned no hits.
- Authority scans returned no obsolete Phase 22 Nobitex assignment or `Software Alignment` title, no out-of-scope changed path, and no MT5-native lifecycle schema in the three active authority documents.
- CMake and CTest were skipped because no source, CMake, test, runtime, or configuration file changed.

This evidence does not satisfy or relax any software-implementation gate.

## Final Outcome

Approved for research-only work by explicit operator instruction after authority reconciliation. Documentation-only offline MT5/prop-account research is authorized within this plan's boundaries. Phase 22 software implementation, MT5 connectivity, credentials, account access, broker or prop-firm selection, real orders, sandbox/live use, live trading, and Phase 23 activation remain Blocked / NO-GO.
