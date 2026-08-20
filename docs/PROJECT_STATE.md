# TradeBot Project State

## Purpose And Authority

- Purpose: authoritative current-state summary for TradeBot.
- Authority level: current-state evidence below active approved plans and above the roadmap; the project workstream map is defined in `WORKSTREAM_ARCHITECTURE.md`, and phase definitions and statuses are delegated to `ROADMAP.md`.
- Audience: operator, maintainers, Codex, contributors, reviewers, and handoff recipients.
- Last repository-wide diagnosis: 2026-08-13 against merged `main` commit
  `7f89c597d3874e5c8782a49a31d42335c8bf1e17`; the diagnosis is the baseline
  for the Repository Remediation Program, not proof of the current candidate
  tree after later edits.
- Last CMake/CTest verification evidence: 2026-08-14 unstaged WP-2 working-tree
  candidate based on `179817972cf469c67384f5f8c2245ecf09044621`; ordinary
  default-off and ASan/UBSan suites passed 14/14 sequentially. This is not
  exact-commit evidence.
- Codex execution/evidence governance is centralized in
  `CODEX_EXECUTION_EVIDENCE.md`; skills carry durable domain procedure and must
  resolve volatile branch, phase, gate, provider, and approval state at use
  time.

This document represents current state only. Historical execution belongs in Git commits, pull requests, issues, ADRs, and handoffs.

## Repository State

- Repository root: `/Users/vaheedgorgeen/TradeBot`.
- Branch, HEAD, and worktree values are runtime metadata. Inspect Git directly; historical snapshots are not current-state authority.
- The merged tree contains Phase 19 revalidation history, infrastructure validation, approved Workstream I artifacts, accepted ADR 0003, and the TradeBot skill-system expansion.
- Relevant ignored artifacts observed at the diagnostic baseline included
  `.DS_Store`, `.env.example2` (not opened), `build/`, `data/`, `handoffs/`,
  `output/`, `src/.DS_Store`, and `tmp/`. Ignore status alone is not evidence
  that every generated filename is contained.

## Current Repository Remediation Program

Wade has locked current implementation focus to the nine work packages in
`REPOSITORY_REMEDIATION_PROGRAM.md`, governed by
`PLAN-20260813-repository-cohesion-remediation`:

- WP-0 Live containment.
- WP-1 Persistence and generated-state containment.
- WP-2 Accounting and quantity correctness.
- WP-3 Risk-state repair.
- WP-4 Unified order lifecycle.
- WP-5 Runtime-mode and data contracts.
- WP-6 Transport and provider integration.
- WP-7 CI and observability.
- WP-8 Authority synchronization.

Current status: focus lock and foundational governance approved by Wade on
2026-08-14. The objective is infrastructure-foundation strengthening, with no
separate preliminary gate. WP-0 is merged and accepted at
`eef91ed4abf35bd09462deb1a55a0c72d95edea7`. WP-1 is merged and accepted
through PRs #36 and #37 at
`c438d1ffae1d6118f3a2b1cb8c7c5c4e10bcb4f2`. Wade authorized WP-2 execution
on 2026-08-14 under `PLAN-20260814-wp2-accounting-correctness`. A local
working-tree candidate exists on `codex/wp2-accounting-correctness`; review,
acceptance, and every Git publication action remain pending. WP-3 through WP-8
remain Planned / NO-GO except for WP-7 evidence and WP-8 synchronization
inside authorized package scope. No older phase, workstream, plan, ADR, or
provider-gate action is the current next action.

WP-0 merged through PR #35. Its accepted boundary adds default-off
compile/runtime containment, pre-startup LIVE rejection, explicit invalid-mode
rejection, unavailable-gateway execution blocking, removal of credential and
provider-specific REST ownership from the legacy data adapter, protective-
trigger retention, PAPER regression coverage, and default-off CI enforcement.
WP-1 merged through PR #36 with its two late review corrections merged through
follow-up PR #37. The accepted boundary provides complete version-12 BACKTEST
state, validation before mutation or checkpoint replacement, atomic writes,
risk-configuration compatibility, fail-closed PAPER/LIVE restart, governed
runtime snapshot paths, actionable checkpoint failure, exact generated-path
containment, and focused state/startup/source-control tests.

