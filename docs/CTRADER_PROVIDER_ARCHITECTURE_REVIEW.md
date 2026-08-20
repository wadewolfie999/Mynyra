# cTrader Provider Architecture Review

## Status And Scope

This is the governing first-refactor-tranche baseline, based on the class definitions and
module wiring in the repository at `179817972cf469c67384f5f8c2245ecf09044621`.
It is an architecture review, not implementation authority. It does not
authorize credential access, provider traffic, account access, order messages,
or live trading. The first local, default-off structural slice is implemented;
the provider process and every capability that could use it remain outside the
implementation boundary.

The objective is to promote cTrader from a collection of isolated proof
executables into a provider subsystem that can attach below `BrokerGateway`
without exposing Open API details to the core.

## First-Tranche Implementation Status

- `ExecutionEngine` now dispatches `NormalizedOrder` through
  `BrokerGateway::dispatchOrder` and consumes normalized acknowledgement and
  execution callbacks. `BrokerFill` remains a compatibility facade for
  un-migrated callers only.
- `MarketDataEvent` and `IMarketDataSource` live in
  `BrokerAdapterContracts.hpp`, the existing owner of normalized adapter
  contracts. A separate header would have introduced an unowned parallel
  contract surface.
- `providers/ctrader/CTraderProviderAdapter` is a no-I/O skeleton that
  implements `IBrokerAdapter` and `IMarketDataSource`, composes named internal
  services, and fails closed for every capability.
- cTrader OAuth correlation support is located under `providers/ctrader/`.
  It remains behaviorally unchanged and is still exercised only by synthetic
  offline tests.

## Findings

### 1. The broker-neutral execution seam exists, but the active execution path does not use it end-to-end

`BrokerAdapterContracts.hpp`, `IBrokerAdapter`, `OrderLifecycleStore`, and
the modern `BrokerGateway` API already express the needed normalized
order-side boundary: `NormalizedOrder`, `OrderAcknowledgement`,
`ExecutionEvent`, `CancelResult`, `AccountSnapshot`, `InstrumentSpec`, and
`AdapterHealthEvent`. `DeterministicBrokerAdapter` proves that an adapter can
sit below that boundary.

However, `ExecutionEngine` still calls the compatibility
`BrokerGateway::submitOrder`/`BrokerFill` path. That path converts `double`
values into the normalized model and then converts execution events back into
`BrokerFill`. A cTrader adapter connected only to the modern callbacks would
therefore be forced through a lossy, legacy compatibility bridge. This is the
highest-priority core migration prerequisite for a real provider.

### 2. The cTrader implementation is proof-shaped, not provider-shaped

`CTraderGate6Runtime.mm` and `CTraderGate7Runtime.mm` each combine OAuth
browser/callback handling, Keychain access, token parsing, strict TLS/TCP,
Protobuf framing, request correlation, request allowlisting, protocol decode,
account discovery, instrument lookup, subscription flow, diagnostics, and
process orchestration. The runtime files are 2,002 and 2,309 lines
respectively. Their companion proof classes are state machines for a specific
evidence corridor, rather than provider services exposed through
`IBrokerAdapter`.

This makes the proof targets valuable safety evidence, but prevents their
reviewed code from becoming a coherent adapter. The same endpoint, OAuth, and
secure-storage constants are duplicated in `CTraderGate6Config` and
`CTraderGate7Config`. `CTraderOAuthCorrelationGuard` is also compiled into
`tradebot_core_lib`, even though it is cTrader-specific infrastructure.

### 3. Provider protocol details have not reached the core, but the ownership boundary is backwards in places

The Gate 7 proof includes `BrokerAdapterContracts.hpp` and emits a
`Gate7QuoteEvidence` containing an `InstrumentSpec`; this is a useful mapping
experiment. It is not an adapter interface, and it retains cTrader account and
symbol ID state in a gate-specific class. The core is clean of Protobuf types,
but cTrader semantic mapping is stranded in a proof class and cannot publish
normalized account, instrument, health, or market-data events to core-owned
consumers.

The target is one-way dependency: core contracts may be used by a cTrader
module; Protobuf-generated headers, cTrader account IDs, payload types, token
formats, callback values, and socket objects may not appear in core headers,
`BrokerGateway`, `ExecutionEngine`, `RiskEngine`, `PortfolioManager`, replay,
or analytics.

### 4. Legacy exchange assumptions must be contained, not adapted to cTrader

