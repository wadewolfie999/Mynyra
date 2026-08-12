---
name: tradebot-plan-authoring
description: Draft, review, or update implementation plans using the TradeBot planning system. Use before nontrivial source changes, architecture changes, financial-sensitive work, data schema changes, dependency changes, performance work, or multi-session work.
---
# tradebot-plan-authoring

## Purpose

Draft, review, or update implementation plans using the TradeBot planning system.

## Shared Operating Contract

- Follow `AGENTS.md` and `docs/CODEX_EXECUTION_EVIDENCE.md`.
- Resolve volatile branch, phase, gate, provider, and approval facts from Git,
  the active plan, `PROJECT_STATE.md`, and `ROADMAP.md`.
- Keep authorization and evidence epochs separate and stop at the exact
  operator-approved boundary.

## Activation Conditions

Use before nontrivial source changes, architecture changes, financial-sensitive work, data schema changes, dependency changes, performance work, or multi-session work.

## Invocation Order

Run `tradebot-authority-state-audit` before phase-sensitive planning. Run
`tradebot-phase-gate-audit` before phase/gate scoping or external execution,
and do not draft blocked provider/broker implementation steps as executable
without exact operator GO.

## Must Not Be Used

Do not use to bypass operator approval. Do not create plans for trivial direct patches where `PLANS.md` says a direct patch is acceptable.

## Required Inputs

- Objective.
- Scope boundaries.
- Active phase or roadmap relation.
- Known constraints.
- Review authority.

## Required Repository Inspection

Read:

- `AGENTS.md`
- `PLANS.md`
- `docs/PROJECT_STATE.md`
- `docs/ROADMAP.md`
- `docs/RISK_POLICY.md`
- Relevant ADRs and affected docs/source/tests.

Run:

```sh
git status --short
git branch --show-current
git log --oneline --decorate -n 5
```

## Procedure

1. Determine whether a plan is required.
2. Assign a plan ID shaped `PLAN-YYYYMMDD-short-topic`.
3. Fill every section of the `PLANS.md` schema.
4. Record assumptions and invariants.
5. Add an authorization boundary naming exact actions, artifacts, environment,
   attempt budget, stop condition, and prohibited adjacent actions.
6. Define evidence epochs for candidate review, staged index, commit,
   exact-commit rebuild, external process, and acceptance when applicable.
7. Identify shared ports, certificate authorities, temporary paths, processes,
   caches, and generated outputs; require sequential cross-build validation
   unless isolation is proven.
8. Define acceptance criteria, verification, rollback, risks, and approval gates.
9. Identify documentation and ADR updates.
10. Mark state as Draft or Proposed until approved.

## Related Skills

- Use `tradebot-integration-architecture-review` for Workstream I or adapter architecture planning.
- Use `tradebot-risk-review` for financial-sensitive, live-capable, order-execution, market-data, or credential planning.
- Use `tradebot-performance-review` and `tradebot-benchmark-review` for performance or benchmark-driven plans.

## Allowed Mutations

Create or update Markdown plan material when the task asks for planning or documentation. No source mutation.

## Prohibited Mutations

No source edits, live-capable configuration changes, commits, pushes, credential edits, or generated-data mutation.

## Verification Commands

```sh
git diff --check
```

For plan-only work, no CMake build is required unless plan content changes verified commands.

## Expected Outputs

- Complete plan with objective, scope, implementation steps, verification, rollback, and risks.
- Explicit approvals required.
- Exact action/attempt/stop boundary and prohibited adjacent actions.
- Evidence-epoch and shared-resource strategy.
- Clear resumption and closure rules.

## Failure Behavior

If scope or approval authority is unclear and cannot be discovered, stop and ask the operator.

## Reporting Format

```markdown
## Plan Summary
- Plan ID:
- Status:
- Objective:
- Scope:
- Out of scope:
- Verification:
- Required approvals:
- Risks:
```

## Authority Documents

- `PLANS.md`
- `docs/WORKFLOW.md`
- `docs/ROADMAP.md`
- `docs/decisions/README.md`
