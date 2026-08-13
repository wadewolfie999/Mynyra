# cTrader Open API Gate 7 — Fresh XAUUSD Market-Data Proof

## Status

Wade separately authorized Gate 7 on 2026-08-10. The isolated implementation,
deterministic offline validation, and subsequent offline-only OAuth diagnostic
hardening are complete. Historical processes stopped at Keychain access,
generic OAuth failure, and a fixed `gate7_oauth_callback_timeout`. The latest
authorized process advanced beyond OAuth, the fixed demo TLS connection,
application authentication, fresh demo-account selection/authentication,
canonical XAUUSD resolution, and full metadata validation, then emitted
`gate7_subscription_failed`. Gate 7 did not pass; the subscription subcause is
unclassified and no accepted subscription, quote, or timestamp evidence exists.
Bounded residual diagnostics and first-single-complete-BBO work were completed
under `PLAN-20260813-ctrader-gate7-residual-diagnostics-and-proof`. That plan is
historical rather than current execution authority. Any continuation must map
to WP-6 in `REPOSITORY_REMEDIATION_PROGRAM.md` after predecessor acceptance and
receive new exact authorization. Provider execution remains blocked. Gates
8–9, orders, and live trading remain unauthorized.

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

The residual Gate 7 path adds the documented account-disconnect event to the
inbound fail-closed controls. It does not broaden the outbound allowlist. Typed
transport outcomes distinguish local send failure, timeout, TLS/peer closure,
common/Open API provider error category, token invalidation, account/client
disconnect, unexpected allowed payload, correlation mismatch, malformed
envelope, rejected inbound type, and resource exhaustion. Raw provider values
are cleared and only fixed reviewed diagnostics are emitted.

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
- A well-formed exact-identity event missing one side or timestamp is
  incomplete, never successful. No side or timestamp is retained or combined
  across events. Gate 7 continues only inside the unchanged absolute spot
  deadline and accepts the first single event satisfying the complete BBO and
  timestamp contract. Wrong identity, malformed input, invalid/overflowing
  prices, raw/normalized crossing, and protocol violations remain immediately
  terminal.
- The provider timestamp is tested as seconds, milliseconds, microseconds, and
  nanoseconds. Exactly one interpretation must be within 120 seconds old and 5
  seconds future of bounded local receipt time. Stale, future, and ambiguous/
  overflowing interpretations remain distinct fixed outcomes.
- During response and spot waits, only the already-allowed heartbeat may be
  sent on a nine-second monotonic cadence. It cannot reconnect or extend the
  original absolute deadline.

## Offline evidence

`ctrader_gate7_tests` covers allowlists, endpoint pinning, fresh account and
canonical symbol selection, metadata and volume rules, integer arithmetic and
overflow, crossed/missing/oversized quotes, generation/correlation/order,
subscription ordering, every typed send/receive/provider mapping, disconnect
controls, partial-event continuation, first-single-complete-BBO behavior,
timestamp classification, heartbeat deadline bounds, malformed/provider/
timeout/cancellation/allocation failures, fixed OAuth listener/browser/timeout/
callback/denial/state-correlation diagnostics, state clearing, and the
inability to place, modify, cancel, or close an order. Both OAuth and residual
diagnostic sets are checked for fixed, bounded, value-free literals. The
callback runtime uses the actual accepted loopback peer address, a nonblocking
accepted socket, and a two-second inactivity deadline capped by the absolute
correlation deadline. Callback buffer allocation failure emits only the fixed
resource-exhaustion category. The normal build remains unchanged unless
`TRADEBOT_ENABLE_CTRADER_GATE7=ON` is supplied.

The residual subscription wait preserves the raw-payload prohibition while
classifying a rejected message type into closed schema-derived categories:
prior-stage response, unrequested unsubscribe response, spot-before-
acknowledgement, symbol-change event, trader-update event, prohibited
order/risk/depth asynchronous event, other pinned-schema payload, or unknown
payload. No category generically admits or ignores the message. The stored
credential path consumes the Gate 6 `TBG6TOK1` envelope using Gate 6's 32-bit
field lengths. Refreshed or exchanged credentials remain in memory for the
bounded process and are not written to Keychain by Gate 7. Successful proof
output contains fixed markers only and omits all quote, timestamp, identifier,
and numeric metadata values.

## Provider outcome

The execution chronology is cumulative: Keychain boundary; generic OAuth
boundary; hardened callback timeout; then latest subscription transition. The
latest process's progress beyond dedicated earlier failure markers is
control-flow evidence that it passed OAuth, fixed demo TLS, application/account
authentication, canonical XAUUSD resolution, and full metadata validation. It
is not evidence that the subscription request was written or accepted, or that
a spot event was received. No account identifier, credential, token, raw price,
or raw payload belongs in repository evidence.

The residual offline patch, full verification matrix, final review, and
sanitized persistent evidence template are complete. Wade authorized one local
commit and exact-commit rebuild; their identities are recorded in the ignored
handoff and do not authorize provider traffic. The old provider checkpoint is
not the current next action. Under the remediation focus lock, future provider
integration is WP-6 work and requires accepted predecessors plus new exact
approval. Do not retry or begin Gate 8.
