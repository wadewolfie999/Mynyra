# Mynyra Engine Residual Gaps

This list contains verified current gaps, not a grant of implementation or
provider authority.

| Priority | Gap | Closure evidence |
| --- | --- | --- |
| P0 | No current redacted repository pointer to the accepted external Demo commissioning evidence | Add a secret-free evidence reference or digest through a separately reviewed evidence task. |
| P0 | Current provider/account/process/deployment state is unobserved | Perform a separately authorized, read-only live-state inspection; repository tests cannot close this. |
| P0 | LIVE readiness remains intentionally absent | A standalone operator-approved readiness program, including kill switch, limits, monitoring, recovery, reconciliation, credentials, and rollback. |
| P1 | Bigi's GitHub identity mapping is not confirmed in repository governance | Wade confirms whether `mehdibeigiii` is Bigi and assigns the first bounded task. |
| P1 | UI has no read-only engine evidence integration | Approved evidence model/API with provenance, epoch, unavailable state, authentication, deployment, tests, and rollback. |
| P1 | Process-crash recovery and persistent Demo ledger qualification remain outside M1 | Reviewed persistence/recovery design and failure-path tests before any further commissioning. |
| P2 | `SSL_ERROR_NONE` remains source-proven unused, but the ASUS deployment revision is unverified | Recover the canonical Node Control inventory, identify the deployed source/binary revision, then remove the declaration after confirming the current TLS success path. |
| P2 | Historical terminology remains in archived and gate documents | Leave archives immutable in meaning; update only active references when they cause operational ambiguity. |

The frozen pre-cleanup backlog is preserved under
`archive/state/RESIDUAL_GAPS_BACKLOG-through-2026-08-30.md`.

The unused `totalPositioned` accumulator was removed without changing the
current total-equity normalization behavior. Its possible invested-capital
normalization semantics are retained only as a dormant historical proposal in
`archive/design-notes/2026-08-31-invested-capital-var-normalization.md`.
