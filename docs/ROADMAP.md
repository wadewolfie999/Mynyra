# TradeBot Roadmap

## Purpose And Authority

- Purpose: define phase sequence, status, and authorization gates within the project-level Workstreams I-VII map.
- Authority level: primary phase-roadmap authority within the repository authority order; below current project state and above workflow details.
- Audience: operator, maintainers, Codex, contributors, reviewers, and research agents.

The roadmap is reconstructed from verified repository evidence, not from unverified legacy claims. Phase names below reflect observed source/test/commit structure. Completion status is recorded only when supported by current files, tests, commits, or accepted docs.

## Roadmap Principles

- Keep the deterministic core testable before adding research or live complexity.
- Treat every live-capable feature as financial-sensitive.
- Require replay, dry-run, paper, or sandbox validation before any live progression.
- Record durable architecture decisions in ADRs.
- Keep Git and GitHub as the execution record; do not recreate deprecated MOP/MOR/SS artifacts.
- Do not skip phases or infer authorization from sequence, completion, approval, or ADR acceptance.

## Roadmap Interpretation Rules

- "Next" identifies sequence; it does not authorize scoping or implementation.
- "Approved" records acceptance of the named artifact or phase outcome; it does not by itself authorize implementation.
- ADR acceptance records an architecture decision; it does not authorize source implementation.
- A blocked phase requires exact operator GO before its blocked activity may begin.
- No phase may be skipped.
- Bounded broker-neutral Phase 22 implementation is complete and accepted under `PLAN-20260624-workstream-i-broker-neutral-completion`. Broker-dependent implementation and live trading remain Blocked / NO-GO.
- `WORKSTREAM_ARCHITECTURE.md` defines the conceptually accepted project-level Workstreams I-VII domain map. Domain inclusion or strategic activity does not authorize implementation.
- Wade selected FIBO Group through cTrader for demo-only XAUUSD. Official
  cTrader Open API is the sole integration path; the Algo Bridge is abandoned,
  non-controlling, and out of scope.
- OANDA is permanently cancelled.
- Workstream III must not imply that a documentation platform has been selected. Platform selection requires an explicit operator decision in Phase 25.
- Nobitex is omitted from active Phase 22 scope. Phase 22 research must remain broker-neutral and must not select a broker, prop firm, program, account, or MT5 bridge topology.
- MT5/prop-account readiness is a downstream compatibility target for Phase 22 broker-neutral contracts. It does not select a broker or prop firm and does not authorize MT5 connectivity, account access, credentials, or broker-dependent implementation.
- Live trading remains disabled and unauthorized unless the operator grants exact approval under `RISK_POLICY.md` and `LIVE_TRADING_READINESS.md`.

## Current Workstream Architecture

`TradeBot Workstream Architecture v1.0` in `WORKSTREAM_ARCHITECTURE.md` is the current project-level map:

- Workstream I — Broker-Neutral Execution Foundation: Complete — Accepted through Phase 22.
- Workstream II — Broker Integration Program: FIBO Group/cTrader selected;
  Gate 1 and Gate 3 revalidated; Gate 2 and Gate 5 accepted by Wade on
  2026-08-07. Gate 5.1 merged in PR #24 and Wade accepted its implementation
  on 2026-08-09. Wade then authorized the Gate 6 umbrella; execution is
  currently stopped on credential rotation and fixed redirect-URI
  registration. Gate 7 and later work remain unauthorized.
- Workstream III — Documentation & Knowledge Architecture: parallel/future unless separately activated.
- Workstream IV — ML Optimization & Strategy Research: parallel/future unless separately activated.
- Workstream V — Core Platform Enhancement: parallel/future unless separately activated.
- Workstream VI — Production Governance & Live Readiness: parallel/future unless separately activated; live trading remains unauthorized.
- Workstream VII — Strategic Expansion Alternatives: parallel/future unless separately activated.

Workstream II includes a demo validation path and a separate live-readiness path.
Current scope is the single authorized Gate 6 sequence: Gate 6A, Wade's
mandatory safe-metadata checkpoint, then Gate 6B in the same process. Its
provider execution is stopped pending credential rotation and fixed redirect
registration. XAUUSD requests, reconnect proof, demo orders, live deployment,
and live trading remain unauthorized.

## Deterministic Phase Authority

This table is the canonical phase state for Workstreams I-III. `PROJECT_STATE.md` summarizes it and must not independently redefine these statuses.

