---
name: tradebot-market-replay-validation
description: Validate TradeBot market replay behavior, replay data policy, and generated replay artifacts. Use when touching LocalDataReplayAdapter, replay CSV or binary files, Phase 18 burn-in, replay timestamps, generated replay data, or replay-driven tests.
---
# tradebot-market-replay-validation

## Purpose

Validate TradeBot market replay behavior, replay data policy, and generated replay artifacts.

## Shared Operating Contract

- Follow `AGENTS.md` and `docs/CODEX_EXECUTION_EVIDENCE.md`.
- Resolve volatile branch, phase, gate, provider, and approval facts from Git,
  the active plan, `PROJECT_STATE.md`, and `ROADMAP.md`.
- Keep authorization and evidence epochs separate and stop at the exact
  operator-approved boundary.

## Activation Conditions

Use when touching `LocalDataReplayAdapter`, replay CSV/binary files, Phase 18 burn-in, replay timestamps, generated replay data, or replay-driven tests.

## Must Not Be Used

Do not use to validate live market-data connectivity. Do not treat generated replay data as historical truth without provenance.

## Required Inputs

- Replay path or changed replay code.
- Expected schema or timestamp units.
- Whether data is synthetic, sampled, historical, or generated.

## Required Repository Inspection

Read:

- `include/LocalDataReplayAdapter.hpp`
- `src/LocalDataReplayAdapter.cpp`
- `tests/phase18_tests.cpp`
- `src/benchmarks/phase18_burnin.cpp`
- `docs/DATA_POLICY.md`
- `docs/RISK_POLICY.md`

## Procedure

1. Identify data provenance and path.
2. Verify CSV/binary schema and timestamp units.
3. Check malformed or empty input behavior.
4. Run replay tests.
5. If burn-in is relevant, document generated outputs and input size.
6. Confirm generated files are ignored or intentionally versioned.
7. Identify fixed temporary paths, generated paths, and caches shared across
   build trees; run conflicting suites sequentially unless isolation is proven.
8. Record the evidence epoch and exact commit/artifact identity when required.

## Related Skills

- Use `tradebot-execution-pipeline-validation` when replay drives order lifecycle or broker-event simulation.
- Use `tradebot-network-live-boundary-review` if replay behavior risks depending on live adapter, network, credential, or wall-clock state.
- Use `tradebot-benchmark-review` when replay data supports benchmark claims.

## Allowed Mutations

Build/test/benchmark commands may generate files under `/tmp`, `build/`, or `data/results/`.

## Prohibited Mutations

No credential access, no live API calls, no committing generated replay data, no schema changes without plan/docs/tests.

## Verification Commands

```sh
cmake --build build
ctest --test-dir build -R phase18_tests --output-on-failure
```

Optional burn-in when scoped:

```sh
build/phase18_burnin 10000
```

## Expected Outputs

- Replay path and provenance.
- Schema/timestamp findings.
- Test results.
- Generated output paths.
- Shared-resource isolation or sequential-run status.
- Evidence epoch.
- Compatibility risks.

## Failure Behavior

If provenance, timestamp units, or schema are unclear, stop and record the missing evidence.

## Reporting Format

```markdown
## Replay Validation
- Data path:
- Provenance:
- Schema:
- Timestamp units:
- Commands:
- Results:
- Risks:
```

## Authority Documents

- `docs/DATA_POLICY.md`
- `docs/TESTING.md`
- `docs/RISK_POLICY.md`
