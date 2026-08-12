# cTrader Open API Gate 7 — Fresh XAUUSD Market-Data Proof

## Status

Wade separately authorized Gate 7 on 2026-08-10. The isolated implementation,
deterministic offline validation, and subsequent offline-only OAuth diagnostic
hardening are complete. The initial provider process stopped at Keychain
access. Wade then authorized one new bounded retry; it reached fresh OAuth
authorization and stopped at the sanitized blocker `gate7_oauth_failed` before
account discovery or fixed-endpoint data traffic. Gate 7 did not pass; no
provider response, symbol metadata, quote, or timestamp evidence was obtained.
Gates 8–9, orders, and live trading remain unauthorized.

## Boundary

Gate 7 is a default-disabled, macOS-only proof target. It is detached from
`BrokerGateway`, `LiveDataAdapter`, `ExecutionEngine`, `RiskEngine`,
`SystemConfig`, and the `BACKTEST`, `PAPER`, and `LIVE` runtime modes. Its
fixed provider endpoint is `demo.ctraderapi.com:5035`; live hosts and endpoint
fallbacks are not representable.

The outbound allowlist contains only heartbeat, application authentication,
access-token account-list discovery, account authentication, non-archived
symbol-list retrieval, full-symbol lookup by a fresh response-derived ID,
subscription/unsubscription for exactly one symbol's spot stream, and the
fixed provider error/heartbeat handling needed to fail closed. Trading, order,
position, depth, trendbar, historical-data, and reconnect payloads are rejected
before serialization or socket write. Gate 6's immutable boundary remains
separate and unchanged.

## Selection and validation rules

- Account selection requires a fresh list, `SCOPE_TRADE`, present `isLive=false`,
  present exact `brokerTitleShort=FIBO`, a positive response-derived ID, and
  exactly one match. No cached ID is accepted.
- Symbol selection excludes archived entries and requires exactly one enabled
  light symbol with a positive response-derived ID and a name that canonicalizes
  only `XAUUSD` or `XAU/USD` by ASCII case folding and removal of at most one
  slash. `GOLD`, suffixes, aliases, and substrings are rejected.
- Full-symbol retrieval must match the fresh light-symbol ID. Required digits,
  pip position, positive volume bounds/step/lot metadata, supported scales,
  checked arithmetic, `minVolume <= maxVolume`, zero-anchored step consistency,
  and light/full consistency are required before constructing `InstrumentSpec`.
- A spot event is accepted only after the matching subscription response and
  current connection generation. Account/symbol IDs, timestamp, positive
  scale-5 bid/ask, exact and normalized non-crossing, and checked spread are
  required. Prices use checked integer conversion and ties-away-from-zero scale
  reduction; no floating-point or saturation path is used.
- The provider timestamp is tested as seconds, milliseconds, microseconds, and
  nanoseconds. Exactly one interpretation must be within 120 seconds old and 5
  seconds future of bounded local receipt time; otherwise the result is
  `timestamp_unit_unproven`.

## Offline evidence

`ctrader_gate7_tests` covers allowlists, endpoint pinning, fresh account and
canonical symbol selection, metadata and volume rules, integer arithmetic and
overflow, crossed/missing/oversized quotes, generation/correlation/order,
subscription ordering, timestamp classification, malformed/provider/timeout/
cancellation/allocation failures, fixed OAuth listener/browser/timeout/callback/
denial/state-correlation diagnostics, state clearing, and the inability to
place, modify, cancel, or close an order. The callback runtime uses the actual
accepted loopback peer address, a nonblocking accepted socket, and a two-second
inactivity deadline capped by the absolute correlation deadline. Callback
buffer allocation failure emits only the fixed resource-exhaustion category.
The normal build remains unchanged unless
`TRADEBOT_ENABLE_CTRADER_GATE7=ON` is supplied.

## Provider outcome

Presence-only configuration/Keychain preflight passed for the retry. Exactly
one new bounded Gate 7 process was started. It passed the actual Keychain read,
entered fresh OAuth authorization, emitted only `gate7_oauth_failed`, and exited
with code 1 before account discovery or fixed-endpoint data traffic. No
reconnect or further provider session occurred. No account identifier,
credential, token, raw price, or raw payload was persisted.

The exact next action is Wade review of the draft PR and updated offline
evidence. The one authorized provider retry is exhausted; do not begin Gate 8
or retry provider traffic again within this task.
