# cTrader Open API Gate 5: OAuth And Secret-Handling Design

## Document Control

- Status: Gate 5 design accepted by Wade on 2026-08-07; Gate 5.1 offline
  controls merged in PR #24 and accepted by Wade on 2026-08-09; Gate 6 is
  separately authorized but provider execution is stopped pending credential
  rotation and fixed redirect-URI registration
- Plans: accepted design `PLAN-20260806-ctrader-open-api-gate5`; offline
  implementation `PLAN-20260809-gate5-oauth-correlation-controls`
- Decision: ADR 0004
- Implementation branch: `codex/gate5-completion-oauth-correlation`
- Implementation base: `75e35eda0846f39773886febb2ade63018561958`
- Gates 1–3 accepted baseline: `400b486a6af64c653a54b7e7080dbb59bce90cd8`
- Application: `TradeBot Demo Integration`; operator-reported `Active` on 2026-08-06
- Target: FIBO Group through cTrader, demo-only XAUUSD
- Live trading: prohibited

This accepted design plus the offline-only correlation guard and synthetic
tests are the Gate 5/Gate 5.1 deliverable. Wade's later Gate 6 directive—not
Gate 5 acceptance—authorizes the bounded real OAuth and read-only account-proof
sequence. cTrader `state` round-trip support remains unverified and must match
exactly before any token exchange can proceed.

## Controlling Sources

cTrader's official Open API documentation controls endpoint, scope, token,
account-discovery, and message-sequence behavior:

