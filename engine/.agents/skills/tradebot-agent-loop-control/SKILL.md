---
name: tradebot-agent-loop-control
description: Prevent repeated TradeBot agentic loops by forcing loop-risk identification, prior-attempt review, bounded execution, invariants, and explicit stop conditions. Use when agents repeat broad rewrites, re-audit the same state, chase stale docs, or continue without new evidence.
---
# tradebot-agent-loop-control

## Purpose

Break unproductive agent loops and force one bounded, evidence-producing next action.

## Global TradeBot Rules

- Follow `AGENTS.md` and `docs/CODEX_EXECUTION_EVIDENCE.md`.
- Resolve volatile branch, phase, gate, provider, and approval facts from Git,
  the active plan, `PROJECT_STATE.md`, and `ROADMAP.md`; do not hardcode them in
  this skill.
- Keep authorization and evidence epochs separate and stop at the exact
  operator-approved boundary.

## Activation Conditions

Use when a workflow repeats inspections or rewrites without new evidence, cycles between docs, broadens scope after failures, or cannot identify a concrete invariant.

## Must Not Be Used

Do not use to skip required verification, ignore failures, or rush through safety-sensitive ambiguity.

## Required Inputs

- Current objective.
- Prior attempts and their outcomes.
- Suspected loop pattern.
- Missing evidence or invariant.
- Proposed bounded next action.
- Remaining attempt/retry budget and the authorization covering the action.

## Required Outputs

- Named loop pattern.
- Missing invariant or evidence.
- Single bounded next action.
- Explicit stop condition and result.

## Required Inspection

Run:

```sh
git status --short
git branch --show-current
git diff --name-status
```

Read the most relevant authority doc, active plan, previous handoff, or failed validation output that explains the repeated loop.

## Procedure

1. Name the loop pattern.
2. List prior attempts and what evidence they added.
3. Identify the missing invariant or decision.
4. Choose one bounded next action that can change knowledge or state.
5. Define a concrete stop condition before taking the action.
6. Confirm the action changes a hypothesis, input, code, environment, or
   precondition; an unchanged retry is not evidence-producing.
7. Stop and report if the bounded action fails to add new evidence or consumes
   the available attempt budget.

## Validation Checklist

- Broad rewrite is rejected unless new evidence justifies it.
- Documentation work starts with authority docs, not lower-order summaries.
- Only one bounded next action is active after loop detection.
- Stop condition is explicit before continuing.
- Current authority is re-read and no adjacent implementation, external,
  credential, publication, order, risk, or live action is inferred.

## Failure Modes Caught

- Repeating audits without decision output.
- Rewriting docs to hide uncertainty.
- Chasing generated artifacts or stale indexes as source truth.
- Reattempting failed validation without a changed hypothesis.
- Expanding from governance work into implementation.

## Hard Prohibitions

- Do not repeat broad rewrites without new evidence.
- Do not modify source, tests, credentials, broker code, or generated artifacts unless explicitly authorized by a later task.
- Do not turn a bounded diagnostic action into retry, reconnect, provider
  traffic, later-gate work, publication, orders, risk changes, or live work.
- Do not stage, commit, push, reset, clean, or discard changes.

## Interaction With Existing Skills

- Use before re-running long multi-skill workflows after repeated failure.
- Run `tradebot-git-safety` after loop containment and before edits.
- Run `tradebot-authority-state-audit` before documentation corrections.
- End unresolved loops with `tradebot-handoff`.

## Example Invocation Prompt

```text
Use $tradebot-agent-loop-control to stop repeated TradeBot docs rewrites and choose one bounded corrective action.
```

## Stop Conditions

Stop if no bounded action can be named, the same missing evidence remains, the
attempt budget is exhausted, or progress requires an unauthorized mutation or
external action.

## Reporting Format

```markdown
## Agent Loop Control
- Loop pattern:
- Prior attempts:
- Missing invariant:
- Bounded next action:
- Stop condition:
- Result:
- Next safe action:
```

## Authority Documents

- `AGENTS.md`
- `docs/WORKFLOW.md`
- `docs/HANDOFF.md`
- `PLANS.md`
