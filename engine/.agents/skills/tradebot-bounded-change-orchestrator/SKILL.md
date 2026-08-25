---
name: tradebot-bounded-change-orchestrator
description: Orchestrate a complete, scoped TradeBot repository change through inspection, authority checks, planning, implementation, specialist review, verification, documentation sync, and handoff. Use for multi-file offline maintenance or implementation tasks where Codex should proceed autonomously within a fixed local authorization boundary.
---

# TradeBot Bounded Change Orchestrator

## Purpose

Complete one review-ready TradeBot change with a bounded local loop while
preserving financial, credential, Git-publication, external-provider, and live
authorization gates.

## Shared Operating Contract

- Follow `AGENTS.md` and `docs/CODEX_EXECUTION_EVIDENCE.md`.
- Resolve current branch, phase, gate, provider, plan, and approval facts from
  Git and authoritative project documents at use time.
- Treat inspection, edit, test, stage, commit, push, PR, merge, release,
  provider execution, retry, orders, risk changes, deployment, and live use as
  separate authorizations.

## Required Inputs

- State the objective, acceptance criteria, and out-of-scope boundary.
- State the exact allowed actions and environment.
- Identify any operator approvals already granted.

## Autonomy Budget

- Perform one inspection/planning pass and one focused implementation pass.
- Allow at most two diagnose-and-correct cycles for the same local failed
  check. Preserve the first failure and state the new evidence before rerunning.
- Never spend the local repair budget on external workflow reruns, provider
  attempts, credential use, or another process. Those require exact approval.
- Stop on the third occurrence of the same blocker or when new evidence cannot
  change the next action.

## Procedure

1. Use `tradebot-repo-inspection`, `tradebot-git-safety`, and
   `tradebot-authority-state-audit`. Preserve unrelated and ignored artifacts.
2. Classify the task as documentation, source, test, build/tooling, data,
   architecture, performance, financial, credential, network, or live-capable.
3. Use `tradebot-plan-authoring` when `PLANS.md` requires a plan. Record the
   action/artifact/environment/attempt/stop authorization tuple.
4. Route the task to every applicable specialist skill:
   - C++/CMake/tests: `tradebot-cpp-build-test`.
   - Risk/execution/live/network: `tradebot-risk-review` plus the relevant
     execution or network boundary skill.
   - Replay/L2/performance: the matching replay, L2, performance, and benchmark
     review skills.
   - Architecture/ADR/phase: the matching architecture, ADR, and phase-gate
     skills before mutation.
5. Implement the minimum diff that satisfies the accepted scope. Do not widen
   runtime modes, permissions, dependencies, financial behavior, or external
   access as a convenience.
6. Run targeted checks after each meaningful change, then the required full
   verification. Run multi-build-tree CTest suites sequentially.
7. Use `tradebot-documentation-sync` after behavior, tooling, workflow, skill,
   governance, risk, architecture, or test-command changes.
8. Use `tradebot-repo-hygiene` and `tradebot-pr-readiness-review`. Produce a
   handoff when another actor or session must continue.

## Allowed Actions

- Inspect repository state and non-secret local artifacts.
- Edit task-scoped repository files after required planning and approval gates.
- Run deterministic offline format, build, test, benchmark, and documentation
  checks authorized by the task.
- Create ignored build/test output needed for verification.

## Prohibited Actions

- Do not inspect secret values or modify credential stores.
- Do not trigger workflows, use provider APIs, open broker sessions, retry an
  external process, place orders, alter financial limits, deploy, or enable
  live behavior.
- Do not stage, commit, push, create/merge a PR, publish, tag, or release unless
  the operator separately authorizes that exact action and scope.
- Do not declare a blocked phase active or reuse prior evidence after a
  relevant artifact changes.

## Stop Conditions

Stop and report when authorization is ambiguous, the task reaches a sensitive
boundary, the diff exceeds scope, unrelated changes overlap, the same blocker
repeats three times, tests cannot be made deterministic locally, or required
evidence is unavailable.

## Expected Output

- Scope and authorization boundary.
- Branch/commit/worktree and changed-file classification.
- Design decisions and exact files changed.
- Commands, evidence epoch, pass/fail results, warnings, and skipped checks.
- Residual risk, rollback, remaining work, and prohibited adjacent actions.

## Authority Documents

- `AGENTS.md`
- `PLANS.md`
- `docs/CODEX_EXECUTION_EVIDENCE.md`
- `docs/WORKFLOW.md`
- `docs/RISK_POLICY.md`
- `docs/RELEASE_POLICY.md`
