# TradeBot Testing Contract

## Purpose And Authority

- Purpose: define how TradeBot behavior is verified.
- Authority level: testing authority below risk and architecture policy, above contributor convenience.
- Audience: Codex, contributors, maintainers, reviewers, and testers.

## Philosophy

Tests are evidence, not ceremony. TradeBot is financial-sensitive, so tests must protect deterministic replay behavior, risk gates, order execution boundaries, credential handling, and generated-output reproducibility.

## Verified Test Infrastructure

- Test framework: CTest with C++ test executables.
- Test registration: `CMakeLists.txt`.
- Registered tests:
  - `phase13_tests`
  - `phase15_tests`
  - `phase16_tests`
  - `phase17_tests`
  - `phase18_tests`
  - `phase22_tests`
  - `wp0_containment_tests`
  - `wp0_live_startup_rejected`
  - `wp1_persistence_tests`
  - `wp1_nonbacktest_resume_rejected`
  - `wp1_generated_paths`
  - `wp2_accounting_tests`
  - `wp2_throughput_correctness`
  - `ctrader_gate5_1_tests`
  - `ctrader_provider_architecture_tests`
  - `mynyra_demo_core_tests`
  - `mynyra_demo_cli_containment`
- The opt-in Gate 6 configuration additionally registers
  `ctrader_gate6_tests`; normal builds remain unchanged.
- The opt-in Gate 7 configuration additionally registers
  `ctrader_gate7_tests`; normal builds remain unchanged.
- The opt-in cTrader DEMO configuration additionally registers
  `ctrader_demo_frame_decoder_tests`, `ctrader_demo_market_state_tests`, and
  `ctrader_demo_provider_private_tests`; all use synthetic/local fakes and
  perform no external traffic or Keychain read.
- All test executables link against `tradebot_core_lib`.
- Some tests create temporary files under `/tmp`.

## Commands

Configure:

```sh
cmake -S . -B build
```

Build:

```sh
cmake --build build
```

Full suite:

```sh
ctest --test-dir build --output-on-failure
```

Ordinary local/CI automation path:

```sh
./scripts/ci_validate.sh build/ci-validation
```

The helper validates repository automation, configures `RelWithDebInfo` with
`BUILD_TESTING=ON`, forces the legacy LIVE runtime, both cTrader proof targets,
and cTrader DEMO OFF, builds with a bounded job count, and runs the full default CTest
suite sequentially. Only after CTest passes does it write a build-local marker
bound to the current full commit and configuration, including
`live_runtime=OFF`; evidence packaging rejects a missing or mismatched marker.

Scheduled/manual deep offline path:

```sh
./scripts/ci_deep_validate.sh build/ci-deep-validation
```

The deep helper uses one isolated Debug build tree with ASan/UBSan, keeps the
legacy LIVE runtime, both cTrader proof targets, and cTrader DEMO OFF, and runs the default
suite sequentially. It does not replace the separately authorized macOS Gate 7
sanitizer procedure below.
Leak detection remains enabled on Linux; the helper disables only that ASan
option on Darwin because Apple ASan does not support it.

Targeted test:

```sh
ctest --test-dir build -R phase18_tests --output-on-failure
ctest --test-dir build -R '^ctrader_gate5_1_tests$' --output-on-failure
ctest --test-dir build -R '^wp0_' --output-on-failure
ctest --test-dir build -R '^wp2_' --output-on-failure
```

Opt-in Gate 6 suite:

```sh
cmake -S . -B build/gate6 -DTRADEBOT_ENABLE_CTRADER_GATE6=ON
cmake --build build/gate6
ctest --test-dir build/gate6 --output-on-failure
```

Opt-in Gate 7 offline suite:

```sh
cmake -S . -B build/gate7 -DTRADEBOT_ENABLE_CTRADER_GATE7=ON
cmake --build build/gate7 --parallel 4
ctest --test-dir build/gate7 --output-on-failure
```

The Gate 7 suite is synthetic and performs no Keychain read, browser flow,
socket connection, provider request, account access, market-data request, or
order operation. Relevant sanitizer coverage is run from a separate
`build/gate7-sanitize` configuration.

