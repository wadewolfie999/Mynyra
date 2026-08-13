# TradeBot Agent Operating Contract

## Purpose And Authority

- Purpose: primary repository instruction contract for Codex and other repository-side agents.
- Authority level: repository-level operating authority, subordinate only to explicit operator instruction for the current task.
- Audience: Codex, human contributors, reviewers, testers, research agents, and future coding agents.
- Related documents: deeper policy lives in `docs/`; reusable Codex execution and evidence rules live in `docs/CODEX_EXECUTION_EVIDENCE.md`; planning rules live in `PLANS.md`; contributor workflow lives in `CONTRIBUTING.md`.

## Repository Identity

- Project name: TradeBot.
- Repository root: `~/TradeBot`.
- Verified local root: `/Users/vaheedgorgeen/TradeBot`.
- CMake project name: `TradeBot-AIIO-Core`.
- Current implementation core: C++20.
- Current build system: CMake 3.14 minimum, verified locally with CMake 4.4.0, Unix Makefiles, Apple clang 21.0.0.
- Financial-sensitive classification: this repository contains live-capable trading-system code and must be treated as financial, credential, and operational-risk sensitive even when a task is documentation-only.

TradeBot is a trading-system research and engineering repository with a deterministic C++ core for market-data replay, strategy execution, portfolio and risk accounting, L2 order-book handling, trigger orders, analytics, metrics, broker/data adapters, tests, and benchmarks. It defaults to non-live operation.

Verified major subsystems:

- `include/SystemConfig.hpp`: runtime mode and shared configuration.
- `include/CsvReader.hpp`, `src/CsvReader.cpp`: candle CSV input.
- `include/LocalDataReplayAdapter.hpp`, `src/LocalDataReplayAdapter.cpp`: local CSV/binary replay ticks.
- `include/L2OrderBook.hpp`, `src/L2OrderBook.cpp`: L2 order book and BBO application.
- `include/IStrategy.hpp`, `SmaCrossStrategy`, `MeanReversionStrategy`: strategy boundary.
- `PortfolioAllocator`, `RegimeDetector`: allocation and regime-aware weighting.
- `PortfolioManager`, `RiskEngine`, `ExecutionEngine`, `TriggerOrderManager`: portfolio, risk, execution, and trigger-order boundaries.
- `LiveDataAdapter`, `BrokerGateway`, `AsyncNetworkClient`, `AuthManager`: live-capable data, broker, network, and credential boundaries.
- `AnalyticsEngine`, `MetricsAggregator`, `LocalMetricsExporter`, `StateSerializer`: result output, metrics, and resume state.

## Current Repository Focus Lock

The sole current implementation focus is the nine-package Repository
Remediation Program defined in `docs/REPOSITORY_REMEDIATION_PROGRAM.md` and
governed by `PLAN-20260813-repository-cohesion-remediation` in `PLANS.md`:

1. WP-0 — Live containment.
2. WP-1 — Persistence and generated-state containment.
3. WP-2 — Accounting and quantity correctness.
4. WP-3 — Risk-state repair.
5. WP-4 — Unified order lifecycle.
6. WP-5 — Runtime-mode and data contracts.
7. WP-6 — Transport and provider integration.
8. WP-7 — CI and observability.
9. WP-8 — Authority synchronization.

The Landing Spot Gate is governance preparation, not a tenth package. The
focus lock is active, but WP-0 through WP-8 implementation remains Planned /
NO-GO until Wade accepts the Landing Spot Gate and separately authorizes the
exact next package action. New findings must map to an existing package or stop
for an explicit scope decision. Historical phase, workstream, ADR, and provider-
gate records remain evidence only and do not pre-empt this queue.

No feature work, provider continuation, later gate, strategy research,
optimization, deployment, or phase activation may proceed outside this focus
without a new explicit operator directive. Completing any package does not
authorize the next package, credentials, provider traffic, orders, risk-limit
changes, release, deployment, or live trading.

## Actor Hierarchy

1. Project owner/operator
   - Final authority for architecture, scope, risk changes, commits, pushes, releases, live-trading transitions, credentials, and external connectivity.
   - Must explicitly approve live-trading unlocks, financial-limit changes, history rewrites, destructive actions, and publication.

