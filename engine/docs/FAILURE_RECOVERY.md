# TradeBot Failure Recovery

## Purpose And Authority

- Purpose: define recovery from interrupted work, failed checks, stale state, generated artifacts, and safety incidents.
- Authority level: recovery workflow below risk/security policy and above routine contributor guidance.
- Audience: operator, maintainers, Codex, contributors, reviewers, and testers.

## Interrupted Work

1. Stop mutation.
2. Capture branch, commit, and worktree status.
3. Identify changed and untracked files.
4. Preserve generated evidence needed for review.
5. Write a handoff using `HANDOFF.md`.
6. Do not revert or delete unrelated work.

## Failed Build Or Test

1. Record the exact command and failure.
2. Identify whether failure is pre-existing or introduced by current work.
3. Avoid broad refactors while diagnosing.
4. Run targeted checks after fixes.
5. Re-run full CTest for shared behavior changes.
6. Report residual warnings and skipped checks.
7. Do not retry an unchanged failure merely to seek a pass. Record the changed
   hypothesis, input, code, environment, or precondition that makes a rerun
   informative.

If several build trees were tested concurrently, treat ports, loopback
listeners, local certificate authorities, fixed temporary paths, processes,
caches, and generated outputs as possible shared resources. Stop parallel
activity and rerun sequentially only after preserving the initial failure.

For local validation failures, use `tradebot-ci-failure-recovery`: preserve the
exact revision, command, environment, and first failing output; reproduce the
narrowest offline equivalent; and allow at most two evidence-backed local
correction cycles. Do not publish or trigger a remote action without separate
operator authorization.

## Documentation Audit Failure

- Fix broken references, stale paths, conflicting authority, or unsafe wording.
- If a link checker is unavailable, state that it was unavailable.
- Review grep hits for financial, credential, and live-trading terms rather than deleting necessary safety language.

## Stale Repository State

If branch, status, or commits differ from handoff:

- Inspect before proceeding.
- Do not assume previous scope remains valid.
- Reconcile with `PROJECT_STATE.md`, active plan, and ADRs.
- Escalate if changes affect scope or safety.
- Identify the last valid evidence epoch. Candidate or pre-commit results do
  not prove a later commit or binary.

## Generated Artifact Containment

- `build/`, `data/results/`, `data/archive/`, `data/historical/`, `output/`,
  `artifacts/`, `handoff/`, and `handoffs/` are directory-ignored. CI asserts
  these rules and verifies that intentional CSV/binary fixtures remain visible.
- Do not stage generated outputs unless approved.
- Do not delete generated outputs that may be needed as evidence for an active plan without approval.

## Snapshot Or Checkpoint Failure

- Version-13 snapshots are BACKTEST-only. Their accounting fields use signed
  scale-8 integer units. PAPER/LIVE resume is rejected before
  file opening, adapter construction, credential access, or broker work because
  broker lifecycle and reconciliation recovery belongs to WP-4.
- Snapshot load validates the complete envelope, checksum, schema, version,
  payload, and cross-field invariants before mutating portfolio, risk, regime,
  allocator, or checkpoint time.
- Snapshot save validates detached portfolio, risk, regime, and allocator state
  before replacing the prior checkpoint. Non-finite values fail the checkpoint.
- The persisted immutable risk configuration must equal the runtime
  configuration, and restored effective limits may never exceed that baseline.
- Unsupported older versions, including version 12, fail with a
  migration-required diagnostic; no implicit migration is attempted.
- A nonempty steady-clock API-error window is not portable across restart and
  makes checkpointing fail. A durable external halt and latency close-only
  state remain conservative and round-trip exactly.
- Checkpoints write a same-directory temporary file and rename it only after a
  complete flush. A failed write retains the previous snapshot and stops the
  event loop with an actionable error.

## Credential Or Secret Incident

Follow `SECURITY.md`: stop propagation, notify operator, revoke/rotate, audit, and document without secret values.

## Live-Capable Incident

If live-capable behavior is unexpectedly implicated:

- Stop new order submission.
- Preserve logs without exposing secrets.
- Reconcile local and external state if authorized.
- Do not resume until the operator approves.

## External Attempt Failure

- Treat a one-process or one-attempt authorization as consumed by the first
  launched external process unless the operator explicitly says otherwise.
- Preflight failure does not authorize credential-value inspection, provider
  launch, retry, reconnect, or remediation outside scope.
- Before any newly authorized attempt, re-establish exact commit/artifact,
  presence-only credential, clock, port, process, endpoint, allowlist, timeout,
  and stop-condition evidence.
- Record the first terminal result and stop when the plan prohibits retry.

## Git Recovery

- Do not run destructive Git commands without operator approval.
- Prefer non-destructive inspection: `git status --short`, `git diff`, `git log`.
- Use commits, branches, and approved reverts for recovery.
