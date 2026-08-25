# Mynyra Trade Control Room

Mynyra Trade is an **offline, non-trading product foundation** for a future software-level control room. It is being prepared as the future product home around TradeBot, but it does not migrate, attach to, or execute the existing TradeBot codebase.

## What this repository contains

The current implementation is a static React interface with local display state only, bundled with a small Node static-serving compatibility layer. It provides a control-room information architecture, explicit boundary messaging, a visual evidence register, and a documented continuation point for a later Codex pass. It has no product backend.

The website source was created by Manus AI and imported as a local snapshot. That provenance does not make Manus a runtime dependency, deployment target, or authority over future Mynyra behavior.

## What it deliberately does not contain

| Category | Current state |
| --- | --- |
| Provider integration | Not configured |
| Credentials or secrets | Not present and must not be added without explicit authorization |
| Accounts, balances, positions, quotes, or performance data | Not represented |
| Order or execution workflow | Prohibited / absent |
| TradeBot source migration | Not started |
| Deployment, publication, commit, or push | Not performed |
| Product backend on `asus-node` | Future target only; not implemented or deployed |
| TradeBot engine ingestion | Future work; requires a dedicated reviewed migration plan |
| Software forge | Radicle only |

## Run locally

```sh
pnpm install
pnpm dev
```

The bespoke visual assets resolve through managed `/manus-storage` paths in the
Manus preview. The interface has CSS fallbacks if those paths are unavailable in
another local environment. Do not replace them with copied provider, account, or
credential artifacts; use separately approved product artwork instead.

The imported Vite configuration also retains Manus development storage and debug
helpers. Do not supply their environment variables or use those helpers as a
runtime/deployment mechanism without a separate approved source-hardening task.

## Verify

```sh
pnpm check
pnpm build
```

## Key files

| File | Purpose |
| --- | --- |
| `AGENTS.md` | Repository-level working and safety instructions for future agents. |
| `ideas.md` | The chosen visual design contract. |
| `ARCHITECTURE.md` | Static-only architecture and deliberate extension seams. |
| `RADICLE.md` | Sole-forge policy, Codex/Radicle workflow, Manus provenance, and `asus-node` backend boundary. |
| `HANDOFF.md` | Exact continuation instructions, verification, and blocked adjacent actions. |
| `client/src/pages/Home.tsx` | Current control-room UI composition and local interaction state. |
| `client/src/index.css` | Design tokens, layout, and responsive presentation. |

## Governing principle

> Observe the system. Do not infer the market.

Any later provider connection, account data, order flow, risk control, or TradeBot source migration needs a separate reviewed plan and an exact authorization boundary.

## Codex and Radicle

Codex is integrated through the repository's `AGENTS.md` contract and
`RADICLE.md` operating guide. Radicle is the only software forge: use the local
Git worktree for inspection and Radicle for authorized peer-to-peer coordination.
Do not add a GitHub remote, GitHub workflow, or other centralized forge.

The `radicle-collaboration:radicle-repo-workflow` Codex skill is available for
Radicle repository work and must be used when present. The local `rad` CLI is
the fallback when the skill is unavailable.
