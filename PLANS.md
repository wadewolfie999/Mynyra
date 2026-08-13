# TradeBot Planning System

## Purpose And Authority

- Purpose: define when and how implementation plans are authored, approved, executed, resumed, closed, abandoned, or superseded.
- Authority level: active approved plans rank below `docs/ARCHITECTURE.md` and above `docs/PROJECT_STATE.md`.
- Audience: operator, maintainers, Codex, contributors, reviewers, and research agents.
- Related documents: `AGENTS.md`, `docs/WORKFLOW.md`, `docs/CODEX_EXECUTION_EVIDENCE.md`, `docs/HANDOFF.md`, `docs/ROADMAP.md`, and `docs/decisions/`.

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

Every plan created or materially revised after this contract update must state:

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
- Exact authorization boundary, attempt budget, stop condition, and prohibited adjacent actions when external, credential, Git-publication, financial, or live-capable actions are possible.
- Evidence epochs when review, staging, commit, exact-commit rebuild, or external execution are separate checkpoints.

Plans must not invent repository facts. If evidence is missing, record the uncertainty and define the inspection needed to resolve it.

Plan approval, phase approval, and ADR acceptance are distinct. None independently authorizes a blocked phase, source implementation, broker selection, documentation-platform selection, credentials, or live trading.

Commit, push, publication, provider execution, retry, later-gate work, order
capability, financial-limit changes, and live use are also distinct approvals.
An earlier approval must not be widened merely because its preconditions later
become satisfiable.

When tests use local ports, certificate authorities, fixed temporary paths,
process names, caches, or generated outputs, plans must name the shared resource
and require sequential cross-build verification unless isolation is proven.

## Workstream Architecture Relationship

`docs/WORKSTREAM_ARCHITECTURE.md` is the current project-level domain and coordination map. Plans must identify the affected workstream and the governing roadmap phase, if any, without treating a workstream label or strategic activity as implementation approval.

Workstream II completed Phase 23 selection with FIBO Group through cTrader as
the demo-only XAUUSD target. ADR 0004 makes official cTrader Open API the sole
integration path, classifies the Algo Bridge as abandoned/non-controlling/out
of scope, and permanently cancels OANDA. Gate 1 and Gate 3 are revalidated;
Gate 2 and Gate 5 were accepted by Wade on 2026-08-07. Gate 5.1 offline
controls were authorized, merged, and accepted on 2026-08-09. Wade separately
authorized Gate 6A, its mandatory account checkpoint, and Gate 6B; market data,
orders, and live actions remain blocked.

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

Identify the last valid evidence epoch. Do not reuse working-tree or pre-commit
results as exact-commit evidence after relevant files changed.

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

## Authorization Boundary

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

# Plan: Complete Gate 5 OAuth Correlation Controls

- Plan ID: `PLAN-20260809-gate5-oauth-correlation-controls`
- Status: Complete — offline implementation and verification complete;
  awaiting Wade acceptance; provider OAuth and Gate 6 remain blocked
- Owner: Wade
- Implementer: Codex
- Review authority: Wade
- Related roadmap phase: Workstream II, Phase 24, Gate 5.1 only
- Related issue or decision: Wade Gate 5 Completion Directive; ADR 0004; PR
  #23 review finding
- Created: 2026-08-09
- Updated: 2026-08-10

## Objective

Remove the `traderLogin` persistence contradiction introduced through PR #23,
implement every offline-verifiable control in the accepted Gate 5.1 design,
and produce a narrow review-ready PR without executing OAuth or beginning Gate
6A.

## Context

PR #23 correctly made `ctidTraderAccountId` memory-only but left optional
`traderLogin`/visible-login metadata and candidate labels in the future Gate 6A
checkpoint and Gate 6B predicate. That contradicts Gate 2 and `SECURITY.md`.
The accepted Gate 5.1 design is sufficient to implement a local one-shot
correlation guard, but official cTrader documentation still does not establish
provider `state` support.

## Scope

- Make `ctidTraderAccountId`, `traderLogin`, account/visible-login numbers, and
  equivalent account-identifying values volatile process-memory only.
- Restrict a future persistent checkpoint predicate to present
  `isLive == false` plus exact byte-for-byte `brokerTitleShort`; fail closed if
  those facts do not identify exactly one intended demo account.
- Add an offline-only correlation guard with operating-system secure random
  generation, fixed loopback binding, monotonic expiry, explicit cancellation,
  single use, exact matching, malformed/duplicate/mismatch/replay rejection,
  code discard, sensitive-state clearing, and bounded redacted diagnostics.
- Add synthetic targeted tests and synchronize affected authority, security,
  risk, architecture, roadmap, testing, and Gate 5 documentation.
- Commit, push, and open one narrow PR against `main` without merging it.

## Out of Scope

- Browser authorization, real provider callback, OAuth code exchange, token
  acquisition/refresh/revocation, cTrader connection, account discovery or
  access, market data, reconnect proof, orders, or trading.
- Gate 6A, the Wade checkpoint, Gate 6B, and Gates 7–9.
- Generated Protobuf bindings, dependency installation, endpoint clients,
  Keychain implementation, listener sockets, broker adapters, runtime-mode
  attachment, risk limits, or live behavior.
- Any abandoned Algo Bridge investigation or change.
- Merge of the resulting PR.

## Preconditions

- Authoritative base is merged `origin/main` commit `75e35eda`.
- The isolated branch is `codex/gate5-completion-oauth-correlation`.
- Wade's directive is explicit source/test/commit/push/PR authority for Gate
  5.1 offline controls only.
- The unrelated operator modification to
  `WORKSTREAM_II_IRAN_COMPATIBLE_PROVIDER_PIVOT.md` remains untouched and
  excluded.

## Assumptions

- The accepted design's “short fixed time window” is implemented as 60
  monotonic seconds, matching the documented one-minute authorization-code
  lifetime while creating no provider-behavior claim.
- Apple/BSD `arc4random_buf` and Linux `getrandom` are the approved local
  operating-system entropy sources; unsupported platforms fail closed.
- A future listener owns socket lifecycle and HTTP response rendering; this
  plan implements only the offline-verifiable correlation state machine.

## Invariants

- `BACKTEST`, `PAPER`, `LIVE`, order routing, risk limits, and existing Gates
  1–4 remain unchanged.
- The guard performs no network or external I/O and is not attached to any
  runtime path.
- Callback code/state/query values never enter diagnostics or persistence.
- Every callback attempt after arming is terminal; later attempts are replay
  rejection.
- Provider `state` support remains unverified until separately authorized real
  evidence exists.
- Gate 6 and all later gates remain blocked.

## Files Expected to Change

- `CMakeLists.txt`, `include/CTraderOAuthCorrelation.hpp`,
  `src/CTraderOAuthCorrelation.cpp`, `tests/ctrader_gate5_1_tests.cpp`.
- `PLANS.md`, `docs/CTRADER_OPEN_API_GATE5.md`, `docs/SECURITY.md`,
  `docs/RISK_POLICY.md`, `docs/ARCHITECTURE.md`, `docs/PROJECT_STATE.md`,
  `docs/ROADMAP.md`, `docs/RESIDUAL_GAPS_BACKLOG.md`,
  `docs/WORKSTREAM_ARCHITECTURE.md`, `docs/TESTING.md`, and ADR 0004.

## Implementation Steps

1. Reconcile merged `main`, worktrees, local changes, and the PR #23 thread.
2. Correct every persisted visible-login/account-identifier selection path.
3. Implement the move-only one-shot correlation guard and synthetic test hook.
4. Add targeted tests for secure generation, fixed binding, expiry, exact
   no-callback timeout, cancellation, match, code discard,
   malformed/duplicate/mismatch/replay rejection, state clearing, and redacted
   diagnostics.
5. Run targeted and full offline builds/tests, documentation checks, diff
   hygiene, and sensitive-data scans.
6. Stage only scoped files, commit, push, open the PR, and create the external
   transfer report.

