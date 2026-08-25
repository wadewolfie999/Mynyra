# Mynyra Engine Offline Replay V1

## Purpose

This is the first Mynyra Engine common-ground contract. It is deliberately an
in-process, deterministic `BACKTEST` replay, not a node-control protocol or a
provider runtime.

`MynyraOfflineRunnerV1` accepts caller-owned local input/configuration bytes,
validates their SHA-256 values against `RunManifestV1`, and processes canonical
CSV records of the form:

```text
epoch_seconds,SYMBOL,open,high,low,close,volume
```

It does not open input/output paths, invoke a shell, create a process, use SSH,
manage a container, access a credential, connect to a broker, or submit an
order. `providerAllowed=false` and `ordersAllowed=false` are required manifest
invariants; any other mode or permission is rejected before replay starts.

## Contract ownership

- `RunManifestV1` names the immutable artifact/input/config identities, mode,
  requested byte/record/time limits, and permissions.
- `EvidenceEnvelopeV1` contains versioned, redacted lifecycle codes, terminal
  result, completeness, and the three pinned hashes.
- `CapabilityReportV1` reports process health, data freshness, broker
  connectivity, reconciliation, and execution eligibility independently. It
  intentionally has no aggregate health field.
- `OfflineRunResultV1` returns the deterministic result hash, processed-record
  count, redacted envelope, and a `memory://` evidence location. Persisting or
  transporting that result belongs to a later, explicitly authorized boundary.

The runner uses `StrategyPipeline` with execution ineligible and may emit only
redacted, `BACKTEST`-mode events through the existing `IEventSink` interface.
An event-sink failure produces `EvidenceIncomplete` and no completed result.

## Acceptance coverage

`mynyra_offline_run_tests` verifies:

- SHA-256 correctness and hash-pinned deterministic repeatability;
- manifest/provider permission and input-hash rejection;
- malformed input;
- cancellation, timeout, and record-limit failures; and
- incomplete evidence caused by a failing `IEventSink`.

The first ASUS proof must rerun this exact offline contract from an immutable
release. It is not cTrader authorization, provider-readiness evidence, or
Stage 3 commissioning authority.