`AsyncNetworkClient` is a WebSocket and REST client with signed-request
semantics. `AuthManager` owns HMAC credentials and retains `BINANCE_API_*`
fallbacks. `LiveDataAdapter` owns a JSON candle parser and direct endpoint
connection from `SystemConfig`; its tests use `BTCUSDT`-shaped payloads.
`SystemConfig` exposes generic WSS/REST endpoints and API-key fields.

These components are legacy exchange infrastructure. cTrader Open API uses a
different OAuth, token, strict TLS/TCP, and Protobuf protocol. Reusing the
legacy transport or auth APIs would leak their assumptions into the new
provider and create a second hybrid protocol boundary. They must not be the
cTrader adapter's transport or credential dependency.

### 5. Market data has no normalized provider ingress

The broker contracts cover order-side events but define no core market-data
event or source interface. `LiveDataAdapter` is a legacy live-like candle
queue, not a generic provider ingress. Without a small normalized market-data
contract, Gate 7's validated quote mapping has nowhere to go except the proof
runtime or a cTrader-specific path in core.

## Proposed First Refactor Tranche

Create a default-disabled `providers/ctrader` module that is the *only*
consumer of cTrader Protobuf, OAuth, Keychain, TLS/TCP, native account IDs,
and provider payload codes. Its exported core-facing surface is an
`IBrokerAdapter` implementation plus a separate normalized market-data source.
Do not add runtime selection, credentials, provider processes, order traffic,
or reconnect behavior in this tranche.

### Target Module Shape

```text
core
  BrokerGateway -> IBrokerAdapter -> CTraderProviderAdapter
  MarketDataIngress <- IMarketDataSource <- CTraderProviderAdapter

providers/ctrader (private implementation)
  CTraderSession
    -> CTraderTransport (strict TLS/TCP + framed bytes)
    -> CTraderCodec (Protobuf <-> provider-private records)
    -> CTraderOAuthClient / CTraderSecureTokenStore
    -> CTraderAccountService / CTraderInstrumentService
    -> CTraderOrderService / CTraderMarketDataService
```

`CTraderProviderAdapter` is the composition root and the only cTrader class
visible outside the module. It owns one ordered `CTraderSession` and maps its
results to the existing normalized broker contracts. The session owns native
ID correlation, request correlation, current connection generation, and
provider-private error classification. Services must not call
`PortfolioManager`, `RiskEngine`, or strategies.

The provider module should be split into the following files rather than
retaining the Gate 6/7 runtime monoliths:

| Component | Responsibility | Core-visible types |
| --- | --- | --- |
| `CTraderProviderAdapter` | Implements `IBrokerAdapter`; installs callbacks and turns normalized requests into session commands | Existing broker contracts only |
| `CTraderSession` | Serializes protocol state and owns current account/session generation | None |
| `CTraderTransport` | Strict TLS/TCP connection, bounded framing, read/write deadlines, and transport health | Provider-private transport result |
| `CTraderCodec` | Protobuf envelope/message encoding and decoding; clears native messages | Provider-private records |
| `CTraderOAuthClient` and `CTraderSecureTokenStore` | OAuth callback correlation, token lifecycle, and secure storage | Presence/status only; never tokens |
| `CTraderAccountService` | Native account discovery/authentication mapped to `AccountSnapshot` | `AccountSnapshot` only |
| `CTraderInstrumentService` | Native symbol metadata mapped to `InstrumentSpec` and canonical aliases | `InstrumentSpec` only |
| `CTraderMarketDataService` | Subscription state and quote mapping | `MarketDataEvent` only |
| `CTraderOrderService` | Submit/cancel/status/reconciliation mapping | Existing lifecycle contracts only |

The existing Gate 6/7 proof executables should remain as default-disabled,
offline-verifiable harnesses until their tested helpers have been extracted.
They should call the provider-private services or fixture seams; they must not
remain the production adapter implementation.

### New Core Contract: Market Data

Add a small provider-neutral contract next to the existing broker contracts:

```cpp
struct MarketDataEvent {
    std::uint32_t schemaVersion{1};
    std::string canonicalSymbol;
    Decimal64 bid;
    Decimal64 ask;
    std::uint64_t sourceTimestampNs{0};
    std::uint64_t sequence{0};
    std::uint64_t instrumentVersion{0};
    AdapterHealthState quality{AdapterHealthState::Unknown};
    std::string eventKey;
};

class IMarketDataSource {
public:
    using Callback = std::function<void(const MarketDataEvent&)>;
    virtual ~IMarketDataSource() = default;
    virtual void setMarketDataCallback(Callback) = 0;
};
```

