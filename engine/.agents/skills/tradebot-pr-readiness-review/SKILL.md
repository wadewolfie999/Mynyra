---
name: tradebot-pr-readiness-review
description: Review whether a TradeBot branch is ready for PR or merge handoff by checking branch state, changed files, validation evidence, evidence epochs, exact artifacts, documentation consistency, residual risk, rollback notes, and phase/gate or live-authorization safety.
---
# tradebot-pr-readiness-review

## Purpose

Determine whether a branch has enough evidence, scope control, and risk clarity for PR or merge handoff.

## Global TradeBot Rules

- Follow `AGENTS.md` and `docs/CODEX_EXECUTION_EVIDENCE.md`.
- Resolve volatile branch, phase, gate, provider, and approval facts from Git,
  the active plan, `PROJECT_STATE.md`, and `ROADMAP.md`; do not hardcode them in
  this skill.
- Keep authorization and evidence epochs separate and stop at the exact
  operator-approved boundary.

## Activation Conditions

Use before PR creation, merge handoff, commit-readiness claims, review transfer, or final multi-skill workflow report.

## Must Not Be Used

Do not use as a substitute for code review, risk review, replay validation, L2
review, or build/test verification. Do not approve readiness if blocked
implementation, provider execution, retry, later-gate, order, risk, or live
authorization is implied without explicit operator GO.

## Required Inputs

- Branch name and commit.
- Changed files and intended scope.
- Validation commands and results.
- Documentation and risk-review evidence.
- Rollback notes.

## Required Outputs

- Ready/not-ready status.
- Changed-file scope summary.
- Validation evidence summary.
- Residual risk and rollback summary.

## Required Inspection

Run:

```sh
git status --short
git status --short --ignored
git branch --show-current
git rev-parse --short HEAD
git diff --stat
git diff --name-status
git diff --cached --name-status
git diff --check
```

Read relevant docs, active plan, ADRs, and review outputs for changed areas.

## Procedure

1. Confirm the branch is not `main`.
2. Check worktree and staging status.
3. Confirm changed files match the stated scope.
4. Verify required validation evidence exists and covers the changed behavior.
5. Confirm each result names its evidence epoch and invalidate candidate results
   superseded by later relevant edits.
6. When exact-artifact evidence is required, verify full commit, clean
   tracked/index state, rebuild configuration, artifact path/hash, and test results.
7. Confirm documentation and indexes are consistent.
8. Confirm residual risk, rollback path, required approvals, attempt budget,
   and prohibited adjacent actions are stated.
9. Fail readiness if blocked implementation, provider execution, retry,
   later-gate, order, risk, or live behavior is implied without approval.

## Validation Checklist

- Branch is not `main`.
- No staged or unstaged surprises.
- Expected files only.
- `git diff --check` passes.
- Required tests or docs audits are reported.
- Risk summary and rollback notes are concise and specific.
- Initial failures and diagnostic reruns are both reported.
- Multi-build suites ran sequentially or shared-resource isolation is proven.

## Failure Modes Caught

- Dirty or wrong branch.
- Missing validation evidence.
- Untracked generated artifacts confusing review.
- Docs/index drift.
- Blocked provider/broker, retry, later-gate, order, risk, or live authorization implied by wording.
- PR readiness claimed before required specialist review.

## Hard Prohibitions

- Do not stage, commit, push, merge, reset, clean, or discard changes.
- Do not approve readiness for source changes without relevant build/test evidence.
- Do not approve readiness if secrets, credentials, live trading, or real orders are ambiguous.
- Do not treat this skill as implementation authorization.

## Interaction With Existing Skills

- Run after relevant code, risk, replay, L2, performance, benchmark, ADR, and documentation reviews.
- Use `tradebot-git-safety` and `tradebot-repo-hygiene` first for worktree safety.
- Use `tradebot-authority-state-audit` and `tradebot-phase-gate-audit` for phase-sensitive readiness.
- Finish with `tradebot-handoff` when another actor resumes.

## Example Invocation Prompt

```text
Use $tradebot-pr-readiness-review to decide whether this TradeBot docs branch is ready for PR handoff.
```

## Stop Conditions

Stop if branch state is unsafe, validation is stale or missing, changed files
exceed scope, exact-artifact identity is unsupported, blocked external/later-
gate behavior is implied active, live authorization is ambiguous, or rollback
is not documented.

## Reporting Format

```markdown
## PR Readiness Review
- Branch:
- Commit:
- Changed files:
- Validation evidence:
- Documentation consistency:
- Residual risk:
- Rollback:
- Status:
```

## Authority Documents

- `AGENTS.md`
- `docs/RELEASE_POLICY.md`
- `docs/WORKFLOW.md`
- `docs/REVIEW_CHECKLIST.md`
- `docs/HANDOFF.md`