Gate 7 ASan/UBSan diagnostic coverage:

```sh
cmake -S . -B build/gate7-sanitize \
  -DTRADEBOT_ENABLE_CTRADER_GATE7=ON \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_OBJCXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build build/gate7-sanitize --target ctrader_gate7_tests --parallel 4
ctest --test-dir build/gate7-sanitize -R '^ctrader_gate7_tests$' --output-on-failure
```

Mynyra Demo M1 offline suite:

```sh
cmake -S . -B build/mynyra-demo \
  -DTRADEBOT_ENABLE_CTRADER_DEMO=ON
cmake --build build/mynyra-demo --parallel 2
ctest --test-dir build/mynyra-demo --output-on-failure --parallel 1
```

Mynyra Demo M1 sanitizer suite:

```sh
cmake -S . -B build/mynyra-demo-sanitize \
  -DTRADEBOT_ENABLE_CTRADER_DEMO=ON \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_OBJCXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build build/mynyra-demo-sanitize --parallel 2
ASAN_OPTIONS='detect_leaks=0:halt_on_error=1' \
UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1' \
ctest --test-dir build/mynyra-demo-sanitize --output-on-failure --parallel 1
```

These commands are offline verification only. They do not invoke OAuth,
Keychain, the Demo endpoint, or an order.

`ctrader_demo_provider_private_tests` additionally asserts that subscription
failures preserve their fixed transport/protocol classification and identify
only the safe subscription leg (`spots` or `live_m1`). Raw provider errors,
identifiers, and payloads are not surfaced by these diagnostics.
It also verifies the fixed redacted categories used when an asynchronous spot
event fails envelope, identity, quote, trendbar, or normalized market-state
validation while another subscription response is pending.
Trendbar fixtures cover Protobuf envelope, low, timestamp, arithmetic overflow,
and OHLC invariants. They also prove that absent optional open/close/high
deltas retain their Protobuf numeric default of zero, including the externally
observed missing-close case.

## Local Validation

`scripts/ci_policy_checks.sh` validates tracked sensitive-looking paths,
private-key material, the `BACKTEST` default, default-disabled legacy LIVE,
Gate 6, Gate 7, and cTrader DEMO options, and the absence of retired `.github/`
automation. `python3 scripts/validate_automation.py` validates skill metadata
and the required local guardrail scripts.

`scripts/ci_validate.sh` runs the default-off configure/build/sequential CTest
path; `scripts/ci_deep_validate.sh` runs its ASan/UBSan matrix. They do not
access credentials or start a cTrader proof process. The local
`scripts/package_offline_artifact.sh` command retains only non-executable
CTest, license, manifest, and checksum evidence. No release, deployment,
provider traffic, or live transition occurs.

## Test Layers

- Unit-style tests: direct class behavior inside phase test executables.
- Integration tests: components wired together through portfolio, risk, execution, adapter, broker, metrics, and trigger-order paths.
- Replay tests: `phase18_tests` covers local CSV and binary replay roundtrip.
- Order-book tests: `phase18_tests` covers BBO application and best quote behavior.
- Financial-mode safety tests: phases 13, 15, 16, and 17 exercise paper/live-capable boundaries without live trading authorization.
- OAuth-correlation tests: `ctrader_gate5_1_tests` uses only synthetic inputs
  to verify fixed loopback binding, secure generation, monotonic expiry, exact
  no-callback timeout, explicit cancellation, exact-deadline behavior, match,
  one-shot consumption, malformed/duplicate/mismatch/replay rejection, code
  discard, terminal state clearing, and bounded redacted diagnostics. It
  performs no socket, browser, provider, token, account, market-data, or order
  action.
- Gate 6 account-proof tests: `ctrader_gate6_tests` uses only synthetic values
  to verify exact demo endpoint/port and outbound message allowlists, strict
  token-response parsing, offline libcurl option acceptance, exact
  `SCOPE_TRADE` account-list ownership/scope/generation predicates,
  live and missing-metadata exclusion, exact safe-metadata selection, fresh
  Gate 6B matching, account-auth response binding, terminal clearing,
  cancellation, immutable Keychain-data copy ownership, allocation-failure
  termination/clearing, and bounded redacted diagnostics. Linking the runtime test
  target does not open a browser, read Keychain, or perform network traffic.
