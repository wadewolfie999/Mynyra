# Plan: cTrader Provider Architecture First Tranche

- Plan ID: `PLAN-20260820-ctrader-provider-architecture-first-tranche`
- Status: In Progress
- Owner: Wade
- Implementer: Codex
- Review authority: Wade
- Related decision: ADR 0004 and `CTRADER_PROVIDER_ARCHITECTURE_REVIEW.md`
- Created: 2026-08-20
- Updated: 2026-08-20

## Objective

Establish an event-native execution seam and a provider-neutral market-data
contract, then add a default-disabled cTrader provider skeleton. The result
must accept a future cTrader adapter below `BrokerGateway` without making any
provider, credential, network, order, or live action possible.

## Scope

- `ExecutionEngine` consumes normalized acknowledgement and execution events
  from `BrokerGateway` instead of the compatibility `BrokerFill` callback.
- `BrokerAdapterContracts.hpp` gains the narrow normalized market-data contract
  because it already owns provider-neutral order, account, instrument, and
  health types used by `IBrokerAdapter`. A dedicated header would create a
  second peer contract surface with no current owner.
- A default-disabled `providers/ctrader` skeleton expresses adapter, session,
  codec, transport, auth, account, instrument, market-data, and order-service
  boundaries using no cTrader dependency or external I/O.
- Synthetic tests prove boundary shape and fail-closed defaults.

## Out Of Scope

- cTrader/Open API traffic, socket creation, TLS, OAuth, browser flow,
  Keychain or token access, account selection, subscriptions, orders,
  cancellation against a provider, reconnect, runtime selection, and live use.
- Changes to `SystemConfig`, `AuthManager`, `AsyncNetworkClient`,
  `LiveDataAdapter`, financial limits, replay semantics, or existing WP-2
  accounting behavior.

## Preconditions

- The review document remains the architectural baseline.
- All verification uses local, default-off, synthetic inputs only.
- Existing worktree changes are preserved. The credential-like untracked file
  is neither opened nor modified.

## Assumptions

- `BrokerAdapterContracts.hpp` remains the existing public contract owner.
- Existing `BrokerFill` APIs may remain as compatibility support for unrelated
  callers, but the new `ExecutionEngine` path must not depend on them.

## Invariants

- `BACKTEST` remains network-free; `PAPER` remains deterministic; `LIVE`
  remains default-disabled and fail-closed.
- The order path is `ExecutionEngine -> RiskEngine -> BrokerGateway ->
  IBrokerAdapter`, with portfolio mutation only from a normalized execution
  event.
- Provider-native values never appear in core contracts or tests.
- The skeleton reports unavailable/unsupported health and rejects submit/cancel
  without emitting external side effects.

## Authorization Boundary

- Allowed: local source/test/doc edits; offline configure/build/CTest; up to
  two evidence-backed repair cycles for the same local failure.
- Artifact/environment: current local dirty worktree, default-off build trees,
  synthetic tests only.
- Stop condition: material contradiction with the review baseline or an action
  requiring external/provider/credential/live authority.
- Prohibited: credential access, provider process or traffic, orders, runtime
  activation, stage, commit, push, merge, release, or deployment.

## Files Expected To Change

- `include/BrokerAdapterContracts.hpp`, `include/BrokerGateway.hpp`,
  `include/ExecutionEngine.hpp`
- `src/BrokerGateway.cpp`, `src/ExecutionEngine.cpp`
- `include/providers/ctrader/*`, `src/providers/ctrader/*`
- `tests/ctrader_provider_architecture_tests.cpp`, `CMakeLists.txt`
- This plan, the architecture review, and the documentation index as needed.

## Implementation Steps

1. Add `MarketDataEvent` and `IMarketDataSource` to the existing normalized
   provider contract header.
2. Add append-only normalized event callbacks to `BrokerGateway`; bind
   `ExecutionEngine` to them and route queued orders through
   `normalizeOrder`/`dispatchOrder`.
3. Retain the legacy `BrokerFill` facade for non-tranche callers, but remove it
   from the `ExecutionEngine` callback and dispatch path.
4. Add a no-I/O cTrader adapter skeleton that implements both public provider
   interfaces and composes named private services.
5. Add synthetic tests for execution lifecycle callbacks, market-data event
   delivery, skeleton fail-closed behavior, and absence of a runtime change.

## Verification

- Configure and build the default build tree.
- Run the new provider-architecture test, `phase13_tests`, `phase22_tests`,
  containment tests, then the full CTest suite sequentially.
- Run `git diff --check` and documentation audits.

## Risks

- `BrokerGateway` and execution files contain unrelated uncommitted WP-2 work;
  edits must be additive and reviewed separately from that work.
- Event callback lifetime/concurrency must remain gateway-owned and no event
  may cause portfolio mutation before lifecycle validation.
- A skeleton must not accidentally link Protobuf, OpenSSL, cURL, Keychain, or
  an opt-in proof runtime into the default build.

## Rollback

Revert only the tranche-scoped additions and restore the legacy
`ExecutionEngine` binding. No provider state exists to roll back.

## Progress Log

- 2026-08-20: Plan created; implementation authorized by the operator as a
  local, default-off, no-provider-activity tranche.
- 2026-08-20: Added event-native `ExecutionEngine` dispatch/callback flow,
  `MarketDataEvent`/`IMarketDataSource`, a fail-closed cTrader provider
  skeleton, and synthetic coverage. Moved cTrader OAuth correlation support
  under the provider path without changing its behavior.
- 2026-08-20: The first full CTest run exposed the WP-2 correctness observer
  still listening to the legacy `BrokerFill` facade. It was migrated to
  `ExecutionEvent`; its targeted rerun passed.

## Deviations

None at plan creation.

## Completion Evidence

- Default configure/build completed with the normal two pre-existing warnings.
- Focused lifecycle/provider tests passed. The first full CTest run found the
  legacy benchmark observer mismatch; after its `ExecutionEvent` migration,
  the targeted correctness workload passed and the final full default suite
  passed 15/15.
- `git diff --check`, offline CI policy checks, and automation validation
  passed. Documentation audit found no placeholder markers.

## Final Outcome

Complete locally. The cTrader provider skeleton remains disconnected and
unsupported; provider process, credential use, subscriptions, orders, and live
use remain prohibited pending separate authority.
