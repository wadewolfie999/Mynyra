# TradeBot Project State

## Purpose And Authority

- Purpose: authoritative current-state summary for TradeBot.
- Authority level: current-state evidence below active approved plans and above the roadmap; the project workstream map is defined in `WORKSTREAM_ARCHITECTURE.md`, and phase definitions and statuses are delegated to `ROADMAP.md`.
- Audience: operator, maintainers, Codex, contributors, reviewers, and handoff recipients.
- Last documentation/state audit: 2026-08-09 against Gate 6 branch
  `codex/gate6-read-only-proof` from merged Gate 5.1 base `42affb3`.
- Last CMake/CTest verification evidence: 2026-08-09.

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
  accepted by Wade on 2026-08-07. PR #24 merged the Gate 5.1 controls and Wade
  accepted their implementation on 2026-08-09. Wade then authorized the Gate
  6 umbrella (`Gate 6A → mandatory Wade checkpoint → Gate 6B`). Its opt-in
  local proof implementation is in progress, but real execution is stopped
  pending credential rotation and registration of the fixed redirect URI.
- Workstreams III–VII: parallel/future domains unless separately activated.
- Phase 21: Complete — Approved; ADR 0003 is Accepted.
- Phase 22: Broker-Neutral Execution Adapter Alignment and MT5/Prop-Account Readiness; Complete — Accepted under `PLAN-20260624-workstream-i-broker-neutral-completion`.
- Phase 23: Complete — FIBO Group through cTrader selected by Wade.
- Phase 24: Gate 1 and Gate 3 are revalidated; Gate 2 and Gate 5 were accepted
  by Wade on 2026-08-07; Gate 5.1 is merged and accepted. Gate 6 is authorized
  but not executed: local implementation and offline validation are in
  progress, and provider traffic is stopped pending credential rotation and
  fixed redirect-URI registration. Market data, reconnect proof, and orders
  remain unauthorized.
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
- Gate 6 opt-in account-proof implementation under
  `PLAN-20260809-ctrader-open-api-gate6-proof`. It contains no market-data or
  order path. No OAuth, token, or cTrader operation has executed.

## Blocked Or Constrained Work

- The cTrader Algo/cBot Bridge is
  `ABANDONED — NON-CONTROLLING — OUT OF SCOPE`; it is not an evidence source or
  fallback and is not to be investigated under Gate 5.
- OANDA is permanently cancelled.
- Authorized Gate 6 provider execution is stopped until the cTrader client
  secret is rotated and the fixed redirect URI is registered. XAUUSD
  metadata/quotes, reconnect proof, every order operation, live accounts, and
  live trading remain Blocked / NO-GO until exact later directives.
- Gates 1-3 now control protocol fit, numeric mapping, and pre-implementation
  baseline integrity. Homebrew Protobuf 35.1 is installed locally; the Gate 6
  opt-in build generates bindings only inside the build tree from SHA-256-
  verified official schemas. OAuth `state` round-trip remains an unresolved
  provider question until the authorized correlated callback succeeds.
  Exact FIBO `brokerTitleShort` and intended demo-account identity must be
  observed in Gate 6A and confirmed by Wade before Gate 6B, not guessed or
  required before discovery. The spot timestamp unit remains later-gate
  evidence.
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

Verified locally on 2026-08-09:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
ctest --test-dir build -R phase22_tests --output-on-failure
ctest --test-dir build -R '^ctrader_gate5_1_tests$' --output-on-failure
cmake -S . -B build/gate6 -DTRADEBOT_ENABLE_CTRADER_GATE6=ON
cmake --build build/gate6
ctest --test-dir build/gate6 --output-on-failure
```

Results:

- Configure succeeded.
- Build succeeded.
- Full CTest suite passed: 7 tests, 0 failed.
- Targeted `ctrader_gate5_1_tests` passed.
- Opt-in Gate 6 full CTest suite passed: 8 tests, 0 failed.
- Gate 5.1 and Gate 6 targeted tests passed under AddressSanitizer and
  UndefinedBehaviorSanitizer: 2 tests, 0 failed.

## Operating Constraints

- Default operation remains offline-first and low-bandwidth-conscious.
- GitHub is the long-term system of record after local work is committed and pushed; inspect Git directly for current branch and commit state.
- External dependency downloads, web lookups, exchange checks, and remote API work should be grouped into planned connectivity windows.
- Live trading remains prohibited without explicit operator authorization and readiness evidence.

## Next Safe Action

Rotate the cTrader client secret, register and verify the fixed loopback
redirect URI without exposing credential fields, then resume the already-
authorized Gate 6 sequence in a clean task context. Re-run the offline
preflight before any provider traffic.

## Next Professional Halting Point

Stop at the credential/redirect prerequisite until remediated. During the
authorized Gate 6 run, stop at Wade's mandatory checkpoint before Gate 6B and
stop again after the read-only account proof. Gate 7 market data, reconnect
testing, orders, and live use require new directives.
