# TradeBot Risk Policy

## Purpose And Authority

- Purpose: authoritative risk policy for financial, execution, data, credential, reproducibility, and AI-agent risks.
- Authority level: below accepted ADRs and above architecture, plans, state, roadmap, workflow, skills, and contributor guidance.
- Audience: operator, maintainers, Codex, contributors, reviewers, testers, research agents, and any actor touching live-capable behavior.

## Default Mode

TradeBot defaults to `BACKTEST`, dry-run, simulation, or paper behavior. Live trading is not permitted by default.

## Current Remediation Safety Hold

The Repository Remediation Program in
`REPOSITORY_REMEDIATION_PROGRAM.md` is the sole current implementation queue.
Its foundational governance is approved. WP-0 and WP-1 are merged and
accepted; WP-2 through WP-8 remain Planned /
NO-GO except for integrated WP-7/WP-8 closure slices. Existing source is
classified as operationally unsafe for live use until accepted package evidence
supports a narrower conclusion.

- No historical phase, ADR, cTrader gate, passing test, credential presence,
  branch, or build artifact authorizes provider continuation or runtime use.
- WP-0 established the default-off runtime containment boundary accepted in
  PR #35. This does not establish live readiness.
- WP-2 and WP-3 may correct existing accounting/risk intent only under exact
  approval; changing financial-limit values is a separate action.
- WP-4 must establish a single confirmed order lifecycle before provider work.
- WP-6 base approval is offline only; every provider process remains separate.
- Completion of all packages still does not authorize live trading.

## Live Trading Unlock Requirements

Live trading requires explicit operator authorization for the exact venue, account, scope, branch, commit, configuration, and time window. Authorization is not inferred from repository access, credentials being present, or the existence of `LIVE` mode in code.

Before live trading, all requirements in `LIVE_TRADING_READINESS.md` must be satisfied, including:

- Verified kill switch.
- Verified no-new-orders halt path.
- Verified credential source and least-privilege key permissions.
- Maximum order size, position limits, daily loss limits, and total drawdown limits.
- Paper or sandbox validation.
- Logging path and log-redaction review.
- State reconciliation plan.
- Network interruption and stale-data behavior.
- Operator monitoring and rollback procedure.

## Financial Risk

- Never change position sizing, drawdown gates, VaR thresholds, stop logic, order-routing behavior, or risk-limit defaults without explicit operator approval.
- Backtests and benchmarks do not prove profitability.
- Every strategy claim must account for fees, slippage, spread, liquidity, timestamp handling, lookahead bias, survivorship bias, and data leakage.
- Risk controls must fail closed when required data is unavailable.

## Order-Execution Risk

- Execution must pass through `ExecutionEngine` and risk gates before opening new positions.
- Live-capable broker interaction belongs at the `BrokerGateway` boundary.
- Paper behavior must remain simulated unless an approved sandbox integration changes it.
- Partial fills, cancellations, retries, and reconciliation must be explicit and venue-aware before live use.
- No actor may enable real orders by default.

## Market-Data Integrity

- Market data must be treated as invalid when malformed, stale, out of order, or inconsistent with expected schema.
- `LiveDataAdapter` integrity callbacks and `RiskEngine` halt behavior must not be bypassed.
- Replay data must preserve event order and timestamp semantics.
- Data gaps must be recorded, not silently filled without provenance.

## Timestamp And Timezone Handling

- Replay ticks use nanosecond timestamps where represented by `ReplayTick::timestampNs`.
- Candle timestamps use `MarketCandle::epochTimestamp`.
- Documentation and generated outputs must state timestamp units when relevant.
- Do not mix local timezone display with execution or replay ordering.
- Use UTC or explicit epoch values for reproducibility unless a file schema states otherwise.

## Replay Correctness

- Replay tests must cover CSV parsing, binary roundtrip, cursor behavior, and BBO shape when replay drives order-book logic.
- Synthetic replay data must be labeled as synthetic.
- Generated replay files must not be mistaken for historical market data.

## Lookahead Bias And Data Leakage

- Strategies may consume only information available at the simulated event time.
- Resume paths must avoid trading on already-processed candles.
- Analytics outputs must not feed future decisions unless an explicit online-feedback boundary is tested and documented.

