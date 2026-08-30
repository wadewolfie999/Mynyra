---
name: tradebot-network-live-boundary-review
description: Review TradeBot network, auth, credential, live-capable, PAPER, LIVE, broker, and external I/O boundaries. Use for AsyncNetworkClient, AuthManager, LiveDataAdapter, BrokerGateway, SystemConfig, external connectivity, logs, env vars, or mode-semantics review.
---
# tradebot-network-live-boundary-review

## Purpose

Protect network, credential, mode, broker, and live-capable boundaries from unsafe defaults or ambiguous external side effects.

## Global TradeBot Rules

- Follow `AGENTS.md` and `docs/CODEX_EXECUTION_EVIDENCE.md`.
- Resolve volatile branch, phase, gate, provider, and approval facts from Git,
  the active plan, `PROJECT_STATE.md`, and `ROADMAP.md`; do not hardcode them in
  this skill.
- Keep authorization and evidence epochs separate and stop at the exact
  operator-approved boundary.

## Activation Conditions

Use when touching or reviewing `AsyncNetworkClient`, `AuthManager`, `LiveDataAdapter`, `BrokerGateway`, `SystemConfig`, environment variables, logs, external I/O, PAPER/LIVE semantics, broker integration, or live-readiness claims.

## Must Not Be Used

Do not use to authorize live trading, inspect secret values, or run external exchange operations.

## Required Inputs

- Changed files or proposed design.
- Runtime modes affected.
- Credential or env-var names involved.
- External I/O or log behavior.
- Operator approval evidence if any live-capable action is requested.
- Exact external action, artifact, endpoint, attempt budget, preconditions, and
  stop condition when network/provider execution is proposed.

## Required Outputs

- Mode-boundary finding.
- Network and external-I/O finding.
- Credential and log-safety finding.
- Live-authorization status.

## Required Inspection

Read:

- `docs/RISK_POLICY.md`
- `docs/LIVE_TRADING_READINESS.md`
- `docs/SECURITY.md`
- `docs/CONFIGURATION.md`
- `docs/WORKSTREAM_I_ADAPTER_CONTRACT.md`
- Affected source/tests when scoped

Run:

```sh
git status --short
git diff --name-status
```

## Mode Rules

- `BACKTEST` performs no external network calls.
- `PAPER` is simulated unless explicitly approved otherwise.
- `LIVE` is prohibited without exact operator approval.
- Live-capable code is not live-enabled behavior.

## Procedure

1. Identify each network, auth, credential, log, broker, and external I/O boundary.
2. Confirm default mode remains non-live.
3. Confirm secrets are never printed, copied, committed, or documented as values.
4. Confirm PAPER behavior remains simulated unless separately approved.
5. Confirm tests do not require real venue, account, or endpoint state.
6. For proposed external execution, require exact commit/artifact identity,
   presence-only credential preflight, bounded clock health, immediate
   port/process checks, fixed endpoint/allowlist, one-process budget, and a
   separately named authorization.
7. Trace sensitive/provider-derived data through caller, callee, return,
   container, callback, temporary, failure, and destruction copies.
8. Reject default-live, real-order, secret-logging, implicit retry, or
   preflight-as-execution ambiguity.

## Validation Checklist

- No secret values are exposed.
- Env-var names may be documented; values must not be.
- `BACKTEST` remains deterministic and offline.
- `PAPER` is not real venue execution by default.
- `LIVE` requires exact operator approval for venue, account, scope, branch, commit, config, and time window.
- Logs redact or avoid sensitive values.
- Preflight and credential presence do not authorize value access or traffic.
- A failed one-process attempt does not authorize retry or reconnect.
- Every volatile sensitive/provider-derived copy has terminal clearing.

## Failure Modes Caught

- Default-live or real-order ambiguity.
- Secret logging or credential fixture leakage.
- External network dependency in deterministic tests.
- PAPER treated as sandbox without approval.
- Broker connection added outside approved boundary.
- Review, commit, exact-artifact proof, or preflight treated as provider authority.
- Retry after a consumed attempt budget.

## Hard Prohibitions

- Do not expose, print, copy, or modify secrets.
- Do not edit `.env`, keys, certificates, credentials, or account IDs.
- Do not run live services, real orders, or exchange operations.
- Do not run provider traffic, retry, reconnect, or a second process unless the
  exact action and attempt are authorized.
- Do not implement broker-specific behavior without approval.
- Do not stage, commit, push, reset, clean, or discard changes.

## Interaction With Existing Skills

- Use with `tradebot-risk-review` for credential, network, live-capable, or broker risk.
- Use with `tradebot-integration-architecture-review` for Workstream I adapter boundaries.
- Use with `tradebot-execution-pipeline-validation` for order lifecycle paths touching broker feedback.
- Feed findings into `tradebot-pr-readiness-review` and `tradebot-handoff`.

## Example Invocation Prompt

```text
Use $tradebot-network-live-boundary-review to verify a LiveDataAdapter change does not make BACKTEST depend on network state.
```

## Stop Conditions

Stop if secret exposure, external action/attempt authority, preflight, clock,
port/process state, endpoint, allowlist, live authorization, PAPER semantics,
or mode defaults are ambiguous.

## Reporting Format

```markdown
## Network/Live Boundary Review
- Modes affected:
- External I/O:
- Credential handling:
- Log safety:
- Live authorization:
- Findings:
- Status:
```

## Authority Documents

- `docs/RISK_POLICY.md`
- `docs/LIVE_TRADING_READINESS.md`
- `docs/SECURITY.md`
- `docs/CONFIGURATION.md`