- [Application and account authentication](https://help.ctrader.com/open-api/account-authentication/)
- [Application registration and redirect URIs](https://help.ctrader.com/open-api/api-application/)
- [Proxies and endpoints](https://help.ctrader.com/open-api/proxies-endpoints/)
- [Connection guidance](https://help.ctrader.com/open-api/connection/)
- [Model messages and error codes](https://help.ctrader.com/open-api/model-messages/)
- [Open API FAQ](https://help.ctrader.com/open-api/faq/)
- [Official proto-message repository at pinned revision](https://github.com/spotware/openapi-proto-messages/tree/3fd8bddfbe0cfc2ecfda079623dc4e498af11e66)

The official cTrader authorization-request table documents `client_id`,
`redirect_uri`, `scope`, and optional `product`. It does not document OAuth
`state`, PKCE, or another request-to-callback correlation mechanism. RFC 6749
recommends `state` for request/callback binding, but that standard guidance is
not evidence that cTrader accepts or returns it. Correlation support is
therefore unverified and blocks token exchange until the authorized Gate 6
callback returns one exact matching value.

## Architecture Disposition

- Official cTrader Open API is the sole integration path.
- The cTrader Algo/cBot Bridge is
  `ABANDONED — NON-CONTROLLING — OUT OF SCOPE`. Gate 5 does not inspect,
  preserve, validate, complete, merge, or delete it.
- OANDA is permanently cancelled.
- The accepted broker-neutral contracts remain controlling. No provider-native
  type may leak above the future adapter boundary, and no floating-point repair
  or implicit rounding may be introduced.

The Gate 6 worktree vendors the exact official proto2 definitions pinned at
upstream commit `3fd8bddfbe0cfc2ecfda079623dc4e498af11e66`, verifies their hashes at
configure time, and generates C++ bindings only in the build tree. It adds no
Bridge dependency and no market-data or order path.

## Gates 1-3 Recovery Status

| Gate | Evidence recovered | Current disposition |
| --- | --- | --- |
| Gate 1 — protocol fit | `CTRADER_OPEN_API_GATE1_PROTOCOL_FIT.md` selects Protobuf over strict TLS/TCP, fixed demo port `5035`, official framing/schema provenance, correlation, heartbeat, auth, symbol subscription, and reconnect behavior. | `GATE 1 REVALIDATED`. Binding generation and dependency installation remain Gate 6A work. |
| Gate 2 — numeric contract | `CTRADER_OPEN_API_GATE2_NUMERIC_CONTRACT.md` maps exact provider types/presence/units to `Decimal64`, defines integer-only price rounding, and fixes the zero-anchored volume predicate. | `GATE 2 ACCEPTED BY WADE`. Actual FIBO XAUUSD metadata and the under-documented spot timestamp unit are Gate 7 runtime stop conditions. |
| Gate 3 — baseline/local-diff integrity | `CTRADER_OPEN_API_GATE3_BASELINE_INTEGRITY.md` establishes base `400b486`, the design-only diff, contamination boundary, invariants, and smallest future source surface. | `GATE 3 REVALIDATED`. Ordinary review of a future implementation diff remains later verification, not Gate 3. |

Gates 1-3 and accepted Gate 5 did not authorize Gate 6. Wade subsequently
accepted Gate 5.1 and explicitly authorized the complete Gate 6A → mandatory
Wade checkpoint → Gate 6B umbrella. OAuth callback correlation with cTrader
remains unverified until the first bounded callback matches. Exact FIBO broker
identity and the intended demo account must still be learned in Gate 6A and
approved by Wade before Gate 6B. Homebrew Protobuf 35.1 is installed and the
opt-in Gate 6 build verifies its matching 7.35.1 runtime headers.

Gate 4 is complete solely from Wade's controlling confirmation that
`TradeBot Demo Integration` is `Active`. This pass did not open the portal or
credentials page and did not operationally re-check that status.

## Fixed Redirect URI

Register this exact callback URI in the cTrader portal:

```text
http://127.0.0.1:18080/ctrader/oauth/callback
```

Rules:

- Use the numeric IPv4 loopback address, fixed port `18080`, and fixed path.
  Do not substitute `localhost`, another address, port, path, scheme, trailing
  slash, or query string.
- The later callback listener must bind only to `127.0.0.1`, never `0.0.0.0`,
  `::`, a LAN address, or a public interface.
- Start the listener only immediately before authorization and close it after
  one accepted callback, one rejection, explicit cancellation, or the fixed
  timeout. The listener owner must call the guard's terminal cancellation or
  monotonic-expiry transition before closing; destruction alone is not the
  timeout mechanism.
- cTrader `state` round-trip support is unverified. Do not claim or depend on
  it as provider behavior. The offline guard now generates a cryptographically
  random, single-use value, enforces exact callback matching, and discards code
  input without returning it. The authorized Gate 6 run may send that value
  and must fail if `state` is rejected,
  omitted, changed, or duplicated. Only an exact match may continue to token
  exchange under the Gate 6 directive.
- Until secure request-to-callback correlation is proven, the fixed loopback
  listener and operator-started short time window reduce exposure but do not
  prevent a malicious local process or browser context from injecting an
  unrelated code. This login-CSRF/callback-substitution limitation blocks OAuth
  execution; no PKCE or other undocumented provider capability is assumed.
- Reject malformed queries, duplicate parameters or requests, unexpected
  methods/paths/hosts, callbacks received before listener arming, and callbacks
  received after the single-use listener closes or times out.
- Accept only `GET`, the exact path, and an expected loopback `Host` header.
  After extracting the callback, return `303 See Other` to the local clean path
  `/ctrader/oauth/complete`, serve a static response with no scripts, remote
  assets, analytics, code, or token content, and then close the listener. This
  removes the credential-bearing query from the visible browser location.
- The authorization request and token exchange must use the same exact,
  percent-encoded redirect URI. Portal registration and a later controlled
  redirect verification are separate prerequisites.
- The portal's default Playground redirect is not valid for TradeBot.

## Gate 5.1 — OAuth Correlation Verification

Gate 5.1 is inserted between Gate 5 design acceptance and every account-proof
gate. Wade accepted its merged offline-verifiable implementation on
2026-08-09. The later Gate 6 directive authorizes one provider round trip and
permits token exchange only after an exact correlation match.

### Offline Implementation Disposition

`CTraderOAuthCorrelationGuard` implements only the local correlation boundary:

- operating-system CSPRNG generation of a 256-bit, unpadded base64url value;
- fixed `127.0.0.1:18080` listener-binding validation and exact loopback
  remote, `GET`, `Host`, and callback-path checks;
- a fixed 60-second monotonic lifetime;
- explicit listener-owner transitions that cancel immediately or terminally
  expire at the exact 60-second deadline, securely clear the correlation
  value, prohibit rearming, and reject every later callback as replay;
- one callback attempt, with mismatch, malformed input, duplicates, expiry,
  unexpected binding data, and every later replay terminally rejected;
- constant-time exact state comparison, immediate state clearing, and
  authorization-code presence checking without returning or retaining code;
- fixed diagnostic categories containing no query, code, state, identifier,
  or caller-supplied value.

`ctrader_gate5_1_tests` verifies those controls with synthetic values and no
socket, browser, provider, credential, token, account, or market operation.
This proves the local implementation only. It does not prove that cTrader
accepts or returns `state`; provider support remains the OAuth stop condition.

### Purpose

Determine whether cTrader reliably accepts and returns an
authorization-request correlation value such as OAuth `state`. Official cTrader
documentation does not currently document that behavior, so the result must be
measured during the authorized Gate 6 run rather than assumed.

### Preconditions

- Wade has accepted Gate 5 and Gate 5.1.
- Wade has confirmed that the portal stores the exact registered redirect URI
  `http://127.0.0.1:18080/ctrader/oauth/callback` character-for-character.
- Wade has explicitly authorized Gate 6 provider execution; credential
  rotation and fixed redirect registration must still be complete first.
- No production integration is running; no live account and no `trading` scope
  is selected or authorized.
- The fixed, single-use listener is bound to `127.0.0.1:18080` and armed before
  the browser authorization request begins.

### Permitted Gate 6 Correlation Actions

1. Generate a cryptographically random, unpredictable, single-use correlation
   value in process memory.
2. Include it in one cTrader authorization request only if the provider accepts
   the parameter; request `accounts` scope only.
3. Open that single authorization request and receive at most one callback on
   the registered loopback URI.
4. Compare returned correlation data byte-for-byte and within the listener's
   short fixed time window.
5. On any non-match, immediately discard any authorization code, close the
   listener, and exit without token or endpoint traffic. Only an exact match
   may continue to the Gate 6 token exchange.

### Evidence Contract

Record only whether the parameter was accepted, whether it was returned,
whether it matched exactly, callback timing, the loopback bind address, the
registered redirect URI, a sanitized outcome category, and numeric exit status.
Never record the authorization code, client ID value, client secret, token,
complete authorization URL, callback query, or full callback URL.

### Stop Conditions

Stop immediately and mark Gate 5.1 failed if the provider rejects the
parameter; the callback omits it; the returned value differs; a callback is
malformed, duplicate, unexpected, early, or late; the listener binds outside
loopback; or secure request-to-callback correlation remains unproven. If
`state` is unsupported or unreliable, return to Wade for an architectural
decision. Do not invent PKCE or proceed to Gate 6A or Gate 6B.

## Wade Portal Actions Before Gate 6 Execution

Before the authorized Gate 6 run, Wade—not Codex or ChatGPT—must:

1. Sign in to the official cTrader Open API portal and open **Applications**.
2. Confirm `TradeBot Demo Integration` still shows `Active`. Stop if its name or
   status differs.
3. Choose **Edit** for that application, find **Redirect URIs**, and add exactly
   `http://127.0.0.1:18080/ctrader/oauth/callback` as a separate entry. Do not
   edit this spelling and do not use the default Playground redirect.
4. Save, reopen the application edit view, and visually verify the stored URI
   character-for-character. If the portal rejects the loopback HTTP URI or
   rewrites it, stop and report the portal behavior without attempting OAuth.
5. Personally open **Credentials** only when local configuration requires it.
   Copy the client ID into local non-secret configuration. Enter the client
   secret only into the interactive Keychain prompt described below. Do not
   paste either value into Codex/ChatGPT, source, a tracked file, terminal
   command arguments, screenshots, or the final report.
6. During any later authorization screen, request `accounts` scope and select
   only the intended FIBO Group demo account. Do not authorize any live
   account. The mere display of another live account is not a failure; leave it
   unselected. Stop if the intended demo account is ambiguous.
7. Do not request `trading` or use Playground tokens. The current directive
   authorizes Gate 6A and Gate 6B in one persistent process, but Gate 6B still
   requires Wade's explicit safe-metadata checkpoint confirmation.

## Staged OAuth Permission Policy

Gate 5 and all initial read-only gates use only:

```text
scope=accounts
```

cTrader documents `accounts` as view-only and incapable of trading
operations. `trading` is prohibited until Wade gives a later explicit
directive for the controlled demo-order gate. A `trading` token is a separate
authorization event: re-authorize deliberately, store it under a distinct
scope-qualified Keychain service, and never let a read-only executable load it.
There is no automatic scope upgrade, scope fallback, or token reuse across
scopes.

## Local Secret Lifecycle

| Material | Classification | Source and storage | Lifetime and rules |
| --- | --- | --- | --- |
| Client ID | Identifier, not a secret | Local environment name `TRADEBOT_CTRADER_CLIENT_ID`; name-only example in `.env.example` | Never hardcode; do not log the value |
| Client secret | Secret | macOS Keychain generic-password item, service `TradeBot.cTraderOpenApi.client-secret`, account = local macOS user | Read only at application-auth/token-exchange time; keep in process memory briefly; never return through the existing copying `AuthManager` API |
| Authorization code | One-time secret | Callback process memory only | cTrader documents a one-minute lifetime; exchange immediately once, then zero/drop it; never persist or retry it |
| Access token | Bearer secret | Scope-qualified Keychain token envelope | Use only for its recorded scope; track absolute expiry; refresh before use when inside the safety margin |
| Refresh token | Long-lived bearer secret | Same atomic Keychain token envelope | Rotate atomically with every successful refresh; cTrader invalidates the old token pair |
| Expiry metadata | Sensitive metadata | Same Keychain envelope with token type and scope | Compute from receipt time plus `expiresIn`; use monotonic elapsed time in-process and a conservative wall-clock expiry across restarts |
| `ctidTraderAccountId` | Private account identity metadata, not an authentication secret | Volatile response/process memory only; never any file, checkpoint artifact, configuration, log, evidence, report, transcript, or tracked/untracked content | Never infer from login, hash/encode for persistence, expose to Wade, or reuse across sessions; Gate 6B obtains it from a fresh authenticated account-list response and uses it solely for that session's account-auth request |
| `traderLogin`, account number, visible login, or equivalent account-identifying value | Private account identity metadata | Volatile response/process memory only; never any file, checkpoint artifact, configuration, reusable label, log, evidence, report, transcript, or tracked/untracked content | Never use as an API ID, checkpoint predicate, human-visible selection aid, hash/encoded label, or cross-session key; clear it with the account-list response state |

The token envelope is one Keychain value under service
`TradeBot.cTraderOpenApi.tokens.accounts`. A refresh is successful only when a
new access token, refresh token, scope, token type, and expiry are written as
one updated value. If the update fails, discard the new response and stop; do
not keep a mixed old/new pair.

Gate 5 does not create Keychain items or read any values.

## macOS Injection Procedure For A Later Authorized Gate

Wade must configure secrets locally, never in Codex or ChatGPT:

1. Set only the non-secret client ID in a private shell session or ignored
   local `.env` using `TRADEBOT_CTRADER_CLIENT_ID`. Do not put it in a command
   transcript intended for sharing.
2. Add the client secret interactively to macOS Keychain:

   ```sh
   security add-generic-password -U \
     -a "$USER" \
     -s "TradeBot.cTraderOpenApi.client-secret" \
     -w
   ```

   `security` prompts for the secret because `-w` is last. Do not append the
   secret to the command, pipe it, export it, or paste it into chat.
3. Let the later authorized OAuth implementation create and rotate the token
   envelope. Wade must not manually copy codes or tokens into files or commands.
4. Verify Keychain item existence by metadata only. Never run a command that
   prints the stored password into a captured terminal.

The future implementation should call macOS Security.framework directly. It
must not spawn `security`, place secrets in command arguments, or add another
credential dependency without review.

## Source-Control And Example-Configuration Rules

- `.gitignore` must ignore `.env`, `.env.*` except `.env.example`, certificate
  and key files, and `config/local/`.
- `.env.example` contains the client-ID variable with a clearly invalid
  placeholder only. Redirect URI, scope, host, and port remain documented code
  constants, not environment variables. The example must never contain a
  secret, token, authorization code, account identifier, or operator-specific
  path.
- No test fixture may contain a real-shaped captured token. Synthetic fixtures
  use clearly invalid values such as `TEST_ONLY_INVALID_TOKEN`.
- Any local OAuth callback capture, token metadata, or account proof output is
  generated sensitive material and must remain outside the repository.

## Log Redaction

The following fields and aliases are always rendered as `[REDACTED]` in logs,
shared transcripts, and review packages, with no prefix, suffix, length, hash,
or encoded form:

- client ID and client secret;
- authorization request URL and callback query;
- `code`, `state`, access token, refresh token, and bearer/authorization header;
- `ctidTraderAccountId`, `traderLogin`, visible account/login number;
- private balances, positions, and raw account responses.

There is no checkpoint-artifact exception for any account-identifying value. A
Gate 6A checkpoint may persist only explicit `isLive` presence/value and
exact `brokerTitleShort` presence/value, plus a bounded non-identifying outcome
category. It must contain no `ctidTraderAccountId`, `traderLogin`, account or
visible-login number, candidate label, per-account ordinal, value derived from
an identifier, credential, token, authorization code, balance, position, or
raw response. If `isLive == false` plus exact `brokerTitleShort` cannot
distinguish exactly one intended demo account, stop without producing a
selection predicate.

Log only bounded event names, non-sensitive internal event sequences unrelated
to OAuth `state`, scope name, demo/live classification, error category/code,
retry count, and redacted account count.
Exception messages and structured payload dumps must pass through the same
field-aware redactor before any sink. Debug mode must not relax this policy.

## Official Authentication Sequence And Staged Account Proof

The official sequence remains application authentication, authenticated
account-list discovery using the access token, extraction of the real
`ctidTraderAccountId`, and account authentication. The visible cTrader login or
account number is never substituted for `ctidTraderAccountId`. To remove the
circular FIBO-title prerequisite, the proof is divided into two phases with a
mandatory Wade checkpoint between them.

### Gate 6A — Authorized Account Discovery

Wade's later Gate 6 directive authorizes Gate 6A only within this scope:

1. Only after Gate 5.1 succeeds and Wade separately authorizes Gate 6A, perform
   one OAuth authorization with `scope=accounts`, exchange the correlated code,
   and obtain a view-only token through the approved secret boundary.
2. Connect only to `demo.ctraderapi.com:5035` using Protobuf over strict
   TLS/TCP. There is no live hostname, runtime override, or fallback.
3. Send `ProtoOAApplicationAuthReq` with the client ID and Keychain-sourced
   client secret; wait for the successful matching application-auth response.
4. Send `ProtoOAGetAccountListByAccessTokenReq` with the access token. Accept
   only the correlated response on the current authenticated connection
   generation, require its required `accessToken` to equal the request token
   without logging either value, and require present `permissionScope` exactly
   equal to `SCOPE_VIEW`.
5. Parse only that response's `ProtoOACtidTraderAccount` entries. Exact schema
   fields are `ctidTraderAccountId` (`uint64`, required), `isLive` (`bool`,
   optional), `traderLogin` (`int64`, optional), and `brokerTitleShort`
   (`string`, optional); generated proto2 presence accessors are mandatory.
6. Exclude every present `isLive == true` entry from candidacy. Record only a
   redacted live-exclusion count; never record or authenticate its identifiers.
   Treat absent `isLive` as an omission/ambiguity finding and exclude that entry.
   The presence of unrelated live entries is not itself failure.
7. Retain every provider account identifier, including
   `ctidTraderAccountId`, `traderLogin`, account number, visible login, or an
   equivalent value, only in volatile process memory and never expose it to
   Wade. Present Wade a private local record containing only exact `isLive`
   presence/value, exact `brokerTitleShort` presence and byte-for-byte value,
   and a bounded non-identifying outcome category. The artifact must contain no
   candidate label, per-account ordinal, identifier-derived value, raw
   response, token, code, secret, balance, position, or market data.
8. Require present `isLive == false` and exact present/non-empty
   `brokerTitleShort` to distinguish exactly one intended demo account. If zero
   or multiple candidates remain indistinguishable, stop without producing a
   persistent selection predicate or exposing an identifier. Clear every
   in-memory account-identifying value when Gate 6A ends.
9. Stop before `ProtoOAAccountAuthReq`. Do not select the first entry, infer a
   missing title, normalize a title, query symbols, request market data, or
   perform any trading operation.

Gate 6A does not require a pre-approved `brokerTitleShort` literal. Its bounded
purpose is to reveal the exact API-provided identity metadata needed for Wade's
decision.

### Mandatory Wade Checkpoint

After Gate 6A, stop. Wade must review only the safe candidate evidence and
approve an exact selection predicate consisting only of present
`isLive == false` and exact byte-for-byte `brokerTitleShort`.
`ctidTraderAccountId`, `traderLogin`, account/visible-login numbers, candidate
labels, per-account ordinals, and equivalent account-identifying values are
never shown or persisted. No account may be authenticated because a title
merely resembles, contains, case-folds to, or otherwise heuristically matches
FIBO. If those two approved facts cannot distinguish exactly one intended demo
account, return to Wade without authentication.

### Gate 6B — Selected Demo-Account Authentication Proof

Gate 6B requires Wade's explicit checkpoint confirmation naming the approved
two-field selection predicate. The active Gate 6 directive permits the same
process to resume without a new execution directive. In a fresh connection
generation it must repeat application authentication and account-list
discovery, then:

1. Exclude every entry with absent `isLive` or present `isLive == true`; an
   unrelated live entry is never a candidate and its mere presence is not
   failure.
2. Reproduce Wade's approved two-field predicate byte-for-byte against the
   fresh authenticated account-list response. Require present
   `isLive == false`, present/non-empty exact broker title, matching
   `SCOPE_VIEW`, current connection generation, and response ownership through
   the current access token. No account identifier, visible login, candidate
   label, per-account ordinal, or derived value may participate in matching.
3. Require exactly one eligible approved demo match. Reject zero or multiple
   matches, missing or contradictory broker metadata, any ownership ambiguity,
   or any attempted live selection. Never choose by list order or title
   resemblance.
4. Retain the uniquely matched, nonzero, Gate 2 range-checked
   `ctidTraderAccountId` only in volatile process memory. Send it only in that
   session's `ProtoOAAccountAuthReq` with the current access token; never write,
   display, hash, encode, or otherwise persist it. Clear it on success, failure,
   disconnect, or session end. Declare proof only after the matching
   account-auth response.
5. Stop without a symbol query, market-data request, order-state request, order,
   modification, or cancellation.

Authorization-code exchange and refresh must use an in-process HTTPS client
with strict certificate/hostname validation. Do not use `curl`, a shell, or a
command-line argument. cTrader's documented token parameters contain bearer
material in the request; the complete request URL, query/form fields, response,
and client-library diagnostics must be excluded from logs.

## Hard Demo-Only Endpoint Boundary

- The fixed outbound host allowlist is `id.ctrader.com` in Wade's browser for
  authorization, `openapi.ctrader.com` for token exchange/refresh, and
  `demo.ctraderapi.com` for Open API messages. No other host is allowed.
- The only allowed Open API message-transport host is
  `demo.ctraderapi.com`.
- The hostname is an immutable code constant, not an environment variable,
  CLI argument, config-file value, DNS alias supplied by the user, or fallback
  list.
- `live.ctraderapi.com` must not appear in runtime code, configuration, tests as
  an accepted value, or connection candidates. A negative test may construct a
  synthetic `LIVE_ENDPOINT_FORBIDDEN` input and must assert rejection before
  DNS resolution.
- TLS peer and hostname verification are mandatory. The current
  `AsyncNetworkClient` default (`strictTlsValidation=false`) is not suitable
  for this boundary and must not be reused unchanged.
- Gate 6A reconnect repeats application authentication and account discovery,
  then stops before account authentication. Gate 6B reconnect repeats the full
  application-auth, discovery, exact approved-account match, and account-auth
  sequence. Neither changes host or scope.

## Failure Behavior

| Failure | Required behavior |
| --- | --- |
| Missing/invalid client ID or secret; `CH_CLIENT_AUTH_FAILURE`; `CH_OA_CLIENT_NOT_FOUND` | Stop before account discovery; no credential guessing or automatic retry; show only a redacted category |
| User rejects authorization; callback is malformed, duplicate, unexpected, or late; correlation is absent or mismatched; callback times out | Close listener, retain no code, perform no token exchange, and keep OAuth blocked pending a separately authorized safe path |
| Authorization code is expired or rejected | Discard it; never retry the same code; require a new authorization |
| Access token expired (`OA_AUTH_TOKEN_EXPIRED`) | Refresh once through the approved token path before reconnect; atomically rotate both tokens; then repeat full authentication |
| Access token invalid (`CH_ACCESS_TOKEN_INVALID`) or refresh rejected | Stop, mark the local envelope unusable without printing it, and require operator-led reauthorization; no loop |
| Account list empty or target unavailable | Gate 6A stops after discovery; do not guess an ID, use a visible login as an ID, or authenticate an account |
| Live account entry present in account list | Exclude it from candidacy and record only a redacted count. Its mere presence is not failure; never authenticate it or change endpoint. |
| Selected/attempted account is live, or no verified demo candidate remains | Hard stop as `LIVE_ACCOUNT_SELECTION_FORBIDDEN` or account-selection failure; never connect to the live host. |
| Missing `isLive` presence, missing/contradictory broker metadata, `isLive == false` plus exact `brokerTitleShort` unable to distinguish one intended demo account, zero/multiple Gate 6B matches, or mismatch with Wade's approved two-field predicate | Stop for Wade review without exposing any account-identifying value; do not choose the first entry, persist a visible login/label, or infer/normalize fields |
| Disconnect, timeout, maintenance, server unavailable | Mark state unauthenticated, clear in-memory IDs, bounded exponential backoff with jitter and a fixed attempt cap, then repeat the full demo-only sequence |
| Rate limit | Honor a bounded cooldown, make no concurrent retry burst, and stop after the configured cap |
| Malformed or unexpected response | Fail closed, redact payload, disconnect, and preserve only non-sensitive diagnostic metadata |

## Gate 6 Read-Only Account-Proof Boundary

The authorized opt-in Gate 6 implementation adds no broker adapter and no order
capability:

- immutable `CTraderOpenApiGateConfig` values for the callback, `accounts`
  scope, and demo host;
- a cTrader-specific `ICTraderSecretStore` with a macOS Keychain
  implementation and deterministic in-memory fake;
- a one-shot `CTraderOAuthCallback` and token-envelope lifecycle;
- a Protobuf/strict-TLS-TCP transport interface fixed to demo port `5035` that
  can send only application auth, account-list, and account-auth messages;
- an `AccountProofStateMachine` with distinct Gate 6A discovery and Gate 6B
  authentication states, a mandatory operator checkpoint between them, exact
  `isLive == false` plus `brokerTitleShort` predicate matching, memory-only
  response-derived account-identifier use,
  demo-only filtering, presence checks, redaction, bounded reconnect, and
  terminal success/failure results;
- deterministic tests for every failure row above, including proof that no
  symbol, quote, order, cancellation, position, or live-endpoint message can be
  constructed.

Do not attach this proof tool to `BrokerGateway`, `ExecutionEngine`,
`LiveDataAdapter`, or a runtime mode. Do not add a trading scope, order schema,
XAUUSD request, or general-purpose configurable endpoint. Pinned Protobuf
generation and strict TLS dependencies are configure-time requirements; Gate 1
remains controlling for serialization, framing, transport, and port.

## Gate 5 Acceptance And Stop

Wade accepted the Gate 5 design on 2026-08-07 and the merged Gate 5.1
implementation on 2026-08-09. Wade then authorized the complete Gate 6A
discovery → mandatory safe-selection checkpoint → Gate 6B authentication
umbrella. Provider execution is currently stopped until the exposed client
secret is rotated and the exact fixed redirect URI is registered. No exact
FIBO title is required before Gate 6A; it must be observed there and approved
before Gate 6B. Gates 7–9 remain blocked.

`GATE 5.1 ACCEPTED — GATE 6 AUTHORIZED BUT PREREQUISITE-BLOCKED`
