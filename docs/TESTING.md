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
  - `ctrader_gate5_1_tests`
- The opt-in Gate 6 configuration additionally registers
  `ctrader_gate6_tests`; normal builds remain unchanged.
- The opt-in Gate 7 configuration additionally registers
  `ctrader_gate7_tests`; normal builds remain unchanged.
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

Targeted test:

```sh
ctest --test-dir build -R phase18_tests --output-on-failure
ctest --test-dir build -R '^ctrader_gate5_1_tests$' --output-on-failure
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

## GitHub Actions Validation

`.github/workflows/validation.yml` runs three independent offline jobs:

- `Offline safety policy` rejects tracked credential-like files, private-key
  material, tracked handoff evidence, provider-capable workflow steps, a
  non-`BACKTEST` default, or enabled-by-default Gate 6/Gate 7 CMake options.
- `C++20 gcc/clang` configures separate default build trees, proves the
  provider proof options are `OFF`, builds, and runs the complete default
  CTest suite.
- `ASan and UBSan` builds and tests the default core with address and
  undefined-behavior sanitizers.

`.github/workflows/codeql.yml` performs scheduled and change-triggered C++
security analysis using the default-off build. None of these workflows accesses
credentials or starts a cTrader proof process.

`.github/workflows/release-candidate.yml` is manual. It repeats policy,
release build, and full default tests before uploading `tradebot_core`, a
SHA-256 manifest, and non-sensitive build metadata for 14 days. The artifact is
a candidate only; no release, deployment, provider traffic, or live transition
occurs.

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
- Performance tests: benchmark executables, governed by `BENCHMARKING.md`, not substitutes for correctness tests.

## Required Coverage By Change Type

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
