# Workstream II Iran-Compatible Provider Pivot

## Document Control

- Status: Superseded historical evidence package — FIBO Group/cTrader was
  selected later; current work is governed by the Repository Remediation
  Program
- Authority: Wade operator directive and explicit research/documentation authorization
- Effective date: 2026-07-29
- Workstream: II — Broker Integration Program
- Formal phase state at creation: Phase 23 Not Started; current phase state is
  controlled by `ROADMAP.md`
- OANDA state: permanently cancelled by the later operator decision
- Non-authorization: no provider selection, account creation, credentials, connectivity, provider-specific implementation, sandbox order, real order, risk change, or live trading

## Controlling Decision

OANDA is no longer the active demo-integration target. Its lane is on hold because an Iran-based operator could not establish lawful account eligibility from the available registration path. TradeBot will not use a VPN location, false residence, another person's identity, or any other circumvention.

The active task is a lawful Iran-compatible replacement-provider search. A candidate cannot pass the evidence gate merely because its website is reachable, offers Persian language, omits Iran from one restriction list, or permits a form submission.

## Mandatory Candidate Gates

Every candidate must provide current, attributable evidence for all of the following before Wade may consider selection:

1. The exact contracting legal entity lawfully accepts a client who is both an Iranian citizen and resident of Tehran.
2. Iranian identity and proof-of-address documents are accepted without location or identity misrepresentation.
3. A demo/practice account is available without funding or live-account activation.
4. XAUUSD, XAU/USD, or an explicitly mapped spot-gold CFD is available in that demo environment.
5. An automation interface supports market data, order submission, status/lifecycle events, account state, and reconciliation.
6. The interface can operate from Iran without a VPN workaround and without violating provider terms.
7. Demo and live permissions, endpoints, credentials, and risk gates remain technically and operationally separated.
8. TradeBot can normalize provider events below `BrokerGateway` and preserve deterministic audit/replay behavior.

Written provider confirmation is required for gates 1, 2, 3, 5, and 6. Marketing pages alone are insufficient.

## Initial Evidence Screen

Evidence was checked on 2026-07-29. Results are provisional and do not select a provider.

| Candidate path | Iran-access evidence | Demo and XAUUSD evidence | Automation evidence | Disposition |
| --- | --- | --- | --- | --- |
| FIBO Group cTrader | Strong first-party signal: FIBO's Persian site explicitly describes rial deposit/withdrawal services for clients residing in Iran. The same site notes that a fuller restricted-country list exists, so direct written confirmation remains mandatory. | FIBO advertises demo cTrader accounts and spot-gold/XAUUSD products. | cTrader Open API officially supports demo accounts, real-time market data, trading operations, and order/deal/position history for cTrader-affiliated broker accounts. | Priority 1 validation candidate; not selected. |
| FXOpen TickTrader | Unverified: current FXOpen INT pages explicitly exclude US residents and include a general local-law caveat, but no Iran-specific acceptance statement was found. | Official pages advertise demo accounts, real-time data, and Gold/XAUUSD. | TickTrader advertises FIX, REST, and WebSocket API access; exact demo API entitlement and terms require confirmation. | Priority 2 technical candidate; blocked on Iran-resident eligibility evidence. |
| AMarkets MT5 | Moderate signal only: Iran is absent from the current official restricted-country list, but absence is not affirmative eligibility evidence. | Official pages advertise a demo account and XAUUSD. | MT5 supports algorithmic trading, but no first-party public remote broker API suitable for direct TradeBot integration was established in this review. | Fallback candidate; legal confirmation and an acceptable bridge design are unresolved. |
| IFC Markets MT5/NetTradeX | Moderate signal only: official pages claim broad global availability and currently name the US, Russian Federation, and BVI as excluded for the referenced entity; Iran-specific eligibility was not affirmatively confirmed. | Official pages advertise demo accounts and XAUUSD on NetTradeX and MT4/MT5. | No first-party public remote execution API suitable for direct TradeBot integration was established in this review. | Fallback candidate; legal confirmation and interface fit are unresolved. |

## Excluded Or Held Paths

- OANDA: ON HOLD pending a lawful Iran-resident path confirmed by OANDA Support.
- RoboForex: excluded from this search because its current first-party site expressly lists Iran among restricted countries.
- Any provider requiring a false country, VPN-based residence substitution, third-party identity, or concealed Iranian documents: rejected.
- Any live-only, deposit-first, or undocumented API path: rejected for the demo milestone.

## Primary Evidence

- FIBO Iran-resident service signal: https://fg-persian.com/clients/deposit-and-withdrawal/
- FIBO cTrader demo/gold path: https://pt.fibogroup.com/products/account-types/ctrader-zero/
- cTrader Open API scope: https://help.ctrader.com/open-api/
- FXOpen demo and instruments: https://fxopen.com/en/forex-demo-account/
- FXOpen TickTrader API surface: https://fxopen.com/en/ticktrader/
- AMarkets restricted countries: https://www.amarkets.com/country-restrictions/
- AMarkets demo: https://www.amarkets.com/trading/open-demo-account/
- AMarkets XAUUSD: https://www.amarkets.com/trading-instruments/xauusd/
- IFC Markets availability: https://www.ifcmarkets.com/
- IFC Markets XAUUSD demo path: https://www.ifcmarkets.com/en/trading-conditions/precious-metals/xauusd
- RoboForex restriction evidence: https://roboforex.com/beginners/analytics/forex-forecast/commodities/xau-usd-gold-forecast-2026-06-22/
- US Iran-sanctions baseline for US-person/provider exposure: https://ofac.treasury.gov/faqs/topic/1551

## Required Support Confirmation

Before any account action, the operator should obtain written answers from the Priority 1 candidate using true Iranian citizenship and Tehran residence:

1. Do you currently accept a new demo-only client who is an Iranian citizen residing in Tehran, Iran?
2. May that client register and connect from Iran without a VPN?
3. Which legal entity would provide the service, and which current terms and restricted-country document apply?
4. Is a cTrader demo account available with XAUUSD/spot gold?
5. Is cTrader Open API enabled for that demo account for streaming data, order submission, order status, positions, and account state?
6. Are application registration or API permissions subject to additional approval, fees, limits, or geographic restrictions?
7. Can the demo account be used without creating, funding, or activating a live account?

Do not send identity documents, credentials, tokens, or account identifiers in an initial support inquiry.

## Historical Wade Checkpoint

At the time of this evidence package, the next decision package was expected to
contain written support evidence for FIBO Group and, if pursued, FXOpen; the
exact legal entity and terms; demo instrument and API confirmation; adapter-fit
comparison; failure-mode implications; and unresolved risks.

That historical checkpoint was superseded by Wade's later FIBO Group/cTrader
selection. It is not a current task. Any provider work now must map to WP-6;
credentials, connectivity, account creation, sandbox orders, and live trading
remain blocked.