## Transaction Costs And Slippage

- Performance and strategy reviews must include fees and slippage assumptions.
- `ExecutionEngine` fee and slippage parameters are part of financial-sensitive behavior.
- Actual fill feedback in live-capable paths must be distinguished from modeled slippage.

## Position Sizing And Drawdown Controls

- Position sizing and drawdown controls are financial-sensitive.
- `RiskEngine` drawdown, max position, VaR, close-only, and halted states must remain active unless a plan and approval change them.
- Clearing a halt is an operator-level action in live-capable contexts.

## Kill Switch

Any live-capable path must support operator-controlled halt behavior that stops new order submission. Cancel behavior must be explicit and venue-aware. A kill switch must be tested before any live unlock.

## Mode Confusion

- `BACKTEST`, `PAPER`, and `LIVE` are verified code modes.
- Sandbox is a governance concept, not a verified code mode.
- Documentation must distinguish dry-run, paper, sandbox, and live behavior.
- The default build rejects `LIVE` before credential lookup and network startup.
  A non-default `TRADEBOT_ENABLE_LIVE_RUNTIME=ON` build also requires
  `--unlock-live-runtime`; neither technical gate grants operational authority.
- A bound but unavailable `BrokerGateway` must reject execution and must never
  fall back to a local fill.
- The legacy `LiveDataAdapter` must not load credentials or construct provider-
  specific REST requests; provider credential use belongs to a separately
  approved provider adapter.

## Credential Exposure

- Never commit credentials, API keys, tokens, private keys, account IDs, or `.env` files.
- Never print secret values.
- Secret-looking files require operator review.
- Documentation may mention environment variable names such as `AIIO_API_KEY` and `AIIO_API_SECRET`, but never values.
- cTrader client secrets and tokens must use macOS Keychain; authorization
  codes, `ctidTraderAccountId`, `traderLogin`, account numbers, visible logins,
  and equivalent account-identifying values remain volatile process-memory
  only for the bounded proof. None is ever written to logs, evidence, reports,
  checkpoint artifacts, configuration, reusable labels, or tracked/untracked
  files. A visible account login is never an API account ID or selection key.

## cTrader Demo-Only Gate

- Official cTrader Open API is the sole integration path; the Algo Bridge is
  abandoned, non-controlling, and out of scope.
- Wade explicitly authorized `trading` scope for the demo-only Gate 6 account
  proof on 2026-08-10. This permission does not authorize any trading message:
  the immutable Gate 6 allowlist remains limited to application auth, account
  list, account auth, and heartbeat. Orders, changes, cancellations, position
  closure, symbols, market data, and live-account access remain prohibited.
- `demo.ctraderapi.com:5035` is the only allowed Open API message endpoint,
  with no configuration or fallback to live.
- Live account entries are excluded from candidacy. Gate 6A may discover only
  account-list identity metadata and must stop for Wade's checkpoint before any
  account authentication. The checkpoint may contain only exact broker-title
  presence/value, exact demo/live presence/value, and a bounded
  non-identifying outcome category. If present `isLive == false` plus exact
  `brokerTitleShort` does not distinguish exactly one intended demo account,
  stop without producing a persistent selection predicate. Candidate labels,
  per-account ordinals, visible logins, and equivalent account identifiers are
  prohibited. Another live entry's mere presence is not failure.
- Gate 6B may authenticate only one exact Wade-approved demo match from a fresh
  authenticated account-list response by reproducing the approved
  `isLive == false` plus exact `brokerTitleShort` selection deterministically.
  Its fresh `ctidTraderAccountId` remains only in
  volatile process memory and is used solely for that session's account-auth
  request. Token ambiguity, absent `isLive`,
  missing/contradictory broker metadata, zero/multiple exact matches, title
  resemblance, an attempted live selection, or exhausted reconnect budget fails
  closed before market data or order capability.
