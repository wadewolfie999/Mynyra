# TradeBot Project State

## Purpose And Authority

- Purpose: authoritative current-state summary for TradeBot.
- Authority level: current-state evidence below active approved plans and above the roadmap; the project workstream map is defined in `WORKSTREAM_ARCHITECTURE.md`, and phase definitions and statuses are delegated to `ROADMAP.md`.
- Audience: operator, maintainers, Codex, contributors, reviewers, and handoff recipients.
- Last documentation/state audit: 2026-08-10 against Gate 7 branch
  `codex/gate7-xauusd-market-data-proof` from authoritative merged
  `origin/main` `7fc244e2f3cc5a1e406d416898807562fcf58c6d`.
- Last CMake/CTest verification evidence: 2026-08-10.

This document represents current state only. Historical execution belongs in Git commits, pull requests, issues, ADRs, and handoffs.

## Repository State

- Repository root: `/Users/vaheedgorgeen/TradeBot`.
- Branch, HEAD, and worktree values are runtime metadata. Inspect Git directly; historical snapshots are not current-state authority.
- The merged tree contains Phase 19 revalidation history, infrastructure validation, approved Workstream I artifacts, accepted ADR 0003, and the TradeBot skill-system expansion.
- Ignored artifacts observed: `build/`, `data/`, and `src/.DS_Store`.

## Current Roadmap State

`WORKSTREAM_ARCHITECTURE.md` defines the current Workstreams I-VII project map; `ROADMAP.md` is the deterministic phase authority. Current summary:

- Workstream I — Broker-Neutral Execution Foundation: Complete — Accepted through Phase 22.
- Workstream II — Broker Integration Program: FIBO Group through cTrader is the
  selected demo-only XAUUSD target. Official cTrader Open API is the sole
  integration path; Gate 1 and Gate 3 are revalidated, Gate 2 and Gate 5 were
  accepted by Wade on 2026-08-07, and Gate 5.1 is merged and accepted. PR #25
  is merged at the authoritative base. Gate 6A succeeded, Wade confirmed exact
  `isLive=false` and exact `brokerTitleShort=FIBO`, Gate 6B account
  authentication succeeded, and Wade accepted the Gate 6 execution. Wade then
  explicitly authorized Gate 7. The isolated Gate 7 implementation and offline
  validation passed, but its one controlled provider process stopped before
  fixed-endpoint traffic at unresolved in-process macOS Keychain access. No
  XAUUSD market-data evidence was obtained. Gates 8–9 and live trading remain
  unauthorized.
- Workstreams III–VII: parallel/future domains unless separately activated.
- Phase 21: Complete — Approved; ADR 0003 is Accepted.
- Phase 22: Broker-Neutral Execution Adapter Alignment and MT5/Prop-Account Readiness; Complete — Accepted under `PLAN-20260624-workstream-i-broker-neutral-completion`.
- Phase 23: Complete — FIBO Group through cTrader selected by Wade.
- Phase 24: Gate 1 and Gate 3 are revalidated; Gate 2 and Gate 5 were accepted
  by Wade on 2026-08-07; Gate 5.1 is merged and accepted; PR #25 is merged;
  and Gate 6 execution completed successfully and Wade accepted it on
  2026-08-10. Gate 7 implementation and offline validation are complete, but
  Gate 7 execution is incomplete at the evidenced Keychain-permission blocker
  before provider traffic. No market-data proof was produced. Gates 8–9,
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

## In-Progress Work

- Repository governance and Codex skill-system maintenance.
- Gate 7 review closeout under
  `PLAN-20260810-ctrader-open-api-gate7-xauusd-market-data-proof`. The target is
  default-disabled, macOS-only, detached from production runtime modes and
  order/risk paths, and offline validation passed. The one provider attempt
  stopped safely at `gate7_keychain_permission_unresolved` before the fixed
  endpoint; no retry is permitted in this task.

## Blocked Or Constrained Work

- The cTrader Algo/cBot Bridge is
  `ABANDONED — NON-CONTROLLING — OUT OF SCOPE`; it is not an evidence source or
  fallback and is not to be investigated under Gate 5.
- OANDA is permanently cancelled.
- Gate 6 provider execution completed and Wade accepted its result. PR #25 is
  merged. Gate 7's separate allowlist/validator implementation is reviewable,
  but its provider execution stopped before network traffic at unresolved
  Keychain access. XAUUSD metadata/quotes, reconnect proof, every order
  operation, live accounts, and live trading remain Blocked / NO-GO.
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
- `.agents/skills/` TradeBot skill files are present locally, including `tradebot-git-safety`.
- `WORKSTREAM_ARCHITECTURE.md` is the current project-level Workstreams I-VII map; `ROADMAP.md` is the canonical phase authority; this document summarizes current state.

## Verification Evidence

Verified locally on 2026-08-10:

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
ctest --test-dir build/gate7-sanitize -R '^ctrader_gate7_tests$' --output-on-failure
```

Results:

- Configure succeeded.
- Build succeeded.
- Full CTest suite passed: 7 tests, 0 failed.
- Targeted `ctrader_gate5_1_tests` passed.
- Opt-in Gate 6 full CTest suite passed: 8 tests, 0 failed.
- Gate 5.1 and Gate 6 targeted tests passed under AddressSanitizer and
  UndefinedBehaviorSanitizer: 2 tests, 0 failed.
- Gate 7 opt-in configure/build passed; its full CTest suite passed: 8 tests,
  0 failed. The Gate 7 targeted sanitizer test passed: 1 test, 0 failed.
- Gate 7 presence-only configuration/Keychain preflight passed. The single
  bounded provider process stopped at the in-process Keychain permission
  boundary before endpoint traffic, with exit code 1 after safe cancellation.
- Gate 6 regression coverage includes immutable Keychain-copy ownership and
  fail-closed allocation injection for correlation, account state, and token
  parsing.

## Operating Constraints

- Default operation remains offline-first and low-bandwidth-conscious.
- GitHub is the long-term system of record after local work is committed and pushed; inspect Git directly for current branch and commit state.
- External dependency downloads, web lookups, exchange checks, and remote API work should be grouped into planned connectivity windows.
- Live trading remains prohibited without explicit operator authorization and readiness evidence.

## Next Safe Action

Review the Gate 7 draft PR and sanitized report, then resolve the local
Keychain-access condition before any separately authorized future attempt. Do
not repeat provider traffic in this task, mark the PR ready, merge it, or begin
Gate 8.

## Next Professional Halting Point

Stop after the Gate 7 incomplete handoff. Gate 7 provider rerun, reconnect
testing, orders, Gates 8–9, and live use require separate future authority.
