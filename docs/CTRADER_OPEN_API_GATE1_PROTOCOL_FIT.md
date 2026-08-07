# cTrader Open API Gate 1: Protocol-Fit Revalidation

## Document Control

- Status: Revalidated for design; no connection or implementation authorized
- Date: 2026-08-07
- Branch: `codex/ctrader-open-api-gate5`
- Accepted baseline: `400b486a6af64c653a54b7e7080dbb59bce90cd8`
- Target: FIBO Group through cTrader, demo-only XAUUSD
- Verdict: `GATE 1 REVALIDATED`

This is an offline design decision. No OAuth flow, token exchange, DNS lookup,
socket connection, account request, symbol request, quote request, or order
operation was performed.

## Controlling Official Sources

- [Getting started](https://help.ctrader.com/open-api/)
- [Proxies and endpoints](https://help.ctrader.com/open-api/proxies-endpoints/)
- [Establish a connection](https://help.ctrader.com/open-api/connection/)
- [Protobuf and JSON](https://help.ctrader.com/open-api/protocol-buffers-json/)
- [Common messages](https://help.ctrader.com/open-api/common-messages/)
- [App and account authentication](https://help.ctrader.com/open-api/account-authentication/)
- [Messages](https://help.ctrader.com/open-api/messages/)
- [Model messages](https://help.ctrader.com/open-api/model-messages/)
- [Attain symbol data](https://help.ctrader.com/open-api/symbol-data/)
- [Official proto-message repository at pinned revision](https://github.com/spotware/openapi-proto-messages/tree/3fd8bddfbe0cfc2ecfda079623dc4e498af11e66)

Official documentation and the pinned schemas are provider facts. The
TradeBot-specific restrictions below are local architecture decisions.

## Selected Transport And Session

The selected protocol is cTrader Protobuf over a persistent TLS/TCP session:

```text
demo.ctraderapi.com:5035
```

- Host and port are compile-time constants for the proof boundary.
- TLS peer-chain and hostname verification, SNI for
  `demo.ctraderapi.com`, and failure before application authentication are
  mandatory.
- JSON/port `5036`, WebSocket, every live hostname, runtime endpoint selection,
  alternate hosts, redirects to user-supplied hosts, and fallback lists are
  rejected.
- The session owns one ordered outbound queue and one incremental inbound
  parser. Concurrent writes cannot interleave frames.
- A disconnect destroys application/account authentication state, pending
  correlations, response-derived account and symbol IDs, and subscriptions.

The existing `AsyncNetworkClient` is not reused unchanged: it is WebSocket/REST
oriented, defaults strict TLS validation to false, and its dynamic OpenSSL
loader is not a verified macOS cTrader TLS boundary. Gate 6A must use a dedicated
strict TLS/TCP transport behind a narrow interface.

## Serialization, Framing, And Binding Strategy

The selected serialization is Protobuf. Every cTrader message is serialized
inside the official `ProtoMessage` envelope:

| Envelope field | Schema type | Rule |
| --- | --- | --- |
| `payloadType` | required `uint32` | Must match the decoded payload message type. |
| `payload` | optional `bytes` | Required locally for every non-empty application message and decoded only as the expected type. |
| `clientMsgId` | optional `string` | Required locally for each request that expects a correlated response. |

Each TCP frame is the documented four-byte message length followed by the
serialized `ProtoMessage`. On this little-endian development machine the
length bytes are reversed as the official guide requires, producing the
most-significant byte first on the wire. The parser accumulates fragmented
reads, handles multiple complete frames in one read, rejects impossible or
locally over-limit lengths before allocation, and rejects truncated or
type-inconsistent payloads.

Gate 6A binding strategy:

1. Vendor the four official `.proto` inputs from upstream commit
   `3fd8bddfbe0cfc2ecfda079623dc4e498af11e66` with their upstream URL,
   commit, license/provenance note, and SHA-256.
2. Generate C++ bindings into the build tree using an explicitly pinned
   `protoc` and matching C++ Protobuf runtime selected through the repository's
   dependency-review process. Do not hand-maintain provider message structs.
3. Compile generated files only into the isolated cTrader proof target; do not
   add them to the deterministic core library or expose them above the
   provider boundary.
4. Preserve proto2 presence accessors and test every optional field used for a
   safety decision.

No `protoc` or Protobuf runtime is installed in the current workspace. Gate 1
selects the strategy and pinned schema; dependency installation, version lock,
generation, and source changes remain Gate 6A prerequisites requiring separate
authorization.

## Request/Response Correlation

- Allocate a unique, non-secret local `clientMsgId` for every request.
- A response is accepted only when its `clientMsgId`, expected payload type,
  connection generation, and expected account ID (when present) all match the
  outstanding request.
- Unknown, absent where locally required, duplicate, late, or already-consumed
  correlations fail the state-machine step; they are never reassigned to the
  oldest request.
- Unsolicited provider events are routed by payload type and validated account
  and symbol IDs, not mistaken for responses.
- Correlation values contain no token, account ID, login, symbol ID, or secret.

## Heartbeat And Session Liveness

Official guidance requires `ProtoHeartbeatEvent` at least once every ten
seconds to avoid inactivity disconnects. The proof transport schedules a
heartbeat on the single outbound queue so the interval never exceeds ten
seconds while connected. A heartbeat is a liveness signal only; it does not
prove application authentication, account authentication, subscription
continuity, or market freshness.

Missing reads, TLS errors, provider disconnect events, account-disconnect
events, token-invalidated events, malformed frames, or an exhausted liveness
deadline transition the session to unauthenticated and enter bounded reconnect
handling.

## Authentication And Account Discovery Sequence

Gate 6 is the umbrella `Gate 6A → mandatory Wade checkpoint → Gate 6B`.
After a separately authorized OAuth/token gate, Gate 6A must:

1. Establish strict TLS/TCP only to the fixed demo endpoint.
2. Send `ProtoOAApplicationAuthReq` with Keychain-sourced application
   credentials and wait for `ProtoOAApplicationAuthRes`.
3. Send `ProtoOAGetAccountListByAccessTokenReq` using the `accounts` token and
   wait for `ProtoOAGetAccountListByAccessTokenRes`.
4. Accept only the correlated current-generation response, require its required
   `accessToken` to equal the request token without logging either value, and
   require `permissionScope` presence and `SCOPE_VIEW` for the read-only proof.
5. Record the bounded candidate identity evidence required by Gate 5 and stop
   before account authentication.

After the mandatory Wade checkpoint and separate Gate 6B authorization, Gate
6B must repeat current-generation application authentication and account
discovery, select exactly one exact Wade-approved demo match, range-check the
fresh response-derived `uint64 ctidTraderAccountId` for the signed `int64`
account-auth field, send `ProtoOAAccountAuthReq`, and accept only the matching
`ProtoOAAccountAuthRes`.

The visible cTrader login/account number is UI metadata only. It is never an
API identifier. No account, symbol, or market request may skip or reorder the
sequence.

## XAUUSD Discovery And Market-Data Fit

The protocol supports the required later read-only milestone:

1. After account authentication, request `ProtoOASymbolsListReq` with archived
   symbols excluded.
2. From `ProtoOASymbolsListRes`, require exactly one enabled, non-archived
   light symbol whose present `symbolName`, after ASCII case normalization and
   removal of at most one `/` separator, equals `XAUUSD`. Do not alias `GOLD`
   or guess broker suffixes.
3. Obtain the response-derived server `symbolId`; never hardcode it.
4. Request `ProtoOASymbolByIdReq` and validate the matching full
   `ProtoOASymbol`, including required `digits` and `pipPosition` and present
   volume/lot metadata required by Gate 2.
5. Subscribe with `ProtoOASubscribeSpotsReq` for only that symbol and set
   `subscribeToSpotTimestamp=true`.
6. Accept a `ProtoOASpotEvent` only for the authenticated account and selected
   symbol. Because `bid` and `ask` are optional, absence is not zero and a
   usable BBO requires both present and positive.

Actual FIBO symbol spelling and metadata are runtime evidence for the later
market-data gate. Zero or multiple canonical matches, suffixes, missing names,
disabled/archived status, or contradictory full metadata stop without a
subscription.

## Reconnect And Resubscribe

Reconnect uses bounded exponential backoff with jitter and a fixed attempt
cap; it never changes host, port, serialization, or scope. Every new connection
generation repeats:

```text
TLS -> application auth -> account list -> deterministic demo selection
    -> account auth -> symbol list -> full symbol metadata -> spot subscription
```

No cached account or symbol ID authorizes a shortcut. A quote is publishable
only after the new subscription acknowledgement and a new-generation spot
event. Exhaustion, authentication failure, changed account eligibility, or
changed symbol identity is terminal for the proof.

## Later Order-Operation Capability

The pinned schema contains `ProtoOANewOrderReq`, execution events, amend,
cancel, close, and reconciliation messages, so the provider protocol can
support the eventual controlled demo-order milestone. Those messages require
trade permission and are outside Gates 1–5. The read-only proof target must not
expose constructors or dispatch paths for them. A later explicit directive,
separate `trading` authorization, numeric validation, risk integration, and
controlled-order gate are mandatory.

## Repository Fit And Future Components

- Preserve `BACKTEST` and all current production behavior unchanged.
- Keep provider schemas below `BrokerGateway` as required by ADR 0003.
- Gate 6—the complete Gate 6A/checkpoint/Gate 6B umbrella—has a smallest source
  surface consisting of a separate proof executable/library, a strict
  TLS/TCP transport interface, frame codec, generated-schema wrapper, OAuth
  callback/token interfaces, account-proof state machine, Keychain adapter,
  deterministic fakes, tests, and CMake target.
- Do not attach the proof to `BrokerGateway`, `ExecutionEngine`,
  `LiveDataAdapter`, `SystemConfig` runtime modes, or `AuthManager`.
- A later broker adapter may reuse reviewed lower-level components, but it must
  remain below `BrokerGateway` and pass separate execution/risk authorization.

## Gate 1 Disposition

Transport, endpoint/port, session, serialization, framing, schema provenance,
binding strategy, request correlation, heartbeat, authentication ordering,
account discovery, XAUUSD discovery/subscription, reconnect, and later provider
order capability are now decided for the current baseline. Installation,
generation, connectivity, OAuth, credentials in use, and all runtime proof are
not authorized.

`GATE 1 REVALIDATED`
