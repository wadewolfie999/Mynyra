# cTrader Open API Gate 3: Baseline And Local-Diff Integrity

## Document Control

- Status: Revalidated pre-implementation boundary
- Date: 2026-08-07
- Active worktree: `/Users/vaheedgorgeen/.codex/worktrees/18ef/TradeBot`
- Branch: `codex/ctrader-open-api-gate5`
- HEAD and accepted base: `400b486a6af64c653a54b7e7080dbb59bce90cd8`
- Verdict: `GATE 3 REVALIDATED`

Gate 3 establishes the clean accepted base, the allowed future implementation
boundary, and the invariants against which Gate 6—the complete Gate 6A →
mandatory Wade checkpoint → Gate 6B umbrella—will be reviewed. It is not a
claim that a future implementation diff has already been inspected.

## Accepted Base Relationship

- The active branch has no upstream.
- Local `origin/main` is exactly
  `400b486a6af64c653a54b7e7080dbb59bce90cd8`.
- `HEAD...origin/main` is `0` ahead / `0` behind and the merge base is the same
  full hash.
- The accepted base contains the broker-neutral Phase 22 contracts and tests,
  ADR 0003, and no cTrader Open API implementation, generated provider binding,
  or Protobuf dependency.
- The current local diff is unstaged documentation/configuration-example work
  for Gates 1–5 only. It contains no `include/`, `src/`, `tests/`, CMake, build,
  generated binding, credential, token, account-data, or market-data change.

## Contamination Check

The abandoned Algo Bridge was not inspected or used as evidence. Contamination
detection was limited to active changed paths/content and accepted-base source
searches relevant to the Open API boundary.

- No active changed path is a Bridge source, generated Bridge artifact, cBot,
  cAlgo, or bridge configuration.
- No Open API source file overlaps an abandoned Bridge file because no Open API
  source file exists yet.
- OANDA names occur only in historical/governance documentation that records
  permanent cancellation; no OANDA source, endpoint, credential, adapter, test,
  or active fallback exists in the current local diff.
- If a future Gate 6A or Gate 6B candidate contains Bridge or OANDA code, configuration,
  endpoint, dependency, generated file, or shared dirty hunk, stop before
  editing or separation and return it to Wade.

## No Production Behavior Change

The Gates 1–5 rebaseline is documentation and safe example configuration only.
It does not change:

- C++ source, headers, tests, CMake targets, dependencies, or generated code;
- `BACKTEST`, `PAPER`, or `LIVE` behavior/defaults;
- `BrokerGateway`, `RiskEngine`, `ExecutionEngine`, `LiveDataAdapter`,
  `AsyncNetworkClient`, `AuthManager`, or `SystemConfig`;
- risk limits, order routing, quantities, prices, reconciliation, logs, or data;
- credentials, Keychain, cTrader portal state, OAuth state, tokens, accounts,
  sockets, subscriptions, or orders.

Gate 5 is accepted design evidence. Its acceptance does not authorize Gate 5.1
or Gate 6, meaning Gate 6A, Wade's checkpoint, or Gate 6B.

## Binding Future Implementation Boundary

Any later Gate 6A or Gate 6B directive within the Gate 6 umbrella must preserve
these invariants:

1. The immutable Open API message endpoint is
   `demo.ctraderapi.com:5035` using Protobuf over strict TLS/TCP.
2. The read-only target can request only application auth, account list, and
   account auth; it cannot construct symbol, quote, order, amend, cancel,
   position, or live-endpoint messages.
3. OAuth uses only `accounts`; `trading` is a distinct later authorization.
4. Provider IDs/types remain below the cTrader boundary. The response-derived
   `ctidTraderAccountId` remains only in volatile process memory and never
   becomes a visible login, log field, evidence/report value, checkpoint
   artifact, configuration value, or tracked content.
5. The exact Gate 2 checked conversions and proto2 presence tests govern.
6. `BACKTEST` remains deterministic and cannot load cTrader dependencies,
   credentials, network state, or wall-clock outcomes.
7. The proof is not attached to `BrokerGateway`, `ExecutionEngine`,
   `LiveDataAdapter`, a runtime mode, or the generic `AuthManager`.
8. Strict TLS and hostname validation are mandatory. The current permissive
   `AsyncNetworkClient` default is not accepted.
9. Secrets/tokens are never placed in source, tracked files, fixtures, logs,
   reports, command arguments, or generated review evidence.
10. Any source implementation, dependency introduction, portal/OAuth action,
    cTrader connection, or account request requires a new Wade directive.

## Smallest Gate 6 Umbrella Source Surface

Subject to a new directive and dependency approval, the bounded review surface
is expected to be limited to:

- `third_party/ctrader-open-api/` — four pinned official `.proto` inputs and
  provenance/license metadata, not hand-edited generated C++;
- `include/ctrader_open_api/` and `src/ctrader_open_api/` — immutable gate
  config, strict TLS/TCP interface, frame codec, redactor, OAuth callback/token
  interfaces, Keychain adapter, and account-proof state machine;
- a separate read-only proof entry point that has no order API;
- `tests/ctrader_open_api_*` deterministic codec/state-machine/security tests;
- focused CMake dependency/generation/proof-target changes;
- the already established Gate 5 security/configuration documentation.

The exact filenames may be refined by an approved Gate 6A or Gate 6B plan, but scope may
not expand into core execution, risk, portfolio, strategy, replay, market-data,
or runtime-mode components merely for convenience.

## Future Diff-Review Procedure

Before Gate 6A edits, capture the accepted Gates 1–5 tree and status. After each
separately authorized Gate 6A or Gate 6B implementation pass, compare every path
and hunk against the source-surface allowlist and invariants above. Stop on
user-owned overlap, source outside the
allowlist, generated files without pinned provenance, test dependence on
external cTrader state, or any live/order capability. That later comparison is
implementation-diff review; it is not performed or claimed here.

## Gate 3 Disposition

The accepted base, current design-only diff, contamination boundary,
production invariants, and smallest future source surface are now explicit and
reproducible. No future implementation acceptance is implied.

`GATE 3 REVALIDATED`