2. Strategic AI partner
   - Provides planning, architecture, review, governance, and safety analysis.
   - Does not replace operator approval.

3. Codex
   - Repository-side inspection and implementation agent.
   - May inspect, plan, edit, test, and report within assigned scope.
   - Must not silently expand scope, reinterpret project authority, or treat chat history as stronger than current operator instruction plus repository authority.

4. Other human or AI actors
   - Includes contributors, reviewers, testers, research agents, local/offline models, and future coding agents.
   - Repository access is not automatic trust. Actors must follow declared permissions, risk controls, documentation authority, and handoff requirements.

See `docs/ACTORS.md` for the full actor model.

## Authority Order

Use this order when instructions conflict:

1. Explicit operator instruction for the current task.
2. This `AGENTS.md`.
3. Accepted ADRs in `docs/decisions/`.
4. `docs/RISK_POLICY.md`.
5. `docs/ARCHITECTURE.md`.
6. Active approved plan under the `PLANS.md` schema.
7. `docs/PROJECT_STATE.md`.
8. `docs/ROADMAP.md`.
9. `docs/WORKFLOW.md`.
10. `docs/CODEX_EXECUTION_EVIDENCE.md`.
11. Skill-specific instructions in `.agents/skills/*/SKILL.md`.
12. `CONTRIBUTING.md`.
13. General documentation.

No document may silently override financial safety or operator authority. Conflicts involving live execution, credentials, order routing, risk limits, generated-data provenance, or architectural boundaries must be reported and halted until the operator resolves them.

## Codex Operating Rules

Codex must:

- Inspect before mutation.
- Identify scope, risk classification, current branch, and worktree status before editing.
- Preserve unrelated work, including user changes and generated outputs.
- Make focused changes aligned with the assigned scope.
- Prefer existing repository patterns over new abstractions.
- Update documentation when behavior, architecture, workflows, risk posture, or verification expectations change.
- Run appropriate verification and report exact commands and outcomes.
- Report exact files changed.
- Report failures, skipped checks, warnings, and limitations honestly.
- Avoid undocumented assumptions; record necessary assumptions in plans or handoffs.
- Stop safely when required evidence is unavailable or a safety-sensitive ambiguity cannot be resolved locally.
- Treat authorization as exact-action, scope, artifact, environment, attempt-budget, and stop-condition specific. Authorization is not transitive across review, correction, staging, commit, push, provider execution, retry, later gates, orders, risk changes, or live work.
- Separate working-tree, staged-index, committed-tree, exact-commit-build, and external-process evidence. Re-run affected checks when the relevant artifact changes.
- Diagnose before retrying. Preserve the initial failure and explain what changed before a rerun.
- Treat local ports, certificate authorities, fixed temporary paths, processes, caches, and generated outputs as shared resources. Run multi-build-tree CTest suites sequentially unless isolation is proven.
- Protect every sensitive or provider-derived memory copy, including caller, callee, return-value, container, callback, and destruction paths.

## Hard Prohibitions

Codex and other repository-side agents must not:

- Expose, print, copy, or modify secrets.
- Edit `.env` files, credentials, key material, account identifiers, or secret stores unless explicitly authorized for that exact action.
- Introduce live-trading behavior.
- Enable real orders.
- Weaken dry-run or backtest defaults.
- Alter financial risk limits, position sizing, drawdown gates, kill-switch behavior, order-routing behavior, or credential handling without explicit operator approval.
- Commit, push, tag, release, or publish unless explicitly instructed.
- Rewrite Git history unless explicitly approved.
- Delete unrelated work.
- Treat generated outputs as source material.
- Fabricate successful test results or hide failures.
- Claim profitability, production readiness, safety, or live readiness from backtests alone.
- Invent architectural facts.
- Silently bypass roadmap, plan, risk, or live-trading gates.
- Make broad refactors during narrowly scoped work.
- Reuse a one-process or one-attempt external authorization for a retry, reconnect, second process, or later gate.
- Treat preflight success, credential presence, a commit, an exact binary hash, or successful offline tests as external-provider or live authorization.

## Verified Repository Map