- Gate 2 and the Gate 5 design were accepted by Wade on 2026-08-07. Gate 5.1
  merged in PR #24 and Wade accepted its implementation on 2026-08-09. Wade
  then authorized the Gate 6 umbrella (`Gate 6A → mandatory Wade checkpoint →
  Gate 6B`). Credential and redirect prerequisites are satisfied, the first
  correlated callback succeeded, and a local libcurl option defect was
  identified before the token request and corrected for a fresh attempt. Gate
  6 makes no financial-limit, market-data, order-routing, or runtime-mode
  change.
- PR #25 is merged. Wade separately authorized Gate 7 on 2026-08-10. Gate 7
  remains a default-disabled macOS-only proof with a minimal non-trading
  allowlist and no production-runtime attachment. Historical attempts stopped
  at Keychain access, generic OAuth failure, and a fixed OAuth callback timeout.
  The latest authorized process passed OAuth, fixed demo TLS,
  application/account authentication, canonical XAUUSD resolution, and full
  metadata validation before `gate7_subscription_failed`. Its subcause remains
  unclassified; no accepted subscription, quote, or timestamp evidence exists.
  No reconnect, order, position, depth, historical-data, Gate 8–9, or live
  action occurred.
- Gate 7 residual work must classify subscription/spot transport outcomes using
  fixed local categories and clear provider material before return. A partial
  spot event is never accepted, cached, or combined with another event. The
  proof may continue only within its unchanged absolute deadline for a single
  event containing both positive sides and a valid timestamp. Heartbeats must
  use the existing allowlist, monotonic cadence, and original deadline without
  reconnect or retry.
- The next provider process is NO-GO until the residual plan, diff, full offline
  and sanitizer matrix, diagnostic redaction, persistent sanitized evidence
  template, local clock health, port/process preconditions, exact binary/commit,
  and a separate one-process Wade approval are all reviewed.

## External API And Network Risk

- Intermittent or degraded connectivity is an operational risk.
- Paper, sandbox, and live-capable workflows must define disconnect detection, timeout thresholds, stale-data handling, bounded retry policy, and reconciliation after reconnect.
- Network-dependent work should be scheduled during planned connectivity windows.

## Partial Execution And Reconciliation

- Partial fills must be tracked and reconciled.
- Local portfolio state must not be assumed correct after disconnect or API failure.
- Reconciliation differences must be logged and reviewed before continuing live-capable operation.

## Malformed Inputs

- Malformed CSV, binary replay, external payload, or config input must fail safely.
- Tests for malformed inputs are required when parsers or external boundaries change.

## Generated-Data Contamination

- Generated data under `data/results/` and `build/` is not source truth.
- Generated benchmark/replay files must not be used as historical source data without provenance and operator approval.

## Dependency And Supply-Chain Risk

- New dependencies require review under `DEPENDENCY_POLICY.md`.
- Avoid adding network-dependent tooling unless it is necessary and approved.
- Dependency updates must not weaken reproducibility or security.

## Reproducibility Risk

- Record seeds, inputs, command lines, build mode, compiler, dataset provenance, and generated-output paths when making research or benchmark claims.
- Do not compare benchmarks across machines without naming the machines and build settings.

## Operator And AI-Agent Error

- Actors are not trusted merely because they have repository access.
- AI agents must inspect before mutation, avoid scope expansion, and report uncertainty.
- Financial-sensitive ambiguity requires halt and operator escalation.
- Authorization is non-transitive: offline work, review, correction, commit,
  exact-artifact proof, provider traffic, retry, later gates, orders, financial
  changes, and live use require their own named boundaries when applicable.
- Evidence must identify its epoch. Working-tree, staged, committed,
  exact-commit-build, and external-process results are not interchangeable.
- A failed one-process external attempt consumes that attempt unless the
  operator explicitly renews it. Repeating an unchanged attempt without a new
  hypothesis or authorization is prohibited.
- Sensitive/provider-derived memory must be cleared from every owned copy and
  terminal path, not only from the primary object.

## Rollback And Containment

- For source changes, rollback is normally a Git revert or branch reset approved by the operator.
- For generated artifacts, remove or archive only within approved scope.
- For credential exposure, follow `SECURITY.md`: revoke, rotate, audit logs, and notify the operator.
- For live-capable incidents, stop new orders, reconcile state, preserve evidence, and do not resume without operator approval.
