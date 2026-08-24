# Mynyra Trade Control Room

Mynyra Trade is an **offline, non-trading product foundation** for a future software-level control room. It is being prepared as the future product home around TradeBot, but it does not migrate, attach to, or execute the existing TradeBot codebase.

## What this repository contains

The current implementation is a static React interface with local display state only. It provides a control-room information architecture, explicit boundary messaging, a visual evidence register, and a documented continuation point for a later Codex pass.

## What it deliberately does not contain

| Category | Current state |
| --- | --- |
| Provider integration | Not configured |
| Credentials or secrets | Not present and must not be added without explicit authorization |
| Accounts, balances, positions, quotes, or performance data | Not represented |
| Order or execution workflow | Prohibited / absent |
| TradeBot source migration | Not started |
| Deployment, publication, commit, or push | Not performed |

## Run locally

```sh
pnpm install
pnpm dev
```

The bespoke visual assets resolve through managed `/manus-storage` paths in the
Manus preview. The interface has CSS fallbacks if those paths are unavailable in
another local environment. Do not replace them with copied provider, account, or
credential artifacts; use separately approved product artwork instead.

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
| `HANDOFF.md` | Exact continuation instructions, verification, and blocked adjacent actions. |
| `client/src/pages/Home.tsx` | Current control-room UI composition and local interaction state. |
| `client/src/index.css` | Design tokens, layout, and responsive presentation. |

## Governing principle

> Observe the system. Do not infer the market.

Any later provider connection, account data, order flow, risk control, or TradeBot source migration needs a separate reviewed plan and an exact authorization boundary.
