# Mynyra Architecture

## Current topology

```text
Browser
  -> static React control room
       -> local display state and explicit unavailable states
       -> no provider, account, credential, or order API

Mynyra Engine (`engine/`)
  -> strategy and market-data boundaries
  -> RiskEngine
  -> ExecutionEngine -> BrokerGateway -> IBrokerAdapter
  -> deterministic BACKTEST/offline evidence by default
  -> compile-time gated cTrader Demo adapter

GitHub
  -> protected main branch
  -> required `validate` check
  -> Clang, sanitizers, frontend checks, and CodeQL
```

The control room and engine share a repository, not a runtime API. The UI does
not invoke engine behavior. Any future read-only evidence API must define its
source, epoch, failure state, authentication boundary, deployment target,
rollback, and proof before implementation.

## Invariants

- `engine/` remains the source-layout boundary for the imported lineage.
- Infrastructure and UI code never become an order bus or financial truth
  source.
- Financial side effects pass only through
  `ExecutionEngine -> RiskEngine -> BrokerGateway -> IBrokerAdapter`.
- Default builds remain provider-free. Demo support is compile-time gated.
- The Demo milestone does not authorize LIVE endpoints, real-money accounts,
  credentials, provider retries, order attempts, deployment, or risk changes.
- Current runtime and node state must be observed live; archived handoffs do not
  prove it.

## Extension method

For each new capability or collaborator, record ownership, inputs, outputs,
authority, evidence, failure semantics, tests, rollback, and stop conditions.
Prefer a narrow module or task-scoped role over broad implicit access.
