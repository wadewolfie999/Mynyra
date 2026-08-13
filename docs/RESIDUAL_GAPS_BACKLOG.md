# TradeBot Residual Gaps Backlog

## Document Control

- Status: Frozen diagnostic input; active work is governed by the nine-package
  Repository Remediation Program
- Baseline: repository-wide diagnosis at
  `7f89c597d3874e5c8782a49a31d42335c8bf1e17` (2026-08-13)
- Scope: preserved residual gaps mapped into WP-0 through WP-8
- Authority: subordinate to `AGENTS.md`, accepted ADRs, `RISK_POLICY.md`, `ROADMAP.md`, and active approved plans
- Non-authorization: this document does not authorize a future OAuth execution, credentials in use, cTrader connectivity, account access, market data, orders, or live trading

## Current Gate

Historical Workstream I, Phase 22, and cTrader gate evidence remains preserved.
It is not the current execution queue. Wade has locked current focus to WP-0
through WP-8 in `REPOSITORY_REMEDIATION_PROGRAM.md`. The foundational
governance is approved; WP-0 is current, WP-1 is approved and queued behind
WP-0, and WP-2 through WP-8 remain Blocked / NO-GO except for integrated
WP-7/WP-8 closure slices. Provider continuation, Gates 8–9, and live trading
remain Blocked / NO-GO.

## Program Crosswalk

| Existing backlog area | Controlling package |
| --- | --- |
| Live readiness, nominal LIVE behavior, unsafe mode/provider fallback | WP-0 |
| ENG-003, snapshot/restart state, generated artifacts, replay provenance | WP-1 |
| Quantity, fees, cash/P&L, multi-symbol marks, benchmark fill ceiling | WP-2 |
| ENG-002, ENG-005, risk sources, drawdown, VaR, halt/close-only | WP-3 |
| ENG-001, ENG-004, ENG-006, pending/trigger/cancel/fill/reconcile | WP-4 |
| DATA-001, DATA-002, runtime topology, timestamps, input contracts | WP-5 |
| BRK-001 through BRK-003, transport, authentication, provider mapping | WP-6 |
| OPS-001, QUAL-001, BENCH-001, missing CI and operational evidence | WP-7 |
| GOV/DOC items, stale claims, file disposition, final consistency | WP-8 |

This crosswalk is classification, not implementation approval. A new finding
must map to one package or stop for Wade's explicit scope decision.

## Priority And Status

- P0 — safety or phase gate; must be resolved before provider connection or live consideration
- P1 — required for a credible provider integration or operational handoff
- P2 — maintainability, reproducibility, or evidence hardening
- Confirmed gap — directly observed in the current tree
- Evidence gap — behavior may exist, but required proof is missing
- Blocked — intentionally waiting for authority, provider selection, or access

## Backlog

