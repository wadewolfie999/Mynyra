# Codex Handoff — Mynyra Trade Control Room Foundation

## Scope completed

An offline, non-trading static control-room website foundation was created for the empty future Mynyra-Trade product repository. The implementation emphasizes visible boundaries, product ownership transition, evidence classification, and a clear handoff surface for future work.

On 2026-08-24, Codex/Radicle repository integration was added as documentation and operating policy only. It records Radicle as the sole forge, identifies the Manus AI provenance of the website source, reserves `asus-node` as the target for a later authorized backend, and preserves the separate TradeBot migration boundary. No application behavior, node, external service, or forge state was changed.

## Source of authority

The operator initially authorized the offline, non-trading Mynyra-Trade control-room foundation without copying TradeBot credentials/provider artifacts, provider traffic, commit, push, deployment, or publication. On 2026-08-24, the operator also authorized Codex/Radicle integration under these rules: use Radicle as the sole forge, treat the site as Manus-originated source, reserve `asus-node` for any later authorized backend, and keep TradeBot ingestion behind a separate reviewed migration plan.

## Files introduced or materially changed

| File | Reason |
| --- | --- |
| `client/src/pages/Home.tsx` | Control-room UI, static tab state, and boundary-aware placeholder interactions. |
| `client/src/App.tsx` | Dark static application shell and routing. |
| `client/src/index.css` | Product design tokens, responsive layout, and reduced-motion-safe presentation. |
| `client/index.html` | Product metadata and generated brand favicon reference. |
| `ideas.md` | Written design exploration and selected design contract. |
| `AGENTS.md` | Future agent operating rules and strict product boundary. |
| `README.md` | Repository orientation, local run/verify commands, and explicit exclusions. |
| `ARCHITECTURE.md` | Static-only architecture and governed extension seams. |
| `RADICLE.md` | Codex/Radicle forge rules, Manus source provenance, and future backend boundary. |
| `HANDOFF.md` | This continuation record. |

## Current implementation facts

1. The site uses local static presentation text and local React state only.
2. Navigation changes the reading frame on the same page; it does not load external data.
3. Buttons display an explicit local-foundation notice; they do not invoke any provider, account, execution, deployment, or external action.
4. The product shell states that TradeBot remains separate. No source migration has been performed.
5. Generated visual assets are referenced through managed static URLs. They are design-only and contain no provider or account information. The UI hides an unavailable managed image and falls back to its CSS field treatment when it is run outside the managed preview.
6. The project is a verified local Radicle project with a Radicle-only upstream. Its current Radicle visibility is public, but that does not prove seeding, peer availability, publication of this documentation change, or any deployed system state.
7. The current public Radicle project description claims engine, real-time analysis, risk control, and execution capabilities that conflict with this repository's offline, non-trading contract. It was observed but intentionally not changed; a separate metadata-alignment authorization is required.

## Verification commands for future source changes

```sh
pnpm check
pnpm build
```

Also run:

```sh
git status --short --branch
git diff --check
```

Use a browser preview to verify desktop and mobile layout, tab changes, and the non-operational action notices.

## Verification executed

| Check | Outcome |
| --- | --- |
| `pnpm check` in the prepared static project | Passed (`tsc --noEmit`). |
| `pnpm build` in the prepared static project | Passed. Vite reported a non-failing chunk-size warning because the initial JavaScript bundle exceeds 500 kB after minification. |
| Desktop preview at 1440 × 1100 | Reviewed. The visual system matches the chosen Night Operations Manual contract. |
| Mobile preview at 390 × 844 | Reviewed. The rail collapses to a compact reading-frame strip and the page remains legible. |
| Key-file parity between prepared project and `~/Mynyra-Trade` | Passed for the UI, styles, package manifest/lock, and handoff documents. |
| `git diff --check` in `~/Mynyra-Trade` | Passed. |
| Potential sensitive-path check in `~/Mynyra-Trade` | No matching paths found. No credential-like files were opened or copied. |
| Current local `pnpm check` for Codex/Radicle integration (2026-08-24) | Passed (`tsc --noEmit`). |
| Current local `pnpm build` for Codex/Radicle integration (2026-08-24) | Passed. Node emitted a `module.register()` deprecation warning; Vite reported unresolved optional analytics placeholders and a non-module analytics script reference. |
| Current Codex/Radicle documentation diff (2026-08-24) | `git diff --check` passed. |
| Current Radicle state snapshot (2026-08-24) | Local project discovered with Radicle-only remote; node running outbound-only and not configured for inbound connections. No Radicle mutation was performed. |

The current local worktree has dependencies available and passed `pnpm check` and `pnpm build` above. The build warnings remain a source-hardening gap; no analytics configuration, Manus helper configuration, deployment, or external request was enabled to address them.

## Remaining work

- Decide the future product data and evidence model before attaching any data source.
- Create an explicit migration plan before importing any non-sensitive TradeBot source or documentation.
- Add automated component tests after a stable data model and interaction contract exist.
- If a product backend is authorized, design it explicitly for `asus-node`; do not deploy or configure it by implication.
- Run a separate source-hardening review before retaining or enabling imported Manus development helpers in a Mynyra runtime or deployment path.
- Use Radicle patches/issues for any authorized collaboration record. Do not introduce GitHub or another centralized forge.
- Review and, if authorized, correct the public Radicle project description so it does not overstate the current offline control-room foundation.

## Prohibited adjacent actions

The completed scope does **not** authorize copying TradeBot source, opening or copying credential-like files, provider traffic, account access, market-data access, orders, risk-limit changes, paper or live trading, Git commit/push, Radicle publish/sync/seed, deployment, publication, GitHub usage, or any retry of an external action.

## Rollback

The website is a self-contained static foundation. Revert or remove the introduced files only with operator approval. No external state exists to clean up.