The authorized WP-2 candidate introduces canonical signed scale-8 financial
values and checked arithmetic; exact cash/cost/fee/P&L accounting; independent
per-symbol marks; deterministic PAPER cost parity; explicit long-only
over-close rejection; snapshot version 13; and a 261-event correctness-only
benchmark assertion. ADR 0005 is Proposed, and neither the candidate nor test
evidence constitutes Wade's acceptance.

## Current Roadmap State

`WORKSTREAM_ARCHITECTURE.md` defines the Workstreams I-VII project map and
`ROADMAP.md` remains the deterministic historical phase authority. The
Repository Remediation Program is the current cross-cutting execution overlay.
Historical summary:

- Workstream I — Broker-Neutral Execution Foundation: Complete — Accepted through Phase 22.
- Workstream II — Broker Integration Program: FIBO Group through cTrader is the
  selected demo-only XAUUSD target. Official cTrader Open API is the sole
  integration path; Gate 1 and Gate 3 are revalidated, Gate 2 and Gate 5 were
  accepted by Wade on 2026-08-07, and Gate 5.1 is merged and accepted. PR #25
  is merged at the authoritative base. Gate 6A succeeded, Wade confirmed exact
  `isLive=false` and exact `brokerTitleShort=FIBO`, Gate 6B account
  authentication succeeded, and Wade accepted the Gate 6 execution. Wade then
  explicitly authorized Gate 7. The isolated Gate 7 implementation and offline
  validation passed. The initial provider attempt stopped at unresolved
  in-process macOS Keychain access; Wade then authorized one new bounded retry.
  A later OAuth-hardened process stopped at the fixed
  `gate7_oauth_callback_timeout` boundary. The latest authorized process then
  advanced through OAuth, fixed demo TLS, application authentication, fresh
  demo-account selection/authentication, canonical XAUUSD resolution, and full
  metadata validation before stopping at the generic
  `gate7_subscription_failed` boundary. No subscription acceptance, BBO, or
  timestamp proof was obtained. Gates 8–9 and live trading remain unauthorized.
- Workstreams III–VII: parallel/future domains unless separately activated.
- Phase 21: Complete — Approved; ADR 0003 is Accepted.
- Phase 22: Broker-Neutral Execution Adapter Alignment and MT5/Prop-Account Readiness; Complete — Accepted under `PLAN-20260624-workstream-i-broker-neutral-completion`.
- Phase 23: Complete — FIBO Group through cTrader selected by Wade.
- Phase 24: Gate 1 and Gate 3 are revalidated; Gate 2 and Gate 5 were accepted
  by Wade on 2026-08-07; Gate 5.1 is merged and accepted; PR #25 is merged;
  and Gate 6 execution completed successfully and Wade accepted it on
  2026-08-10. Gate 7 implementation and offline validation are complete, but
  Gate 7 execution is incomplete at the latest evidenced
  `gate7_subscription_failed` transition after complete XAUUSD metadata
  validation. The subcause is unclassified; no accepted subscription, BBO, or
  timestamp proof was produced. Residual diagnostics and complete-BBO handling
  were corrected and reviewed offline under
  `PLAN-20260813-ctrader-gate7-residual-diagnostics-and-proof`. Gates 8–9,
  orders, and live use remain unauthorized.
- Phase 25: Not Started; no documentation platform is selected.
- Phase 26: Blocked pending Phase 25 selection and operator-approved documentation architecture.
- Live trading: disabled and unauthorized.

## Completed Work Verified In Current Tree

- C++20 core library and `tradebot_core` executable are configured in `CMakeLists.txt`.
- Tests exist for phases 13, 15, 16, 17, 18, and 22.
- Benchmarks exist for `throughput_bench`, `phase18_burnin`, and `apply_bbo_microbench`.
- Existing accepted ADRs:
  - ADR 0001: deprecate offline MOP/MOR/SS workflow.
  - ADR 0002: GitHub as long-term system of record after local cleanup is committed and pushed.
  - ADR 0003: Workstream I broker-neutral integration architecture.
  - ADR 0004: cTrader Open API is the sole integration path; implementation
    remains separately gated.
