# TradeBot Review Checklist

## Purpose And Authority

- Purpose: provide a practical checklist for code, docs, risk, data, security, and benchmark reviews.
- Authority level: review aid below risk, architecture, testing, and workflow policy.
- Audience: reviewers, maintainers, Codex review agents, and operator.

## Repository State

- Branch and commit identified.
- Worktree status reviewed.
- Untracked and ignored artifacts understood.
- Scope matches task or plan.
- Authorization names the exact action, artifact, environment, attempt budget, and stop condition.
- Commit, push, publication, provider execution, retry, later gates, orders, risk changes, and live work are not inferred from adjacent approval.
- Candidate, staged, committed, exact-commit-build, and external-process evidence are distinguished.

## Repository Remediation Program

- Program-foundation and package-specific approval are identified.
- Change maps to exactly one of WP-0 through WP-8 in
  `REPOSITORY_REMEDIATION_PROGRAM.md`.
- Predecessor package evidence is accepted or an explicit safe exception is
  recorded.
- Current-source facts were revalidated; diagnosis-baseline claims were not
  copied forward without inspection.
- Scope, exclusions, units, ownership, lifecycle, tests, rollback, stop
  condition, and exact operator approval match the package slice.
- WP-7 evidence and WP-8 documentation synchronization are included.
- No phase, provider gate, next package, order, risk change, deployment, or
  live authority is inferred from package acceptance.

## Source Review

- Change is focused.
- No unrelated refactor.
- Existing subsystem boundaries preserved.
- No generated outputs used as source.
- Error handling fails safely.
- Tests cover changed behavior.
- Sensitive or provider-derived data is cleared from caller, callee, returned,
  container, callback, temporary, and destruction copies where applicable.

## Risk Review

- No live behavior enabled by default.
- No real orders introduced.
- Risk limits not weakened without approval.
- Kill-switch or halt behavior preserved.
- Position sizing and drawdown changes approved.

## Security Review

- No credentials or secret values committed.
- `.env` untouched unless explicitly authorized.
- Logs redacted.
- Dependency changes reviewed.
- Network behavior understood.
- Presence-only preflight does not expose values or authorize provider traffic.
- Evidence templates contain no sensitive value-bearing fields.

## Data Review

- Data provenance documented.
- Generated outputs kept ignored.
- Timestamp units clear.
- Replay compatibility reviewed.
- No account/private data committed.

## Benchmark Review

- Correctness tests pass.
- Command, input size, build mode, compiler, and platform reported.
- Baseline comparison provided.
- No unsupported production or profitability claim.

## Documentation Review

- Authority docs updated where behavior changed.
- ADR added or updated for durable decisions.
- Project state remains current-state only.
- Links and indexes reviewed.
- Skills contain durable procedure rather than copied volatile phase/gate state.
- Changed skills pass the available skill validator.

## Final Evidence

- Commands run.
- Results and warnings.
- Skipped checks and reasons.
- Remaining risks.
- Rollback path.
- Exact commit/artifact hash when required.
- Initial failures and subsequent diagnostic reruns are both reported.
- Cross-build suites were sequential or shared-resource isolation was proven.
- Tracked/index status and relevant ignored artifacts are reported separately.