| Workstream | Phase | Status | Authorized scope and gate |
| --- | --- | --- | --- |
| I — Broker-Neutral Execution Foundation | Phase 21: Infrastructure Alignment | Complete — Approved | Internal TradeBot integration architecture and boundary alignment. ADR 0003 is Accepted; Phase 21 planning artifacts are approved. |
| I — Broker-Neutral Execution Foundation | Phase 22: Broker-Neutral Execution Adapter Alignment and MT5/Prop-Account Readiness | Complete — Accepted | `PLAN-20260624-workstream-i-broker-neutral-completion` closed the provider-neutral contracts, deterministic simulation, execution/risk alignment, persistence, replay, tests, and documentation boundary. No broker or prop firm selection, connection method, connectivity, credentials, account access, real or sandbox orders, broker-dependent implementation, or live trading is authorized. |
| II — Broker Integration Program | Phase 23: Broker Selection | Complete — Selected | Wade selected FIBO Group through cTrader for demo-only XAUUSD. OANDA is permanently cancelled. Selection does not authorize OAuth, connectivity, credentials, account access, market data, orders, or live trading. |
| II — Broker Integration Program | Phase 24: Connection Protocol | Gate 1/Gate 3 revalidated; Gate 2/Gate 5 accepted; Gate 5.1 merged and accepted; Gate 6 authorized but stopped on prerequisites | Official Open API is sole. The opt-in Gate 6 proof remains read-only and demo-only. Credential rotation and fixed redirect registration block provider execution; market data, reconnect proof, order proof, and live use require separate directives. |
| III — Documentation & Knowledge Architecture | Phase 25: Documentation Platform Evaluation & Selection | Not Started | Evaluate and select the documentation platform. No platform is selected. |
| III — Documentation & Knowledge Architecture | Phase 26: Core Documentation Architecture & Drafting | Blocked | Build the core documentation structure and draft canonical documentation only after Phase 25 selection and operator approval of the documentation architecture. |

## Historical And Current Phase Model

The Foundation through Phase 19 entries below are historical context reconstructed from repository evidence. Phases 21-26 use the statuses in the deterministic table above; Workstreams IV-VII remain parallel/future domains without phase activation.

### Foundation And Deterministic Core

- Purpose: establish C++20 core, CMake build, CSV input, strategies, portfolio accounting, execution simulation, analytics, and repeatable local tests.
- Entry conditions: repository builds locally.
- Scope: deterministic source paths, no live-order side effects.
- Expected artifacts: core headers/source, tests, analytics outputs under generated paths.
- Validation: CMake build and CTest.
- Exit condition: deterministic baseline passes local tests.
- Status: partially verified by current CMake project and phase tests.

### Phase 9: Resume And Advanced Orders

- Purpose: support snapshot/resume and pending order behavior.
- Evidence: `--resume` handling in `src/main.cpp`; `StateSerializer`; pending order state in `PortfolioManager`.
- Validation: relevant CTest coverage and resume-specific checks when changed.
- Stop/go gate: do not change resume semantics without data/replay compatibility review.
- Status: implementation evidence exists; completion beyond current tests is not independently asserted here.

### Phase 10-11: Multi-Strategy Allocation And Regime Handling

- Purpose: support multiple strategies, portfolio allocation, and regime-aware weighting.
- Evidence: `PortfolioAllocator`, `RegimeDetector`, SMA and mean-reversion strategies wired in `src/main.cpp`.
- Validation: CTest plus strategy/allocation-specific regression when changed.
- Stop/go gate: no profitability claims from strategy behavior without research evidence that models fees, slippage, data leakage, and lookahead bias.
- Status: implementation evidence exists.

### Phase 12-17: Live-Capable Infrastructure And Resilience

- Purpose: runtime modes, live-data adapter, broker gateway, async network client, authentication, TLS/local validation, metrics, fault injection, and resilience checks.
- Evidence: `SystemConfig`, `LiveDataAdapter`, `BrokerGateway`, `AsyncNetworkClient`, `AuthManager`, `LocalMetricsExporter`, `phase15_tests`, `phase16_tests`, `phase17_tests`.
- Validation: CTest phases 15-17; security review for credential/network changes.
- Stop/go gate: live-capable code must remain default-off and must not infer live authorization.
- Status: implementation and tests exist; live trading remains unauthorized.

### Phase 18: Replay, L2 Order Book, Trigger Orders, And Burn-In

