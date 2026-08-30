---
name: tradebot-authority-state-audit
description: Audit authoritative TradeBot project state before documentation sync, phase transitions, ADR updates, PR readiness, or implementation planning. Use when current phase, approval status, branch/commit freshness, roadmap state, ADR status, or documentation authority may be stale or disputed.
---
# tradebot-authority-state-audit

## Purpose

Determine authoritative TradeBot project state before synchronization, phase movement, planning, review, or handoff work.

## Global TradeBot Rules

- Follow `AGENTS.md` and `docs/CODEX_EXECUTION_EVIDENCE.md`.
- Resolve volatile branch, phase, gate, provider, and approval facts from Git,
  the active plan, `PROJECT_STATE.md`, and `ROADMAP.md`; do not hardcode them in
  this skill.
- Keep authorization and evidence epochs separate and stop at the exact
  operator-approved boundary.

## Activation Conditions

Use before `tradebot-documentation-sync`, phase transition review, ADR status review, PR readiness review, implementation planning, or any task that depends on current phase, approval, branch, or commit state.

## Must Not Be Used

Do not use to override `AGENTS.md`, financial safety policy, or explicit operator instruction. Do not invent current state when evidence is stale or missing.

## Required Inputs

- Task objective.
- Claimed current phase, ADR status, or approval state.
- Current branch and commit.
- Documents or plans that will be edited or relied on.

## Required Outputs

- Verified authority-state summary.
- Mismatch classification.
- Blocking contradiction list.
- Next safe action.

## Required Inspection

Read:

- `AGENTS.md`
- `docs/PROJECT_STATE.md`
- `docs/ROADMAP.md`
- `PLANS.md`
- `docs/decisions/README.md`
- Relevant ADR files
- Relevant architecture or Workstream I documents
- Relevant indexes such as `docs/README.md`

Run:

```sh
git status --short
git branch --show-current
git rev-parse --short HEAD
git log --oneline --decorate -n 5
```

## Authority-State Evidence Order

For project-state facts, after respecting explicit operator instruction and `AGENTS.md`, compare evidence in this order:

1. Explicit operator decision.
2. Accepted ADRs.
3. `docs/PROJECT_STATE.md`.
4. `docs/ROADMAP.md`.
5. `PLANS.md`.
6. Architecture and Workstream I documents.
7. README and index documents.
8. Historical notes.

Do not let lower-order history override higher-order current evidence.

## Procedure

1. Record current branch, commit, and worktree state.
2. Extract claimed state from `PROJECT_STATE`, `ROADMAP`, plans, ADR index, and relevant ADRs.
3. Record the action, artifact, environment, attempt budget, stop condition,
   and prohibited adjacent actions in the current authorization.
4. Classify each mismatch as stale-current-state, approval mismatch, index drift, unmarked history, branch/commit staleness, evidence-epoch mismatch, or missing evidence.
5. Confirm the current phase/gate, ADR, broker/provider, credential, risk, and
   live authorization state when relevant.
6. Stop if a safety-sensitive contradiction exists.
7. If only non-blocking staleness exists, report it and proceed only within the scoped task.

## Validation Checklist

- Earlier phase completion is not treated as later-phase authorization.
- Accepted ADRs are not treated as implementation approval.
- `PROJECT_STATE` and `ROADMAP` conflicts are identified before docs sync.
- Branch and commit staleness are reported, not silently copied.
- Live trading remains unauthorized unless exact operator approval exists.

## Failure Modes Caught

- Stale branch or commit claims.
- Current-state claims contradicted by ADR or plan status.
- Phase status drift across docs.
- Index entries that imply missing or unauthorized work.
- Historical notes being reused as current authority.

## Hard Prohibitions

- Do not activate any blocked implementation, provider process, retry, later
  phase/gate, order, risk, or live action without exact operator GO.
- Do not enable live trading or real orders.
- Do not make broker-specific assumptions without plan evidence.
- Do not modify source, tests, credentials, or generated artifacts.
- Do not stage, commit, push, reset, clean, or discard changes.

## Interaction With Existing Skills

- Run after `tradebot-git-safety` and before `tradebot-documentation-sync`.
- Pair with `tradebot-phase-gate-audit` for phase or gate movement.
- Pair with `tradebot-adr-review` before ADR status mutation.
- Feed findings into `tradebot-pr-readiness-review` and `tradebot-handoff`.

## Example Invocation Prompt

```text
Use $tradebot-authority-state-audit to determine the authoritative current phase, ADR, approval, branch, and evidence state before updating TradeBot docs.
```

## Stop Conditions

Stop if current phase/gate, ADR acceptance, action authorization, evidence
epoch, live status, credential handling, or branch safety cannot be established
from inspected evidence.

## Reporting Format

```markdown
## Authority-State Audit
- Branch:
- Commit:
- Claimed state:
- Verified state:
- Mismatches:
- Blocking contradictions:
- Next safe action:
```

## Authority Documents

- `AGENTS.md`
- `docs/PROJECT_STATE.md`
- `docs/ROADMAP.md`
- `PLANS.md`
- `docs/decisions/README.md`
