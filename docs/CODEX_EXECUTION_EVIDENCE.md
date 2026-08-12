# Codex Execution And Evidence Contract

## Purpose And Authority

- Purpose: define durable execution, authorization, evidence, retry, resource-isolation, and sensitive-memory rules for Codex and other repository-side agents.
- Authority level: workflow policy below `AGENTS.md`, accepted ADRs, risk policy, architecture, active approved plans, current state, roadmap, and `WORKFLOW.md`; above skill-specific instructions and contributor convenience.
- Audience: operator, Codex, maintainers, implementers, reviewers, testers, and handoff recipients.

This document contains reusable procedure. It must not duplicate volatile phase, gate, branch, commit, provider, or approval state. Resolve those facts from the current operator instruction, Git, the active plan, `PROJECT_STATE.md`, and `ROADMAP.md` at the start of each task.

## Authorization Is Non-Transitive

Treat authorization as a tuple containing:

- Exact action.
- In-scope files or subsystem.
- Artifact or commit, when relevant.
- Environment and external-effect boundary.
- Attempt or process budget.
- Stop condition.
- Whether staging, commit, push, publication, credentials, network traffic, provider execution, retry, later gates, orders, risk changes, or live behavior are included.

Permission for one tuple element does not grant another. In particular:

- Review does not authorize correction unless correction is included.
- Correction does not authorize staging or commit.
- Commit does not authorize push, publication, merge, release, or deployment.
- Offline build, sanitizer, preflight, or proof preparation does not authorize network or provider traffic.
- One external process does not authorize reconnect, retry, a second process, or a later gate.
- Credential presence does not authorize reading, displaying, exporting, changing, or using credential values.
- Live-capable code, demo evidence, or provider success does not authorize orders or live trading.

Record ambiguous authority as a blocker. Do not widen it from intent, prior chat, branch names, configured credentials, successful tests, or adjacent completed work.

## Evidence Epochs

Keep these evidence epochs separate:

1. Inspected repository state.
2. Unstaged candidate diff.
3. Staged index.
4. Created commit.
5. Exact-commit rebuild and verification.
6. External or provider process using the reviewed artifact.
7. Post-process handoff and acceptance.

Evidence supports only the epoch and artifact that produced it. A source, test, build-definition, fixture, dependency, or relevant documentation change invalidates later claims based on an earlier candidate until affected checks are rerun.

When exact-artifact proof is required:

- Record full commit SHA.
- Build from that committed tree after confirming the index and tracked worktree state.
- Record the binary or artifact path and SHA-256.
- Record toolchain, configuration, commands, test totals, sanitizer results, warnings, and skipped checks.
- Distinguish pre-commit candidate hashes from the final exact-commit hash.
- Do not call a working-tree binary an exact-commit binary.
- Do not treat an ignored handoff or evidence file as part of the committed artifact.

## Repository And Diff Boundaries

At task start and before handoff, inspect:

```sh
git branch --show-current
git rev-parse HEAD
git status --short
git status --short --ignored
git diff --name-status
git diff --cached --name-status
```

Before an authorized commit, use an explicit path allowlist, review the staged diff independently from the unstaged diff, and run `git diff --cached --check`. After the commit, confirm the tracked worktree and index are clean before claiming an exact-commit rebuild.

A clean tracked/index status does not eliminate ignored-artifact reporting.
Report relevant ignored build, data, handoff, evidence, or credential-like paths
without opening secret-bearing files. Never stage an ignored evidence artifact
merely to make it reviewable.

## Verification And Shared Resources

- Run targeted checks after each meaningful correction and broader checks required by the affected boundary.
- Preserve the first failing command and output. Do not erase it after a passing rerun.
- Classify a failure as introduced, pre-existing, environmental, or unresolved only from evidence.
- Treat local ports, callback listeners, certificate authorities, fixed temporary paths, process names, caches, and generated files as potentially shared resources.
- Run full CTest suites from multiple build trees sequentially unless the tests and every shared resource are proven isolated.
- If a suspected resource collision occurs, stop parallel activity, inspect the shared resource, rerun sequentially, and report both the initial failure and the resolution.
- Do not use a retry to replace diagnosis. State the changed hypothesis, input, code, environment, or precondition that makes the rerun informative.

## External Actions And Retry Budgets

Before any separately authorized external or provider process:

- Verify the exact commit/artifact pair.
- Use presence-only credential preflight; never expose values.
- Establish required wall-clock or clock-skew health with bounded evidence.
- Recheck required ports and conflicting processes immediately before launch.
- Confirm endpoint, account class, mode, allowlist, process count, timeout, and stop condition.
- Confirm the operator's authorization names this exact action and attempt budget.

A failed external attempt consumes its authorization unless the operator explicitly states otherwise. Preflight failure does not authorize remediation that reads secret values, and fixing a precondition does not automatically authorize launch. Stop on the first terminal result when the plan says one process or no retry.

## Sensitive Data And Volatile Memory

Protect every copy, not only the nominal owner:

- Identify caller, callee, pass-by-value, return-value, optional, container, callback, temporary, exception, and allocation-failure copies.
- Clear sensitive or provider-derived material on success, failure, cancellation, timeout, and destruction paths.
- Prefer ownership that minimizes copies; when copies are required, give each copy a terminal-clearing path.
- Do not print, hash, encode, partially redact, or persist values that policy requires to remain unreported.
- Keep evidence schemas free of value-bearing fields that invite secret, token, identifier, raw payload, raw quote, account, balance, position, or order material.
- Never run broad environment dumps or open ignored credential-like files during scans.

## Handoff And Evidence Records

A resumable handoff must state:

- Scope received and exact authorization boundary.
- Branch, full commit, tracked/index state, and relevant ignored artifacts.
- Candidate versus exact-commit evidence.
- Files inspected and changed.
- Commands, results, warnings, failures, skipped checks, and limitations.
- Artifact path and hash when applicable.
- External process count and whether traffic occurred.
- Credential-access status without values.
- Remaining preconditions, prohibited next actions, retry budget, and required approval.

If an ignored evidence record is updated, identify its path and SHA-256 in the handoff. For a record containing its own hash, define and preserve a deterministic normalized-hash rule rather than claiming an impossible self-hash.

## Professional Halt

Stop after the authorized outcome. Do not continue into adjacent cleanup, retries, provider traffic, staging, commit, push, PR creation, later gates, orders, risk changes, or live work merely because the next action appears obvious.
