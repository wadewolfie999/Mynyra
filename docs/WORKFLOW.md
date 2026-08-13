# TradeBot Work Protocol

## Purpose And Authority

- Purpose: define the end-to-end protocol for tasks, plans, implementation, verification, review, commit preparation, handoff, and halt.
- Authority level: workflow policy below roadmap and above skill-specific instructions.
- Audience: operator, maintainers, Codex, contributors, reviewers, and testers.

## Standard Workflow

1. Task intake: capture objective, scope, risk class, and expected output.
2. Authorization ledger: record exact actions, files/subsystems, artifact,
   environment, attempt budget, stop condition, and explicitly prohibited
   adjacent actions.
3. State inspection: run repository status, inspect relevant docs, inspect source/tests before mutation.
4. Scope definition: identify in-scope and out-of-scope files and behavior.
5. Risk classification: normal, architecture-sensitive, data-sensitive, security-sensitive, financial-sensitive, or live-capable.
6. Planning: use `PLANS.md` when required.
7. Approval: get operator or review-authority approval for nontrivial, sensitive, live-capable, or destructive work.
8. Implementation: edit only in scope and preserve unrelated changes.
9. Candidate verification: run targeted and broad checks appropriate to risk and record the working-tree evidence epoch.
10. Review: prioritize bugs, regressions, missing evidence, safety, docs, and architecture drift.
11. Correction: fix findings or record why unresolved issues remain; rerun affected checks.
12. Commit preparation: if authorized, review status, explicit staged path allowlist, staged diff, and `git diff --cached --check`; do not stage unrelated files.
13. Commit and exact-artifact verification: if authorized, commit locally, confirm tracked/index state, rebuild from that commit, and record the full commit and artifact hash.
14. External action checkpoint: provider traffic, retries, later gates, orders,
    and live actions require separate exact approval after current preconditions
    pass.
15. Push or release: perform only when explicitly instructed; commit authority does not imply either action.
16. Documentation update: update state, architecture, risk, testing, data, ADRs, or plans as needed.
17. Handoff: provide `HANDOFF.md` fields when work continues elsewhere.
18. Professional halt: stop at the authorized evidence boundary when further action needs approval, access, or new scope.

## Small Fix Workflow

- Inspect state and affected files.
- Confirm no plan is required.
- Patch narrowly.
- Run targeted verification.
- Record the evidence epoch; if the patch changes after verification, rerun affected checks.
- Report files changed, commands, results, and residual risk.

## Feature Work Workflow

- Draft or locate an active plan.
- Inspect architecture, risk, tests, and affected boundaries.
- Implement in increments.
- Add or update tests.
- Run full CTest when shared behavior is touched.
- Run suites from different build trees sequentially unless every shared resource is isolated.
- Update docs and handoff.

## Bounded Autonomous Change Workflow

- Use `tradebot-bounded-change-orchestrator` for one scoped offline change that
  spans planning, implementation, specialist review, verification, and docs.
- Budget one implementation pass and no more than two diagnose-and-correct
  cycles for the same local failure mechanism.
- Preserve the first failure and state the new evidence before every rerun.
- Stop before staging, commit, push, workflow dispatch, provider access,
  deployment, orders, risk changes, or live use unless the operator separately
  authorizes the exact action.

## CI Failure Workflow

- Use `tradebot-ci-failure-recovery` to preserve the exact revision, runner,
  first failing command, and relevant redacted diagnostics.
- Classify deterministic repository defects separately from workflow defects,
  toolchain differences, resource contention, and GitHub infrastructure.
- Reproduce the narrowest equivalent check locally before correcting it.
- Never weaken tests, sanitizers, permissions, redaction, or default-off gates
  merely to obtain a green run. Triggering an external rerun is a separate
  operator action.

## Phase Work Workflow

- Confirm phase marker in `ROADMAP.md` and `PROJECT_STATE.md`.
- Use a plan with phase ID.
- Preserve phase gates and validation requirements.
- Record benchmark/test evidence.
- Do not declare phase completion without operator review.

## Architecture Change Workflow

- Inspect existing architecture and ADRs.
- Draft plan and ADR if the decision is durable.
- Identify prohibited coupling and affected tests.
- Implement only after approval.
- Update `ARCHITECTURE.md`, `ROADMAP.md` if phase scope changes, and relevant skills.

## Documentation Change Workflow

- Inspect existing docs and authority order.
- Avoid duplication and invented facts.
- Update indexes.
- Run `git diff --check` and doc audit grep.
- Validate every changed skill with the repository-available skill validator.
- Report skipped link-checking if no local tool exists.

## Research Experiment Workflow

- State objective, dataset, provenance, config, seeds, fees, slippage, and success criteria.
- Keep generated outputs ignored unless approved.
- Avoid production or profitability claims from backtests alone.
- Handoff reproducibility evidence.

## Financial-Sensitive Workflow

- Stop and require explicit operator approval before mutation unless the task is inspection-only.
- Inspect `RISK_POLICY.md`, `LIVE_TRADING_READINESS.md`, and relevant code.
- Create a plan.
- Verify dry-run/paper/sandbox behavior before any live consideration.
- Include kill switch, stale-data, partial-fill, and reconciliation evidence.

## Emergency Containment Workflow

- Stop new risky actions.
- Preserve evidence without exposing secrets.
- Identify branch, commit, worktree, running processes if relevant, and generated outputs.
- Notify operator.
- Do not clean up, revert, or rotate secrets without approval unless the operator has delegated that exact authority.
- Produce a handoff with prohibited next actions.

## Professional Halt Conditions

Halt when:

- Required approval is absent.
- Credentials or live trading are implicated unexpectedly.
- Tests fail in a safety-relevant way.
- Repository state changes unexpectedly and cannot be attributed.
- Evidence is insufficient for a claim.
- Scope would need expansion.
- Network or external access is required but unavailable.
- The attempt budget is exhausted or the next action is a retry, reconnect,
  provider process, staging/commit, publication, later gate, order, risk, or
  live action not explicitly authorized.