- Purpose: local replay adapter, L2 BBO handling, stop-loss/OCO trigger execution, and burn-in latency validation.
- Evidence: `LocalDataReplayAdapter`, `L2OrderBook`, `TriggerOrderManager`, `phase18_tests`, `phase18_burnin`.
- Validation: `ctest --test-dir build -R phase18_tests --output-on-failure`; benchmark and burn-in evidence when performance is changed.
- Stop/go gate: replay correctness and trigger-order safety must not regress.
- Status: test target exists and passed during 2026-06-06 verification.

### Phase 19: L2 BBO Performance Revalidation

- Purpose: validate `applyBbo` hot-path performance and preserve functional correctness.
- Evidence: merged Phase 19 history on `main`; `src/benchmarks/apply_bbo_microbench.cpp`; generated Phase 19 logs under ignored build path.
- Scope: L2 BBO performance evidence and regression checks.
- Expected artifacts: benchmark outputs, CTest results, and source changes when approved by a task.
- Validation: `cmake --build build`, full CTest, targeted Phase 18 test, `build/apply_bbo_microbench <updates>`.
- Exit condition: operator-reviewed evidence shows functional tests pass and benchmark results are recorded with environment/input size.
- Dependencies: Phase 18 behavior and benchmark harness.
- Rollback: revert Phase 19 source changes or restore baseline commit if benchmark or tests regress, with operator approval.
- Status: implementation history is merged on `main`; future performance claims still require fresh benchmark evidence.

### Phase 21: Workstream I Infrastructure Alignment

- Purpose: define broker-neutral integration architecture, subsystem boundaries, adapter lifecycle contracts, risk controls, and replay compatibility gates before implementation.
- Evidence: `docs/WORKSTREAM_I_INTEGRATION_ARCHITECTURE.md`, `docs/WORKSTREAM_I_ADAPTER_CONTRACT.md`, `docs/WORKSTREAM_I_RISK_MATRIX.md`, `docs/WORKSTREAM_I_REPLAY_COMPATIBILITY_CHECKLIST.md`, and ADR 0003.
- Scope: documentation-only planning and operator review.
- Expected artifacts: Phase 21 planning docs, ADR 0003 disposition, and an explicit Phase 22 NO-GO until separate scoping is approved.
- Validation: documentation audit, cross-document consistency check, and operator approval for ADR/plan status.
- Exit condition: operator accepts ADR 0003 and approves Phase 21 artifacts.
- Dependencies: Phase 18 replay/L2 behavior, Phase 19 BBO benchmark harness, existing risk and live-readiness gates.
- Rollback: revise documentation-only Phase 21 artifacts with operator approval.
- Status: Complete — Approved.

### Phase 22: Broker-Neutral Execution Adapter Alignment and MT5/Prop-Account Readiness

- Purpose: implement TradeBot execution boundaries around broker-neutral contracts and deterministic simulation while retaining offline MT5/prop-account research as downstream compatibility evidence without selecting a provider.
- Entry conditions: Phase 21 artifacts approved, ADR 0003 accepted, bounded plan approved, explicit operator implementation GO, required tests and rollback defined, and broker-dependent decisions kept outside scope.
- Authorized scope: broker-neutral contracts and source implementation covering order lifecycle, deterministic adapter simulation, account/equity snapshots, position reconciliation, account and symbol metadata, deterministic quantity normalization, lot sizing, execution-result mapping, failure handling, synthetic/configurable prop-rule dimensions, deterministic fixtures, replay validation, persistence, metrics, measurements, risks, and non-activation safeguards.
- Compatibility boundary: MT5-specific transport, terminal bridges, broker APIs, credentials, account access, network connectivity, real order routing, broker selection, and prop-firm selection remain outside the authorization.
- Research artifact: `PHASE22_OFFLINE_MT5_PROP_RESEARCH.md`.
- Documentation validation: Git diff hygiene, documentation audits, cross-document authority consistency, broker-neutrality review, deterministic contract review, and non-activation wording review.
- Implementation closure evidence: PR #14 merged broker-neutral lifecycle contracts and PR #15 merged the accepted adapter-gateway implementation with local CMake/CTest evidence and passing GitHub validation.
- Stop/go gate: Phase 22 is closed at the accepted broker-neutral boundary. Broker-specific implementation, MT5 connection or login, credentials, account access, broker or prop-firm selection, real or sandbox orders, live trading, risk-limit changes, and Phase 23 activation remain prohibited without separate explicit operator approval.
- Status: Complete — Accepted; broker-dependent and live work Blocked / NO-GO.

