# TradeBot Residual Gaps Backlog

## Document Control

- Status: Open governance and engineering backlog
- Baseline: accepted Open API base `400b486` (2026-08-07 inspection)
- Scope: residual gaps observed after the accepted broker-neutral Phase 22 boundary
- Authority: subordinate to `AGENTS.md`, accepted ADRs, `RISK_POLICY.md`, `ROADMAP.md`, and active approved plans
- Non-authorization: this document does not authorize OAuth execution, credentials in use, cTrader connectivity, account access, market data, orders, or live trading

## Current Gate

Workstream I and Phase 22 are Complete — Accepted. Wade selected FIBO Group through cTrader for demo-only XAUUSD, closing Phase 23. Official Open API is the sole integration path; Gate 1 and Gate 3 are revalidated, Gate 2 and Gate 5 were accepted by Wade on 2026-08-07, and Gate 5.1 is merged and accepted. The Algo Bridge is abandoned/non-controlling/out of scope and OANDA is permanently cancelled. Gate 6 is authorized but provider execution is stopped pending credential rotation and fixed redirect registration; every Gate 7–9 and live gate remains Blocked / NO-GO.

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
| GOV-002 | P0 | Phase 24 | Gate 5.1 accepted; Gate 6 authorized but prerequisite-blocked | Local correlation and Gate 6 account-proof controls have synthetic evidence. Credential rotation, fixed redirect registration, cTrader `state` verification, Gate 6A evidence, and Wade's mandatory checkpoint remain unresolved. Exact FIBO identity is a Gate 6A observation, not a prerequisite to discovery. |
| ENG-001 | P0 | Execution lifecycle | Confirmed gap | `ExecutionEngine` still uses the compatibility `BrokerFill` path. Migrate it to acknowledgement/execution/cancel/health lifecycle callbacks and retain portfolio mutation only from confirmed execution or approved reconciliation. |
| ENG-002 | P0 | Risk integration | Confirmed gap | The main gateway route does not obtain a concrete final normalized-quantity decision from `RiskEngine`; the compatibility route supplies an always-allowed decision after the preliminary `canTrade()` check. Add a real final decision API and end-to-end tests. |
| ENG-003 | P0 | Persistence | Confirmed gap | `OrderLifecycleStore` is held in memory and is not included in `StateSerializer` snapshots. Persist lifecycle state, deduplication keys, audit history, and migration/version behavior before restart-sensitive use. |
| ENG-004 | P0 | Safety tests | Evidence gap | Direct Phase 22 tests cover lifecycle primitives and gateway simulation, but not the complete strategy → allocation → execution → risk → gateway → adapter path. Add tests for cancel/fill races, timeout/expiry, halt/close-only, stale snapshots, restart deduplication, and rejected/partial fills through `ExecutionEngine`. |
| ENG-005 | P0 | Adapter health | Evidence gap | Gateway health events exist, but current `main` wiring does not visibly connect adapter degradation to `RiskEngine` halt/close-only behavior. Define and test fail-closed health propagation and recovery without automatic halt clearing. |
| ENG-006 | P0 | Reconciliation | Evidence gap | Reconciliation snapshots and mismatch classification exist, but end-to-end policy and tests for position/account mismatch escalation are incomplete. Define when new exposure is blocked, how state is reviewed, and how reconciliation is resolved. |
| BRK-001 | P0 | Provider adapter | Blocked | No provider-specific adapter exists; only `IBrokerAdapter` and deterministic local simulation are present. A future adapter must attach below `BrokerGateway` and translate provider schemas into neutral contracts. |
| BRK-002 | P1 | Provider evidence | Selected; runtime evidence pending | Open API approval is operator-reported. Authentication, real demo account identity, XAUUSD metadata/data, reconnect behavior, and demo-order proof remain unexecuted. |
| BRK-003 | P1 | Practice/sandbox validation | Blocked | No external practice or sandbox validation has been authorized or performed. Prove authentication, market data, order lifecycle, disconnects, partial fills, cancellation, reconciliation, and rate-limit behavior in a non-live environment before any live consideration. |
| RISK-001 | P0 | Live readiness | Blocked | The live-readiness checklist is not satisfied. Missing evidence includes exact venue/account approval, kill switch, limits, monitoring, stale-data handling, disconnect recovery, reconciliation, rollback, and operator stop authority. |
| RISK-002 | P0 | Credentials/security | Credential rotation required before authorized Gate 6 execution | Gate 5 defines scope, Keychain source, rotation, redaction, and staged Gate 6A/checkpoint/Gate 6B selection. Local generation/binding/expiry/single-use/redaction controls are implemented; provider correlation remains unverified and the exposed secret must be rotated. |
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
2. Preserve Wade's Gate 5.1 acceptance and Gate 6 authorization evidence.
3. Rotate the exposed secret and register the fixed redirect URI before the
   Gate 6A discovery, Wade checkpoint, and Gate 6B authentication sequence.
   Keep market data, reconnect, demo-order, and live gates separate.

## Recommended Sequencing

1. Resolve ENG-001 through ENG-006 and add the missing end-to-end evidence while remaining broker-neutral.
2. Complete the credential/redirect remediation and re-run Gate 6 offline
   preflight.
3. Execute the already-authorized provider OAuth verification, Gate 6A, and
   Gate 6B with Wade's checkpoint between discovery and authentication.
4. Execute later market-data, reconnect, and controlled demo-order gates separately.
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
