# Mynyra-Trade Repository Instructions

## Product boundary

Mynyra-Trade is the product-level home for the broader software system around
TradeBot. `engine/` is a history-preserving import of the frozen TradeBot M1
candidate and is product-facing as **Mynyra Engine** while retaining its
internal `tradebot` namespaces. The source predecessor remains a retained,
read-only recovery checkout during the cutover window.

The current source tree is a Manus AI export for the Mynyra control-room website. Treat it as imported product source, not as a live Manus project or a deployment configuration. Retained template code and development helpers require review before they are enabled, preserved in a production path, or used to make external requests.

## Current authorized scope

The present implementation is an **offline, non-trading control-room and engine
foundation**. The UI remains static local presentation only. `engine/` may run
only its hash-pinned `BACKTEST` replay with provider and order permissions
false. No part of the repository may connect to an external provider, read or
represent credentials, access accounts, retrieve market data, create orders,
change risk limits, or imply that any such capability exists.

The repository includes a Node static-serving compatibility entrypoint, but no product backend is enabled or deployed. If a product backend is later authorized, its target node is `asus-node`; that target is not deployment authority, a current service claim, or permission to alter the node.

## Working rules

1. Inspect Git state, this file, `README.md`, `ARCHITECTURE.md`, `RADICLE.md`, and `HANDOFF.md` before mutation.
2. Keep UI claims evidence-aware. Distinguish static local copy, observed evidence, inference, and unavailable state.
3. Preserve the explicit offline and non-trading posture. Unavailable controls must say so rather than silently simulate success.
4. Preserve the imported engine lineage and its current exclusion of
   `.github/`. Never copy credential-like files, provider traces, account
   identifiers, generated runtime artifacts, or ignored outputs into this
   repository.
5. Add focused verification for behavior changes. Record commands and results in the handoff.
6. Use Radicle as the only software forge. Do not add GitHub remotes, GitHub Actions, GitHub pull-request workflows, `gh` commands, GitHub links as a collaboration surface, or a second centralized forge without separately explicit authorization. Local Git remains the repository format beneath Radicle.
7. For Radicle repository work, use the available `radicle-collaboration:radicle-repo-workflow` Codex skill. If it is unavailable in a future environment, use the verified local `rad` CLI and follow `RADICLE.md`. Do not install or configure a plugin merely to explore it.
8. Do not commit, push, publish, sync, seed, deploy, execute provider traffic, or enable any execution capability without separately explicit authorization.

## Architecture reminders

- The current site is a static React frontend with local interaction state and a Node static-serving compatibility layer; it has no product backend behavior.
- `engine/` owns strategy, risk, accounting, broker translation, lifecycle,
  reconciliation, and the default-off Mynyra Engine contracts. Infrastructure
  may never become an order bus or risk authority.
- `client/src/pages/Home.tsx` owns the present control-room composition and local interaction state.
- `client/src/index.css` owns design tokens and responsive behavior.
- `ideas.md` is the visual design contract.
- `ARCHITECTURE.md` records present boundaries and planned extension seams;
  `engine/docs/MYNYRA_OFFLINE_REPLAY.md` records the first engine proof.
- `RADICLE.md` records the forge, publication, and future `asus-node` backend boundary.

## Reporting contract

Report the exact source and documentation files changed, commands run, verification outcome, warnings, skipped checks, residual risks, Git/Radicle status, and whether external/provider actions were intentionally not performed.
