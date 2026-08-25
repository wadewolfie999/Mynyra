---
name: tradebot-phase-gate-audit
description: Audit TradeBot phase, workstream, and gate transitions, entry and exit criteria, approval status, GO or NO-GO state, and unauthorized implementation risk. Use before phase/gate scoping, transition claims, roadmap updates, implementation planning, external execution, or review handoffs.
---
# tradebot-phase-gate-audit

## Purpose

Protect TradeBot phase transitions from authority drift, implicit approvals, and premature implementation.

## Global TradeBot Rules

- Follow `AGENTS.md` and `docs/CODEX_EXECUTION_EVIDENCE.md`.
- Resolve volatile branch, phase, gate, provider, and approval facts from Git,
  the active plan, `PROJECT_STATE.md`, and `ROADMAP.md`; do not hardcode them in
  this skill.
- Keep authorization and evidence epochs separate and stop at the exact
  operator-approved boundary.

## Activation Conditions

Use before any phase/gate scoping or status change, roadmap update,
implementation plan, external execution, or review that depends on GO/NO-GO
evidence.

## Must Not Be Used

Do not use to self-approve a phase transition or substitute for operator GO. Do not treat planning approval as implementation approval.

## Required Inputs

- Phase or workstream being reviewed.
- Claimed status and approval evidence.
- Related plan, ADR, roadmap, and project-state references.
- Proposed next action.

## Required Outputs

- Verified phase/workstream status.
- GO/NO-GO evidence summary.
- Missing approval list.
- Next safe action.

## Required Inspection

Read:

- `docs/PROJECT_STATE.md`
- `docs/ROADMAP.md`
- `PLANS.md`
- Relevant ADRs
- Relevant Workstream I documents
- `docs/RISK_POLICY.md`
- `docs/LIVE_TRADING_READINESS.md` when live-capable paths are involved

Run:

```sh
git status --short
git branch --show-current
git rev-parse --short HEAD
```

## Distinctions To Enforce

- Complete is not automatically approved.
- Proposed is not accepted.
- Planning is not implementation.
- Blocked is not authorized.
- Live-capable is not live-enabled.
- Accepted ADR does not authorize source implementation.
- Offline implementation, review, preflight, commit, and exact-artifact proof
  do not authorize provider execution, retry, a later gate, orders, risk
  changes, or live use.

## Procedure

1. Identify the phase, workstream, and proposed transition.
2. Verify entry and exit criteria from roadmap, plans, ADRs, and operator decision.
3. Require explicit GO/NO-GO evidence before any transition.
4. Classify the proposed next action as documentation, scoping, implementation, live-capable, or live.
5. Classify evidence by candidate, staged, committed, exact-commit-build, and
   external-process epoch.
6. Protect the current blocked action and every later phase/gate from implicit activation.
7. Report missing approvals, exhausted attempt budgets, unresolved gates, and required verification.

## Validation Checklist

- Current phase/gate status is sourced from authority documents at invocation time.
- ADR, plan, phase, review, commit, and preflight status are not treated as adjacent action authority.
- Provider/broker-specific implementation or execution is absent unless separately approved.
- Live trading remains disabled unless exact operator approval exists.

## Failure Modes Caught

- Premature phase transition.
- Roadmap or plan status drift.
- ADR acceptance confused with implementation approval.
- Live-capable code treated as live authorization.
- Broker-specific assumptions added during scoping.

## Hard Prohibitions

- Do not activate blocked implementation, provider execution, retry, later
  phases/gates, orders, risk changes, or live work without explicit operator GO.
- Do not alter risk limits, credentials, live mode, order routing, or broker code.
- Do not downgrade blocked status without operator GO.
- Do not stage, commit, push, reset, clean, or discard changes.

## Interaction With Existing Skills

- Run after `tradebot-authority-state-audit` for phase-sensitive work.
- Run before `tradebot-plan-authoring` for phase- or gate-sensitive planning.
- Pair with `tradebot-integration-architecture-review` and `tradebot-risk-review` for Workstream I scoping.
- Feed final gate state into `tradebot-pr-readiness-review` and `tradebot-handoff`.

## Example Invocation Prompt

```text
Use $tradebot-phase-gate-audit to verify the current phase/gate state and whether the proposed next action has exact GO or remains NO-GO.
```

## Stop Conditions

Stop if GO/NO-GO evidence is missing, contradictory, or implies source, broker, credential, risk, or live behavior outside explicit operator approval.

## Reporting Format

```markdown
## Phase-Gate Audit
- Phase/workstream:
- Claimed status:
- Verified status:
- GO/NO-GO evidence:
- Missing approvals:
- Blockers:
- Next safe action:
```

## Authority Documents

- `docs/PROJECT_STATE.md`
- `docs/ROADMAP.md`
- `PLANS.md`
- `docs/decisions/`
- `docs/RISK_POLICY.md`
