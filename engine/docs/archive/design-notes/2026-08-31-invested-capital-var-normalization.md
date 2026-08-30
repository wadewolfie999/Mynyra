# Dormant Design Note: Invested-Capital VaR Normalization

## Status and authority

- Archived: 2026-08-31.
- Status: dormant historical proposal; not approved current risk behavior.
- Current authority remains the implementation, accepted decisions, and
  `docs/RISK_POLICY.md`.
- Reconsidering or implementing this proposal requires a separately reviewed
  risk-design decision and focused regression evidence.

## Preserved idea

`RiskEngine::updateDiversifiedVaR()` currently normalizes each asset position
against total portfolio equity:

```text
weight_i = positionValue_i / totalEquity
```

This denominator includes cash. A cash-heavy portfolio therefore has smaller
position weights and lower diversified portfolio volatility than an otherwise
identical fully invested portfolio.

The removed `totalPositioned` accumulator suggested a possible alternative:

```text
investedCapital = sum(positionValue_i)
weight_i = positionValue_i / investedCapital
```

That alternative would normalize the represented positions independently of
cash. It would materially change VaR semantics and must not be inferred merely
from the former unused variable.

## Questions that must be resolved before reconsideration

- Whether `investedCapital` means gross exposure, signed net exposure, or
  another explicitly defined quantity.
- How mixed long and short positions affect the denominator and weights.
- What happens when invested capital is zero, negative, or nearly zero.
- Whether and how cash should dilute risk in cash-heavy portfolios.
- Which quantity should remain comparable with the configured VaR limit.
- Whether the final multiplication by `totalEquity` remains correct under a
  different normalization basis.

## Required closure evidence

Any future proposal must define invariants and provide golden VaR fixtures for:

- a cash-heavy portfolio;
- a fully invested portfolio with identical positions;
- mixed long and short exposure;
- zero, negative, and near-zero candidate denominators;
- no open positions;
- scale changes that should preserve normalized risk;
- covariance and position-order permutations.

Until that evidence and a risk-policy decision exist, equity normalization is
the current behavior and this note has archival value only.
