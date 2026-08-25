---
name: tradebot-ci-failure-recovery
description: Diagnose and recover failing TradeBot GitHub Actions or equivalent local CI checks using preserved logs, exact revision context, bounded reproduction, root-cause classification, minimal fixes, and fresh verification. Use when a workflow, build, test, sanitizer, governance, or artifact-delivery check fails or becomes flaky.
---

# TradeBot CI Failure Recovery

## Purpose

Turn a CI failure into a minimal, locally verified correction or an explicit
environmental/blocking diagnosis without blind reruns.

## Shared Operating Contract

- Follow `AGENTS.md` and `docs/CODEX_EXECUTION_EVIDENCE.md`.
- Use `tradebot-git-safety`, `tradebot-repo-inspection`, and
  `tradebot-authority-state-audit` before editing.
- Do not expose secret-masked values, provider-derived identifiers, or complete
  logs that may contain sensitive material.

## Required Inputs

- Capture the workflow/job/step, run URL or log artifact, event, runner OS,
  exact commit SHA, and first failing command.
- Capture whether the failure occurred in governance, configure, compile,
  link, test, sanitizer, packaging, upload, or GitHub infrastructure.
- Identify whether the failing revision is still the current candidate.

## Recovery Budget

- Inspect once, reproduce once, and permit at most two scoped fix-and-verify
  cycles for the same failure mechanism.
- Do not rerun an external workflow merely to seek a passing result.
- Triggering or rerunning GitHub Actions is an external mutation and requires
  separate operator authorization.

## Procedure

1. Preserve the first failure, relevant logs, exact revision, runner image, and
   timestamps. Redact sensitive values without erasing the error category.
2. Compare the failed workflow commands with `.github/workflows/`, `scripts/`,
   `docs/TESTING.md`, and the exact commit diff.
3. Classify the cause:
   - deterministic repository defect;
   - workflow/configuration defect;
   - runner/toolchain difference;
   - shared-resource contention;
   - intermittent GitHub service failure;
   - stale or superseded evidence;
   - safety/authorization stop.
4. Reproduce the narrowest equivalent command locally when safe. Use the same
   default-off CMake options and run CTest suites sequentially.
5. Form one evidence-backed hypothesis. Make the minimum scoped correction only
   when implementation is authorized.
6. Rerun the failed check, affected targeted checks, and the required full
   suite. Keep initial and rerun outcomes in the report.
7. Use the applicable C++, risk, network, execution, replay, L2, performance,
   documentation, and hygiene skills before declaring readiness.
8. If local reproduction passes, identify the environmental difference instead
   of weakening assertions, timeouts, sanitizers, risk gates, or redaction.

## Prohibited Recovery Shortcuts

- Do not disable tests, sanitizers, warnings, permission restrictions, secret
  scanning, or default-off live/provider gates to make CI green.
- Do not add retries around deterministic failures.
- Do not use `pull_request_target`, write permissions, repository secrets,
  self-hosted runners, live endpoints, OAuth, broker access, or orders.
- Do not force-push, amend, merge, release, or deploy without exact approval.

## Stop Conditions

Stop after two unsuccessful scoped correction cycles, on a repeated unchanged
failure, when logs are missing for a material conclusion, when reproduction
would use credentials or external/provider access, or when the proposed fix
would widen safety or runtime behavior.

## Expected Output

- Exact failing revision, workflow/job/step, and first failure.
- Root-cause classification and evidence.
- Local reproduction and correction commands.
- Files changed, initial result, fresh result, and evidence epoch.
- Residual flake/environment risk and whether an authorized rerun remains.

## Authority Documents

- `AGENTS.md`
- `docs/CODEX_EXECUTION_EVIDENCE.md`
- `docs/TESTING.md`
- `docs/FAILURE_RECOVERY.md`
- `docs/SECURITY.md`
- `docs/RELEASE_POLICY.md`
