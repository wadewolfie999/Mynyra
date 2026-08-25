# Mynyra Engine Radicle Workflow

## Scope

`engine/` is the imported Mynyra Engine source lineage. Local Git is the
repository format; Radicle is the only collaboration forge. This directory has
no GitHub automation, remote, pull-request, release, or deployment workflow.

## Review and verification

Before proposing a Radicle patch, inspect the exact revision and run the
relevant local checks. The default offline baseline is:

```sh
./scripts/ci_policy_checks.sh
python3 scripts/validate_automation.py
./scripts/ci_validate.sh build/local-validation
./scripts/ci_deep_validate.sh build/local-deep-validation
```

These commands retain the `BACKTEST` default and all live-capable and cTrader
options disabled. They do not authorize a provider process, credentials,
orders, release, deployment, or publishing.

Prepare non-executable verification evidence only with
`scripts/package_offline_artifact.sh` from a clean exact-commit tree. Its
manifest and checksums can be attached to a Radicle patch discussion; they are
not a release artifact.

## Publication boundary

Creating commits, publishing, seeding, accepting a patch, delegating a DID,
or changing remotes each requires explicit authorization. Before a Radicle
operation, follow the repository-root `RADICLE.md`, record the RID, device DID,
revision, and verification result, and keep source bundles available for
recovery.