- Gate 7 proof tests: `ctrader_gate7_tests` uses synthetic account, light/full
  symbol, metadata, scale, volume, quote, timestamp, generation, correlation,
  allowlist, malformed-frame, allocation-failure, typed send/receive outcomes,
  provider-error categories, disconnect/token/account controls, timeout,
  cancellation, partial-event continuation, first-single-complete-BBO,
  timestamp stale/future classification, heartbeat cadence, terminal-clearing,
  and fixed OAuth listener/browser/timeout, callback, denial, and
  state-correlation diagnostic inputs. It verifies that neither OAuth nor
  residual diagnostics contain value-like or provider-supplied material. It
  also verifies the callback inactivity deadline is capped by the absolute
  correlation deadline, injects callback-buffer allocation failure to prove
  the fixed resource-exhaustion result, and proves the heartbeat wait cannot
  extend the absolute deadline. It proves the target cannot construct trading/
  order/position/depth/trendbar/historical/reconnect payloads. Provider
  execution is reported separately and is never a substitute for offline tests.
- Provider-architecture tests: `ctrader_provider_architecture_tests` verifies
  the normalized market-data contract, independent gateway acknowledgement and
  execution callback fan-out, and that the cTrader adapter skeleton remains
  disconnected, unsupported, and free of provider side effects.
- Mynyra Demo core tests: `mynyra_demo_core_tests` covers long/short exact
  intent risk decisions, direction-bound expected margin, direction-specific
  BBO reference prices, margin/freshness/account/instrument gates,
  risk-reducing close behavior, broker-mirror idempotency, explicit and
  fill-implied acceptance, partial/final fills, transport-ambiguous and
  confirmed-partial entry reconciliation without entry retry, native logical
  close, local-fill/broker-quantity mismatch recovery, one exact residual close
  after partial or zero-fill ambiguity, recovery-only handling when a locally
  complete close leaves broker exposure, final flat success,
  recovery-required outcomes, event redaction, and parity between extracted
  `StrategyPipeline` decisions and the legacy SMA(12/26), BB20/RSI14, regime,
  allocator path.
- DEMO CLI tests: `mynyra_demo_cli_containment` proves the default build rejects
  DEMO and the opt-in build rejects CSV, resume, endpoint/account/volume
  overrides, unsupported symbols/timeframes/providers, and Demo-only flags in
  other modes.
- DEMO provider-private tests use fake Keychain/HTTP/browser services to cover
  stored-token validation, one refresh, invalid grant, typed HTTP classes,
  fresh OAuth bypass, persistence deferred until provider validation, scope
  mismatch, Keychain failure, the clean OAuth completion response, immutable
  Demo endpoint, and exact `brokerTitleShort=FIBO` plus `isLive=false`
  selection.
- DEMO framing and market-state tests cover partial/multiple/malformed frames,
  terminal buffer clearing, bounded buffering, bid/ask independence,
  conservative BBO freshness,
  crossed quotes, historical ordering/deduplication/completion, live M1
  rollover, out-of-order updates, and queue overflow.
- Performance tests: benchmark executables, governed by `BENCHMARKING.md`, not substitutes for correctness tests.
- WP-2 accounting tests: `wp2_accounting_tests` covers scale-8 conversion,
  rounding and overflow vectors; buy/add/reduce/close and reversal rejection;
  multi-symbol marks; deterministic PAPER costs; malformed fill rejection;
  and 1,000 round trips. `wp2_throughput_correctness` runs 261 deterministic
  zero-cost PAPER events and requires one confirmed fill per consumed event.
  It skips performance thresholds and makes no throughput claim.
- Execution pipeline correctness tests: `execution_pipeline_correctness_tests`
  verifies RiskEngine decision propagation, duplicate/stale event idempotency,
  rejected-acknowledgement cleanup, partial/final sells, and transactional
  over-close rejection without portfolio or execution-context mutation.

## Local Evidence

