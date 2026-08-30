---
name: tradebot-integration-architecture-review
description: Review TradeBot Workstream I and broker-exchange integration architecture for broker-neutrality, subsystem boundary integrity, deterministic replay safety, and risk isolation. Use for post-Phase-22 boundary review, adapter architecture, BrokerGateway boundaries, or Workstream II readiness review.
---
# tradebot-integration-architecture-review

## Purpose

Review Workstream I integration architecture without allowing broker-specific implementation drift or boundary bypass.

## Global TradeBot Rules

- Follow `AGENTS.md` and `docs/CODEX_EXECUTION_EVIDENCE.md`.
- Resolve volatile branch, phase, gate, provider, and approval facts from Git,
  the active plan, `PROJECT_STATE.md`, and `ROADMAP.md`; do not hardcode them in
  this skill.
- Keep authorization and evidence epochs separate and stop at the exact
  operator-approved boundary.

## Activation Conditions

Use for Workstream I, post-Phase-22 boundary review, adapter architecture, `BrokerGateway`, `ExecutionEngine`, `RiskEngine`, `SystemConfig`, `LiveDataAdapter`, `AuthManager`, `L2OrderBook`, or replay-boundary design.

## Must Not Be Used

Do not use to authorize implementation, broker-specific APIs, credentials, external connectivity, or live trading.

## Required Inputs

- Proposed architecture or diff.
- Related phase/plan/ADR.
- Affected subsystem boundaries.
- Claimed broker-neutral behavior.

## Required Outputs

- Boundary-integrity findings.
- Broker-neutrality finding.
- Determinism and risk-isolation finding.
- Required approvals or blockers.

## Required Inspection

Read:

- `docs/WORKSTREAM_I_INTEGRATION_ARCHITECTURE.md`
- `docs/WORKSTREAM_I_ADAPTER_CONTRACT.md`
- `docs/WORKSTREAM_I_RISK_MATRIX.md`
- `docs/WORKSTREAM_I_REPLAY_COMPATIBILITY_CHECKLIST.md`
- ADR 0003
- `docs/ARCHITECTURE.md`
- `docs/RISK_POLICY.md`
- Affected source if implementation is already authorized

Run:

```sh
git status --short
git diff --name-status
```

## Procedure

1. Verify any provider/broker-dependent implementation or external process is
   scoped only if exact operator GO exists for that action and artifact.
2. Check broker-neutrality and reject broker-specific assumptions without plan evidence.
3. Verify strategy, allocation, replay, L2, analytics, and portfolio code stay free of broker schemas.
4. Confirm future adapters attach below `BrokerGateway`.
5. Confirm new exposure passes through `ExecutionEngine` and `RiskEngine`.
6. Confirm deterministic replay and `BACKTEST` remain independent of network, broker, credential, and wall-clock state.

## Validation Checklist

- `ExecutionEngine`, `RiskEngine`, `BrokerGateway`, `LiveDataAdapter`, `AuthManager`, `SystemConfig`, `L2OrderBook`, and replay boundaries remain intact.
- Direct strategy-to-broker coupling is rejected.
- Risk bypass is rejected.
- Live/default behavior changes are rejected.
- Workstream I remains architecture/governance unless implementation is separately authorized.

## Failure Modes Caught

- Broker schemas leaking into core logic.
- Adapter attached beside or above `BrokerGateway`.
- Risk gates bypassed by convenience paths.
- Replay tests requiring live or wall-clock state.
- Provider/broker-dependent work starting from ADR, plan, phase, gate, review,
  commit, or offline-test acceptance alone.

## Hard Prohibitions

- Do not implement or execute blocked provider/broker work, retries, later
  gates, orders, risk changes, or live actions without exact operator GO.
- Do not add broker-specific API assumptions without approved plan evidence.
- Do not weaken risk controls or deterministic defaults.
- Do not expose credentials or enable live trading.
- Do not stage, commit, push, reset, clean, or discard changes.

## Interaction With Existing Skills

- Run after `tradebot-authority-state-audit` and `tradebot-phase-gate-audit`.
- Pair with `tradebot-risk-review` for financial-sensitive architecture.
- Pair with `tradebot-execution-pipeline-validation` for order lifecycle changes.
- Pair with `tradebot-network-live-boundary-review` for network, auth, PAPER, LIVE, or broker boundaries.

## Example Invocation Prompt

```text
Use $tradebot-integration-architecture-review to verify a broker-dependent adapter design stays below BrokerGateway and does not bypass accepted broker-neutral boundaries.
```

## Stop Conditions

Stop if architecture requires broker-specific details, risk-gate changes, live/default behavior changes, replay nondeterminism, credential work, or unapproved source implementation.

## Reporting Format

```markdown
## Integration Architecture Review
- Scope:
- Boundary findings:
- Broker-neutrality:
- Replay determinism:
- Risk isolation:
- Required approvals:
- Status:
```

## Authority Documents

- `docs/WORKSTREAM_I_INTEGRATION_ARCHITECTURE.md`
- `docs/WORKSTREAM_I_ADAPTER_CONTRACT.md`
- `docs/WORKSTREAM_I_RISK_MATRIX.md`
- `docs/decisions/0003-workstream-i-integration-architecture.md`