- Accepted Phase 22 broker-neutral implementation under `PLAN-20260624-workstream-i-broker-neutral-completion`; broker-dependent connectivity remains unauthorized.
- Runtime modes verified in `SystemConfig`: `BACKTEST`, `PAPER`, `LIVE`; default is `BACKTEST`.
- Credential loading verified through `AuthManager` and `SystemConfig` env names `AIIO_API_KEY` and `AIIO_API_SECRET`.
- `PLAN-20260813-autonomous-skills-ci-delivery` is complete. PRs #30 and #29
  merged three bounded repository skills, offline-policy-governed GCC/Clang and
  sanitizer CI, scheduled deep validation, pinned CodeQL, review-only monthly
  Dependabot proposals, and manual checksum-bearing non-executable evidence
  delivery. Final-main ordinary validation, CodeQL, deep validation, and
  evidence delivery passed on commit `6325a8b2e8356464b661de3f32c8d4816149ea9a`.
  Dependabot later opened PR #32 without auto-merge; PR #32 was merged into the
  diagnosis baseline commit
  `7f89c597d3874e5c8782a49a31d42335c8bf1e17`.

## In-Progress Work

- WP-0 is merged and accepted through PR #35 at
  `eef91ed4abf35bd09462deb1a55a0c72d95edea7`.
- WP-1 is complete and accepted through PRs #36 and #37 at
  `c438d1ffae1d6118f3a2b1cb8c7c5c4e10bcb4f2`.
- WP-2 is In Progress with an authorized unstaged local candidate; acceptance
  and Git actions are pending.
- WP-3 through WP-8 remain Planned / NO-GO except for WP-7/WP-8 slices required
  to close authorized package work.

## Blocked Or Constrained Work

- The cTrader Algo/cBot Bridge is
  `ABANDONED — NON-CONTROLLING — OUT OF SCOPE`; it is not an evidence source or
  fallback and is not to be investigated under Gate 5.
- OANDA is permanently cancelled.
- Gate 6 provider execution completed and Wade accepted its result. PR #25 is
  merged. Gate 7's separate allowlist/validator implementation is reviewable.
  Historical attempts stopped at Keychain access, generic OAuth failure, and a
  later fixed OAuth callback timeout. The latest process passed OAuth and full
  XAUUSD metadata validation but stopped at `gate7_subscription_failed`.
  Accepted subscription response, XAUUSD quote/timestamp proof, reconnect proof,
  every order operation, live accounts, and live trading remain Blocked /
  NO-GO.
- Prior Gate 7 residual plans and execution authority are historical and do not
  remain in the current action queue. Any future provider integration must map
  to WP-6, satisfy predecessor packages, and receive new exact authorization.
- Gates 1-3 now control protocol fit, numeric mapping, and pre-implementation
  baseline integrity. Homebrew Protobuf 35.1 is installed locally; the Gate 6
  opt-in build generates bindings only inside the build tree from SHA-256-
  verified official schemas. OAuth `state` remains undocumented, but one
  authorized callback matched exactly on 2026-08-10; every fresh attempt must
  still match or fail closed before token exchange.
  Gate 6A observed exact `isLive=false` and exact `brokerTitleShort=FIBO`; Wade
  confirmed those approved facts before Gate 6B. Gate 6B reproduced the
  predicate from a fresh list and completed account authentication without
  persisting an identifier. The spot timestamp unit remains later-gate evidence.
- Phase 26 is blocked until Phase 25 selects a documentation platform and the operator approves documentation architecture.
- GitHub-dependent sync remains constrained by intermittent or costly global connectivity.
- Ubuntu compute-node commands are not verified in this workspace.
- Markdown link-checking is unavailable locally because no link-check tool was found.
- Static-analysis and formatter commands are unconfigured locally.