## Verification

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build -R '^ctrader_gate5_1_tests$' --output-on-failure
ctest --test-dir build --output-on-failure
git diff --check
```

Review the complete diff, run the documented Markdown/security scans, verify
no tracked or untracked scoped artifact contains a real secret/account value,
and confirm the operator's unrelated pivot-document diff hash is unchanged.

## Risks

- Local tests cannot prove that cTrader accepts or returns `state`; claiming
  otherwise would reopen the login-CSRF/callback-substitution gap.
- A diagnostic API that accepts caller text could leak code/state; diagnostics
  must remain fixed categories.
- Reusing or rearming a guard could enable replay; the implementation is
  move-only and terminal after its first callback attempt.
- Broad documentation edits could imply Gate 6 authority; status updates must
  say Gate 5.1 implementation-complete awaiting Wade acceptance and keep every
  external operation blocked.

## Rollback

With operator approval, revert only this plan's commit. No external state,
credential, token, account, provider connection, or order exists to unwind.
The pre-existing operator document and evidence archives remain untouched.

## Progress Log

- 2026-08-09: Reconciled the isolated branch at `75e35eda`, confirmed the
  operator pivot-document diff hash remains
  `9ad9450f1925f2b6c4435bb1c79fb6caab20b353faf14522513a9007b9b34fca`,
  and identified two pre-existing untracked evidence archives for exclusion.
- 2026-08-09: Retrieved PR #23's resolved review thread and confirmed the
  technical finding remains applicable to Gate 5, security policy, and Gate 2.
- 2026-08-09: Began the offline guard, synthetic tests, and persistence-policy
  correction. No prohibited external operation occurred.
- 2026-08-09: Implemented the move-only correlation guard with OS CSPRNG,
  fixed binding checks, monotonic expiry, terminal callback consumption,
  constant-time state comparison, sensitive-state clearing, code discard, and
  fixed diagnostics. Targeted and full CTest passed.
- 2026-08-09: Added explicit, time-aware no-callback expiry and cancellation
  transitions after PR #24 review. Exact-deadline expiry and cancellation now
  clear correlation state, prohibit rearming, and make later callbacks replay
  rejection; synthetic coverage records those invariants.
- 2026-08-09: Reconciled persistent account selection to present
  `isLive == false` plus exact `brokerTitleShort` only. All account identifiers,
  visible logins, candidate labels, ordinals, and derived identifiers are
  volatile and prohibited from the checkpoint/predicate.

## Deviations

- The isolated worktree contains two pre-existing untracked Gate 5 evidence
  archives. They are unrelated to this plan, are preserved, and will not be
  staged or published.

## Completion Evidence

- `cmake -S . -B build`: passed.
- `cmake --build build`: passed; the initial clean target build reproduced the
  two known unrelated warnings in `AsyncNetworkClient.cpp` and
  `RiskEngine.cpp`; the changed source emitted no warning.
- `ctest --test-dir build -R '^ctrader_gate5_1_tests$' --output-on-failure`:
  passed.
- `ctest --test-dir build --output-on-failure`: 7/7 passed.
- Diff, documentation, sensitive-data, unrelated-work, and publication
  evidence is recorded in the PR and external transfer report.

## Final Outcome

The Gate 5.1 offline implementation is complete and ready for Wade review.
Only Wade may accept Gate 5.1. cTrader `state` round-trip behavior remains
unverified; provider OAuth, Gate 6A, the Wade checkpoint, Gate 6B, and all later
gates remain blocked.

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

# Plan: Execute cTrader Open API Gate 6 Read-Only Proof

- Plan ID: `PLAN-20260809-ctrader-open-api-gate6-proof`
- Status: Execution Complete — Accepted by Wade; implementation closure remains
  tied to review and merge of draft PR #25; Gates 7–9 remain blocked
- Owner: Wade
- Implementer: Codex
- Review authority: Wade
- Related roadmap phase: Workstream II, Phase 24, Gate 6 only
- Related issue or decision: Full Gate 6 Single-Goal Execution Directive; ADR
  0004; accepted Gate 5/Gate 5.1 contract
- Created: 2026-08-09
- Updated: 2026-08-10

## Objective

Produce a verified read-only proof of access to the intended FIBO cTrader demo
account through the official Open API while keeping every account identifier
and secret volatile, non-logging, and outside all evidence. Execute Gate 6A,
pause at the mandatory Wade safe-metadata checkpoint while the process remains
alive, then execute Gate 6B in that same process after explicit selection. Stop
before Gate 7.

## Context

PR #24 merged the accepted Gate 5.1 offline controls at authoritative
`origin/main` commit `42affb33478b56526807feab85a02d0b75f7cf64` on
2026-08-09. Wade explicitly accepted the implementation afterward. The merged
repository contains no Gate 6 transport, token path, Keychain adapter, generated
Protobuf binding, or account-proof state machine. The official Protobuf schemas
are pinned at Spotware commit `3fd8bddfbe0cfc2ecfda079623dc4e498af11e66`;
Protobuf was not installed at preflight.

## Scope

- Vendor the four pinned official proto2 inputs with license, provenance, and
  SHA-256 evidence; generate C++ bindings only into the build tree.
- Pin and review the local Protobuf compiler/runtime and use a dedicated strict
  TLS/TCP transport fixed to `demo.ctraderapi.com:5035`.
- Add an isolated Gate 6 proof target, not a broker adapter or runtime mode.
- Add macOS Keychain boundaries for the client secret and scope-qualified token
  envelope, plus an in-process strict HTTPS token client.
- Add a fixed loopback, one-shot OAuth callback path that verifies the accepted
  correlation controls before exchanging a code.
- Implement synthetic Gate 6 state-machine, allowlist, redaction, selection,
  replay, timeout, live-account exclusion, and zero/multiple-match tests before
  any provider traffic.
- Execute Gate 6A read-only discovery, show Wade only exact present `isLive`
  and `brokerTitleShort`, and keep the volatile candidate mapping alive.
- After Wade's exact confirmation, repeat discovery in a fresh demo connection
  generation and perform exactly one account-authentication proof.
- Clear volatile identifiers on every terminal path and create sanitized
  external evidence.

## Out of Scope

- Gate 7 symbol or market-data requests and every Gate 8–9 action.
- Positions, balances, order state, orders, modifications, cancellations, or
  any trading operation.
- Trading messages, live accounts, live endpoints, endpoint configuration,
  fallback hosts, broker adapters, `BrokerGateway`, `ExecutionEngine`,
  `LiveDataAdapter`, or runtime-mode attachment.
- Acceptance, locking, or merge of Gate 6 by Codex.
- Merge of any resulting PR.

## Preconditions

- Authoritative base is merged `origin/main` commit `42affb3`.
- Wade explicitly accepted Gate 5.1 on 2026-08-09.
- Gate 6A, the mandatory checkpoint, and Gate 6B are explicitly authorized by
  the Full Gate 6 Single-Goal Execution Directive.
- The isolated branch is `codex/gate6-read-only-proof`.
- Exact callback remains
  `http://127.0.0.1:18080/ctrader/oauth/callback`; Wade's 2026-08-10 override
  fixes Gate 6 authorization to `trading` scope.
- Secret values must be configured by Wade through the accepted local
  environment/Keychain procedure and must never enter Codex-visible output.

## Assumptions

- cTrader OAuth `state` remains undocumented provider behavior. One controlled
  callback matched exactly on 2026-08-10; every fresh callback must prove the
  same exact round-trip correlation, and absence,
  rejection, mismatch, duplication, or timeout terminates before token exchange.
- Homebrew `protoc 35.1` with pkg-config package `35.1.0` and its matching
  CMake-reported C++ runtime `7.35.1` is the reviewed local toolchain; the
  build requires that exact runtime and schema hashes.
- System libcurl `8.7.1` is used only as an in-process HTTPS client with peer
  and hostname verification and no verbose or raw error output.
- OpenSSL `3.6.3` supplies the dedicated Protobuf TLS/TCP transport with SNI,
  peer-chain verification, and exact hostname verification.
- A fresh token may be acquired if the scope-qualified Keychain envelope is
  absent or unusable; refresh is attempted at most once when required.

## Invariants

- `ctidTraderAccountId`, `traderLogin`, account numbers, visible logins, and
  equivalent identifiers exist only inside the live proof process and are
  never printed, persisted, hashed, encoded, labelled, or placed in command
  arguments, environment variables, reports, screenshots, or artifacts.
- Client secret, authorization code, access token, refresh token, callback
  query, authorization URL, and raw provider responses never reach output or
  persistence outside the approved Keychain token envelope.
- Only `id.ctrader.com`, `openapi.ctrader.com`, and
  `demo.ctraderapi.com:5035` are reachable by the proof; no live hostname or
  fallback is representable.
- The proof transport can send only application auth, account-list, account
  auth, and heartbeat messages. Account auth is impossible before Wade's
  checkpoint confirmation.
- OAuth scope and token storage are fixed to `trading`; the token's broader
  capability does not widen the immutable protocol payload allowlist.
- Every live account is excluded; absent `isLive` or broker title is ineligible;
  zero or multiple exact demo matches fail closed.
- The checkpoint process remains alive. Process death, timeout, disconnect,
  cancellation, or ambiguity clears volatile selection state and requires a
  complete restart.
- `BACKTEST`, `PAPER`, `LIVE`, risk limits, order routing, and Gates 1–5 remain
  behaviorally unchanged.

## Files Expected to Change

- `CMakeLists.txt`, `PLANS.md`.
- `vendor/ctrader/openapi-proto/` four pinned `.proto` files, `LICENSE`, and
  `PROVENANCE.md`.
- `include/CTraderGate6Proof.hpp`, `src/CTraderGate6Proof.cpp`.
- `src/CTraderGate6Runtime.mm`, `src/ctrader_gate6_main.cpp`.
- `tests/ctrader_gate6_tests.cpp`.
- Only affected Gate 5, security, testing, roadmap, project-state,
  architecture, and ADR documentation.

## Implementation Steps

1. Reconcile merged authority, worktrees, official sources, local toolchain,
   and ignored/unrelated changes.
2. Record this plan and vendor the pinned official schemas with provenance.
3. Install/pin Protobuf 35.1 and configure build-tree-only generated bindings.
4. Implement the offline account-proof state machine, immutable demo endpoint,
   strict request allowlist, sensitive-memory clearing, and fixed diagnostics.
5. Implement macOS Keychain, strict in-process token HTTPS, fixed loopback
   callback/browser handoff, and dedicated strict TLS/TCP Protobuf transport.
6. Add synthetic tests for every Gate 6 failure boundary and run the complete
   offline suite and sensitive-data scan.
7. Check secret prerequisites by presence only, then run Gate 6A and pause with
   the proof process alive at the sanitized Wade checkpoint.
8. On Wade's exact confirmation, run Gate 6B in the same process using a fresh
   connection/list response, clear all volatile state, and stop before Gate 7.
9. Re-run validation/scans, stage and commit only scoped changes, update the
   canonical sanitized transfer report, push the authorized branch, and keep
   PR #25 unmerged for Wade's acceptance.

## Verification

```sh
cmake -S . -B build/gate6 -DTRADEBOT_ENABLE_CTRADER_GATE6=ON
cmake --build build/gate6
ctest --test-dir build/gate6 -R '^ctrader_gate6_tests$' --output-on-failure
ctest --test-dir build/gate6 --output-on-failure
git diff --check
```

