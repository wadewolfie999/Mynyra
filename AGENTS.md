# Mynyra Repository Instructions

## Product and authority

Mynyra is the product repository for the control room and the preserved engine
lineage under `engine/`. GitHub (`wadewolfie999/Mynyra`) is the repository
system of record. Historical Radicle material is retained under `docs/archive/`
and `engine/docs/archive/`; it is not current authority and no Radicle action is
part of the normal workflow.

The operator accepted the bounded Mynyra Demo milestone on 2026-08-30. That
statement records operator acceptance; it does not prove current provider,
account, process, deployment, or node state. The repository remains default-off
and contains no LIVE endpoint or LIVE-account authorization.

## Working rules

1. Inspect Git status, this file, `README.md`, `ARCHITECTURE.md`, `HANDOFF.md`,
   and the relevant `engine/docs/` authority before mutation.
2. Preserve the `engine/` layout and its Git lineage. Do not re-flatten engine
   source into the repository root.
3. Keep evidence epochs explicit: distinguish repository checks, operator
   reports, archived evidence, and live observations.
4. Preserve the canonical financial side-effect path:
   `ExecutionEngine -> RiskEngine -> BrokerGateway -> IBrokerAdapter`.
5. Default builds and the control-room UI must not contact a provider, request
   credentials, access accounts, retrieve market data, or place orders.
6. LIVE support, real-money deployment, credential handling, risk-limit
   changes, provider retries, and order attempts require separate explicit
   operator authority.
7. Use GitHub pull requests and protected checks for collaboration. Do not add
   another forge or revive archived Radicle workflow without explicit authority.
8. Add focused tests for behavior changes and report exact commands, results,
   warnings, skipped checks, and residual proof gaps.
9. Preserve user work. Do not reset, stash, discard, overwrite, or publish
   unrelated changes.

## Repository layout

- `client/`, `server/`, `shared/`: static evidence-aware control-room surface
  and loopback-only compatibility server.
- `engine/`: Mynyra Engine C++ source, tests, provider adapters, and historical
  engineering documentation.
- `.github/`: protected CI, CodeQL, dependency maintenance, and offline artifact
  workflows supporting the `engine/` layout.
- `docs/archive/`, `engine/docs/archive/`: historical records with no current
  authority.

## Reporting contract

Report files changed, commits and GitHub objects changed, tests and checks run,
warnings, skipped checks, remaining uncertainty, and confirmation that no
provider traffic, credential access, deployment, or financial action occurred.
