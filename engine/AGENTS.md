# Mynyra Engine Instructions

## Identity and authority

`engine/` is the history-preserving Mynyra Engine source boundary. Internal
`tradebot` names may remain where renaming would damage lineage or create
unrelated risk. GitHub is the repository system of record; archived Radicle
documents are historical only.

The operator accepted the bounded Demo milestone on 2026-08-30. Treat that as
an operator decision, not evidence of a currently running provider session,
account, order, service, or deployment. Default builds remain offline and no
LIVE endpoint or LIVE-account path is authorized.

## Required invariants

- Preserve `ExecutionEngine -> RiskEngine -> BrokerGateway -> IBrokerAdapter`
  as the only financial side-effect path.
- Keep BACKTEST/offline behavior deterministic and provider-free by default.
- Keep cTrader Demo support compile-time gated and Demo-endpoint-only.
- Never add credentials, tokens, account identifiers, provider traces, or
  generated runtime evidence to Git.
- Do not initiate provider traffic, OAuth, credential access, market-data
  requests, order attempts, deployment, or risk-limit changes without separate
  explicit authority.
- Run focused tests for touched behavior, then the proportionate offline suite.

## Current authority order

1. Explicit operator instruction for the task.
2. Root and engine `AGENTS.md`.
3. Accepted ADRs and `docs/RISK_POLICY.md`.
4. `docs/ARCHITECTURE.md`.
5. `PLANS.md`.
6. `docs/PROJECT_STATE.md` and `docs/ROADMAP.md`.
7. Other current documentation.

Archive files preserve history and do not outrank current documents.
