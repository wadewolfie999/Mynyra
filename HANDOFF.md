# Mynyra Current Handoff

## Authority epoch

- Date: 2026-08-30.
- Repository system of record: GitHub `wadewolfie999/Mynyra`.
- Layout: control room at repository root; Mynyra Engine under `engine/`.
- Milestone: Demo reached and operator-accepted on 2026-08-30.
- Evidence limitation: this repository cleanup does not itself contain or
  recreate a redacted external commissioning transcript, nor does it prove
  current provider, account, process, deployment, ASUS, or service state.
- LIVE support and real-money authority: absent.

## Current boundaries

- Default frontend and engine checks are provider-free.
- cTrader Demo source is compile-time gated.
- The only financial side-effect path is
  `ExecutionEngine -> RiskEngine -> BrokerGateway -> IBrokerAdapter`.
- No provider process, OAuth flow, credential read, account request, market-data
  request, order attempt, deployment, or node mutation belongs to repository
  maintenance.

## Collaboration

- Wade owns final scope, merge, release, provider, credential, deployment, and
  financial authority.
- Bigi is registered in `engine/docs/ACTORS.md` as a task-scoped human
  operator-contributor. The GitHub collaborator account `mehdibeigiii` is
  observed with write access, but its mapping to Bigi requires Wade's explicit
  confirmation before it is treated as identity evidence.
- ChatGPT and Codex remain advisory/task-scoped agents without self-approval or
  live-trading authority.

## Continue safely

1. Refresh `git status`, branch protection, and open pull requests.
2. Read `AGENTS.md`, `engine/AGENTS.md`, and the current engine state/roadmap.
3. Work through a bounded issue or pull request with named acceptance criteria.
4. Run frontend and focused engine checks before the full offline suite.
5. Record results and proof gaps without converting old evidence into current
   runtime claims.

Historical Radicle records, old plans, prior state snapshots, and the ASUS
cutover envelope are under archive directories and have no current authority.
