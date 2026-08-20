# TradeBot Live Trading Readiness

## Purpose And Authority

- Purpose: define the explicit requirements before any live trading transition.
- Authority level: live-readiness gate under `RISK_POLICY.md` and above workflow convenience.
- Audience: operator, maintainers, Codex, reviewers, and testers.

Live trading is not authorized by repository code, branch name, credentials, or test success. It requires explicit operator approval for the exact scope.

The normal `tradebot_core` build rejects `LIVE` before credential or network
initialization. `TRADEBOT_ENABLE_LIVE_RUNTIME=ON` plus
`--unlock-live-runtime` are containment gates only; satisfying them does not
satisfy this readiness checklist or authorize provider traffic or orders.

Offline verification, preflight, a commit, an exact binary hash, demo-provider
evidence, or a prior bounded process does not authorize live use, orders,
retry, reconnect, autonomous restart, or a later gate.

## Current Program Block

The nine-package Repository Remediation Program is the current prerequisite
overlay. Its foundational governance is approved; WP-0 and WP-1 are merged and
accepted, and WP-2 has an authorized local candidate awaiting acceptance. Live
readiness cannot be evaluated as satisfied until every
package is accepted and the final WP-8 closeout explicitly reassesses this
checklist. Even then, live trading requires a new exact operator authorization;
program completion is not an unlock.

## Required Approval

Operator approval must name:

- Branch and commit.
- Venue or broker.
- Account scope.
- Runtime mode and configuration.
- Instruments.
- Maximum order size.
- Position limits.
- Loss limits.
- Time window.
- Monitoring actor.
- Rollback and kill-switch procedure.

## Required Evidence

Before live unlock:

- Full local build and tests pass.
- Relevant integration tests pass.
- Paper or sandbox validation is complete.
- Kill switch stops new orders.
- Halt/close-only behavior is verified.
- Stale data handling is verified.
- Network disconnect handling is verified.
- Partial fill and reconciliation behavior is verified.
- Logs are redacted and written to an approved path.
- Credentials are least-privilege and not committed.
- Withdrawal permissions are disabled.
- Operator can monitor and stop the system.
- Exact committed source and reviewed binary/artifact identity are recorded.
- Presence-only credential, clock-skew, port, process, endpoint, and allowlist
  preconditions are rechecked immediately before the authorized process.
- Attempt count, retry policy, stop condition, and post-failure authority are explicit.

## Prohibited Until Ready

- Real orders.
- Live account mutation.
- Live exchange credential use.
- Raising limits beyond approved values.
- Autonomous live restart after failure.

## Review Checklist

- `RISK_POLICY.md` reviewed.
- `SECURITY.md` reviewed.
- `CONFIGURATION.md` reviewed.
- Active plan approved.
- ADR created if live architecture changes.
- Handoff prepared for monitoring and rollback.

## Failure During Live Validation

Stop new order submission, preserve evidence, reconcile state, rotate credentials if exposure is suspected, and do not resume without operator approval.
