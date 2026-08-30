# Mynyra Engine Documentation

## Current authority

- `ARCHITECTURE.md`: current component and safety boundaries.
- `PROJECT_STATE.md`: evidence-aware repository state at the latest cleanup.
- `ROADMAP.md`: current sequence after the Demo milestone.
- `RESIDUAL_GAPS_BACKLOG.md`: verified remaining gaps only.
- `../PLANS.md`: active bounded work and entry gates.
- `ACTORS.md`: named actor registry, including Bigi's task-scoped role.
- `RISK_POLICY.md`, `SECURITY.md`, `TESTING.md`: enduring risk, security, and
  verification policy.
- `decisions/`: architecture decisions. ADR 0006 records the accepted Demo
  design and operator-reported milestone.

## Historical material

`archive/` contains the pre-cleanup planning ledger, prior state/roadmap,
Radicle workflow, and superseded actor registry. These files remain useful as
lineage and evidence but are not current authority.

Provider gates and phase records outside `archive/` may describe historical
work. They authorize no new provider process, credential use, retry, account
access, order, deployment, or LIVE transition.

## Evidence rule

Repository tests prove source behavior at a revision. Operator acceptance
records a decision. Archived external evidence records a prior epoch. None of
these alone proves current provider, account, process, node, or service state.