### Phase 23: Workstream II Broker Selection

- Purpose: identify and evaluate the Most Optimized Broker (M.O.B.).
- Entry conditions: satisfied by Wade's selection of FIBO Group through cTrader.
- Operating rhythm: Strategy 2 — Parallel Evidence Lanes With Wade Checkpoints. Wade owns authority, scope, gates, broker selection, and acceptance; Bigi owns technical evidence, adapter-fit audit, failure-mode checklist, and demo/live semantics analysis; ChatGPT/review assistant supports prompt structure, output review, and governance-drift detection.
- Scope: closed with the explicit FIBO Group/cTrader selection; connection
  implementation is not implied.
- Stop/go gate: selection does not imply connection or implementation authority.
- Governance overlay: Strategy 3 gate labels may be added later; no full Kanban system is authorized now.
- Status: Complete — Selected. OANDA is permanently cancelled.

### Phase 24: Workstream II Connection Protocol

- Purpose: define the TradeBot-to-selected-M.O.B. account connection protocol.
- Entry conditions: Phase 23 selection is complete. Wade authorized bounded
  Gates 1-3 revalidation and final Gate 2/Gate 5 correction. Wade accepted
  Gate 2 and Gate 5 on 2026-08-07.
- Scope: connection protocol only within the separately approved scope.
- Stop/go gate: no account connection, credential use, or live trading before exact operator approval and applicable risk gates.
- Status: Gate 1 and Gate 3 revalidated; Gate 2 and Gate 5 accepted by Wade;
  Gate 5.1 merged and accepted. Gate 6 is explicitly authorized but provider
  execution is stopped pending credential rotation and fixed redirect
  registration. Gate 7 and every trading/live gate remain blocked pending
  separate operator approval.

### Phase 25: Workstream III Documentation Platform Evaluation And Selection

- Purpose: evaluate and select the documentation platform.
- Entry conditions: prior phases are not skipped and the operator authorizes platform evaluation.
- Scope: platform evaluation and an explicit operator selection decision.
- Stop/go gate: evaluation must not imply that a platform has already been selected.
- Status: Not Started; no documentation platform is selected.

### Phase 26: Workstream III Core Documentation Architecture And Drafting

- Purpose: build the core documentation structure and draft canonical documentation.
- Entry conditions: Phase 25 records an explicit platform selection and the operator approves the documentation architecture.
- Scope: documentation architecture and drafting only within the separately approved scope.
- Stop/go gate: no platform-specific architecture or drafting may begin from an assumed selection.
- Status: Blocked on Phase 25 selection and operator approval.

### Future Research And Optimization Boundary

- Purpose: future Python or Julia research, optimization, ML-assisted strategy work, or offline analysis.
- Entry conditions: deterministic outputs, data policy, reproducible configs, and explicit interfaces.
- Scope: offline research only until promoted through validation.
- Validation: documented datasets, seeds, fees, slippage, leakage checks, and no production-readiness claims from backtests alone.
- Stop/go gate: research cannot bypass risk, execution-mode, logging, or live-readiness gates.
- Status: no tracked Python or Julia components currently exist.

### Live Trading Readiness

- Purpose: controlled path from replay/dry-run/paper/sandbox validation to live authorization.
- Entry conditions: explicit operator request plus all `LIVE_TRADING_READINESS.md` requirements.
- Scope: real venue/account operation only after approval.
- Validation: paper or sandbox evidence, kill switch, reconciliation, credentials, risk limits, logging, stale-data handling, and rollback plan.
- Stop/go gate: absent explicit operator authorization, live trading is prohibited.
- Status: not authorized.

## Recovery Considerations

- If `PROJECT_STATE.md` conflicts with the phase table, stop and reconcile the summary against this roadmap and higher authority before acting.
- If an ADR conflicts with roadmap direction, the accepted ADR governs until superseded.
- If tests fail during a phase gate, halt implementation and follow `FAILURE_RECOVERY.md`.

## Next Roadmap Action

Remediate the Gate 6 credential and redirect prerequisites, then resume the
authorized Gate 6A discovery, mandatory Wade checkpoint, and Gate 6B account
authentication proof. Stop before XAUUSD data, reconnect testing, orders, live
accounts, live trading, or risk-limit changes; those remain unauthorized.