## Known Defects And Warnings

- The repository-wide diagnosis classified the repository as operationally
  unsafe for live use and identified the nine package problem areas now
  controlled by `REPOSITORY_REMEDIATION_PROGRAM.md`.
- The WP-2 baseline `throughput_bench 261` consumed 261 ticks but reported 260
  fills because accumulated modeled costs reached the unchanged drawdown gate.
  The candidate's declared zero-cost correctness workload requires all 261
  fills while nonzero costs remain covered by accounting tests. This is not a
  performance claim, and acceptance remains pending.
- WP-1 replaces extension-wide generated-state rules with exact directory
  containment and explicit fixture-visibility checks.
- Previous full compile output emitted existing warnings:
  - `src/AsyncNetworkClient.cpp`: unused `SSL_ERROR_NONE`.
  - `src/RiskEngine.cpp`: `totalPositioned` set but not used.
- No failing tests were observed during local verification.
- `data/.DS_Store` and `src/.DS_Store` exist as ignored local artifacts.

## Documentation System Status

- Root `AGENTS.md`, `PLANS.md`, and `CONTRIBUTING.md` are present locally.
- Dedicated testing, data, security, actor, workflow, handoff, benchmarking, dependency, configuration, style, failure-recovery, live-readiness, glossary, review, release, project workstream architecture, Workstream I, and Phase 22 offline-research documents are present locally.
- `CTRADER_OPEN_API_GATE1_PROTOCOL_FIT.md`,
  `CTRADER_OPEN_API_GATE2_NUMERIC_CONTRACT.md`,
  `CTRADER_OPEN_API_GATE3_BASELINE_INTEGRITY.md`,
  `CTRADER_OPEN_API_GATE5.md`, and ADR 0004 define the sole Open API protocol,
  numeric, baseline, OAuth, secret, account-ID, and demo-only boundary.
- `.agents/skills/` contains 23 TradeBot skills, including bounded change
  orchestration, CI failure recovery, offline artifact delivery, and Git
  safety workflows.
- `CODEX_EXECUTION_EVIDENCE.md` defines shared authorization, evidence-epoch,
  exact-artifact, retry, resource-isolation, sensitive-memory, and handoff
  procedure for those skills.
- `WORKSTREAM_ARCHITECTURE.md` is the current project-level Workstreams I-VII map; `ROADMAP.md` is the canonical phase authority; this document summarizes current state.

## Verification Evidence

Verified locally through 2026-08-13:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
ctest --test-dir build -R phase22_tests --output-on-failure
ctest --test-dir build -R '^ctrader_gate5_1_tests$' --output-on-failure
cmake -S . -B build/gate6 -DTRADEBOT_ENABLE_CTRADER_GATE6=ON
cmake --build build/gate6
ctest --test-dir build/gate6 --output-on-failure
cmake -S . -B build/gate7 -DTRADEBOT_ENABLE_CTRADER_GATE7=ON
cmake --build build/gate7 --parallel 4
ctest --test-dir build/gate7 --output-on-failure
cmake -S . -B build/gate7-sanitize \
  -DTRADEBOT_ENABLE_CTRADER_GATE7=ON \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_OBJCXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build build/gate7-sanitize --target ctrader_gate7_tests --parallel 4