- C++ headers: `include/`.
- C++ source: `src/`.
- C++ tests: `tests/`.
- Benchmarks: `src/benchmarks/`.
- Documentation: `docs/`.
- ADRs: `docs/decisions/`.
- Sample data directory: `data/samples/` exists and is currently empty.
- Historical data path referenced by benchmark code: `data/historical/`.
- Generated result path: `data/results/`; current `.gitignore` ignores CSV and
  binary artifacts by extension but does not ignore every file in the
  directory. Exact containment is owned by WP-1.
- Generated archive path: `data/archive/`; current `.gitignore` ignores CSV and
  binary artifacts by extension but does not ignore every file in the
  directory. Exact containment is owned by WP-1.
- Build directory: `build/`, ignored by Git.
- Phase 19 generated logs/replay artifacts observed under `build/phase19_revalidation/`.
- Codex skills: `.agents/skills/`.
- Codex execution/evidence contract: `docs/CODEX_EXECUTION_EVIDENCE.md`.
- Python runtime components: none. A standard-library-only automation validator
  is tracked under `scripts/`.
- Julia components: none tracked at the time this contract was created.
- Scripts/tooling: tracked local wrappers, offline CI/sanitizer helpers, an
  offline policy checker, skill/workflow guardrails, and non-executable
  exact-build evidence packaging exist under `scripts/`, with
  `.githooks/pre-push` and four GitHub Actions workflows under
  `.github/workflows/`. Validation and evidence delivery retain read-only
  repository permissions; CodeQL adds only `security-events: write`. CMake
  remains the underlying build and test entrypoint.

## Verified Commands

Run from `~/TradeBot`.

Configure:

```sh
cmake -S . -B build
```

Validation wrapper configure:

```sh
./scripts/configure.sh
```

Build:

```sh
cmake --build build
```

Validation wrapper build:

```sh
./scripts/build.sh
```

Full CTest suite:

```sh
ctest --test-dir build --output-on-failure
```

Run full suites from multiple build trees sequentially unless their ports,
local certificate authority, temporary paths, processes, and other shared
resources are proven isolated.

Validation wrapper test:

```sh
./scripts/test.sh
```

Offline CI policy:

```sh
./scripts/ci_policy_checks.sh
```

Validate repository skills and workflow safety guardrails:

```sh
python3 scripts/validate_automation.py
```

Run the same default-off configure/build/sequential-test path used by ordinary
CI:

```sh
./scripts/ci_validate.sh build/ci-validation
```

Run the scheduled default-off ASan/UBSan path:

```sh
./scripts/ci_deep_validate.sh build/ci-deep-validation
```

Targeted phase test:

```sh
ctest --test-dir build -R phase18_tests --output-on-failure
```

Targeted build:

```sh
cmake --build build --target phase18_tests
cmake --build build --target apply_bbo_microbench
```

Phase 19 BBO microbenchmark:

```sh
build/apply_bbo_microbench 10000
```

Repository hygiene:

```sh
git status --short
git diff --check
git diff --stat
git diff --name-status
```

Documentation audit commands:

```sh
find .agents/skills -name SKILL.md -print | sort
find docs -maxdepth 3 -type f -print | sort
grep -RInE 'TO''DO|TB''D|FIX''ME|PLACE''HOLDER|example ''only' AGENTS.md PLANS.md CONTRIBUTING.md docs .agents 2>/dev/null || true
grep -RInE 'live trading|live-trading|real order|API key|credential|secret' AGENTS.md PLANS.md CONTRIBUTING.md docs .agents 2>/dev/null || true
```

Verified local warnings during `cmake --build build`:

- `src/AsyncNetworkClient.cpp`: unused `SSL_ERROR_NONE`.
- `src/RiskEngine.cpp`: `totalPositioned` set but not used.

No configured formatter, static analyzer, Markdown linter, or Markdown link checker was found locally when this contract was written. `clang-format`, `clang-tidy`, `markdown-link-check`, `lychee`, and `markdownlint` were not available.

Ubuntu compute-node commands are not verified in this workspace. Use the same CMake/CTest commands only after confirming compiler, CMake, and dependency availability on that host.

## Runtime Modes

Verified code modes in `SystemConfig`:

