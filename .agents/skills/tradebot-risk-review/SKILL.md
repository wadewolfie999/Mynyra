---
name: tradebot-risk-review
description: Review TradeBot changes for financial, execution, data, credential, live-trading, reproducibility, and AI-agent risks. Use for changes involving SystemConfig, RiskEngine, ExecutionEngine, BrokerGateway, LiveDataAdapter, AuthManager, order sizing, risk limits, credentials, market data, replay semantics, or live-capable behavior.
---
# tradebot-risk-review

## Purpose

Review TradeBot changes for financial, execution, data, credential, live-trading, reproducibility, and AI-agent risks.

## Shared Operating Contract

- Follow `AGENTS.md` and `docs/CODEX_EXECUTION_EVIDENCE.md`.
- Resolve volatile branch, phase, gate, provider, and approval facts from Git,
  the active plan, `PROJECT_STATE.md`, and `ROADMAP.md`.
- Keep authorization and evidence epochs separate and stop at the exact
  operator-approved boundary.

## Activation Conditions

Use for changes involving `SystemConfig`, `RiskEngine`, `ExecutionEngine`, `BrokerGateway`, `LiveDataAdapter`, `AuthManager`, order sizing, risk limits, credentials, market data, replay semantics, or live-capable behavior.

## Must Not Be Used

Do not use to authorize live trading. Only the operator can authorize live transitions.

## Required Inputs

- Diff or files to review.
- Claimed behavior.
- Verification evidence.
- Whether live-capable paths are affected.

## Required Repository Inspection

Read:

- `docs/RISK_POLICY.md`
- `docs/LIVE_TRADING_READINESS.md`
- `docs/SECURITY.md`
- `docs/DATA_POLICY.md`
- Affected source/tests.

Run:

```sh
git status --short
git diff --name-status
```

## Procedure

1. Classify risk: financial, credential, market-data, replay, execution, dependency, reproducibility, or AI-agent.
2. Check default mode remains non-live.
3. Check risk gates and kill-switch behavior are not weakened.
4. Check secrets are not exposed.
5. Trace sensitive/provider-derived material through caller, callee, return,
   container, callback, temporary, allocation-failure, and destruction copies.
6. Check data provenance and timestamp assumptions.
7. Separate offline review, commit, preflight, provider execution, retry,
   later-gate, order, risk, and live authorizations.
8. Check tests cover affected safety behavior and identify their evidence epoch.
9. Identify required operator approvals and exhausted attempt budgets.

## Related Skills

- Use `tradebot-network-live-boundary-review` for live, network, auth, credential, PAPER, LIVE, broker, external I/O, or logging risk.
- Use `tradebot-execution-pipeline-validation` for order lifecycle, fill, cancel, reconciliation, halt, close-only, or risk-bypass risk.
- Use `tradebot-integration-architecture-review` for Workstream I architecture, broker-neutrality, adapter boundary, or subsystem-boundary risk.
- Use `tradebot-phase-gate-audit` when risk review depends on phase/gate,
  provider/broker-dependent, or live GO/NO-GO state.

## Allowed Mutations

Risk review is inspection/reporting only unless separately assigned documentation updates.

## Prohibited Mutations

No source edits, credential edits, live operations, commits, pushes, or financial-limit changes.

## Verification Commands

Use relevant commands from `docs/TESTING.md`; for review-only work, repository inspection may be sufficient.

## Expected Outputs

- Findings ordered by severity.
- Required approvals.
- Missing tests.
- Live-readiness status if applicable.
- Residual risks.
- Evidence epoch, attempt budget, and prohibited adjacent actions when relevant.

## Failure Behavior

If live-capable safety cannot be established, state that live use is not authorized and identify missing evidence.

## Reporting Format

```markdown
## Risk Review
- Risk class:
- Findings:
- Required tests:
- Required approvals:
- Live authorization status:
- Residual risk:
```

## Authority Documents

- `docs/RISK_POLICY.md`
- `docs/LIVE_TRADING_READINESS.md`
- `docs/SECURITY.md`
- `docs/DATA_POLICY.md`
