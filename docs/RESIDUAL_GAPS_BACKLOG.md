# TradeBot Residual Gaps Backlog

## Document Control

- Status: Open governance and engineering backlog
- Baseline: `main` at `6bdc548` (2026-07-29 inspection)
- Scope: residual gaps observed after the accepted broker-neutral Phase 22 boundary
- Authority: subordinate to `AGENTS.md`, accepted ADRs, `RISK_POLICY.md`, `ROADMAP.md`, and active approved plans
- Non-authorization: this document does not select a broker, open Phase 23/24, authorize provider code, use credentials, connect an account, place orders, or enable live trading

## Current Gate

Workstream I and Phase 22 are Complete — Accepted. Phase 23 is formally Not Started; the Iran-compatible pre-phase evidence search under `PLAN-20260729-iran-compatible-provider-search` is the only active broker-related scope. OANDA is ON HOLD and no replacement is selected. Phase 24, broker-specific implementation, external connectivity, credentials, sandbox orders, and live trading remain Blocked / NO-GO until separate operator approval.

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
| GOV-001 | P0 | Phase 23 | Blocked | OANDA is ON HOLD and no replacement has been selected. Close with an operator-approved Phase 23 decision backed by a provider comparison, written Iran-resident eligibility evidence, and an evidence register. |
| GOV-002 | P0 | Phase 24 | Blocked | No approved connection scope exists. Close only after broker selection, account/environment scope, configuration, rollback, and operator GO are recorded. |
| ENG-001 | P0 | Execution lifecycle | Confirmed gap | `ExecutionEngine` still uses the compatibility `BrokerFill` path. Migrate it to acknowledgement/execution/cancel/health lifecycle callbacks and retain portfolio mutation only from confirmed execution or approved reconciliation. |
| ENG-002 | P0 | Risk integration | Confirmed gap | The main gateway route does not obtain a concrete final normalized-quantity decision from `RiskEngine`; the compatibility route supplies an always-allowed decision after the preliminary `canTrade()` check. Add a real final decision API and end-to-end tests. |
| ENG-003 | P0 | Persistence | Confirmed gap | `OrderLifecycleStore` is held in memory and is not included in `StateSerializer` snapshots. Persist lifecycle state, deduplication keys, audit history, and migration/version behavior before restart-sensitive use. |
| ENG-004 | P0 | Safety tests | Evidence gap | Direct Phase 22 tests cover lifecycle primitives and gateway simulation, but not the complete strategy → allocation → execution → risk → gateway → adapter path. Add tests for cancel/fill races, timeout/expiry, halt/close-only, stale snapshots, restart deduplication, and rejected/partial fills through `ExecutionEngine`. |
| ENG-005 | P0 | Adapter health | Evidence gap | Gateway health events exist, but current `main` wiring does not visibly connect adapter degradation to `RiskEngine` halt/close-only behavior. Define and test fail-closed health propagation and recovery without automatic halt clearing. |
| ENG-006 | P0 | Reconciliation | Evidence gap | Reconciliation snapshots and mismatch classification exist, but end-to-end policy and tests for position/account mismatch escalation are incomplete. Define when new exposure is blocked, how state is reviewed, and how reconciliation is resolved. |
| BRK-001 | P0 | Provider adapter | Blocked | No provider-specific adapter exists; only `IBrokerAdapter` and deterministic local simulation are present. A future adapter must attach below `BrokerGateway` and translate provider schemas into neutral contracts. |
| BRK-002 | P1 | Provider evidence | In progress — evidence only | The initial Iran-compatible screen exists in `WORKSTREAM_II_IRAN_COMPATIBLE_PROVIDER_PIVOT.md`, but written eligibility, exact legal-entity, demo, XAUUSD, API entitlement, rate-limit, failure, and jurisdiction evidence remains incomplete. A selected-provider register may be frozen only after Wade's Phase 23 decision. |
| BRK-003 | P1 | Practice/sandbox validation | Blocked | No external practice or sandbox validation has been authorized or performed. Prove authentication, market data, order lifecycle, disconnects, partial fills, cancellation, reconciliation, and rate-limit behavior in a non-live environment before any live consideration. |
| RISK-001 | P0 | Live readiness | Blocked | The live-readiness checklist is not satisfied. Missing evidence includes exact venue/account approval, kill switch, limits, monitoring, stale-data handling, disconnect recovery, reconciliation, rollback, and operator stop authority. |
| RISK-002 | P0 | Credentials/security | Blocked | No provider credential scope or least-privilege review exists. Define secret source, permissions, rotation, redaction, account-ID handling, and audit procedures without committing or exposing values. |
| DATA-001 | P1 | Deterministic inputs | Confirmed gap | `data/samples/` is empty and the executable's default CSV input is absent. Add approved, provenance-labelled fixtures or require explicit input paths for reproducible end-to-end demonstrations. |
| DATA-002 | P1 | Replay/provenance | Evidence gap | Replay tests exist, but provider-event fixtures, timestamp units, event ordering, and generated-data provenance need a documented integration fixture policy before provider work. |
| OPS-001 | P1 | Operations | Evidence gap | No provider-specific monitoring, audit-log schema, reconciliation dashboard/process, incident runbook, or rollback handoff has been demonstrated. Define these before any account-connected test. |
| QUAL-001 | P2 | Tooling | Confirmed gap | Build emits two known warnings; no formatter, static analyzer, or Markdown link checker is configured. Resolve warnings and establish proportionate quality tooling. |
| BENCH-001 | P2 | Performance evidence | Evidence gap | `apply_bbo_microbench` runs, but no current comparative baseline or cross-environment evidence is recorded. Keep benchmark claims scoped to the named command, build, machine, and input size. |
| DOC-001 | P2 | Documentation freshness | Confirmed gap | `PROJECT_STATE.md` contains historical verification dates older than the current inspection. Refresh current-state metadata after the next accepted verification boundary; do not treat stale dates as current evidence. |
| DOC-002 | P2 | Legacy cleanup | Confirmed gap | Source comments still contain deprecated MOP/workstream terminology and legacy compatibility notes. Remove or clearly mark historical wording during an authorized maintenance pass. |

