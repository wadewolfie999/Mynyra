# Mynyra-Trade Radicle Operations Contract

## Forge authority

Radicle is the sole software forge for this repository. The repository is a normal local Git working tree whose only configured upstream is the `rad` remote. Do not add GitHub, GitLab, or any other centralized forge remote, automation, pull-request workflow, or mirror unless the operator explicitly changes this policy.

The current Radicle project identity and configured remotes are operational facts, not durable documentation constants. Verify them before any forge action:

```sh
git status --short --branch
git remote -v
rad inspect
rad remote
```

The usual local Git inspection commands remain appropriate. Use Radicle for peer-to-peer project coordination, including issues and patches, when that work is explicitly authorized.

## Codex capability

Use the `radicle-collaboration:radicle-repo-workflow` skill for Radicle repository, node, project, and collaboration work when it is available in the Codex environment. The skill establishes repository, identity, node, project, and visibility evidence before proposing a mutation. Its availability does not authorize initialization, publication, seeding, syncing, metadata edits, or collaboration records.

If that skill is not available in a future environment, use the local `rad` CLI as the fallback and retain the same authorization boundaries.

## Codex workflow

1. Read `AGENTS.md`, `README.md`, `ARCHITECTURE.md`, this file, `HANDOFF.md`, and the relevant source before changing anything.
2. Preserve a dirty worktree and inspect the exact diff. Do not treat a clean working tree, a local commit, or a healthy Radicle node as authorization to publish.
3. For a review, use read-only evidence first: `git status`, `git diff`, `git log`, `rad inspect`, and `rad remote`.
4. Use `rad issue` and `rad patch` only when the requested collaboration record is in scope. Review the target and resulting persistent record before reporting it.
5. Treat `git commit`, `git push`, `rad publish`, `rad sync`, `rad seed`, and follow-policy changes as state-changing forge/network operations. They require explicit authorization after their exact target and consequence are reported.

The Radicle project is currently public in local storage, while its current project description exceeds this repository's offline, non-trading authority. Treat that as a metadata-alignment gap; do not edit the project identity or infer a deployed/active engine without a separately authorized update.

## Explicit exclusions

- No GitHub remote, `gh` command, GitHub Actions file, GitHub pull request, GitHub issue, or GitHub-hosted release is part of this repository's workflow.
- Do not equate Radicle presence with public publication. `rad publish`, syncing, and seeding can make repository data available to peers and remain separately controlled actions.
- Do not create a centralized-CI substitute by default. Any automation must be designed for Radicle-based review and receive its own authorization.

## Manus source provenance

The control-room website was created by Manus AI and imported as source into this repository. Manus is not an active source of truth, a deployment target, or an authority for runtime behavior.

Some imported files retain Manus-oriented development helpers and managed-asset paths. In particular, `vite.config.ts` contains development storage/debug helpers that can make external requests only when their environment configuration is supplied. Do not set those values, run them as a deployment path, or represent those helpers as Mynyra infrastructure without a separate source-hardening task and explicit authorization.

## Future architecture boundaries

The frozen TradeBot M1 source lineage is imported under `engine/` by the
authorized cutover plan. No credential-like material, runtime artifact, account
identifier, provider trace, ignored output, or `.github/` automation entered
the current imported tree. Further imports remain forbidden unless a reviewed
scope names the files, verification, rollback, and authority.

`asus-node` is the target primary runtime node. Its first engine proof is a
release-built, immutable, hash-pinned offline replay. Provider traffic, Linux
credential custody, OAuth, engine services, UI evidence APIs, and operational
controls remain separately gated. Before any node change, identify the service
owner, source revision from Radicle, network exposure, configuration/secrets
boundary, persistence, observability, health checks, rollback, and operator
authorization.