Tests must prove no-callback timeout/cancellation, correlation mismatch/replay,
fixed demo endpoint and scope, request allowlist, strict response correlation,
live/absent-metadata exclusion, exact single match, zero/multiple rejection,
checkpoint gating, fresh Gate 6B discovery, identifier clearing, redacted
diagnostics, token-envelope validation, and inability to emit symbol, market,
position, order, modification, cancellation, or live-endpoint traffic.

Real evidence may contain only timestamps, boolean outcomes, exact approved
`isLive`/`brokerTitleShort`, fixed non-sensitive categories, test results, and
commit identities. Final scans cover tracked and untracked in-scope artifacts.

## Risks

- Provider rejection or omission of undocumented OAuth `state` blocks the
  complete Gate 6 sequence before token exchange.
- A callback, TLS, Protobuf, JSON, Keychain, or error path could leak secrets
  unless every diagnostic is fixed and raw data is cleared before return.
- Generated schemas expose many provider message types; the proof transport
  must enforce a strict numeric payload allowlist before serialization/write.
- Process termination at the checkpoint destroys the only permitted account
  mapping and requires restarting Gate 6A.
- A live or ambiguous account list must not be made usable by title similarity,
  order, login, or any persisted/derived identifier.

## Rollback

With operator approval, revert only the Gate 6 branch commit and delete no
user-owned worktree or evidence. Remove the scope-qualified token envelope only
through a separately authorized Keychain action if provider state must be
revoked. No account or market state is modified by this read-only proof.

## Progress Log

- 2026-08-09: PR #24 merged at `42affb3`; Wade explicitly accepted Gate 5.1.
- 2026-08-09: Created isolated branch `codex/gate6-read-only-proof` from merged
  `origin/main`; preserved all unrelated worktrees and ignored artifacts.
- 2026-08-09: Reconfirmed the original official view-only scope, token endpoint,
  authentication sequence, demo Protobuf endpoint/port, and pinned schema
  provenance. Found Protobuf compiler/runtime absent.
- 2026-08-09: Installed Homebrew Protobuf 35.1 and verified its CMake-reported
  C++ runtime version 7.35.1 plus every vendored schema SHA-256.
- 2026-08-09: Added the opt-in Gate 6 proof target, strict demo transport,
  volatile account-proof state machine, macOS Keychain boundary, and synthetic
  tests. The Gate 5.1 and Gate 6 targeted tests passed offline.
- 2026-08-09: Stopped before OAuth or provider traffic after the cTrader
  application Edit page unexpectedly rendered credential fields into the
  browser inspection transcript. No credential value is repeated in repository
  material. Secret rotation and a clean execution context are required before
  resuming the real Gate 6 sequence.
- 2026-08-09: Confirmed the fixed loopback redirect URI was not registered in
  the application. Registration and verification remain operator prerequisites.
- 2026-08-09: Hardened the token exchange, TLS, correlation, one-shot account
  selection, volatile identifier clearing, and fail-closed terminal paths.
- 2026-08-09: Completed the default and opt-in offline suites, targeted
  sanitizer checks, repository documentation synchronization, and path-only
  sensitive-data scans without provider traffic.
- 2026-08-10: Pushed implementation commit `ac9028d` and opened draft PR #25.
  Wade confirmed rotated credentials, client-ID configuration, and exact
  redirect registration.
- 2026-08-10: The first callback attempt failed closed before token exchange.
  A fresh attempt then proved exact OAuth correlation; token setup failed
  locally before network transfer because an empty libcurl redirect-protocol
  list is invalid. A credential-free direct endpoint probe returned HTTP 200.
- 2026-08-10: Wade explicitly authorized `trading` scope for the intended demo
  account while withholding every trading message and Gate 7. The token
  transport correction now uses a valid HTTPS-only redirect protocol setting
  while keeping redirects disabled; targeted offline tests pass.
- 2026-08-10: A fresh Gate 6A run succeeded. Wade confirmed the only approved
  checkpoint facts, exact `isLive=false` and exact `brokerTitleShort=FIBO`.
  Gate 6B then reproduced that predicate from a fresh authenticated account-list
  response and completed account authentication without exposing or persisting
  an account identifier. The process stopped before Gate 7.
- 2026-08-10: Wade accepted the Gate 6 execution result. Draft PR #25 remains
  the implementation-closure boundary; it is not merged or ready for merge
  until its corrective review findings are independently re-reviewed.

## Deviations

The earlier credential disclosure and missing redirect were remediated by Wade.
The accepted Gate 5 `accounts`-scope baseline is superseded for Gate 6 only by
Wade's 2026-08-10 `trading`-scope directive. A local libcurl option defect was
found after correlation and corrected before any credential-bearing token
request left the process.

## Completion Evidence

Gate 6A, the mandatory Wade checkpoint, and Gate 6B completed successfully.
Wade confirmed only exact `isLive=false` and exact `brokerTitleShort=FIBO`, and
accepted the Gate 6 execution result. No account identifier entered output or
persistence, and the process stopped before Gate 7. Implementation closure is
not yet complete: it remains tied to independent re-review and merge of draft
PR #25 after its corrective findings are addressed.

## Final Outcome

Gate 6 execution is complete and accepted by Wade under the fixed `trading`
scope and non-trading message allowlist. Draft PR #25 remains the sole
implementation-closure boundary and is not accepted or merged by this plan.
Gates 7–9 remain unauthorized; no further OAuth or provider execution is part
of this closure work.

# Plan: Execute cTrader Open API Gate 7 Fresh XAUUSD Market-Data Proof

- Plan ID: `PLAN-20260810-ctrader-open-api-gate7-xauusd-market-data-proof`
- Status: Implementation and offline validation complete; the one newly authorized retry stopped at the evidenced OAuth failure
- Owner: Wade
- Implementer: Codex
- Review authority: Wade
- Related roadmap phase: Workstream II, Phase 24, Gate 7 only
- Related issue or decision: Wade Gate 7 Fresh XAUUSD Market-Data Proof Directive; ADR 0004; accepted Gate 2 numeric contract
- Created: 2026-08-10
- Updated: 2026-08-10

## Objective

Implement and execute one isolated, default-disabled, macOS-only cTrader
Open API proof that discovers a fresh FIBO demo account, resolves exactly one
response-derived canonical XAUUSD symbol, validates complete runtime metadata,
subscribes to only that symbol's spot stream, proves one current fresh BBO,
converts it deterministically into broker-neutral values, clears volatile
provider state, and stops before Gate 8.

Success requires one controlled process to use only the fixed demo endpoint,
fresh response-derived account and symbol identifiers, the Gate 7 outbound and
inbound allowlists, the accepted Gate 2 integer contract, and the bounded
timestamp-unit proof. No cached identifier, live account, order-capable path,
runtime mode, or reconnect may be used.

## Context

The authoritative base is merged `origin/main` commit
`7fc244e2f3cc5a1e406d416898807562fcf58c6d`, which contains the accepted Gate 6
implementation from PR #25. Existing current-state wording that still calls
PR #25 draft/unmerged or Gate 7 unauthorized is stale relative to Wade's
explicit 2026-08-10 directive and will be corrected without erasing history.
Gate 6 remains an immutable account-proof boundary; its executable and
allowlist must remain reproducible and unchanged in behavior.

The provider spot timestamp is documented only as Unix time without a unit.
Gate 7 must evaluate seconds, milliseconds, microseconds, and nanoseconds
against the local receipt time and fail closed unless exactly one interpretation
is within the allowed past/future bounds.

## Scope

- Create or safely resume worktree `/Users/vaheedgorgeen/TradeBot-gate7` on branch `codex/gate7-xauusd-market-data-proof` from exact `origin/main`.
- Record this active plan and synchronize the controlling Gate 7 authority, architecture, configuration, testing, risk, residual-gap, roadmap, project-state, and relevant ADR/index wording.
- Add a separate `TRADEBOT_ENABLE_CTRADER_GATE7` default-off macOS-only proof target reusing Gate 6 transport, Keychain, OAuth, framing, correlation, and authentication boundaries where safe.
- Add narrow provider-boundary state, message allowlists, current-generation/correlation checks, XAUUSD canonical resolution, complete symbol metadata validation, deterministic Decimal64 conversion, spot subscription, timestamp proof, bounded timeout, sanitized evidence, and volatile-state clearing.
- Add deterministic synthetic tests for every specified acceptance and fail-closed condition, including an explicit proof that no order/position/depth/trendbar/historical/reconnect message can be emitted.
- Run the required offline validation, sanitizer coverage, sensitive-data scan, and presence-only Keychain/configuration preflight before one provider session.
- Execute exactly one bounded Gate 7 provider session, record only sanitized evidence, and stop immediately after the first usable fresh BBO or the first terminal failure.
- Commit only Gate 7 files, push this branch, open a draft PR against `main`, and create the canonical sanitized transfer report.

## Out of Scope

- Any Gate 8 or Gate 9 implementation, reconnect/recovery experiment, depth,
  trendbar, historical tick/data, positions, balances, margins, deals, order
  state, order placement/modification/cancellation/closure, or live account.
- Any live hostname, live mode, broker adapter, `BrokerGateway`,
  `LiveDataAdapter`, `ExecutionEngine`, `RiskEngine`, `SystemConfig`,
  `BACKTEST`, `PAPER`, or `LIVE` attachment.
- Any modification of Gate 6's immutable allowlist, accepted behavior, or
  historical evidence beyond accurate current-state references.
- Any hardcoded account or symbol identifier, broker suffix/alias, cached
  provider identifier, token reuse from Playground, credential inspection,
  persistence, logging, hashing, or reporting.