| ID | Priority | Area | Status | Residual gap and closure evidence |
| --- | --- | --- | --- | --- |
| GOV-001 | P0 | Phase 23 | Closed | Wade selected FIBO Group through cTrader for demo-only XAUUSD. |
| GOV-002 | P0 | Phase 24 | Gate 6 accepted; Gate 7 incomplete at subscription transition | Local correlation, Gate 6 account proof, and prior Gate 7 synthetic controls have evidence. The latest Gate 7 process passed full XAUUSD metadata validation and stopped at `gate7_subscription_failed`; the subcause, accepted subscription, quote, and timestamp remain unproven. The residual offline patch is reviewed for the authorized local commit/exact rebuild; provider execution still requires its separate preconditions and exact approval. |
| ENG-001 | P0 | Execution lifecycle | Confirmed gap | `ExecutionEngine` still uses the compatibility `BrokerFill` path. Migrate it to acknowledgement/execution/cancel/health lifecycle callbacks and retain portfolio mutation only from confirmed execution or approved reconciliation. |
| ENG-002 | P0 | Risk integration | Confirmed gap | The main gateway route does not obtain a concrete final normalized-quantity decision from `RiskEngine`; the compatibility route supplies an always-allowed decision after the preliminary `canTrade()` check. Add a real final decision API and end-to-end tests. |
| ENG-003 | P0 | Persistence | Confirmed gap | `OrderLifecycleStore` is held in memory and is not included in `StateSerializer` snapshots. Persist lifecycle state, deduplication keys, audit history, and migration/version behavior before restart-sensitive use. |
| ENG-004 | P0 | Safety tests | Evidence gap | Direct Phase 22 tests cover lifecycle primitives and gateway simulation, but not the complete strategy → allocation → execution → risk → gateway → adapter path. Add tests for cancel/fill races, timeout/expiry, halt/close-only, stale snapshots, restart deduplication, and rejected/partial fills through `ExecutionEngine`. |
| ENG-005 | P0 | Adapter health | Evidence gap | Gateway health events exist, but current `main` wiring does not visibly connect adapter degradation to `RiskEngine` halt/close-only behavior. Define and test fail-closed health propagation and recovery without automatic halt clearing. |
| ENG-006 | P0 | Reconciliation | Evidence gap | Reconciliation snapshots and mismatch classification exist, but end-to-end policy and tests for position/account mismatch escalation are incomplete. Define when new exposure is blocked, how state is reviewed, and how reconciliation is resolved. |
| BRK-001 | P0 | Provider adapter | Blocked | No provider-specific adapter exists; only `IBrokerAdapter` and deterministic local simulation are present. A future adapter must attach below `BrokerGateway` and translate provider schemas into neutral contracts. |
| BRK-002 | P1 | Provider evidence | Gate 7 incomplete; subscription/quote evidence pending | Open API approval is operator-reported. Control-flow evidence now reaches fresh demo-account authentication and full canonical XAUUSD metadata validation, but the generic subscription transition failed. Accepted subscription, BBO/timestamp proof, reconnect behavior, and demo-order proof remain unexecuted. |
| BRK-003 | P1 | Practice/sandbox validation | Blocked | No external practice or sandbox validation has been authorized or performed. Prove authentication, market data, order lifecycle, disconnects, partial fills, cancellation, reconciliation, and rate-limit behavior in a non-live environment before any live consideration. |
| RISK-001 | P0 | Live readiness | Blocked | The live-readiness checklist is not satisfied. Missing evidence includes exact venue/account approval, kill switch, limits, monitoring, stale-data handling, disconnect recovery, reconciliation, rollback, and operator stop authority. |
| RISK-002 | P0 | Credentials/security | Gate 6 credential boundary active | Gate 5 defines Keychain, redaction, and staged Gate 6A/checkpoint/Gate 6B selection. Wade confirmed the configured credential state and authorized `trading` scope; exact callback correlation succeeded. No credential or token value may enter evidence. |
| DATA-001 | P1 | Deterministic inputs | Confirmed gap | `data/samples/` is empty and the executable's default CSV input is absent. Add approved, provenance-labelled fixtures or require explicit input paths for reproducible end-to-end demonstrations. |
| DATA-002 | P1 | Replay/provenance | Evidence gap | Replay tests exist, but provider-event fixtures, timestamp units, event ordering, and generated-data provenance need a documented integration fixture policy before provider work. |
| OPS-001 | P1 | Operations | Evidence gap | No provider-specific monitoring, audit-log schema, reconciliation dashboard/process, incident runbook, or rollback handoff has been demonstrated. Define these before any account-connected test. |
| QUAL-001 | P2 | Tooling | Confirmed gap | Build emits two known warnings; no formatter, static analyzer, or Markdown link checker is configured. Resolve warnings and establish proportionate quality tooling. |
| BENCH-001 | P2 | Performance evidence | Evidence gap | `apply_bbo_microbench` runs, but no current comparative baseline or cross-environment evidence is recorded. Keep benchmark claims scoped to the named command, build, machine, and input size. |
| DOC-001 | P2 | Documentation freshness | Confirmed gap | `PROJECT_STATE.md` contains historical verification dates older than the current inspection. Refresh current-state metadata after the next accepted verification boundary; do not treat stale dates as current evidence. |
| DOC-002 | P2 | Legacy cleanup | Confirmed gap | Source comments still contain deprecated MOP/workstream terminology and legacy compatibility notes. Remove or clearly mark historical wording during an authorized maintenance pass. |

## Cancelled OANDA Lane And Selected cTrader Path

OANDA is permanently cancelled. FIBO Group through cTrader is selected, and official Open API is the sole architecture under ADR 0004.

The current gate sequence is:

1. Preserve Wade's Gate 2 and Gate 5 acceptance record; acceptance grants no execution authority.
2. Preserve Wade's Gate 5.1 and completed/accepted Gate 6 evidence; the prior
   credential/redirect remediation is historical, not a current action.
3. Complete Gate 7 residual offline review, then require a separate exact Wade
   approval before one provider process. Keep reconnect, demo-order, and live
   gates separate.

## Recommended Sequencing

The canonical sequence is defined only in
`REPOSITORY_REMEDIATION_PROGRAM.md`: the approved program foundation, the
WP-0 → WP-6 safety spine, WP-7 evidence integrated with each package, and WP-8
authority closeout after each package and at program completion. This backlog
must not establish a competing sequence.

## References

- `docs/PROJECT_STATE.md`
- `docs/REPOSITORY_REMEDIATION_PROGRAM.md`
- `docs/ROADMAP.md`
- `docs/ARCHITECTURE.md`
- `docs/RISK_POLICY.md`
- `docs/LIVE_TRADING_READINESS.md`
- `docs/WORKSTREAM_I_ADAPTER_CONTRACT.md`
- `docs/WORKSTREAM_I_RISK_MATRIX.md`
- `docs/TESTING.md`
- `PLANS.md`