ctest --test-dir build/gate7-sanitize -R '^ctrader_gate7_tests$' --output-on-failure
```

Results:

- The foundational governance documentation candidate on
  `codex/repository-remediation-lock` passed Git diff hygiene, automation
  validation for 23 skills/four offline workflows, CI policy, exact nine-
  package and no-out-of-range-package checks, current-action drift review,
  placeholder/reference checks, and governance-only changed-path review.
  Markdown link/lint tools were unavailable locally. No CMake build or CTest
  rerun was required because the candidate changes documentation only.

- Configure succeeded.
- Build succeeded.
- Full CTest suite passed: 7 tests, 0 failed.
- Targeted `ctrader_gate5_1_tests` passed.
- Opt-in Gate 6 full CTest suite passed: 8 tests, 0 failed.
- Gate 5.1 and Gate 6 targeted tests passed under AddressSanitizer and
  UndefinedBehaviorSanitizer: 2 tests, 0 failed.
- Gate 7 opt-in configure/build passed; its full CTest suite passed: 8 tests,
  0 failed. The Gate 7 targeted sanitizer test passed: 1 test, 0 failed.
- Gate 7 presence-only configuration/Keychain preflight passed. The initial
  bounded provider process stopped at the in-process Keychain permission
  boundary. Wade's one authorized retry reached fresh OAuth authorization and
  stopped with `gate7_oauth_failed`, exit code 1, before account discovery or
  endpoint data traffic.
- Subsequent observed history includes a hardened callback timeout and a latest
  bounded process that reached the subscription transition after full XAUUSD
  metadata validation, then emitted `gate7_subscription_failed`. Those later
  results do not change the previously recorded offline test totals and do not
  prove subscription acceptance or a quote.
- The residual Gate 7 typed-outcome, redaction, bounded-heartbeat, and
  first-single-complete-BBO patch was built and retested on 2026-08-13. The
  default suite passed 7/7, the Gate 6 opt-in suite passed 8/8, the Gate 7
  opt-in suite passed 8/8, and the targeted Gate 7
  AddressSanitizer/UndefinedBehaviorSanitizer test passed 1/1. The final
  post-edit Gate 7 full suite again passed 8/8.
- Only local Gate 7 `--preflight` was invoked after that patch. It stopped with
  `gate7_client_id_missing`; no browser, DNS, TLS, or provider process was
  started. A privileged network-time query was unavailable, and observing the
  local time daemon running is not evidence of bounded clock skew. The
  pre-commit candidate binary hash is historical; the exact local commit and
  post-commit binary identity are recorded in the ignored handoff and do not
  authorize provider execution.
- Gate 6 regression coverage includes immutable Keychain-copy ownership and
  fail-closed allocation injection for correlation, account state, and token
  parsing.

WP-2 candidate evidence on 2026-08-14, from the unstaged working tree based on
`179817972cf469c67384f5f8c2245ecf09044621`:

- Fresh `RelWithDebInfo` candidate configure/build succeeded; six selected
  WP-1/WP-2/execution regressions passed 6/6.
- The explicit 261-event correctness workload reported 261 produced, 261
  consumed, and 261 fills; it skipped CSV output and made no performance claim.
- `./scripts/ci_validate.sh build/wp2-ci-validation` passed 14/14 default-off
  tests after validating 23 skills and four offline workflows.
- `./scripts/ci_deep_validate.sh build/wp2-ci-deep-validation` passed the same
  14/14 tests under ASan/UBSan. Full suites ran sequentially.
- Repository automation validation, offline CI policy, and `git diff --check`
  passed after documentation synchronization. The two existing compile
  warnings remain. Markdown formatter/link/lint tooling is not configured.
- No provider process, credential access, external traffic, order, risk-limit
  change, workflow dispatch, stage, commit, push, PR, merge, deployment,
  release, or live action occurred. This is candidate-tree evidence only.

## Operating Constraints

- Default operation remains offline-first and low-bandwidth-conscious.
- GitHub is the long-term system of record after local work is committed and pushed; inspect Git directly for current branch and commit state.
- External dependency downloads, web lookups, exchange checks, and remote API work should be grouped into planned connectivity windows.
- Live trading remains prohibited without explicit operator authorization and readiness evidence.

## Next Safe Action

Complete the local WP-2 verification/review handoff and stop. Wade must
separately decide package acceptance and any stage/commit/push/PR/merge action.
WP-3, provider activity, credentials, orders, financial-limit changes,
workflow dispatch, deployment, release, and live use remain prohibited.

## Next Professional Halting Point

The review-ready unstaged WP-2 candidate and this WP-8 authority closeout are
the current professional transition point. Workflow dispatch, credential
access, provider execution, reconnect, orders, risk-limit changes, deployment,
release, WP-3 and later packages, and live use remain prohibited. WP-2
acceptance and Git actions require separate Wade decisions.