- Worktree deletion, cleanup, relocation, reset, branch deletion, history
  rewrite, PR merge, approval, ready-for-merge state, or auto-merge.

## Preconditions

- `origin/main` has been fetched and verified at exact commit
  `7fc244e2f3cc5a1e406d416898807562fcf58c6d`.
- Permanent workspace `/Users/vaheedgorgeen/TradeBot` is clean of uncommitted
  operator work; existing unrelated worktrees remain untouched.
- Wade's directive authorizes authority reconciliation, implementation,
  offline/provider validation, documentation, commit, push, draft PR, and
  sanitized report within this exact Gate 7 scope.
- Fixed endpoint is `demo.ctraderapi.com:5035`; fixed callback and accepted
  local Keychain/configuration boundaries remain unchanged.
- Provider credentials/configuration are available only through the accepted
  local presence-only preflight; no secret value may enter output or files.

## Assumptions

- Homebrew Protobuf, curl, OpenSSL, AppKit, Security, and the pinned vendored
  proto2 schemas used by Gate 6 remain locally available; if the exact reviewed
  dependency contract is unavailable, offline execution stops before provider
  traffic.
- Gate 6 helper boundaries are reused only where they preserve Gate 7's stricter
  scope, and any shared source changes preserve Gate 6 behavior and tests.
- The provider returns account, symbol, and spot metadata with proto2 presence
  bits available; absent safety-relevant fields are not interpreted as defaults.
- The first usable fresh BBO is sufficient evidence; no second quote, reconnect,
  or recovery attempt is permitted.

## Invariants

- Only `demo.ctraderapi.com:5035` is representable and reachable.
- Every provider account and symbol ID is fresh, response-derived,
  process-local, positive, checked, and securely cleared on every terminal
  path; no identifier is persisted, logged, hashed, encoded, or reported.
- Account selection is exactly present `isLive=false` plus present exact
  `brokerTitleShort=FIBO`; zero or multiple matches fail closed.
- Symbol selection canonicalizes only ASCII case and removes at most one `/`;
  only `XAUUSD` and `XAU/USD` forms are eligible, never `GOLD`, suffixes, or
  substring matches.
- Complete `InstrumentSpec` construction occurs only after every required
  light/full metadata, scale, volume, lot, contradiction, and checked-arithmetic
  rule in the accepted Gate 2 contract passes.
- Only heartbeat, application auth, token-account-list, account auth,
  non-archived symbol list, full symbol by response-derived ID, one-symbol spot
  subscription/unsubscription, and required fixed heartbeat/error responses are
  admitted. All trading/order/position/depth/trendbar/historical payloads are
  rejected before serialization or socket write.
- Spot events are accepted only from the current connection generation and
  after matching subscription response, with matching account/symbol IDs,
  present positive bounded bid/ask, exact and normalized non-crossing, checked
  spread, and exactly one proven timestamp unit.
- Provider scale-5 prices are converted only by checked integer arithmetic using
  `NearestTiesAwayFromZero` for scale reduction and checked multiplication for
  expansion; no `double`, locale parsing, saturation, or unchecked cast is used.
- Raw prices, exact pre-rounded values, tokens, codes, callback queries, and all
  sensitive provider material remain ephemeral. Final evidence is sanitized.
- The target is opt-in and detached from all normal runtime modes and order/risk
  paths; default configure/build/test remain offline.

## Expected Files Or Subsystems

- `CMakeLists.txt` and new Gate 7 headers/source/runtime/main/test files under
  the existing isolated cTrader proof surface.
- `PLANS.md` and only the controlling current-state, roadmap, architecture,
  configuration, testing, risk/security, Gate 7, residual-gap, and relevant ADR
  documentation required by the observed implementation and outcome.
- Canonical sanitized report under
  `~/SR-Workspace/SR-Res-OUTBOX/required-final-output-logs/` (outside the Git
  worktree and never containing credentials or account identifiers).

## Implementation Steps

1. Reconcile authority, phase/ADR status, Git/worktree safety, Gate 6 source,
   pinned schemas, and local dependency/configuration boundaries.
2. Add the opt-in Gate 7 provider-boundary implementation with a strict
   pre-serialization outbound allowlist and response dispatcher.
3. Add fresh account selection, symbol resolution, metadata/volume/scale
   validation, timestamp-unit classifier, BBO conversion, timeout, and secure
   terminal cleanup.
4. Add synthetic tests for all requested happy paths, rejection paths,
   allocation/cancellation/malformed-frame/provider-error paths, and no-order
   capability proof.
5. Run default build/CTest, Gate 5.1, Gate 6 opt-in, Gate 7 suite, sanitizer
   coverage, diff/documentation/security audits, and presence-only preflight.
6. Run exactly one controlled provider session; do not reconnect or retry.
7. Synchronize current documentation and plan outcome, rerun applicable checks,
   run PR-readiness/hygiene/handoff review, commit, push, open draft PR, and
   write/hash the sanitized report.

## Verification

Required evidence includes:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
cmake -S . -B build/gate6 -DTRADEBOT_ENABLE_CTRADER_GATE6=ON
cmake --build build/gate6
ctest --test-dir build/gate6 --output-on-failure
cmake -S . -B build/gate7 -DTRADEBOT_ENABLE_CTRADER_GATE7=ON
cmake --build build/gate7
ctest --test-dir build/gate7 --output-on-failure
git diff --check
```

Also run the Gate 5.1 targeted suite, relevant Gate 6/Gate 7 ASan/UBSan
coverage, documentation audits, tracked/in-scope-untracked sensitive-data
scans, and presence-only Keychain/configuration preflight. Provider execution
is forbidden until all required offline checks pass.

## Risks And Decision Points

- Missing or ambiguous spot timestamp units are a terminal `timestamp_unit_unproven`
  outcome, not a reason to guess or broaden the freshness window.
- Contradictory or incomplete metadata, cross-market values, wrong generation,
  payload/correlation mismatch, malformed frames, allocation failure, provider
  error, timeout, cancellation, or missing quote side fail closed.
- Any secret/account-identifier exposure, unexpected outbound payload,
  live-endpoint possibility, Gate 6 regression, or source-scope expansion is a
  safety blocker requiring stop and report.
- Provider failure does not authorize reconnect, repeated session, fallback,
  live use, or a weaker validator. The implementation may still be committed
  and proposed if its offline evidence is complete and the failure is sanitized.

## Rollback

With operator approval, revert only the Gate 7 branch commit(s); do not delete
or alter any existing worktree, credential, Keychain item, provider account,
or external state. No provider session in this gate may mutate external trading
state. A failed proof must remain recorded as a sanitized outcome and cannot be
made successful by reinterpretation.

## Progress Log

- 2026-08-10: Wade authorized complete Gate 7 execution in one goal.
- 2026-08-10: Verified clean permanent `main` at authoritative merge commit
  `7fc244e`; existing worktrees are preserved; created isolated Gate 7
  worktree/branch from exact `origin/main`.
- 2026-08-10: Authority audit found stale Gate 7/PR #25 wording that will be
  corrected as current-state documentation without erasing historical records.
- 2026-08-10: Default-disabled Gate 7 implementation and deterministic offline
  coverage completed. Default, Gate 5.1, Gate 6, Gate 7, and sanitizer
  validation passed; the only observed build warnings are existing warnings in
  `AsyncNetworkClient.cpp` and `RiskEngine.cpp`.
- 2026-08-10: Presence-only Keychain/configuration preflight passed. Exactly
  one bounded Gate 7 provider process was started. It remained at the
  in-process macOS Keychain access boundary without fixed-endpoint network
  output; the same process was stopped safely after the bounded wait. No
  provider response or market-data evidence was obtained, and no retry or
  reconnect was attempted.
- 2026-08-10: Wade authorized one new bounded retry from this checkpoint. The
  retry passed the in-process Keychain read, entered fresh OAuth authorization,
  emitted only `gate7_oauth_failed`, and exited with code 1 before account
  discovery or the fixed demo data endpoint. No further retry was attempted.

## Deviations

The earlier attempt stopped at unresolved Security.framework Keychain access.
Wade then authorized exactly one new retry. That retry passed Keychain access
but failed at fresh OAuth authorization with `gate7_oauth_failed` before
account discovery or the fixed demo data endpoint. It exited with code 1; no
provider data proof, reconnect, or repeated retry occurred. This is a
sanitized execution failure, not an offline implementation failure.

## Completion Evidence

Implementation and controlling documentation changed only in the 21 files
listed by the original closeout report, followed by the bounded-retry status
updates. The implementation commit is `53cc26a`
(`feat: add Gate 7 XAUUSD market-data proof`) on
`codex/gate7-xauusd-market-data-proof`, based on authoritative
`origin/main` `7fc244e2f3cc5a1e406d416898807562fcf58c6d`. Draft PR:
`https://github.com/wadewolfie999/TradeBot/pull/26`.

Default CTest passed 7/7; Gate 6 opt-in CTest passed 8/8; Gate 7 opt-in CTest
passed 8/8; Gate 5.1/Gate 6 sanitizer coverage passed 2/2; Gate 7 ASan/UBSan
coverage passed 1/1; presence-only preflight exited 0; and `git diff --check`
passed. The single provider process stopped before endpoint traffic at
`gate7_oauth_failed`, exit code 1. No canonical symbol, instrument metadata,
quote, or timestamp evidence exists.
Sanitized report:
`/Users/vaheedgorgeen/SR-Workspace/SR-Res-OUTBOX/required-final-output-logs/20260809T231550Z-tradebot-gate7-xauusd-market-data-proof.md`.
Report SHA-256 excluding its hash line:
`54d40b6a54b6f832d2c302531e5b77bca53542c6d11c308653fc1e09d963f9bf`.
Gates 8–9 and live trading remain unauthorized regardless of Gate 7 outcome.

