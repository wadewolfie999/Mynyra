# Mynyra-Trade Repository Instructions

## Product boundary

Mynyra-Trade is the future product-level home for the broader software system around TradeBot. It is not yet a migration of the existing TradeBot repository. Treat TradeBot as a separate source repository until a dedicated, reviewed transfer plan explicitly names the files, architecture, evidence, and authorization boundary.

The current source tree is a Manus AI export for the Mynyra control-room website. Treat it as imported product source, not as a live Manus project or a deployment configuration. Retained template code and development helpers require review before they are enabled, preserved in a production path, or used to make external requests.

## Current authorized scope

The present implementation is an **offline, non-trading control-room website foundation**. It may contain static local presentation content and product documentation only. It must not connect to an external provider, read or represent credentials, copy provider artifacts, access accounts, retrieve market data, create orders, change risk limits, or imply that any such capability exists.

The repository includes a Node static-serving compatibility entrypoint, but no product backend is enabled or deployed. If a product backend is later authorized, its target node is `asus-node`; that target is not deployment authority, a current service claim, or permission to alter the node.

## Working rules

1. Inspect Git state, this file, `README.md`, `ARCHITECTURE.md`, `RADICLE.md`, and `HANDOFF.md` before mutation.
2. Keep UI claims evidence-aware. Distinguish static local copy, observed evidence, inference, and unavailable state.
3. Preserve the explicit offline and non-trading posture. Unavailable controls must say so rather than silently simulate success.
4. Avoid importing files from TradeBot without an exact approved migration scope. Never copy credential-like files, provider traces, account identifiers, generated runtime artifacts, or ignored outputs.
5. Add focused verification for behavior changes. Record commands and results in the handoff.
6. Use Radicle as the only software forge. Do not add GitHub remotes, GitHub Actions, GitHub pull-request workflows, `gh` commands, GitHub links as a collaboration surface, or a second centralized forge without separately explicit authorization. Local Git remains the repository format beneath Radicle.
7. For Radicle repository work, use the available `radicle-collaboration:radicle-repo-workflow` Codex skill. If it is unavailable in a future environment, use the verified local `rad` CLI and follow `RADICLE.md`. Do not install or configure a plugin merely to explore it.
8. Do not commit, push, publish, sync, seed, deploy, execute provider traffic, or enable any execution capability without separately explicit authorization.

## Architecture reminders

- The current site is a static React frontend with local interaction state and a Node static-serving compatibility layer; it has no product backend behavior.
- `client/src/pages/Home.tsx` owns the present control-room composition and local interaction state.
- `client/src/index.css` owns design tokens and responsive behavior.
- `ideas.md` is the visual design contract.
- `ARCHITECTURE.md` records present boundaries and planned extension seams.
- `RADICLE.md` records the forge, publication, and future `asus-node` backend boundary.

## Reporting contract

Report the exact source and documentation files changed, commands run, verification outcome, warnings, skipped checks, residual risks, Git/Radicle status, and whether external/provider actions were intentionally not performed.
