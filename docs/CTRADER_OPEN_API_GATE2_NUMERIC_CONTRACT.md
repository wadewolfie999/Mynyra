# cTrader Open API Gate 2: Numeric-Contract Revalidation

## Document Control

- Status: Accepted by Wade on 2026-08-07; no runtime implementation authorized
- Date: 2026-08-07
- Accepted baseline: `400b486a6af64c653a54b7e7080dbb59bce90cd8`
- Provider schema: official `proto2` sources at
  `3fd8bddfbe0cfc2ecfda079623dc4e498af11e66`
- Verdict: `GATE 2 ACCEPTED BY WADE`

This contract maps provider values to the accepted broker-neutral
`Decimal64`, `InstrumentSpec`, account, and event contracts. It defines checked
conversions; it does not retrieve FIBO metadata, implement a codec, or authorize
market data or orders.

## Pinned Schema Evidence

| Official file | SHA-256 |
| --- | --- |
| `OpenApiCommonMessages.proto` | `9816cd24b340dcc4eb28548eb4dd16735995d2a61889337591e5c4d8021652a2` |
| `OpenApiCommonModelMessages.proto` | `b95d7df670a7e890a53ec08f676198ace7bb0a074a4b07ff0b493c4be00a0dea` |
| `OpenApiMessages.proto` | `a84df9b528e69a494e197d48e21d6291b3d76db31396663a8188721eef9fcf35` |
| `OpenApiModelMessages.proto` | `56338dcac45a149227678b7c23637d64f4e2b607f6649af82394ce5c0957fedf` |