## Final Outcome

Gate 7 is incomplete — stopped safely at the evidenced blocker:
`gate7_oauth_failed`. Implementation, offline validation, the one newly
authorized retry, documentation synchronization, commit, push, draft PR, and
the canonical sanitized report are complete. The exact next action is Wade
review of draft PR #26/report; do not begin Gate 8 or retry provider traffic
again within this task.

# Plan: Harden Gate 7 OAuth Diagnostics Offline

- Plan ID: `PLAN-20260811-ctrader-gate7-oauth-diagnostics`
- Status: Complete — offline implementation, review corrections, and verification complete
- Owner: Wade
- Implementer: Codex
- Review authority: Wade
- Related roadmap phase: Workstream II, Phase 24, Gate 7 review closeout only
- Related issue or decision: PR #26 review; ADR 0004; Gate 7 incomplete OAuth outcome
- Created: 2026-08-11
- Updated: 2026-08-12

## Objective

Replace Gate 7's generic OAuth failure output with fixed, non-sensitive failure
categories and close the reviewed local callback peer-address and receive-timeout
gaps without initiating provider traffic.

## Scope

- Add a Gate 7-only diagnostic enum and fixed diagnostic mapping.
- Preserve the fixed authorization/token/demo endpoints, `trading` scope,
  allowlists, one-shot correlation, secret clearing, and Gate 6 boundary.
- Report listener, URL, browser, timeout, callback, denial, state-correlation,
  code-extraction, cancellation, and resource-exhaustion categories.
- Use the actual accepted loopback peer address and bounded callback reads.
- Add deterministic offline coverage for all categories and correlation mappings.
- Synchronize Gate 7 security, testing, and current-state documentation.

## Out of Scope

- OAuth, browser, Keychain, token, cTrader, account, symbol, quote, timestamp,
  reconnect, order, Gate 8, Gate 9, or live execution.
- Removing or weakening `state` correlation.
- Gate 6 source, behavior, executable, allowlist, or accepted evidence.
- Commits, pushes, PR state changes, credentials, or external provider traffic.

## Invariants

- Diagnostics contain fixed literals only and never provider text, query data,
  state, code, token, account identifier, peer address, or secret material.
- Every OAuth terminal path clears correlation state and callback material.
- The fixed demo endpoint and existing Gate 7 payload allowlist are unchanged.
- Offline tests perform no Keychain, browser, socket, provider, or account action.

## Files Expected To Change

- `CMakeLists.txt`
- `include/CTraderGate7OAuthDiagnostics.hpp`
- `src/CTraderGate7OAuthDiagnostics.cpp`
- `src/CTraderGate7Runtime.mm`
- `tests/ctrader_gate7_tests.cpp`
- `PLANS.md`
- `docs/CTRADER_OPEN_API_GATE7.md`
- `docs/PROJECT_STATE.md`
- `docs/SECURITY.md`
- `docs/TESTING.md`

## Verification

```sh
git diff --check
cmake -S . -B build/gate7 -DTRADEBOT_ENABLE_CTRADER_GATE7=ON
cmake --build build/gate7 --target ctrader_gate7_tests ctrader_gate7_proof --parallel 4
ctest --test-dir build/gate7 -R '^ctrader_gate7_tests$' --output-on-failure
```

Run the full offline default/Gate 7 suites and sanitizer coverage after the
targeted checks. Do not run the Gate 7 provider executable.

## Risks

- A diagnostic mapping error could hide the true local failure or expose data;
  fixed-category tests and redaction assertions are required.
- Callback I/O changes must retain one-shot terminal behavior and must not alter
  the accepted correlation contract.
- Provider support for `state` remains unverified; this patch must not infer or
  authorize a weaker correlation policy.

## Rollback

With operator approval, revert only this offline hardening diff. Do not alter
credentials, Keychain items, provider state, Gate 6, PR state, or history.

## Progress Log

- 2026-08-11: Wade explicitly authorized this offline-only diagnostic hardening
  patch and prohibited provider retry or traffic.
- 2026-08-11: Added fixed OAuth diagnostics, actual peer capture, bounded
  callback receive timeout, deterministic category/mapping tests, and synchronized
  Gate 7 documentation.
- 2026-08-11: Opt-in Gate 7 target built and targeted test passed; full offline
  and sanitizer verification completed after the targeted check.
- 2026-08-12: Review identified that the callback receive loop needed an
  absolute deadline in addition to its per-receive timeout and that callback
  buffer allocation failure could escape the fixed diagnostic path. Wade
  authorized the bounded offline corrections and repeat verification.
- 2026-08-12: Added the absolute callback read deadline, allocation-safe append
  classification, deterministic regression coverage, and synchronized
  documentation. Targeted Gate 7, full default, full Gate 6, full Gate 7, and
  Gate 7 ASan/UBSan verification all passed without warnings.

## Final Outcome

Offline Gate 7 OAuth diagnostic hardening and its authorized review corrections
are complete. Gate 7 provider execution remains incomplete at the historical
`gate7_oauth_failed` outcome. No provider retry, account discovery, market-data
proof, Gate 8, Gate 9, or live action is authorized by this plan.

# Plan: Complete Gate 7 Residual Diagnostics and Proof

- Plan ID: `PLAN-20260813-ctrader-gate7-residual-diagnostics-and-proof`
- Status: Blocked at provider-execution checkpoint — bounded offline
  implementation, final review, and verification complete; Wade authorized one
  local commit and exact-commit rebuild, while provider execution still requires
  bounded clock-skew evidence, passing local preflight, clear immediate
  port/process state, and separate exact Wade approval
- Owner: Wade
- Implementer: Codex
- Review authority: Wade
- Related roadmap phase: Workstream II, Phase 24, Gate 7 only
- Related issue or decision: Gate 7 failure-boundary and residual-risk inquest;
  ADR 0004; accepted Gate 2 numeric contract
- Created: 2026-08-13
- Updated: 2026-08-13

## Objective

Classify the current subscription-transition failure, remove the known
single-event BBO ambiguity, instrument the remaining subscription-to-quote
corridor with fixed redacted outcomes, and prepare a reviewed evidence contract
for one later separately authorized provider process. Success for the offline
stage requires deterministic coverage and the complete verification matrix;
success for Gate 7 itself still requires one reviewed process to emit
`gate7_provider_sequence_complete` and `gate7_exit_code=0`.

## Context

The merged Gate 7 implementation and OAuth hardening are present at
`origin/main` commit `f63a03b63a81d37203376d1e2dea267ece115c89`. A later
operator-observed process advanced beyond OAuth, fixed demo TLS, application
authentication, fresh demo-account selection and authentication, canonical
XAUUSD resolution, and full metadata validation, then emitted
`gate7_subscription_failed`. The current boolean transport collapses send,
timeout, disconnect, provider-error, unexpected-payload, correlation, parsing,
and proof-rejection outcomes. The spot path also terminally rejects the first
provider-documented partial event instead of waiting within the existing
absolute deadline for the first single complete BBO event.

## Scope

- Reconcile current-state documentation with the callback-timeout history and
  latest subscription-boundary evidence without erasing earlier attempts.
- Add Gate-7-only typed send/receive/provider-error outcomes for the remaining
  subscription and spot path.
- Map vetted provider error codes into closed fixed non-sensitive categories;
  never print provider text, code, retry value, maintenance timestamp,
  correlation, identifier, or payload.
- Distinguish subscription state, send, timeout, transport, provider,
  correlation, malformed response, account mismatch, proof, and resource
  failures.
- Accept only the first single spot event containing both positive sides and a
  valid timestamp; never merge quote sides across events.
- Continue past well-formed incomplete spot events only within the existing
  absolute spot deadline, then emit a fixed incomplete-BBO timeout category.
- Distinguish spot transport, payload, identity, numeric, crossing, timestamp,
  arithmetic, incomplete-event, and resource outcomes.
- Send only the already-allowed heartbeat on a bounded monotonic cadence while
  waiting, without reconnecting or extending an absolute deadline.
- Add deterministic tests for every new outcome, mapping, state transition,
  redaction invariant, heartbeat deadline, and allocation path.
- Prepare a persistent sanitized evidence template under the local TradeBot
  handoff/evidence directory, never under `/tmp`.

## Out of Scope

- Gate 8, Gate 9, reconnect, recovery, retry within a process, or endpoint
  fallback.
- Any live hostname, live account, order, position, balance, margin, deal,
  depth, trendbar, historical-data, or production runtime path.
- `BrokerGateway`, `LiveDataAdapter`, `ExecutionEngine`, `RiskEngine`,
  `SystemConfig`, financial limits, or runtime-mode coupling.
- Gate 6 source, executable, allowlist, behavior, or accepted evidence.
- Credential inspection, export, logging, copying, modification, or new
  storage.
- The independent OAuth completion-page defect.
- Provider execution before a separate post-review Wade approval.
- Commit, push, PR publication, merge, or release in this task unless Wade
  issues an exact later instruction.

## Preconditions

- Work starts from clean tracked commit
  `f63a03b63a81d37203376d1e2dea267ece115c89`, equal to `origin/main`, on the
  scoped branch `codex/gate7-residual-diagnostics-and-proof`.
- Wade's 2026-08-13 directions authorize the bounded offline plan,
  implementation, final review, one local commit, and exact-commit rebuild
  before the separate provider-run checkpoint.
