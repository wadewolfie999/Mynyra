# Mynyra Engine Architecture

## Core ownership

- Market input and provider translation terminate at explicit adapter
  boundaries.
- Strategy evaluation produces intent; it does not own financial side effects.
- `RiskEngine` owns acceptance and containment decisions.
- `ExecutionEngine -> RiskEngine -> BrokerGateway -> IBrokerAdapter` is the
  canonical order path.
- Broker-authoritative Demo state is mirrored separately from BACKTEST/PAPER
  spot-cash accounting.
- Evidence sinks record redacted, versioned events; they do not grant authority
  or become financial truth sources.

## Modes

- BACKTEST/offline: default, deterministic, provider-free.
- DEMO: compile-time gated, cTrader Demo endpoint only, with explicit account,
  instrument, market-data, reconciliation, and one-shot controller checks.
- LIVE: no enabled or authorized path in this repository.

## Failure posture

Unknown mode, stale data, incomplete reconciliation, provider degradation,
ambiguous execution, invalid numeric conversion, or missing authority fails
closed. Recovery never silently clears a halt or manufactures account state.

## Non-goals

The engine does not make infrastructure an order bus, expose UI execution
controls, store credentials in Git, or infer current provider/runtime state
from historical evidence.
