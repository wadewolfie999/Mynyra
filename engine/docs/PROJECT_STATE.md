# Mynyra Engine Project State

## Evidence epoch

- Repository cleanup date: 2026-08-30.
- Product layout: engine source preserved under `engine/`.
- System of record: GitHub `wadewolfie999/Mynyra`.
- Demo milestone: operator-accepted on 2026-08-30.
- Limitation: the repository does not recreate a redacted external
  commissioning transcript and this state file does not prove a current
  provider session, account, process, deployment, or node service.

## Implemented boundaries

- Default-off BACKTEST/offline engine and deterministic validation.
- Broker-neutral lifecycle, risk, accounting, reconciliation, and persistence
  seams.
- cTrader provider module and Demo-only M1 runtime behind compile-time gates.
- Versioned redacted event/evidence contracts.
- GitHub CI supporting the root frontend and `engine/` layout.

## Current safety state

- LIVE endpoint/account support: absent and unauthorized.
- UI-to-engine execution controls: absent.
- Repository credentials/account identifiers/provider traces: prohibited.
- Provider traffic during repository cleanup: none.
- Next provider attempt, credential access, deployment, or financial action:
  requires a separately scoped operator decision.