- Gate 7 remains default-disabled, macOS-only, demo-only, and detached from all
  production modes and order/risk paths.
- The provider execution gate remains NO-GO until source, tests, sanitizers,
  diff, evidence template, port/process preconditions, and non-sensitive local
  clock-health evidence are reviewed.

## Assumptions

- The operator-supplied latest markers and control-flow boundary are current
  execution evidence; no credential or provider payload is required to record
  that boundary.
- The pinned official proto2 schema remains unchanged and locally available.
- `ProtoMessage.clientMsgId` is expected to echo request correlation on the
  subscription response; mismatch remains a terminal protocol anomaly.
- Provider spot fields are optional, so an identity-valid event missing a
  quote side or timestamp is incomplete rather than successful.

## Invariants

- Authorization, token, callback, and fixed demo endpoints remain unchanged.
- OAuth scope remains `trading`; no trading message becomes representable.
- The outbound allowlist is unchanged. The inbound allowlist may add only the
  documented account-disconnect control event needed for fail-closed
  classification.
- Fresh response-derived account and symbol identifiers stay volatile,
  positive, exact, and unreported.
- Diagnostics are fixed literals only; raw provider text and payloads are
  cleared before return.
- Exact correlation, generation, account, symbol, scale, timestamp, and
  arithmetic checks remain fail closed.
- Incomplete spot events never contribute a side or timestamp to a later
  event.
- Heartbeats do not reconnect, widen message scope, or extend an absolute
  deadline.
- Default builds/tests remain offline and Gate 7 remains default-disabled.
- Gate 6 files and behavior remain unchanged.

## Files Expected to Change

- `include/CTraderGate7Proof.hpp`, `src/CTraderGate7Proof.cpp`,
  `src/CTraderGate7Runtime.mm`, and `tests/ctrader_gate7_tests.cpp`.
- `PLANS.md`, `docs/PROJECT_STATE.md`, `docs/ROADMAP.md`,
  `docs/ARCHITECTURE.md`, `docs/RISK_POLICY.md`, `docs/SECURITY.md`,
  `docs/TESTING.md`, `docs/CTRADER_OPEN_API_GATE5.md`,
  `docs/CTRADER_OPEN_API_GATE7.md`,
  `docs/WORKSTREAM_ARCHITECTURE.md`, `docs/RESIDUAL_GAPS_BACKLOG.md`, and ADR
  0004.
- One ignored local sanitized evidence template under `handoffs/`.

`src/CTraderGate6Runtime.mm`, Gate 6 headers/tests, normal runtime components,
and financial-risk code are explicitly unchanged.

## Implementation Steps

1. Record the latest incomplete execution history and this bounded plan.
2. Define typed Gate-7-only send, receive, provider-category, residual-failure,
   timestamp, and heartbeat contracts.
3. Implement subscription classification with immediate payload clearing.
4. Implement first-single-complete-BBO handling within the existing absolute
   spot deadline without combining events.
5. Add bounded outbound heartbeats during response/spot waits.
6. Add deterministic outcome, redaction, state-machine, allocation, malformed,
   partial-event, deadline, and timestamp tests.
7. Create and inspect the persistent sanitized evidence template.
8. Run default, Gate 6, Gate 7, sanitizer, security, scope, and documentation
   verification; inspect the complete diff.
9. Stop and obtain separate Wade approval for exactly one provider process.
10. After that future run, preserve/hash the sanitized evidence, synchronize
    the observed outcome, and obtain Wade acceptance before marking Gate 7
    complete.

## Verification

```sh
git status --short
git diff --name-status
git diff --cached --name-status
git diff --name-status origin/main
git diff --check
git diff --cached --check
git diff --exit-code origin/main -- src/CTraderGate6Runtime.mm
cmake -S . -B build
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
cmake -S . -B build/gate6 -DTRADEBOT_ENABLE_CTRADER_GATE6=ON
cmake --build build/gate6 --parallel 4
ctest --test-dir build/gate6 --output-on-failure
cmake -S . -B build/gate7 -DTRADEBOT_ENABLE_CTRADER_GATE7=ON
cmake --build build/gate7 --parallel 4
ctest --test-dir build/gate7 --output-on-failure
cmake -S . -B build/gate7-sanitize \
  -DTRADEBOT_ENABLE_CTRADER_GATE7=ON \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_OBJCXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build build/gate7-sanitize --target ctrader_gate7_tests --parallel 4
ctest --test-dir build/gate7-sanitize -R '^ctrader_gate7_tests$' --output-on-failure
```

Also verify every runtime diagnostic against the closed fixed set, confirm the
synthetic tests perform no browser/Keychain/DNS/TLS/provider operation, run
Gate 7 `--preflight` only, scan tracked in-scope material without opening
ignored credential files, record pre-existing warnings separately, and verify
the evidence template contains no secret/identifier value or value-bearing
field intended to capture one.

## Risks

- Typed diagnostics narrow a category but cannot independently prove a
  provider-side root cause.
- Incorrect error-code mapping could misclassify an external condition; unknown
  codes therefore remain a fixed generic provider rejection.
- Heartbeat scheduling inside a partial frame read must preserve frame state
  and the original absolute deadline.
- Partial-event continuation could become unsafe if sides or timestamps are
  cached; the proof retains none of them between events.
- Local wall-clock correctness remains required for freshness proof and must be
  established before a provider run.
- A provider run can still stop at OAuth, transport, subscription, partial BBO,
  or timestamp failure and never authorizes a retry.

## Rollback

With Wade approval, revert only this residual Gate 7 patch. Do not reset
history, delete unrelated work, alter Gate 6, remove credentials, or modify
provider/account state. The independent OAuth completion-page repair remains a
separate reversible change.

## Progress Log

- 2026-08-13: The residual-diagnostics plan was first defined as `Proposed` by
  the operator-supplied failure-boundary inquest.
- 2026-08-13: Wade directed Codex to execute the attachment, authorizing the
  bounded offline plan and implementation while the attachment's separate
  post-review provider-execution approval remains mandatory.
- 2026-08-13: Verified clean tracked branch
  `codex/gate7-merged-verification` at `f63a03b`, equal to `origin/main`, and
  created `codex/gate7-residual-diagnostics-and-proof` without disturbing
  ignored build, data, credential-like, or handoff artifacts.
- 2026-08-13: Implemented Gate-7-only typed subscription/spot send, receive,
  provider, disconnect, timeout, protocol, identity, proof, and resource
  outcomes; fixed-literal redacted diagnostics; bounded heartbeat cadence; and
  first-single-complete-BBO handling without cross-event aggregation.
- 2026-08-13: Added deterministic mapping, diagnostic-set, timestamp,
  heartbeat, partial-event, identity, and allocation tests; synchronized the
  authoritative Gate 7, roadmap, architecture, risk, security, testing,
  workstream, backlog, project-state, and ADR consequence records.
- 2026-08-13: Created and reviewed the ignored persistent sanitized evidence
  template at
  `handoffs/PLAN-20260813-gate7-provider-evidence-template.md`; it contains no
  credential, account-ID, symbol-ID, raw payload, raw provider value, or raw
  quote value and no value-bearing field intended to capture one.
- 2026-08-13: Completed default, Gate 6, Gate 7, and Gate 7 sanitizer builds
  and tests with zero failures. The final Gate 7 build and full eight-test
  suite, plus the targeted sanitizer test, passed after the last source edit.
- 2026-08-13: Ran only the local `--preflight` proof mode. It emitted
  `gate7_client_id_missing` and started no provider traffic. A privileged
  network-time query was unavailable; the running local time daemon alone
  does not prove bounded wall-clock skew. Provider execution therefore remains
  NO-GO.
- 2026-08-13: Wade accepted the offline implementation, verification, risks,
  and NO-GO boundary, then authorized one final review, concrete in-scope
  corrections, one local commit, and an exact-commit rebuild. Provider traffic,
  credential-value access, publication, Gates 8–9, and live/order/risk changes
  remain prohibited.
- 2026-08-13: Final review added terminal clearing to caller-side residual
  subscription/spot evidence copies and corrected commit-state wording that
  would otherwise become stale immediately after the authorized local commit.
  The targeted Gate 7 build/test and sanitizer test passed after this change.
- 2026-08-13: Concurrent CTest runs in three build trees caused the shared
  offline Phase 17 local-CA test to fail in the default and Gate 7 trees while
  the identical Gate 6 test passed. Sequential default and Gate 7 reruns passed
  7/7 and 8/8 respectively, confirming cross-build local-test resource
  contention rather than a Gate 7 regression.

## Deviations

- The proposed provider process was not run. This is the required safe
  deviation because local preflight failed, bounded clock skew was not proved,
  immediate port/process state must be rechecked at launch time, and no separate
  exact provider-process approval was issued. The authorized local
  commit/rebuild checkpoint does not authorize provider traffic.
- macOS `systemsetup` clock-source queries required administrator access and
  were not escalated. The observed running `timed` daemon was recorded only as
  partial local state, not accepted as clock-skew evidence.

## Completion Evidence

- Base commit:
  `f63a03b63a81d37203376d1e2dea267ece115c89`, equal to `origin/main`; scoped
  branch `codex/gate7-residual-diagnostics-and-proof`. The resulting authorized
  local commit SHA and post-commit binary hash are recorded in the ignored
  handoff rather than self-referenced in this tracked plan.
- Default build: configure/build succeeded; CTest passed 7/7.
- Gate 6 opt-in build: configure/build succeeded; CTest passed 8/8.
- Gate 7 opt-in build: configure/build succeeded; CTest passed 8/8.
- Gate 7 AddressSanitizer/UndefinedBehaviorSanitizer target: build succeeded;
  targeted `ctrader_gate7_tests` passed 1/1.
