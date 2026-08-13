# Contributing To TradeBot

## Purpose And Authority

- Purpose: contributor workflow for humans and AI implementation agents.
- Authority level: workflow guidance below `AGENTS.md`, accepted ADRs, risk policy, architecture, and active approved plans.
- Audience: contributors, maintainers, Codex, reviewers, testers, and research agents.
- Related documents: `AGENTS.md`, `PLANS.md`, `docs/WORKFLOW.md`, `docs/TESTING.md`, `docs/RISK_POLICY.md`, and `docs/SECURITY.md`.

## Development Environments

Verified local environment:

- macOS workspace at `~/TradeBot`.
- CMake 4.4.0.
- Apple clang 21.0.0.
- CMake generator: Unix Makefiles.
- Build cache: Release.

Expected but not verified here:

- Ubuntu or other Linux compute node with a C++20 compiler, CMake, and pthread support.

Do not assume package-manager, registry, or GitHub access is available. The project currently documents intermittent global connectivity and offline-first development.

## Repository Setup

```sh
cd ~/TradeBot
git status --short
git branch --show-current
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Do not edit secrets, `.env` files, generated outputs, or build artifacts as part of setup.

## Branches And Worktrees

- Use focused branches named by phase or topic, such as `phase/workstream-i-alignment` or `docs/governance-system`.
- Keep source/build separation: generated files belong under ignored paths such as `build/` or `data/results/`.
- Use additional worktrees only when the operator or maintainer wants parallel branches. Record the worktree path in handoffs.

## Dependencies

- CMake is the active build entrypoint.
- No tracked Python or Julia components are currently present.
- Do not add dependencies without a plan and dependency review.
- Network-dependent installs should be grouped into planned connectivity windows.
- See `docs/DEPENDENCY_POLICY.md`.

## Build And Test Procedure

Configure:

```sh
cmake -S . -B build
```

Build:

```sh
cmake --build build
```

Full test suite:

```sh
ctest --test-dir build --output-on-failure
```

Offline policy check:

```sh
./scripts/ci_policy_checks.sh
```

Targeted test:

```sh
ctest --test-dir build -R phase18_tests --output-on-failure
```

Benchmark commands are evidence-gathering tools, not tests unless the task explicitly defines benchmark acceptance criteria:

```sh
build/apply_bbo_microbench 10000
```

`throughput_bench` and `phase18_burnin` write generated CSVs under `data/results/`.

## Automated Validation

Pull requests and pushes to `main` run:

- repository and workflow boundary checks;
- the default-disabled C++ build and full CTest suite with GCC and Clang;
- the default core under ASan and UBSan;
- CodeQL C++ security analysis.

The workflows use non-persisted checkout credentials and least-privilege
permissions; only CodeQL can write security-analysis results. They do not
enable Gate 6 or Gate 7 proof targets, access repository secrets, contact
provider endpoints, perform OAuth, or execute orders.

`Offline Artifact Delivery` is manually dispatched and uploads only a CTest
log, license, manifest, and checksums. It deliberately excludes the
live-capable executable and does not create a release, deploy software, enable
a runtime mode, or authorize external/provider execution.

## Formatting And Linting

No repository-configured formatter or static analyzer was found when this guide was created. `clang-format` and `clang-tidy` were not available locally.

Until tooling is added:

- Keep C++ style consistent with nearby files.
- Avoid broad style-only churn.
- Use compiler warnings from `cmake --build build` as baseline evidence.
- Do not claim formatting or static analysis was run unless it actually was.

## Commits

- Keep commits focused.
- Do not stage unrelated files.
- Do not commit generated outputs unless intentionally versioned and approved.
- Do not commit credentials, private keys, `.env` files, account identifiers, logs containing secrets, or local machine paths.
- Commit messages should identify the subsystem or document and the purpose, for example `docs: add TradeBot governance system` or `phase19: refine applyBbo benchmark evidence`.
- Codex must not commit unless explicitly instructed by the operator.
- Commit authorization does not include push, PR creation, publication, merge,
  release, deployment, provider traffic, or later-gate work unless those
  actions are separately named.
- When an exact-commit artifact is required, rebuild after commit and record
  the full commit SHA and artifact SHA-256. Label pre-commit binaries as
  candidates, not exact-commit evidence.

## Documentation Updates

Update documentation when changing:

- Runtime modes or configuration.
- Architecture or subsystem boundaries.
- Risk controls, order execution, credential handling, or live-capable behavior.
- Test commands or expected evidence.
- Data schemas, result outputs, replay behavior, timestamps, or provenance.
- Benchmarks or performance claims.

Use ADRs for decisions expected to remain stable across future work. Do not create retrospective ADRs for decisions that cannot be verified.

## Generated Files And Data

- `build/` is generated and ignored.
- `data/results/` is generated and ignored.
- `data/archive/` is ignored.
- `data/samples/` exists but was empty when this guide was created.
- `data/historical/` is referenced by benchmark code but was not present as a tracked directory at creation time.
- Large files require operator review before Git tracking.
- See `docs/DATA_POLICY.md`.

## Pull Requests And Reviews

A reviewable change should include:

- Scope and risk classification.
- Files changed.
- Commands run.
- Test or benchmark results.
- Known warnings and skipped checks.
- Documentation updates.
- Rollback path for nontrivial changes.
- Handoff if work may continue in another session.

Reviewers should prioritize bugs, safety regressions, data leakage, lookahead bias, mode confusion, credential exposure, stale documentation, and unsupported performance claims.

Run full CTest suites from separate build trees sequentially unless fixed
ports, local certificate authorities, temporary paths, processes, and other
shared resources are proven isolated. Preserve and report any initial
contention failure before the passing sequential rerun.

## Repository Automation

- Run `python3 scripts/validate_automation.py` after changing any repository
  skill or GitHub Actions workflow.
- Run `./scripts/ci_validate.sh build/ci-validation` to reproduce the ordinary
  GitHub Actions path locally. It forces Gate 6 and Gate 7 OFF and executes the
  full default CTest suite sequentially.
- Use `./scripts/ci_deep_validate.sh build/ci-deep-validation` for the default
  ASan/UBSan path used by the scheduled deep workflow.
- Treat workflow dispatch, rerun, artifact upload, release, deployment, and
  live use as separate operator-approved actions.
- The manual artifact-delivery workflow uploads a short-lived candidate with
  an exact-commit manifest, CTest log, and checksums. It deliberately excludes
  the live-capable executable; it does not create a release, deploy, connect to
  a provider, or authorize orders or live trading.

## Security-Sensitive Changes

Security-sensitive changes include credentials, `.env` handling, network clients, TLS, API signing, shell commands, logging, dependency changes, and remote services.

Requirements:

- Follow `docs/SECURITY.md`.
- Use least privilege.
- Redact logs.
- Avoid printing secret values.
- Require operator approval for credential access or rotation.

## Financial-Sensitive Changes

Financial-sensitive changes include runtime mode handling, live-capable adapters, broker gateway behavior, execution logic, order sizing, risk gates, drawdown limits, kill switch behavior, reconciliation, and market-data integrity checks.

Requirements:

- Follow `docs/RISK_POLICY.md` and `docs/LIVE_TRADING_READINESS.md`.
- Default to `BACKTEST`, dry-run, or paper behavior.
- Never infer live-trading authorization.
- Require operator approval before any live-capable change that could affect real orders.

## Prohibited Contributions

Do not contribute changes that:

- Hardcode secrets.
- Enable live orders by default.
- Remove dry-run or risk gates.
- Hide test failures.
- Treat backtests as proof of profitability.
- Add unreviewed dependencies.
- Mix generated artifacts into source changes.
- Rewrite architecture boundaries without ADR or approval.
- Recreate the deprecated MOP/MOR/SS workflow.

## Handoff Expectations

When stopping mid-task or handing to another actor, provide the fields in `docs/HANDOFF.md`: branch, commit, worktree status, active plan, objective, changed files, commands run, results, unresolved questions, risks, next exact action, safe stopping condition, prohibited next actions, and required approvals.
