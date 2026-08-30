# ADR 0002: GitHub as System of Record

## Status

Accepted

## Decision

GitHub repository `wadewolfie999/Mynyra` is the system of record. The protected
`main` branch and its required `validate` check govern integration.

## Context

The repository must support auditable collaboration for the consolidated root
control room and `engine/` layout. Historical Radicle records are preserved in
archive directories but no longer define the collaboration topology.

## Consequences

- Work from a fresh branch or worktree based on current GitHub state.
- Use pull requests and protected checks for integration.
- Do not assume local or remote state is current without refreshing it.
- Keep active docs in the repository concise and current.
- Do not revive a second forge without a new explicit architecture decision.