- `scripts/ci_validate.sh` and `scripts/ci_deep_validate.sh` are invoked
  explicitly from a reviewed local tree and retain default-off provider targets
  and sequential CTest execution.
- `scripts/package_offline_artifact.sh` packages a CTest log and exact-revision
  SHA-256 provenance, verifies the checksum list, and excludes the
  live-capable executable.
- A local test pass or retained artifact is evidence for that exact revision
  only. It is not release, deployment, provider, order, or live authorization.

## Required Coverage By Change Type

### Repository Remediation Program Evidence

Every package in `REPOSITORY_REMEDIATION_PROGRAM.md` must have named tests that
trace its acceptance criteria. Minimum package emphasis:

- WP-0: mode/startup rejection, no credential/network side effect, unready
  gateway fail-closed behavior, protective-trigger retention, opt-in build gate
  composition, and explicit BACKTEST/PAPER regression.
- WP-1: zero/one/many-position round-trip, risk/lifecycle/dedup state,
  version/migration, corrupt/partial/atomic-write failure, and exact generated-
  path containment.
- WP-2: fixed-unit golden vectors, overflow/rounding, buy/sell/reduce/close/
  reversal/partial-fill accounting, multi-symbol marks, and the 261-tick fill-
  ceiling regression.
- WP-3: daily drawdown, VaR aggregation, final normalized quantity, independent
  halt/close-only sources, stale health/reconciliation data, persistence, and
  operator-clear semantics.
- WP-4: one end-to-end lifecycle with rejection, partial/full fill, cancel/fill
  race, timeout, duplicate/out-of-order event, trigger/pending retention,
  restart deduplication, and reconciliation.
- WP-5: mode topology, explicit inputs, timestamp units/clock domains, stale/
  future/out-of-order data, replay schema/version/cursor, ownership, shutdown,
  and concurrency.
- WP-6: fully synthetic provider-adapter framing, partial I/O, TLS validation,
  backpressure, timeout, bounded reconnect, translation, health, redaction,
  lifecycle, and reconciliation. Normal tests remain network- and secret-free.
- WP-7: CI-to-criterion traceability, sanitizer/analyzer/warning policy,
  benchmark correctness, observability redaction, and artifact allowlists.
- WP-8: authority/index consistency, tracked-file disposition, stale-claim
  scans, documentation links when tooling exists, and exact evidence epoch.

A package test pass does not accept the package. The corresponding WP-8 sync
and Wade acceptance are also required, and the next package remains NO-GO.

- Order-book changes: targeted tests for valid/invalid BBO, level mutation, recentering if affected, and `apply_bbo_microbench` when performance is claimed.
- Replay changes: CSV, binary, malformed input, timestamp ordering, cursor, and generated binary compatibility.
- Risk changes: drawdown, position cap, VaR, close-only, halt, latency, error-rate, and live volatility scaling.
- Execution changes: buy/sell, close behavior, fees, slippage, blocked orders, partial fills, trigger orders, broker callback behavior.
- Credential/network changes: env loading, redaction, signing, malformed payloads, reconnect, gap-fill, TLS/local validation where relevant.
- OAuth-correlation changes: secure generation, fixed callback binding,
  no-callback expiry, explicit cancellation, exact matching, single use,
  mismatch/replay rejection, state clearing, code discard, and fixed
  non-sensitive diagnostics.
- Gate 7 residual transport changes: every typed send/receive result, closed
  provider-category mapping, fixed diagnostic redaction, account/client/token
  disconnect handling, exact subscription correlation/account proof,
  incomplete-event non-retention, first-single-complete-BBO behavior, raw and
  normalized crossing, timestamp missing/stale/future/unit ambiguity, bounded
  heartbeat cadence, absolute deadlines, terminal clearing, and allocation
  failure.
- Mynyra Demo changes: default-off/no-live containment, fake credential/token
  services, strict framing/partial I/O, heartbeat/rate-limit bounds,
  reconnect/reconcile behavior, historical/live market-data ordering, strategy
  parity, warmup non-execution, direction-bound exact minimum-volume risk,
  long/short lifecycle, fill-implied acceptance, duplicate/stale/overfill
  rejection, native close, entry reconciliation mismatch, residual recovery,
  final account-wide flat reconciliation, event redaction, and
  failure-without-success markers.
