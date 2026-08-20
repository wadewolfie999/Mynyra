# ADR 0005: Canonical Financial Value Contract

## Status

Proposed

## Context

TradeBot previously moved price, quantity, cash, fees, slippage, position cost,
and profit-and-loss values between `double`, integer, and dynamic-scale
`Decimal64` representations without one arithmetic owner. The WP-2 baseline
also showed lost add-on fees, aggregate marks that did not retain each
symbol's latest price, and no checked overflow rule across every accounting
boundary.

WP-2 is financial-sensitive. Its implementation authorization permits a local
candidate and offline validation, but does not itself accept this ADR, change
risk-limit values, authorize publication, or unlock provider or live work.

## Decision

Adopt one internal fixed-scale contract for portfolio and execution accounting:

- `Financial::Price`, `Financial::Quantity`, `Financial::Money`, and
  `Financial::Fraction` store signed 64-bit integer units at scale 8
  (`100,000,000` units per whole value).
- Prices and held quantities must be positive at owning boundaries. Money is
  signed because realized and unrealized P&L may be negative. Rates and fees
  must be non-negative where consumed.
- External `double` compatibility values normalize once at their owning
  boundary. Quantity sizing rounds toward zero; monetary multiplication and
  allocation use nearest rounding with ties away from zero unless a caller
  explicitly requests rejection of unaligned input.
- Addition, subtraction, multiplication, division, proportional allocation,
  fee calculation, slippage, and notional sizing use checked arithmetic.
  Non-finite, unrepresentable, zero-where-positive, or overflowing results fail
  before portfolio mutation.
- Buys debit notional plus fee. Sells credit notional minus fee. Entry fees are
  retained and proportionally assigned to reductions. Unsupported short
  reversal or over-close fails before mutation.
- Each open position owns its cost basis, accumulated entry fee, average entry
  price, and latest mark. Aggregate equity is cash plus all stored marks.
- BACKTEST snapshot version 13 persists financial fields as signed fixed-scale
  integer units. Earlier versions are rejected with a migration-required
  diagnostic; no implicit reinterpretation is allowed.
- Broker-neutral `Decimal64` remains the adapter contract. Conversion to the
  canonical accounting types occurs at the execution/accounting boundary;
  provider schemas remain below `BrokerGateway`.

## Alternatives Considered

- Keep `double` as the accounting owner: rejected because rounding and
  conservation behavior would remain implicit and platform-sensitive.
- Use dynamic-scale `Decimal64` everywhere: rejected because each operation
  would still need scale negotiation and it would couple internal accounting
  to the broker adapter contract.
- Add a decimal or arbitrary-precision dependency: rejected for WP-2 because
  scale 8 and checked two-limb arithmetic cover the repository's current
  contract without a new supply-chain dependency.
- Add short or margin accounting during the migration: rejected as scope
  expansion; WP-2 remains long-only.

## Consequences

Benefits:

- Cash, fees, cost basis, positions, and P&L have deterministic rounding and
  overflow behavior.
- PAPER simulation and local execution use the same fee/slippage contract.
- Multi-symbol valuation retains independent marks.
- Snapshot incompatibility is explicit instead of silently reinterpreting
  financial data.

Costs and risks:

- Public compatibility getters still return `double`; callers must not treat
  them as a second accounting owner.
- Scale 8 imposes a finite resolution and range. Values below resolution or
  beyond signed 64-bit units fail closed.
- Version-12 checkpoints do not load in version-13 code. Rollback requires
  discarding generated v13 checkpoints or a separately reviewed migration.
- Risk drawdown/VaR lifecycle and unified order lifecycle remain owned by WP-3
  and WP-4; this decision does not close those packages.

## Validation

- Golden vectors cover conversion, rounding, multiplication, division,
  proportional allocation, slippage, and overflow.
- Accounting tests cover buy, add, reduce, close, reversal rejection, fees,
  multi-symbol marks, deterministic PAPER costs, malformed fill rejection, and
  1,000 round trips.
- Persistence tests round-trip version-13 state and reject version mismatch or
  inconsistent accounting before mutation.
- The 261-event correctness workload requires one confirmed fill per consumed
  event with explicitly zero modeled costs and makes no performance claim.
- Default-off full CTest, offline CI, and ASan/UBSan validation are required
  before package acceptance.

## Reversal Conditions

Supersede this decision if approved instrument requirements exceed scale-8
resolution/range, an accepted provider contract requires a different canonical
representation, or verified accounting tests show that the fixed contract
cannot conserve required values. Reversal requires an explicit snapshot
migration/rejection plan and financial-risk review.

## Supersession

This ADR does not supersede an existing ADR.