- Final-review Gate 7 targeted test passed 1/1, sanitizer targeted test passed
  1/1, and the sequential full Gate 7 suite passed 8/8. The sequential default
  suite passed 7/7 after the concurrent Phase 17 contention described above.
- Pre-commit candidate binary SHA-256, superseded by the exact-commit rebuild:
  `b3a5c831a6719c9a940c2554bf3dc76466a5bbc4bd74c20f76737986d2d3c38f`.
- Evidence-template SHA-256:
  `e74208ac5dd609f190f0c08f1a509f54314ad863b5b9e172c5119857d443b50c`.
- CMake 4.4.0, Apple clang 21.0.0 for arm64-apple-darwin25.5.0,
  libprotoc 35.1, pinned proto provenance commit
  `3fd8bddfbe0cfc2ecfda079623dc4e498af11e66`, and all four recorded proto
  source hashes matched.
- Gate 6 sources/tests, vendor proto inputs, default CMake behavior,
  `SystemConfig`, `BrokerGateway`, `LiveDataAdapter`, `ExecutionEngine`, and
  `RiskEngine` have no diff from `origin/main`.
- `git diff --check` passed; the complete final hygiene and documentation audit
  is recorded in the plan handoff.

## Final Outcome

The bounded offline residual-diagnostics and proof patch is complete and
verified. Gate 7 itself remains incomplete at the last evidenced subscription
transition because no reviewed post-patch provider process was run. Provider
execution is blocked by failed local preflight, unproved bounded clock skew,
the required immediate port/process recheck, and the still-required separate
Wade approval. The authorized local commit and exact-commit rebuild establish
reviewable source/binary identity only; they do not grant provider authority.
Gates 8–9, orders, live accounts, production runtime coupling, and live trading
remain blocked regardless of this offline result.

# Plan: Synchronize Codex Governance And Skill Learning

- Plan ID: `PLAN-20260813-codex-governance-learning-sync`
- Status: Complete — documentation-only update validated; Wade authorized
  staging, commit, push, PR creation, and merge on 2026-08-13
- Owner: Wade
- Implementer: Codex
- Review authority: Wade
- Related roadmap phase: Repository governance; no phase transition
- Related issue or decision: Wade request to update `AGENTS.md`, all TradeBot
  skills, and Codex-related modules from recurring recent context
- Created: 2026-08-13
- Updated: 2026-08-13

## Objective

Turn recurring review and Gate 7 lessons into durable repository-side agent
procedure without changing TradeBot runtime behavior or copying volatile phase
state into every skill.

## Context

Recent work repeatedly exposed non-transitive approvals, candidate-versus-
exact-commit evidence drift, shared local test-resource contention, retries
without new evidence, ignored handoff provenance, and caller-side sensitive
copies surviving callee clearing. Several skills also embedded obsolete phase
and gate snapshots.

## Scope

- Update `AGENTS.md` and the planning, actor, workflow, handoff, review,
  testing, recovery, security, risk, live-readiness, contributor, index, and
  current-state modules.
- Add one reusable Codex execution/evidence contract.
- Update every tracked TradeBot `SKILL.md` and the existing skill UI metadata.
- Validate structure, authority consistency, stale-state removal, and diff
  hygiene.

## Out of Scope

- C++ source, headers, tests, CMake, runtime behavior, credentials, ignored
  evidence, provider traffic, retries, later gates, orders, risk limits, live
  trading, staging, commit, push, PR, publication, or release.
- Modification of system-owned Codex skills outside this repository.

## Preconditions

- Start from tracked-clean branch
  `codex/gate7-residual-diagnostics-and-proof` at
  `febcaf72872b771d2d39fc17dae616cdfd91e326`.
- Preserve every ignored build, data, handoff, and credential-like artifact
  without inspecting credential values.

## Assumptions

- “All skills” means every tracked TradeBot skill under `.agents/skills/`.
- “Codex-related modules” means the repository-owned documents that govern
  agent authority, planning, workflow, evidence, review, testing, security,
  risk, recovery, contribution, state, and handoff.

## Invariants

- Current phase/gate facts remain authoritative only in the operator
  instruction, Git, active plan, `PROJECT_STATE.md`, and `ROADMAP.md`.
- Skills contain reusable domain procedure and may narrow but never widen
  higher-order authority.
- No source, test, build, credential, provider, order, risk, or live behavior
  changes.

## Authorization Boundary

Wade authorized repository documentation and skill updates for this task. No
authorization was given for staging, commit, push, publication, credential
access, provider execution, retry, later gates, orders, risk changes, or live
work. Stop after the documentation/skill diff and local validation report.

## Files Expected to Change

- `AGENTS.md`, `PLANS.md`, `CONTRIBUTING.md`.
- `docs/CODEX_EXECUTION_EVIDENCE.md` and affected Codex/governance modules.
- Every `.agents/skills/*/SKILL.md` and existing matching UI metadata.

## Implementation Steps

1. Inspect Git, authority documents, every skill, and current Gate 7 context.
2. Define reusable authorization, evidence-epoch, retry, resource-isolation,
   sensitive-memory, exact-artifact, and handoff rules.
3. Centralize those rules and remove volatile phase snapshots from skills.
4. Add only domain-specific consequences to each skill.
5. Validate every skill, YAML metadata, links/indexes, terminology, and diff.

## Verification

- Run `git diff --check`, status/name/stat audits, documentation greps, and a
  stale-skill-state scan.
- Run the installed skill-creator `quick_validate.py` for every tracked skill.
- Parse tracked skill YAML/frontmatter and UI metadata.
- Confirm no source, test, CMake, credential, ignored evidence, or provider
  path changed.

## Risks

- Repeating the full shared contract in every skill would waste context;
  mitigate with one central module and short skill references.
- Removing old phase text could erase safety boundaries; replace it with
  stronger generic non-transitive authorization rules while current state
  remains in authoritative documents.
- Broad documentation edits can introduce authority conflicts; validate the
  authority order and current-state wording explicitly.

## Rollback

With operator approval, revert only this documentation/skill diff. Do not alter
the preceding Gate 7 commit or ignored artifacts.

## Progress Log

- 2026-08-13: Inspected a tracked-clean branch, all 20 TradeBot skills, skill
  metadata, and the controlling Codex/governance documents.
- 2026-08-13: Added the shared execution/evidence module and synchronized every
  skill plus affected governance modules. Validation is in progress.
- 2026-08-13: All 20 skill validators passed; skill frontmatter and the existing
  UI metadata parsed successfully; diff, scope, placeholder, stale-state, and
  authority-reference audits passed. No source build was required for this
  documentation-only change.
- 2026-08-13: Wade explicitly authorized staging, committing, pushing, opening
  a PR, and merging the validated branch. Provider traffic, credentials, later
  gates, orders, risk changes, and live behavior remain outside that authority.

## Deviations

None recorded.

## Completion Evidence

- Installed skill-creator `quick_validate.py`: 20/20 TradeBot skills passed.
- YAML frontmatter and existing `agents/openai.yaml` metadata parsed
  successfully. The first metadata command used an unavailable older Ruby/
  Psych convenience API; the supported `safe_load(File.read(...))` rerun
  passed without a repository change.
- `git diff --check` passed.
- All 20 tracked skills reference `CODEX_EXECUTION_EVIDENCE.md`; all 20
  `SKILL.md` files changed and passed the stale volatile phase/gate scan.
- Changed paths are limited to `AGENTS.md`, `PLANS.md`, `CONTRIBUTING.md`,
  `docs/`, and `.agents/skills/`. CMake, source, headers, tests, scripts,
  credentials, ignored evidence, and external state are unchanged.
- No CMake/CTest run was required because runtime and test code are unchanged.

## Final Outcome

The recurring lessons are captured once in the shared Codex execution/evidence
module and applied through every TradeBot skill plus the affected governance
documents. Volatile phase/gate snapshots were removed from skills. The update
is validated and Wade authorized its Git publication and merge. That authority
does not include provider traffic, credentials, later gates, orders, risk
changes, or live behavior; Git and GitHub retain the publication evidence.

# Plan: Add Bounded Autonomous Skills And Offline CI Delivery

- Plan ID: `PLAN-20260813-autonomous-skills-ci-delivery`
- Status: Publishing — local implementation and verification complete; Wade
  authorized the remaining Git/GitHub execution path on 2026-08-13
- Owner: Operator
- Implementer: Codex
- Review authority: Operator
- Related roadmap phase: Repository governance and offline engineering tooling;
  no phase transition
- Related issue or decision: Operator request for autonomous project skills and
  GitHub Actions CI/CD
- Created: 2026-08-13
- Updated: 2026-08-13

## Objective

Increase safe repository autonomy with reusable bounded-maintenance skills,
continuous offline verification, and manually requested delivery of verified
non-executable build/test evidence.

## Context

The repository has one basic Ubuntu validation workflow and a broad set of
domain review skills, but it lacks a reusable autonomous change coordinator,
CI-failure recovery procedure, artifact-delivery procedure, scheduled deeper
validation, and exact-artifact manifests produced by GitHub Actions.

## Scope

- Add three repository-owned skills for bounded offline change orchestration,
  CI failure recovery, and offline artifact delivery.
- Harden pull-request and push validation with least-privilege permissions,
  concurrency control, deterministic build settings, logs, and job summaries.
- Add scheduled sanitizer validation and a manual, test-gated evidence-delivery
  workflow with SHA-256 provenance but no executable payload.
- Synchronize documentation and validate skills, workflows, scripts, and the
  default C++ build/test path.

## Out of Scope