- `BACKTEST`: default deterministic CSV-driven path.
- `PAPER`: live-data-like adapter path with simulated local broker behavior.
- `LIVE`: live data plus live-capable broker execution path.

Governance vocabulary:

- Simulation: offline or deterministic run with no external trading side effects. Usually `BACKTEST`.
- Dry-run: non-live validation where actions are recorded or simulated, not sent as real orders.
- Paper trading: `PAPER` mode or equivalent simulated execution.
- Sandbox: an external test venue or broker sandbox; not a distinct verified code flag.
- Live trading: any workflow capable of real orders, external account impact, or live venue state mutation.

TradeBot defaults to `BACKTEST` or dry-run behavior. Live trading is prohibited unless explicitly authorized by the operator and all requirements in `docs/LIVE_TRADING_READINESS.md` and `docs/RISK_POLICY.md` are met.

## Documentation Authority Map

- Codex instructions: `AGENTS.md`.
- Planning system: `PLANS.md`.
- Current remediation program and package gates:
  `docs/REPOSITORY_REMEDIATION_PROGRAM.md`.
- Current state: `docs/PROJECT_STATE.md`.
- Roadmap and phase gates: `docs/ROADMAP.md`.
- Architecture: `docs/ARCHITECTURE.md`.
- Financial and operational risk: `docs/RISK_POLICY.md`.
- Testing contract: `docs/TESTING.md`.
- Data governance: `docs/DATA_POLICY.md`.
- Security and credentials: `docs/SECURITY.md`.
- Actor permissions: `docs/ACTORS.md`.
- Work protocol: `docs/WORKFLOW.md`.
- Codex execution and evidence: `docs/CODEX_EXECUTION_EVIDENCE.md`.
- Handoffs: `docs/HANDOFF.md`.
- ADRs: `docs/decisions/`.
- Contribution workflow: `CONTRIBUTING.md`.
- Reusable Codex workflows: `.agents/skills/*/SKILL.md`.

## Definition Of Done

Documentation-only changes:

- Accurate to verified repository state.
- No invented facts, stale paths, or unsafe live-trading implications.
- Relevant indexes updated.
- `git diff --check` passes.
- Documentation audit grep commands reviewed.
- Every changed repository skill passes the skill validator when available.

Source changes:

- Scope is recorded.
- Relevant tests are run.
- Documentation is updated when behavior, interfaces, risk posture, or architecture changes.
- Warnings, skipped tests, and residual risk are reported.

Behavior changes:

- Acceptance criteria are defined in a plan unless the change is small and low risk.
- Existing regression tests pass.
- New or adjusted tests cover the changed behavior.
- Rollback path is documented.

Architecture changes:

- `docs/ARCHITECTURE.md` is updated.
- An ADR is created or updated when the decision should remain stable across future work.
- Risk and testing impacts are reviewed.

Performance changes:

- Benchmark command, environment, input size, and output are reported.
- No performance claim is made without comparative evidence.
- Generated benchmark outputs remain out of Git unless intentionally versioned.

Financial-sensitive changes:

- Operator approval is required before implementation.
- `docs/RISK_POLICY.md` and `docs/LIVE_TRADING_READINESS.md` gates are checked.
- Tests must prove fail-safe behavior for affected paths.
- Handoff must state prohibited next actions and required approvals.

Repository Remediation Program changes:

- Work maps to exactly one of WP-0 through WP-8 and satisfies that package's
  entry, acceptance, test, rollback, and authorization requirements.
- WP-7 evidence and WP-8 authority synchronization close with each accepted
  package.
- The next package remains NO-GO until separately authorized.

## Reporting Contract

Every Codex task report must include:

- Scope received.
- Repository state found: branch, commit, worktree status, relevant ignored artifacts.
- Files inspected.
- Files changed.
- Design decisions.
- Commands run.
- Test results.
- Failures, warnings, skipped checks, or limitations.
- Risks.
- Remaining work.
- Git diff summary.
- Exact authorization boundary and prohibited adjacent actions when the task is sensitive or externally capable.
- Evidence epoch and exact commit/artifact identity when completion depends on a committed build or external process.

For long-running or interrupted tasks, create or update a handoff using `docs/HANDOFF.md`.