The controlling field descriptions are also published in cTrader's official
[messages](https://help.ctrader.com/open-api/messages/),
[model messages](https://help.ctrader.com/open-api/model-messages/), and
[symbol-data guide](https://help.ctrader.com/open-api/symbol-data/).

## General Conversion Rules

- Required proto2 fields must be initialized; optional fields used for a
  safety decision must have their generated `has_*` presence bit set.
- Never interpret an absent optional scalar as its Protobuf default.
- Perform overflow checks before every unsigned-to-signed cast and every time
  unit multiplication.
- `Decimal64.scale` must not exceed `12`; otherwise reject rather than round.
- Provider-native IDs remain inside the cTrader boundary and never become
  canonical symbols, local order IDs, or visible login IDs.
- Unknown enum values, NaN/infinity, negative values where the domain is
  non-negative, and inconsistent metadata fail closed.
- Runtime XAUUSD values are evidence, not constants. Gate 7 must capture and
  validate the actual metadata without logging sensitive account material.

## Value Matrix

| Value | Wire field/type | Internal type | Conversion | Validation/failure rule |
| --- | --- | --- | --- | --- |
| `ProtoOACtidTraderAccount.ctidTraderAccountId` | `uint64`, required | Ephemeral provider account ID | Require `0 < raw <= INT64_MAX`, then checked cast only when an `int64` request field requires it. | Gate 6A discovery and Gate 6B authentication proof. Never use `traderLogin`. |
| `ProtoOACtidTraderAccount.isLive` | `bool`, optional | Presence-aware eligibility predicate | Eligibility requires presence and `false`; test generated proto2 presence before reading. | Gate 6A/6B. A present `true` account is excluded, not globally fatal. |
| `ProtoOACtidTraderAccount.traderLogin` | `int64`, optional | None in runtime identity | No numeric conversion; optional human confirmation only. | Never an API ID and never persisted/logged. |
| `ProtoOALightSymbol.symbolId` / `ProtoOASymbol.symbolId` | `int64`, required | Ephemeral provider symbol ID | Retain signed value and require positive exact equality between light/full/spot messages. | Gate 7 runtime metadata. Never hardcode. |
| `ProtoOALightSymbol.symbolName` | `string`, optional | `executionAlias` / canonical-match input | Retain exact response string inside provider boundary; canonical comparison only under Gate 1's bounded `XAUUSD` rule. | Gate 7. Missing/ambiguous name rejects. |
| `ProtoOASymbol.digits` | `int32`, required | `Decimal64` scale and `InstrumentSpec.tickSize` | Require `0 <= digits <= Decimal64::MAX_SCALE`; construct `Decimal64{1, digits}`. | Factor fixed here; actual XAUUSD value Gate 7. |
| `ProtoOASymbol.pipPosition` | `int32`, required | Provider metadata `Decimal64` pip size | Require `0 <= pipPosition <= Decimal64::MAX_SCALE`; construct `Decimal64{1, pipPosition}`. | Actual XAUUSD value Gate 7. Reject contradiction with `digits`. |
| `ProtoOASpotEvent.bid`, `ask`, `sessionClose` | `uint64`, optional | Transient raw `uint64_t`, exact `Decimal64{int64(raw), 5}`, then canonical broker-neutral `Decimal64{normalizedUnits, digits}` | Require presence when consumed, `0 < raw <= INT64_MAX`, and valid symbol `digits`; divide exactly by `100000`, then apply the integer-only price-normalization contract below. | Gate 7 quote proof. Both bid and ask required for usable BBO; validate exact and normalized bid <= ask. |
| Spread | No dedicated wire field; derived from present `ask` and `bid` | Exact diagnostic spread at scale 5 and broker-neutral spread at symbol `digits` | Perform checked subtraction separately after the exact prices and normalized prices pass ordering checks. | Missing side, ask < bid at either stage, or subtraction overflow rejects the BBO; never infer spread from symbol metadata. |
| Display/execution normalization of spot price | Required `digits` plus fixed-scale spot value | Canonical `Decimal64{normalizedUnits, digits}` and `InstrumentSpec.tickSize = Decimal64{1, digits}` | Use exact integer arithmetic and TradeBot `NearestTiesAwayFromZero` policy when `digits < 5`; append checked zero units when `digits > 5`. Never route the authoritative conversion through `double`. | Official cTrader guidance requires division by `100000` and rounding to symbol digits but does not specify a provider-independent midpoint rule. The selected tie rule is TradeBot policy, not a wire guarantee. |
| `ProtoOASymbol.minVolume`, `maxVolume`, `stepVolume` | `int64`, optional, cents | `InstrumentSpec` quantity `Decimal64` fields at scale 2 | Require presence and positive raw values; construct `Decimal64{raw, 2}` exactly. | Require `min <= max`, `min % step == 0`, and `max % step == 0`; otherwise metadata is contradictory/incomplete. Actual values remain Gate 7 evidence. |
| `ProtoOASymbol.lotSize` | `int64`, optional | `InstrumentSpec.contractSize` | Require present/positive; construct `Decimal64{raw, 2}` units per lot. | Actual value Gate 7. Do not assume 100 ounces or any broker lot size. |
| `ProtoOATradeData.volume`, request `volume`, `executedVolume` | `int64`, required/optional by message, cents | Quantity `Decimal64{raw, 2}` | Map exactly at scale 2. A future outbound cTrader serializer accepts only a final broker-neutral quantity that rescale-checks exactly to cents and satisfies the zero-anchored valid-volume predicate below. | No provider-boundary rounding or clamping. Positive required for new/close request; zero is permitted only where a specific non-order message documents it. Later trading gate only. |
| Absolute order/result prices | Protobuf `double`, optional | Tick-aligned `Decimal64` at core boundary | Later: normalize to tick, convert to finite `double`, serialize/parse, and prove round-trip to the same `Decimal64`. | Reject failed round-trip. Later explicit controlled-order gate. |
| `relativeStopLoss`, `relativeTakeProfit` | `int64`, optional | `Decimal64` distance | If later authorized, require non-negative and construct `Decimal64{raw, 5}`. | Later trading design; not enabled now. |
| Monetary raw values plus `moneyDigits` | `int64`/`uint64` plus optional `uint32` | Monetary `Decimal64` | Require exponent presence/`<= 12`, checked unsigned range, then construct `Decimal64{raw, uint8(moneyDigits)}`. | Missing exponent makes amount unavailable, not zero. Later account-state gate. |
| Millisecond timestamps explicitly documented as such | usually `int64`/`uint64` | `uint64_t timestampNs` | Checked `raw * 1,000,000`. | Require non-negative and `raw <= UINT64_MAX / 1,000,000`; only fields explicitly documented as milliseconds. |
| `ProtoOASpotEvent.timestamp` | optional `int64` | Opaque provider timestamp plus separate ingestion ns | Do not assign a unit or map to core nanoseconds yet; retain only for controlled validation. | Presence requested; unit verification is a Gate 7 stop condition. |
| `ProtoOATrendbar.utcTimestampInMinutes` | optional `uint32` | `uint64_t timestampNs` | If later used, checked `raw * 60,000,000,000`. | Not required for demo XAUUSD spot proof. |
| `expiresIn` token response | integer seconds | Checked duration and absolute expiry metadata | Compute absolute expiry from receipt time using checked duration arithmetic and retain original seconds. | Require positive bounded seconds; later OAuth execution. |
| `ProtoOAClientPermissionScope` | optional enum | Read-only scope state | Require presence and exact `SCOPE_VIEW`. | Every other/unknown value rejects in Gate 6A and Gate 6B. |
| Trade/order enums | required/optional enums | Broker-neutral side/type/status/execution enums | Map by explicit exhaustive tables only. | Unknown values map to `Unknown` and fail state mutation requiring certainty. |

## Deterministic Price Conversion And Normalization

The official cTrader symbol-data guide instructs clients to divide relative
spot prices by `100000` and round the result to the symbol's `digits`. The guide
does not define one cross-language midpoint rule: its examples delegate to
language runtimes. The midpoint rule below is therefore an explicit TradeBot
policy selected because the accepted broker-neutral contract already names
`NearestTiesAwayFromZero`. It is not represented as a cTrader wire guarantee.

The future cTrader boundary must process every consumed price through these
four distinct stages:

| Stage | Exact type and scale | Formula | Failure and retention rule |
| --- | --- | --- | --- |
| 1. Raw wire integer | `uint64_t rawWire`, provider scale 5 | Read the present proto2 field without conversion. | Reject `rawWire == 0` or `rawWire > INT64_MAX`. Retain only in the current message boundary until validation completes. |
| 2. Exact decimal | `Decimal64 exact{int64(rawWire), 5}` | `exact = rawWire / 100000` with the pair `{units=rawWire, scale=5}`; no binary floating point. | Construction is exact after the checked cast. Retain transiently with the raw value for in-process diagnostics; neither may be printed or persisted as callback/account evidence. |
| 3. Symbol-normalized decimal | `Decimal64 normalized{normalizedUnits, digits}` | Apply the integer algorithm below using authoritative `ProtoOASymbol.digits`. | Missing required `digits`, `digits < 0`, `digits > Decimal64::MAX_SCALE`, or checked-arithmetic failure rejects the symbol metadata and the price. |
| 4. Broker-neutral price | canonical `Decimal64{normalizedUnits, digits}` | Copy only the validated stage-3 pair into the broker-neutral event/value. | Provider raw fields do not cross the adapter boundary. `InstrumentSpec.tickSize` is exactly `Decimal64{1, digits}`. |

For `digits == 5`, `normalizedUnits = int64(rawWire)`. For `digits > 5`,
compute `normalizedUnits = int64(rawWire) * 10^(digits - 5)` with checked
`int64_t` multiplication; the extra digits are zeros, not inferred precision.
For `digits < 5`, let `factor = 10^(5 - digits)`,
`q = magnitude(exact.units) / factor`, and
`r = magnitude(exact.units) % factor`. The rounded magnitude is `q` when
`2*r < factor` and `q + 1` when `2*r >= factor`; restore the original sign.
Thus an exact midpoint is rounded away from zero. Powers and increments are
checked before use. The provider spot wire type is unsigned, so a negative
wire price is impossible; the signed rule is still specified for deterministic
tests and any internal rescale helper.

`RejectUnaligned` is not used for inbound fixed-scale cTrader prices merely
because normalization discards nonzero scale-5 digits. Such values are valid
inputs to cTrader's documented rounding step. No `double`, `long double`,
locale-sensitive decimal parse, saturation, or platform rounding mode may
participate in the authoritative conversion.

Comparison and serialization rules are stage-specific:

- raw identity compares the `uint64_t` values; exact price comparison compares
  scale-5 signed units after range validation;
- normalized and broker-neutral prices use the canonical symbol scale
  `digits`; equality is pair equality after canonicalization and ordering uses
  checked integer comparison at that common scale;
- the provider frame serializes the provider raw integer only; any later
  broker-neutral state/fixture serialization writes the signed decimal units
  and numeric scale as integers, plus an optional canonical base-10 rendering,
  never a binary floating-point value;
- real raw and exact pre-rounded diagnostic values remain ephemeral and are
  excluded from logs and review artifacts. Synthetic deterministic fixtures
  may contain clearly labelled non-account test values.

Worked scale-5 to `digits = 4` examples:

| Case | Exact input | Integer result | Broker-neutral value |
| --- | --- | --- | --- |
| Already aligned | `rawWire=123450`, `1.23450` | `r=0`, units `12345` | `Decimal64{12345, 4}` = `1.2345` |
| Below midpoint | `rawWire=123454`, `1.23454` | `r=4 < 5`, units `12345` | `1.2345` |
| Exact midpoint | `rawWire=123455`, `1.23455` | `r=5`, tie away from zero, units `12346` | `1.2346` |
| Above midpoint | `rawWire=123456`, `1.23456` | `r=6 > 5`, units `12346` | `1.2346` |
| Signed-helper test only | `Decimal64{-123455, 5}` | tie away from zero, units `-12346` | `Decimal64{-12346, 4}` = `-1.2346`; a provider wire price with this sign is rejected |
| Overflow | `rawWire=INT64_MAX`, `digits=12` | required multiplication by `10^7` overflows | reject; do not saturate or reduce scale |
| Impossible metadata | any price with `digits=13` | exceeds `Decimal64::MAX_SCALE` | reject the incomplete symbol spec |
| Missing metadata | any price with absent required `digits` | proto2 required-field/metadata validation fails | reject; do not assume five digits |

## Deterministic Volume-Step Contract

The pinned official schema defines `minVolume`, `maxVolume`, and `stepVolume`
as optional `int64` values in cents and describes `stepVolume` as the order
volume step. It does not state that the step is anchored at `minVolume`. The
accepted `BrokerGateway::normalizeToStep` invariant quantizes positive values
in whole step units from zero. Gate 2 therefore selects the following
conservative TradeBot policy rather than attributing an undocumented anchor to
cTrader:

```text
metadataValid = min > 0
             && max > 0
             && step > 0
             && min <= max
             && min % step == 0
             && max % step == 0

validVolume(volume) = metadataValid
                   && volume >= min
                   && volume <= max
                   && volume % step == 0
```

All operands above are checked `int64_t` raw cents. Boundaries are inclusive.
Internal quantities use `Decimal64{rawCents, 2}`; conversion to raw cents
requires an exact checked rescale to scale 2. The modulo operation is evaluated
only after presence, positivity, order, and range checks, so zero or negative
divisors and signed-overflow paths are impossible.

The cTrader provider boundary never rounds, clamps, raises-to-minimum, or
selects a nearby volume. Existing broker-neutral order normalization may reduce
a positive requested quantity toward zero to a whole zero-anchored step before
the provider boundary, preserving the repository invariant that normalization
must not increase exposure. The future cTrader serializer then revalidates the
final quantity with `validVolume`; failure rejects the request without an
external side effect. An absent, zero, or negative step, absent boundary,
`min > max`, or a min/max boundary not divisible by the positive step makes the
entire `InstrumentSpec` incomplete and blocks ordering. This stricter policy
may reject an unusual provider configuration, but it cannot silently create an
unsafe or provider-undocumented volume.

Synthetic examples use cents only and are not FIBO/XAUUSD claims. With
`min=100`, `max=500`, and `step=50`:

| Case | Predicate | Result |
| --- | --- | --- |
| Minimum `100` | inclusive bounds and `100 % 50 == 0` | valid (`Decimal64{100,2}`) |
| One step above minimum `150` | `150 == 100 + 50`; `150 % 50 == 0` | valid |
| Intermediate `125` | within bounds but `125 % 50 != 0` | reject; no normalization in provider boundary |
| Maximum `500` | inclusive bounds and `500 % 50 == 0` | valid |
| Above maximum `550` | `550 > 500` | reject |
| Contradictory metadata | `min=125`, `max=500`, `step=50` | reject metadata because `min % step != 0` |
| Invalid step | `min=100`, `max=500`, `step=0` | reject before modulo |

## XAUUSD InstrumentSpec Construction

Gate 7 may construct a complete `InstrumentSpec` only after one authenticated
full-symbol response supplies all required metadata:

```text
canonicalSymbol   = "XAUUSD"
executionAlias    = exact response-derived symbolName
tickSize          = Decimal64{1, digits}
contractSize      = Decimal64{lotSize, 2}
minimumQuantity   = Decimal64{minVolume, 2}
maximumQuantity   = Decimal64{maxVolume, 2}
quantityStep      = Decimal64{stepVolume, 2}
version           = monotonic local metadata generation
complete          = true only after every check succeeds
```

No broker value is assumed in this document. Missing volume/lot fields,
out-of-range digits, non-positive or misaligned quantities, a changed symbol
ID/name, or contradictory metadata keeps `complete=false` and blocks quote or
order promotion.

## Optionality And Failure Semantics

- Proto2 required-field parse failure rejects the complete frame.
- `bid` without `ask`, `ask` without `bid`, or zero values are incomplete BBO,
  not zero-price quotes.
- Missing `isLive` or broker identity is an ineligible account.
- Missing `moneyDigits` means no monetary normalization.
- Missing symbol volume metadata means no complete `InstrumentSpec`.
- A value that cannot fit the accepted signed core type is rejected; wrapping,
  saturation, truncation, and exception-driven fallback are prohibited.
- Decimal rescaling never uses `TowardZero` for provider-to-core acceptance.

## Baseline Applicability

The accepted baseline already provides `Decimal64` with scale/overflow checks,
`InstrumentSpec`, explicit order normalization, broker-neutral IDs, and
fail-closed adapter contracts. This revalidation supplies the missing cTrader
field/unit mapping without changing those contracts. Provider types stay below
the future adapter/proof boundary.

## Gate 2 Disposition

Every field needed for application/account authentication, demo selection,
XAUUSD identity/metadata, spot quotes, and the later order capability has an
explicit type, presence rule, conversion, overflow rule, and owning gate.
Runtime FIBO values and the under-documented spot timestamp unit are expressly
deferred to the authorized metadata/quote proof and cannot be guessed.

Wade accepted this deterministic numeric contract on 2026-08-07. Acceptance
does not authorize implementation, provider access, market data, or orders.

`GATE 2 ACCEPTED BY WADE`