## OANDA Hold And Iran-Compatible Pivot

OANDA is ON HOLD because the available registration path did not establish lawful eligibility for an Iranian citizen residing in Tehran. It may re-enter evaluation only if OANDA Support provides a lawful, documented path that requires no VPN-location substitution, false residence, third-party identity, or concealed documents.

The replacement search is governed by `WORKSTREAM_II_IRAN_COMPATIBLE_PROVIDER_PIVOT.md`:

1. Treat lawful Iran-resident eligibility and direct access from Iran as hard P0 evidence gates.
2. Obtain written confirmation of the contracting entity, accepted KYC, demo access, XAUUSD, and API permissions.
3. Validate FIBO Group cTrader first; retain FXOpen as a technical candidate blocked on Iran eligibility; keep AMarkets and IFC Markets as interface-fit fallbacks.
4. Stop at Wade's Phase 23 selection checkpoint. Candidate priority is not selection.
5. After a separate selection and Phase 24 GO, create a provider adapter plan below `BrokerGateway`, deterministic provider-shaped fixtures, failure/reconciliation tests, security controls, and rollback evidence.
6. Keep practice validation, live-readiness review, and live authorization as separate future decisions.

## Recommended Sequencing

1. Resolve ENG-001 through ENG-006 and add the missing end-to-end evidence while remaining broker-neutral.
2. Obtain written Iran-resident, demo, XAUUSD, and API evidence for the leading validation candidate.
3. Assemble Phase 23 provider-comparison and risk evidence; stop for Wade's selection decision.
4. Execute BRK-001 through BRK-003 only after a selected provider, a new approved plan, and the Phase 24 connection gate.
5. Treat practice validation, live-readiness review, and live authorization as separate decisions; do not collapse them into broker selection.

## References

- `docs/PROJECT_STATE.md`
- `docs/ROADMAP.md`
- `docs/ARCHITECTURE.md`
- `docs/RISK_POLICY.md`
- `docs/LIVE_TRADING_READINESS.md`
- `docs/WORKSTREAM_I_ADAPTER_CONTRACT.md`
- `docs/WORKSTREAM_I_RISK_MATRIX.md`
- `docs/TESTING.md`
- `PLANS.md`
