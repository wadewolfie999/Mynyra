---
name: tradebot-offline-artifact-delivery
description: Prepare, verify, identify, and hand off non-executable Mynyra Engine build/test evidence with exact commit provenance, default-off provider options, full offline tests, SHA-256 manifests, and explicit release/deployment gates. Use for local evidence delivery, checksum verification, or exact-commit rebuild review without distributing the live-capable executable.
---

# TradeBot Offline Artifact Delivery

## Purpose

Create reviewable non-executable evidence artifacts without distributing the
live-capable binary or turning build success into release, deployment,
provider, order, or live authorization.

## Shared Operating Contract

- Follow `AGENTS.md`, `docs/CODEX_EXECUTION_EVIDENCE.md`, and
  `docs/RELEASE_POLICY.md`.
- Use `tradebot-git-safety`, `tradebot-repo-hygiene`,
  `tradebot-cpp-build-test`, and `tradebot-risk-review` as applicable.
- Keep candidate, staged, committed, exact-commit rebuild, uploaded artifact,
  release, deployment, and live evidence separate.

## Required Inputs

- Identify the exact full commit SHA or explicitly label the output as a dirty
  working-tree candidate.
- Identify runner OS/architecture, compiler, CMake version, build type, target,
  and evidence retention destination.
- Identify the operator approval for any upload, release, or deployment action.

## Procedure

1. Inspect branch, full commit, tracked/index state, submodules, and relevant
   ignored artifacts. Refuse exact-commit claims from a dirty tracked tree.
2. Configure a fresh isolated build with `BUILD_TESTING=ON`, Gate 6 OFF, and
   Gate 7 OFF. Do not load `.env` or credential stores.
3. Build the default `tradebot_core` path and run the full CTest suite
   sequentially. Require the successful validation marker to match the current
   full commit and default-off configuration.
4. Run `scripts/package_offline_artifact.sh <build-dir> <new-output-dir>`.
5. Verify the package contains only the CTest log, license, manifest, and
   checksum list. The manifest may identify the built binary by SHA-256 but the
   executable must not be included.
6. Record the full commit, binary hash, manifest hash, build/test result,
   local evidence location, and retention period.
7. Treat the local evidence package as review material. Require a separate
   operator release decision after review.

## Local Evidence Boundary

- Use only the tracked local artifact command from a clean exact-commit tree.
- Use no repository/environment secrets, provider endpoints, or deployment
  environment.
- Retain evidence only after governance, configure, build, and full CTest pass.
- Do not create a tag, publication, package, container, deployment, or
  automatic promotion.

## Prohibited Claims And Actions

- Do not label a working-tree binary as exact-commit evidence or distribute the
  default executable, which retains a separately gated live-capable mode.
- Do not enable Gate 6/Gate 7, `PAPER`, `LIVE`, provider traffic, credentials,
  account access, market data, orders, or financial-limit changes.
- Do not interpret an artifact hash, CI pass, upload, release, or deployment as
  live-trading authorization.
- Do not overwrite an existing output directory or delete artifacts without
  explicit cleanup approval.

## Stop Conditions

Stop if the ref is ambiguous, the tracked/index tree is dirty for an
exact-commit request, tests fail, the package contains an executable or other
unexpected file,
checksums differ, the evidence destination is not approved, or publication,
release, deployment, provider, order, or live authority is absent.

## Expected Output

- Full commit and tracked/index state.
- Build configuration, toolchain, commands, and sequential test result.
- Evidence paths, file allowlist, SHA-256 values, and evidence epoch.
- Evidence location and retention when separately authorized.
- Release/deployment/live status, residual risk, and rollback/expiry guidance.

## Authority Documents

- `AGENTS.md`
- `docs/CODEX_EXECUTION_EVIDENCE.md`
- `docs/TESTING.md`
- `docs/SECURITY.md`
- `docs/RELEASE_POLICY.md`
- `docs/LIVE_TRADING_READINESS.md`