- Provider traffic, browser OAuth, credentials, account access, market data,
  orders, risk-limit changes, Gates 8–9, deployment to any runtime environment,
  GitHub Releases, package registries, production infrastructure, or live
  trading.
- Automatic commits, pushes, merges, releases, issue creation, or secret use.
- C++ runtime behavior, public interfaces, or dependencies.

## Preconditions

- Start from tracked-clean `main` commit
  `c9e7cc0fbacf1096ce744690da5d9d21eb909708`.
- Work on scoped branch `codex/autonomous-skills-ci`.
- Preserve ignored build, data, output, handoff, and credential-like artifacts.

## Assumptions

- “CD” means delivery of an offline validation-evidence bundle to the manually
  dispatched GitHub Actions run, not executable distribution, deployment, or
  release publication.
- GitHub-hosted Ubuntu runners are suitable for the default, provider-free
  build. macOS-only opt-in provider proof targets remain excluded.

## Invariants

- `BACKTEST` remains the default and no workflow enables Gate 6, Gate 7,
  `PAPER`, `LIVE`, broker connectivity, or credential loading.
- Normal CI requires no repository secrets and has read-only repository
  permissions.
- Full CTest suites in different build trees run sequentially.
- Skills may execute reversible local inspection/edit/verification only within
  the active task; Git publication and every external or live action retain
  separate operator gates.

## Authorization Boundary

The operator initially authorized repository-local autonomous skills, GitHub
Actions CI/CD definitions, local edits, and offline verification. On
2026-08-13 Wade explicitly authorized the remaining task-finalization path:
stage only this plan's 25-path allowlist, commit, push the scoped branch, open
and update its PR, dispatch and rerun these task-specific workflows as needed,
make bounded corrections supported by new CI evidence, and merge after all
required checks pass. This does not authorize executable release, deployment,
provider execution, credential use, later gates, orders, financial changes, or
live use. Stop after the merged `main` state and workflow evidence are verified.

## Files Expected to Change

- `.agents/skills/tradebot-bounded-change-orchestrator/`
- `.agents/skills/tradebot-ci-failure-recovery/`
- `.agents/skills/tradebot-offline-artifact-delivery/`
- `.github/workflows/validation.yml`
- `.github/workflows/deep-validation.yml`
- `.github/workflows/offline-artifact-delivery.yml`
- `scripts/`, `docs/`, `CONTRIBUTING.md`, and this plan.

## Implementation Steps

1. Initialize and author the three skills with narrow authority and explicit
   stop conditions.
2. Add deterministic local helpers shared by developers and GitHub Actions.
3. Harden ordinary CI, add scheduled sanitizer coverage, and add manual
   checksum-bearing artifact delivery.
4. Synchronize testing, workflow, release, security, contributor, and skill
   index documentation.
5. Validate skills and YAML, run shell syntax checks, execute the helpers, run
   the default build/full CTest suite, and review diff/hygiene evidence.

## Verification

- Validate each new skill with the installed skill-creator validator.
- Parse all workflow YAML and inspect action permissions/triggers.
- Run `bash -n` for changed shell scripts and execute offline validation helpers.
- Run the default CMake build and full CTest suite sequentially.
- Run documentation audits, `git diff --check`, changed-file classification,
  secret-like path review, and final diff review.

## Risks

- A workflow could accidentally widen external or live authority; prevent this
  with default-only CMake options, no secrets, read-only permissions, and
  explicit prohibited-action checks.
- The default executable retains an explicitly gated live-capable mode; never
  distribute it through this workflow. Deliver only its hash, the manifest, and
  offline test evidence.
- Automated evidence delivery could be mistaken for release approval; keep it
  manual, ephemeral, non-deploying, and documented as evidence only.
- Scheduled sanitizer output can be flaky under shared-resource contention;
  use one build tree and sequential CTest execution.

## Rollback

With operator approval, revert only this plan's skills, workflows, helpers, and
documentation changes. No provider, credential, account, order, deployment, or
live state exists to unwind.

## Progress Log

- 2026-08-13: Inspected Git, current phase/gate authority, release/risk policy,
  existing skills, scripts, CMake targets, tests, and the current workflow.
- 2026-08-13: Created the scoped local branch and recorded the offline-only
  automation boundary.
- 2026-08-13: Initialized and authored three skills, added deterministic
  automation helpers, and implemented ordinary, scheduled sanitizer, and manual
  candidate-delivery workflows with read-only/no-secret/default-off guards.
- 2026-08-13: Synchronized affected testing, workflow, failure-recovery,
  release, security, contributor, state, index, and evidence documentation.
- 2026-08-13: The first deep-validation test run aborted all seven tests because
  Apple ASan does not support `detect_leaks=1`. The helper now disables leak
  detection only on Darwin while retaining it on Linux; a fresh informative
  rerun is pending.
- 2026-08-13: Network/live-boundary review confirmed the default executable can
  enter a separately gated live-capable mode. Candidate delivery was therefore
  narrowed to non-executable CTest/manifests/checksum evidence; no binary is
  uploaded.
- 2026-08-13: Final evidence review added a build-local success marker written
  only after sequential CTest passes and bound it to the exact commit and
  default-off configuration; packaging rejects stale or mismatched build trees.
- 2026-08-13: Independent read-only forward tests passed: CI recovery refused
  blind reruns/check disabling, and evidence delivery refused executable
  release/deployment from the dirty candidate tree.
- 2026-08-13: Final workflow review duplicated the exact four-entry evidence
  allowlist and non-release/non-deployment/non-live assertions in the workflow
  itself so a changed packaging helper cannot silently widen the upload.
- 2026-08-13: Official GitHub documentation and action repositories confirmed
  current Checkout v6.0.2 and Upload Artifact v7.0.1 behavior. All action uses
  were upgraded and pinned to their immutable full release commit SHAs.
- 2026-08-13: Wade authorized staging this plan's exact 25-path scope, commit,
  push, PR publication/update, task-specific workflow dispatch/rerun, bounded
  evidence-backed CI fixes, and merge after green checks. Executable release,
  deployment, credentials, provider traffic, later gates, orders, risk changes,
  and live use remain outside scope.
- 2026-08-13: Wade's finalization authority was used to install host-only
  Actionlint 1.7.12 and ShellCheck 0.11.0. Initial Actionlint found only three
  equivalent summary-redirection style findings; workflow summaries were
  mechanically grouped and fresh verification was started. No repository
  runtime or required dependency was added.

## Deviations

- Direct execution of the system skill scaffold failed because the file was not
  executable; invoking the same script through Python succeeded without a
  repository or dependency change.
- The first deep validation failed as recorded above. The failure and
  platform-specific correction were preserved; two subsequent sanitizer runs
  passed 7/7, including the final evidence epoch.
- One packaging-fixture command was rejected before execution because its
  temporary cleanup used a recursive removal. The safe rerun left isolated
  `/tmp/tradebot-package-*` fixtures and did not alter repository artifacts.
- The first checksum-fixture check ran from the parent directory and therefore
  could not resolve relative entries. The unchanged package passed when checked
  from its intended directory, matching the workflow.
- The first comprehensive whitespace/status audit used zsh's reserved `path`
  variable as a loop name, which temporarily removed `git` from that audit
  process's command search path. The corrected read-only audit used a neutral
  variable and passed; repository state was not mutated by the failed attempt.

## Completion Evidence

- Branch `codex/autonomous-skills-ci` derives from tracked-clean `main`/
  `origin/main` commit `c9e7cc0fbacf1096ce744690da5d9d21eb909708`.
- `scripts/validate_automation.py` passed for all 23 repository skills and all
  three workflows. The installed skill-creator validator passed 23/23; all
  skill UI metadata and workflow YAML parsed successfully.
- GitHub action dependencies are pinned to official Checkout v6.0.2 commit
  `de0fac2e4500dabe0009e67214ff5f5447ce83dd` and Upload Artifact v7.0.1
  commit `043fb46d1a93c77aae656e7c1c64a875d1fc6a0a`.
- Shell syntax, ShellCheck 0.11.0, Actionlint 1.7.12, and `git diff --check`
  passed after the three mechanical workflow-summary style corrections. Ruby
  YAML parsing and repository-specific workflow guardrails also passed.
- `./scripts/ci_validate.sh build/autonomy-validation` configured and built the
  default-off `RelWithDebInfo` tree, then passed the full sequential CTest suite
  7/7. The build emitted only the two existing documented warnings in
  `AsyncNetworkClient.cpp` and `RiskEngine.cpp`.
- `./scripts/ci_deep_validate.sh build/autonomy-deep-validation` passed the
  final default-off ASan/UBSan sequential suite 7/7. The preceding informative
  rerun also passed 7/7 after the recorded Apple leak-detection correction.
- Isolated packaging fixtures proved exact commit/config/success-marker
  binding, checksum validity, the four-file non-executable allowlist,
  executable exclusion, and dirty-tree, existing-output, and stale-marker
  refusal.
- Documentation audit found zero placeholder hits. All 442 safety-term hits and
  the 17 changed-line hits were reviewed as intentional safety/authorization
  language.
- No C++ source, headers, tests, CMake, dependencies, credentials, provider
  state, orders, risk limits, runtime modes, or live behavior changed. No
  workflow was dispatched and no external upload, release, or deployment
  occurred.

## Final Outcome

Three bounded automation skills, hardened ordinary CI, scheduled sanitizer CI,
and manual non-executable validation-evidence delivery are locally verified.
The default live-capable executable is intentionally not distributed. Wade has
authorized the remaining scoped Git/GitHub publication and merge sequence;
executable publication, deployment, release, provider action, credential use,
later-gate work, order action, risk change, and live action remain prohibited.