The exact name may vary, but its fields must be normalized, timestamp-unit
explicit, sequenceable, and free of native symbol/account IDs and raw payloads.
`CTraderProviderAdapter` may implement both `IBrokerAdapter` and
`IMarketDataSource`; `BrokerGateway` consumes only the former. A later,
separately approved market-data ingress can consume the latter. Do not route
cTrader quotes through the legacy JSON parser.

### Ordered Implementation Slice

1. Add `MarketDataEvent` and `IMarketDataSource`, with no provider selection
   or runtime behavior change.
2. Migrate `ExecutionEngine` from `BrokerFill` to
   `OrderAcknowledgement`/`ExecutionEvent` callbacks. Keep the old compatibility
   API only as an isolated test adapter bridge until callers are migrated.
3. Move cTrader-only support under `include/providers/ctrader/` and
   `src/providers/ctrader/`; deduplicate endpoint/security policy and keep
   `CTraderOAuthCorrelationGuard` inside that module. This is a move and
   extraction, not a change to provider capability.
4. Extract codec, strict transport, OAuth/secure storage, account, instrument,
   and market-data services behind provider-private interfaces. Preserve the
   current default-disabled build and fixture-only tests.
5. Add a default-disabled `CTraderProviderAdapter` that implements the core
   interfaces with injected fake transport/codec seams. Its order methods must
   fail closed until a separately approved order-lifecycle scope exists.

Do not add an adapter factory, wire it into `SystemConfig`, make it selectable
from the CLI, or alter `BACKTEST`, `PAPER`, or `LIVE` in this tranche. That
prevents a structural refactor from implicitly becoming connectivity or order
work.

## Required Invariants

- `BACKTEST` has no network, OAuth, Keychain, wall-clock, or provider-state
  dependency.
- `PAPER` remains the deterministic adapter; it cannot silently select cTrader.
- Every risk-increasing order follows `ExecutionEngine` -> `RiskEngine` ->
  `BrokerGateway` -> `IBrokerAdapter`; no cTrader class bypasses this path.
- Acknowledgement is not a fill. Execution, cancel, health, and reconciliation
  events are idempotent by a stable normalized event key.
- An incomplete or stale instrument/account snapshot blocks normalization or
  dispatch. Unknown cancel, disconnect, auth failure, malformed input, or
  reconciliation mismatch fail closed for new exposure.
- Only the cTrader module handles Protobuf messages, native IDs, OAuth codes,
  tokens, Keychain objects, TLS sockets, callback requests, and raw provider
  errors. They are cleared and never published to logs, callbacks, state,
  replay fixtures, or analytics.
- Market-data events are accepted only with valid bid/ask, explicit timestamp
  conversion, known instrument version, current session generation, and a
  monotonic per-session sequence. Provider-native IDs do not cross the boundary.
- The generic `AsyncNetworkClient`, `AuthManager`, `LiveDataAdapter`, and
  `SystemConfig` are not dependencies of the cTrader provider module.

## Acceptance Criteria And Tests

The first implementation plan should be accepted only when all of the
following have evidence:

| Acceptance criterion | Required offline tests |
| --- | --- |
| No cTrader native type leaks across public core headers | Compile-only boundary test plus source include/dependency check |
| Existing core lifecycle path is event-native | Unit tests for reject, acknowledgement-without-fill, partial fill, duplicate event, cancel race, and unknown state |
| Provider codec maps only normalized records | Protobuf fixture tests for account, instrument, quote, acknowledgement, execution, cancel, health, and malformed envelopes |
| cTrader state is session-safe | Current-generation, correlation, native-ID clearing, timestamp-unit, stale/incomplete metadata, and duplicate sequence tests |
| Transport/auth does not affect core determinism | Fake transport/token-store tests; default CTest runs without provider dependencies or credentials |
| Quote ingress is safe | Valid BBO, crossed/zero/missing side, unknown symbol, stale/future timestamp, and out-of-order event tests |
| Defaults remain contained | Tests prove no cTrader selection in `BACKTEST`/`PAPER`, no runtime mode change, and all order methods fail closed without explicit future authorization |

Run the ordinary default-off configure/build/CTest path first. Run cTrader
module tests only against synthetic fixtures and fake transport/token-store
implementations. Provider connectivity, OAuth browser flow, Keychain value
access, account selection, subscriptions, orders, reconnect, and live trading
are expressly outside the test plan.

## Decisions Required Before Implementation

1. Approve this module boundary and whether the proposed market-data contract
   belongs in `BrokerAdapterContracts.hpp` or a dedicated provider-contract
   header.
2. Approve a separate implementation plan with file scope, migration order,
   rollback, and test matrix.
3. Keep any provider process, credential access, market-data subscription,
   order lifecycle activation, and live use under separate explicit authority.
