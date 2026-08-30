# ADR 0006: Mynyra Demo Runtime And Broker-Mirrored Ledger

## Status

Accepted. Wade reported the bounded Demo commissioning milestone complete and
accepted it on 2026-08-30. The repository does not currently contain a redacted
external evidence pointer or digest for that commissioning epoch, so this ADR
records the operator decision without claiming current provider, account,
process, deployment, or node state. Acceptance does not authorize LIVE support,
another Demo entry, a provider retry, credential access, or deployment.

## Context

The first Mynyra milestone must prove the existing TradeBot skeleton against
cTrader Demo market data and one closed-loop Demo position. The existing
BACKTEST/PAPER accounting represents spot cash and must not model leveraged
cTrader CFD fills as full-notional cash debits. The normalized provider seam
already requires order side effects to pass through `ExecutionEngine`,
`RiskEngine`, `BrokerGateway`, and `IBrokerAdapter`.

The commissioning evidence must remain useful to a future SQLite sink without
adding persistence or process-crash recovery to M1. The provider boundary must
remain Demo-only and must not contain a live endpoint or live-account path.

## Decision

- Add a compile-time default-off `SystemMode::DEMO` runtime dedicated to
  `demo.ctraderapi.com:5035`, XAUUSD, and completed M1 candles.
- Preserve `ExecutionEngine -> RiskEngine -> BrokerGateway -> IBrokerAdapter`
  as the only order path. Risk evaluates the exact provider minimum quantity;
  downstream components may reject but may not resize it.
- Represent direction and economic intent explicitly with `PositionSide`,
  `PositionEffect`, `OrderIntent`, and a coherent `OrderRiskContext`.
- Keep BACKTEST/PAPER on a local `PortfolioManager` view. Use a separate
  `BrokerPortfolioMirror` in DEMO, mutating it only from validated execution
  events and authoritative reconciliation.
- Extract a side-effect-free `StrategyPipeline`. Historical candles warm its
  state but cannot commission an order; DEMO decisions enter a one-shot
  controller only after one completed live candle and fresh account,
  instrument, BBO, and reconciliation evidence.
- Close the confirmed commissioning position through cTrader's native
  position-close request, not an opposite market order.
- Emit versioned, redacted console and NDJSON events through `IEventSink`.
  SQLite and process-crash recovery remain outside M1.
- Treat the Demo milestone as complete only for the accepted 2026-08-30
  commissioning epoch. Any later process must establish fresh authority and
  fresh account-wide reconciliation evidence.

## Alternatives Considered

- Reuse `PortfolioManager` for DEMO: rejected because leveraged CFD economics
  are not equivalent to the existing spot-cash ledger.
- Close with an opposite market order: rejected because it can create or
  enlarge exposure under venue position semantics.
- Add SQLite before commissioning: deferred because a storage-neutral event
  sink provides the M1 seam without expanding the first proof.
- Allow a configurable endpoint or live-account switch: rejected because it
  weakens the Demo-only containment boundary.

## Consequences

The core gains explicit position semantics, a read-only portfolio boundary,
and a strategy-evaluation seam. DEMO can reconcile broker-authoritative CFD
state without changing BACKTEST/PAPER cash behavior. The one-shot controller
can recover bounded in-process disconnect ambiguity, but a process crash can
leave exposure that must be inspected and closed in cTrader Demo before a new
run.

The provider adds macOS, Protobuf, TLS, OAuth, and Keychain dependencies only
when `TRADEBOT_ENABLE_CTRADER_DEMO=ON`. Default builds remain provider-free.
All live-account and live-endpoint support remains absent from this module.

## Validation

- Default-off and DEMO-enabled full CTest suites.
- Synthetic OAuth/Keychain/HTTP, framing, market-data, strategy-parity, risk,
  lifecycle, redaction, and long/short commissioning tests.
- ASan/UBSan, repository policy, automation validation, and diff hygiene.
- Three external Demo stages: fresh OAuth/account authentication, read-only
  XAUUSD ingestion, and exactly one commissioning entry followed by controlled
  close and final flat reconciliation.

Only the third external stage could validate the milestone. Offline test
success did not establish that external result; the 2026-08-30 status change is
based on Wade's operator acceptance. A future run must not reuse that acceptance
as current runtime evidence or retry authority.

## Reversal Conditions

Replace or revise this decision if cTrader's authoritative position model
cannot be represented by the broker mirror, native close semantics cannot be
reconciled deterministically, provider requirements invalidate the Demo-only
transport boundary, or persistent recovery becomes a prerequisite before any
further commissioning attempt.

## Supersession

This ADR does not supersede ADR 0003 or ADR 0004. It applies their
broker-neutral and cTrader-sole-path decisions to the bounded Mynyra Demo M1
runtime.
