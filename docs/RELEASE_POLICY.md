# TradeBot Release Policy

## Purpose And Authority

- Purpose: govern commits, pushes, releases, publication, and live-transition gates.
- Authority level: release governance below operator instruction, `AGENTS.md`, ADRs, risk, and architecture.
- Audience: operator, maintainers, Codex, reviewers, and release preparers.

## Current Remediation Release Hold

The Repository Remediation Program is the sole current implementation queue.
While any of WP-0 through WP-8 remains unaccepted:

- package work may be committed or pushed only under its own exact operator
  authorization;
- no program package, passing test, artifact, historical phase, or provider
  gate makes the repository releasable or deployable; and
- software release, executable distribution, deployment, provider execution,
  orders, and live transition remain NO-GO.

After all packages close, release readiness must be assessed separately from
the final exact commit; program completion is not release approval.

## Local Commit Gate

Before a commit:

- Worktree reviewed.
- Scope matches task or plan.
- Unrelated files excluded.
- `git diff --check` passes.
- Relevant tests pass or skipped checks are justified.
- Documentation updates are included.
- Operator explicitly authorizes Codex to commit.

## Push Gate

Before push:

- Local commit history reviewed.
- Remote state considered.
- Connectivity window available.
- No secrets or generated artifacts staged.
- Operator explicitly authorizes push.

## Release Gate

Before release:

- Release scope documented.
- Full tests pass.
- Known warnings and risks documented.
- ADRs and docs current.
- Data and generated outputs reviewed.
- Security review complete.
- Operator approves release.

## Offline Validation Evidence Gate

The manually dispatched offline artifact workflow delivers non-executable
review evidence, not a release or deployment:

- The operator selects one reviewed ref and initiates one workflow dispatch.
- Repository permissions remain read-only; no repository secrets, provider
  targets, deployment environments, or live-capable options are used.
- Governance, default configure/build, and the full sequential CTest suite must
  pass before packaging.
- The evidence bundle records the full commit, toolchain, disabled legacy LIVE
  runtime and provider targets, test status, and built-binary SHA-256, but
  deliberately excludes the live-capable executable. It expires from GitHub
  Actions after 14 days.
- Dispatch authority covers that evidence upload only. It does not authorize
  a tag, GitHub Release, package/container publication, deployment, provider
  process, order, risk change, or live transition.
- A separate operator decision and release artifact are required for any
  promotion or release.

## Live Transition Gate

Any live transition is governed by `RISK_POLICY.md` and `LIVE_TRADING_READINESS.md`. A software release is not live-trading authorization.

## Rollback

Rollback path must be identified for source, docs, configuration, dependencies, and live-capable behavior. Destructive Git operations require explicit operator approval.
