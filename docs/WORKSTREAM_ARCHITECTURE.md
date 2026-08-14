# TradeBot Workstream Architecture v1.0

## Purpose And Authority

- Purpose: define the current project-level workstream map, ownership, and coordination model.
- Authority level: accepted project planning and governance map below repository architecture and risk policy; phase status and gates remain authoritative in `ROADMAP.md`.
- Status: Conceptually Accepted for documentation, planning, and evaluation coordination.
- Audience: operator, maintainers, Codex, technical evidence owners, reviewers, and research agents.

This architecture defines domains and coordination boundaries. It does not authorize source implementation, broker selection, broker connection, external broker calls, credential handling, sandbox orders, live-account setup or action, risk-limit changes, or live trading. Workstream activation never overrides phase gates, plans, ADRs, risk policy, or explicit operator approval requirements.

## Current Cross-Workstream Focus

The nine-package Repository Remediation Program in
`REPOSITORY_REMEDIATION_PROGRAM.md` is the sole current implementation overlay
across Workstreams I-VII. It does not replace this domain map or rewrite
historical workstream status. It pauses provider continuation, new phase work,
feature work, research, optimization, documentation-platform selection, and
deployment work while the approved infrastructure-foundation packages are
executed.

WP-0 and WP-1 are merged and accepted; WP-2
through WP-8 remain Planned / NO-GO except for WP-7/WP-8 slices integrated
into WP-0 and WP-1. Any workstream proposal must map to one of those packages
or stop for Wade's explicit scope revision.

## Current Workstream Map

| Workstream | Domain | Current position |
| --- | --- | --- |
| Workstream I — Broker-Neutral Execution Foundation | Deterministic broker-neutral contracts, lifecycle, execution/risk alignment, replay, persistence, and local simulation foundation. | Complete — Accepted through Phase 22. |
| Workstream II — Broker Integration Program | FIBO/cTrader demo target, official Open API integration gates, failure semantics, and future broker-hosted validation/readiness paths. | Historical Phase 23/Gate 1–7 evidence is preserved. Gate 7 remains incomplete at `gate7_subscription_failed`; continuation is paused by the remediation focus lock and must map to WP-6. Provider execution, Gate 8–9, and later remain blocked. |
| Workstream III — Documentation & Knowledge Architecture | Documentation platform, information architecture, canonical knowledge, and maintenance workflows. | Parallel/future domain unless separately activated. |
| Workstream IV — ML Optimization & Strategy Research | Reproducible offline optimization, ML-assisted research, and strategy evidence. | Parallel/future domain unless separately activated. |
| Workstream V — Core Platform Enhancement | Broker-neutral core capability, reliability, maintainability, and performance improvements. | Parallel/future domain unless separately activated. |
| Workstream VI — Production Governance & Live Readiness | Operational controls, monitoring, release governance, and readiness evidence required before any live consideration. | Parallel/future domain unless separately activated; live trading remains unauthorized. |
| Workstream VII — Strategic Expansion Alternatives | Alternative venues, products, deployment models, and longer-horizon options. | Parallel/future domain unless separately activated. |

Workstreams III–VII may be researched or planned only within separately authorized scope. Their presence in this map is not activation, sequencing authority, or permission to implement.

## Workstream II Amendment

Workstream II contains two distinct future-facing paths:

1. Demo/Sandbox environment setup path
   - Purpose: prepare evidence and requirements for a future broker-hosted non-live validation environment.
   - Current authorization: documentation, evaluation criteria, adapter-fit evidence, and failure-mode analysis only.
   - Prohibited now: broker connection, external broker calls, credential use, account access, and sandbox order placement.

2. Live Account readiness path
   - Purpose: prepare governance, safety, monitoring, rollback, and approval requirements for possible future live-account consideration.
   - Current authorization: readiness analysis and checklist preparation only.
   - Prohibited now: live-account setup or action, credential handling, broker connection, live deployment, and live trading.

The paths are separate. Demo/sandbox evidence cannot be treated as live approval, and live-readiness preparation cannot create or mutate an account. Live deployment remains blocked until the operator grants exact live-readiness approval under `RISK_POLICY.md` and `LIVE_TRADING_READINESS.md`.

## Phase 23 Operating Rhythm

The primary rhythm is Strategy 2 — Parallel Evidence Lanes With Wade Checkpoints.

- Wade owns authority, scope, gates, the broker-selection decision, and acceptance.
- Bigi owns technical evidence, the adapter-fit audit, the failure-mode checklist, and demo/live semantics analysis.
- ChatGPT/review assistant supports prompt structure, output review, and governance-drift detection.

This rhythm produced Wade's FIBO Group/cTrader selection. ADR 0004 makes official Open API the sole integration path; the Algo Bridge is abandoned, non-controlling, and out of scope. The selection itself did not authorize later execution. Wade separately authorized the Gate 6 umbrella (`Gate 6A → mandatory Wade checkpoint → Gate 6B`) and later authorized Gate 7. Gate 7 history progressed through Keychain, OAuth, and callback-timeout boundaries; the latest process reached `gate7_subscription_failed` after full metadata validation. Orders, Gate 8–9, and live trading remain unauthorized.

Strategy 3 gate labels may be added later as a governance overlay. A full Kanban system is intentionally not part of v1.0.

## Decision And Gate Rules

- Phase 22 remains Complete — Accepted at the Workstream I broker-neutral boundary.
- Workstream II completed Phase 23 selection. Gate 2 and Gate 5 were accepted
  by Wade on 2026-08-07; Gate 5.1 was merged and accepted on 2026-08-09; PR #25
  is merged; Gate 6 execution was accepted; and Gate 7 implementation/prior
  offline validation completed. The latest Gate 7 process reached
  `gate7_subscription_failed` after full XAUUSD metadata validation. Residual
  subscription/spot diagnostics and first-single-complete-BBO handling are
  authorized offline; provider execution remains separately blocked.
- Only Wade may authorize each Open API execution gate.
- Every future Phase 24 execution step remains blocked until Wade separately approves its exact scope.
- Workstream III–VII status changes require separate activation.
- No evidence lane may use credentials, call a broker, connect an account, place sandbox or real orders, or enable live behavior.
- No workstream label, checkpoint, artifact, or acceptance statement authorizes live trading.

## Professional Halting Point

WP-0 and WP-1 are merged and accepted. Stop before WP-2 until Wade separately
authorizes its exact action. Provider execution, Gates 8–9, orders, risk-limit
changes, workflow dispatch, deployment, release, and live actions remain
outside current authorization.