- Analytics/output changes: generated CSV path, schema, reproducibility, and no secret leakage.
- Documentation-only changes: `git diff --check`, doc grep audit, and index review.

## Determinism And Reproducibility

- Prefer deterministic inputs.
- Record random seeds if randomness is introduced.
- Use fixed temporary paths only when tests cleanly isolate them.
- Do not require external exchanges, network services, or credentials for normal tests.
- Tests must not depend on ignored generated outputs unless they create them inside the test.
- Record the evidence epoch: working tree, staged index, commit, or exact-commit
  rebuild. A relevant edit after a pass invalidates that pass for completion
  until affected checks are rerun.
- Exact-commit evidence must name the full commit, build configuration,
  artifact path/hash when applicable, and final tracked/index status.

## Fixtures And Test Data

- `data/samples/` exists but is currently empty.
- Tests may create temporary CSV/binary fixtures under `/tmp`.
- Historical data under `data/historical/` is referenced by benchmark code but not tracked in the verified tree.
- Fixture provenance must be documented before committing new data.
- `wp1_persistence_tests` covers zero/one/many open-position round trips,
  version-13 fixed-unit accounting, risk halt/close-only state, pending-order
  identity, regime and
  allocator state, corrupt/malformed/legacy rejection without mutation,
  atomic-write failure, non-finite pre-write rejection, risk-configuration
  mismatch rejection, transient API-error incompatibility, and event-loop
  checkpoint failure.
- `wp1_nonbacktest_resume_rejected` proves PAPER/LIVE resume and uncontrolled
  resume paths fail before startup side effects. `wp1_generated_paths` proves exact generated-path
  ignores without hiding intentional CSV/binary fixtures.

## Time And Timezone Tests

When changing time behavior, test:

- Epoch ordering.
- Nanosecond vs candle timestamp units.
- Resume behavior at checkpoint boundaries.
- Stale or out-of-order data handling.

## Concurrency Tests

Concurrency-sensitive areas include lock-free ring buffers, live-data queueing, broker callbacks, reconnect worker behavior, and metrics aggregation. Changes there require stress or integration tests that exercise queue overflow, dropped events, and shutdown.

Test-run concurrency is separate from concurrency coverage. Full CTest suites
from different build trees must run sequentially unless fixed ports, loopback
listeners, local certificate authorities, fixed temporary paths, process names,
caches, and generated outputs are all proven isolated. If parallel execution
causes a suspected collision, preserve the initial failure, inspect the shared
resource, rerun sequentially, and report both results.

## Performance And Benchmarks

Benchmark executables are separate from tests:

- `build/apply_bbo_microbench 10000`
- `build/throughput_bench <tick-count>`
- `build/phase18_burnin <tick-count> [replay-path]`

Benchmark output is not enough to accept behavior unless correctness tests also pass.

## Failure Reporting

Reports must include:

- Command run.
- Exit code or pass/fail summary.
- Relevant output lines.
- Warnings.
- Skipped checks and why.
- Whether generated outputs were produced.
- Evidence epoch and exact artifact identity when relevant.
- Whether suites ran concurrently; if so, how shared resources were isolated.

Do not summarize a failed test as passed. Do not hide compiler warnings.

## Minimum Evidence For Completion

- Documentation-only: `git diff --check` and documentation audit grep reviewed.
- Skill/governance changes: documentation-only evidence plus every changed
  skill passing the available skill validator and a scan for stale volatile
  phase/gate assertions.
- Narrow source change: build plus targeted tests.
- Shared behavior change: build plus full CTest suite.
- Performance claim: build, relevant tests, benchmark command, environment, input size, and comparative evidence.
- Financial-sensitive change: full tests plus risk-specific tests and operator approval.
- A Mynyra Demo M1 external success claim additionally requires the exact
  three-stage acceptance evidence and the final
  `mynyra_demo_m1_succeeded` marker. Offline suites cannot substitute for it.
