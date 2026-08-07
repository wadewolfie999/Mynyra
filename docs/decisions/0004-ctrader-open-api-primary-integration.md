# ADR 0004: cTrader Open API Is The Sole Integration Path

## Status

Accepted architecture direction; Gate 2 and Gate 5 were accepted by Wade on
2026-08-07, Gate 5.1 and the Gate 6 umbrella are blocked, and
implementation remains separately gated

## Context

Wade reported the cTrader Open API application `TradeBot Demo Integration` as
`Active` on 2026-08-06. Approval removes the blocker that justified parallel
work on a cTrader Algo/cBot Bridge. The target remains FIBO Group through
cTrader, demo-only XAUUSD, with the eventual milestone of one separately
authorized controlled demo order. OANDA is permanently cancelled and live
trading remains prohibited.

The accepted broker-neutral boundary in ADR 0003 remains authoritative. The
Open API adapter must stay below `BrokerGateway`; provider messages, account
identifiers, credentials, and endpoint behavior cannot leak into strategy,
risk, portfolio, replay, or analytics code.

## Decision

- Official cTrader Open API is the sole integration path.
- The cTrader Algo/cBot Bridge is
  `ABANDONED — NON-CONTROLLING — OUT OF SCOPE`; it is not an alternative,
  fallback, evidence source, or implementation dependency.
- Gate 5 is accepted design-only evidence and follows
  `docs/CTRADER_OPEN_API_GATE5.md`; acceptance does not authorize execution.
- Initial authorization uses only the cTrader `accounts` scope. `trading`
  requires a later explicit operator authorization.
- The fixed local callback is
  `http://127.0.0.1:18080/ctrader/oauth/callback` and must be registered and
  verified before an OAuth execution gate.
- cTrader's official authorization parameter table does not document `state`,
  PKCE, or another request-to-callback correlation mechanism. OAuth execution
  remains blocked until separately authorized Gate 5.1 proves secure
  correlation or Wade approves a reviewed alternative. Gate 5.1 stops before
  code exchange or token request.
- Secrets and tokens use macOS Keychain; authorization codes are memory-only;
  tracked files, fixtures, logs, commands, and reports contain no values.
- The only runtime Open API message endpoint is
  `demo.ctraderapi.com:5035`, using Protobuf over strict TLS/TCP. There is no
  live-host option, automatic fallback, configurable fallback, or live-account
  authentication path.
- Account IDs are extracted only from
  `ProtoOAGetAccountListByAccessTokenRes`. A visible login/account number is
  never treated as `ctidTraderAccountId`.
- Account proof is staged. Separately authorized Gate 6A retrieves the
  authenticated account list with `accounts` scope, excludes live and
  environment-ambiguous entries from candidacy, retains each
  `ctidTraderAccountId` only in volatile process memory, and presents Wade only
  safe selection metadata: broker-title presence/value, demo/live status,
  optional API-supplied visible account metadata, and a non-reusable local
  candidate label when needed. It stops before account authentication and does
  not require a previously known FIBO title. If safe metadata cannot
  distinguish exactly one intended demo account, it stops without exposing the
  API identifier.
- Only after Wade approves an exact safe-field selection predicate may
  separately authorized Gate 6B repeat authenticated discovery, reproduce that
  selection deterministically, require exactly one exact demo match, retain the
  fresh response-derived `ctidTraderAccountId` only in volatile process memory,
  and use it solely for that session's account-authentication request. A visible
  login is optional human confirmation, never the API ID. The mere presence of
  another live entry is not failure; resemblance or partial title matching is
  never selection.
- Wade-authorized Gates 1-3 revalidation selects and pins transport, framing,
  official proto2 schema provenance, cTrader numeric mapping, and the
  pre-implementation baseline/local-diff boundary. The evidence is design-only
  and does not authorize dependencies, generation, implementation, or the Gate
  6 umbrella (`Gate 6A → mandatory Wade checkpoint → Gate 6B`).
- Gate 5 defines but does not execute Gate 5.1, Gate 6A, or Gate 6B and does not
  authorize OAuth, connectivity, account requests, market data, orders, source
  implementation, merge, push, or live behavior.

## Alternatives Considered

- Reuse a visible cTrader login as the API account ID: rejected because the
  official API returns a distinct `ctidTraderAccountId` during discovery.
- Permit both demo and live hosts in configuration: rejected because a runtime
  switch or fallback creates an unnecessary live-account hazard.
- Store tokens in `.env` or repository-local files: rejected because refresh
  tokens are long-lived bearer secrets and macOS Keychain is available.
- Request `trading` scope immediately: rejected because the current gates are
  read-only and least privilege requires `accounts`.

## Consequences

Benefits:

- The official provider interface becomes the single active architecture path.
- A future separately authorized read-only proof can exclude order capability.
- Demo/live separation is enforced before DNS and again during account
  discovery.
- Secret rotation and expiry are isolated from source and repository state.
- API account identifiers never enter files, checkpoints, logs, evidence, or
  reports.

Costs and risks:

- A cTrader-specific OAuth, Keychain, TLS/TCP, Protobuf, and reconnect boundary
  is required before account proof.
- cTrader does not document `state`; a fixed loopback listener and short
  operator-started window do not by themselves prevent local callback
  injection or login CSRF.
- The active repository contains no generated cTrader C++ bindings or approved
  Protobuf/TLS toolchain. The pinned proto2 schema supports presence semantics,
  but generation and dependency verification remain Gate 6A prerequisites.
- OAuth callback correlation remains an unresolved Gate 5.1 blocker. Exact
  FIBO identity/account evidence is intentionally deferred to Gate 6A and a
  mandatory Wade checkpoint before Gate 6B, eliminating the prior circular
  prerequisite.

## Validation

- Gate 5 document covers redirect URI, staged scopes, every secret/token,
  Keychain injection, ignore/example rules, redaction, Gate 5.1, Gate
  6A/checkpoint/Gate 6B authentication ordering, demo-only endpoint, failure
  behavior, and the next minimal boundary.
- Authority documents identify Open API as sole and the Bridge as abandoned,
  non-controlling, and out of scope.
- Gates 1-3 evidence documents record their required verdicts and preserve the
  complete Gate 6 umbrella as blocked.
- `.env.example` contains placeholders only and `.gitignore` excludes local
  secret-bearing files.
- Documentation/security scans and `git diff --check` pass.
- No OAuth, token exchange, cTrader connection, account request, market-data
  request, or order operation is executed.

## Reversal Conditions

Reconsider the sole-path decision only through a new explicit Wade architecture
directive. Open API failure does not reactivate the abandoned Bridge or
authorize live trading.

## Supersession

This ADR does not supersede accepted ADR 0003. Abandoned Bridge history is
non-controlling and outside this ADR's implementation or evidence scope.
