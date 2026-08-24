# Codex Handoff — Mynyra Trade Control Room Foundation

## Scope completed

An offline, non-trading static control-room website foundation was created for the empty future Mynyra-Trade product repository. The implementation emphasizes visible boundaries, product ownership transition, evidence classification, and a clear handoff surface for future work.

## Source of authority

The operator authorized exactly this action: create the offline, non-trading Mynyra-Trade control-room website foundation without copying TradeBot credentials/provider artifacts, without provider traffic, and without commit, push, deployment, or publication.

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
| `HANDOFF.md` | This continuation record. |

## Current implementation facts

1. The site uses local static presentation text and local React state only.
2. Navigation changes the reading frame on the same page; it does not load external data.
3. Buttons display an explicit local-foundation notice; they do not invoke any provider, account, execution, deployment, or external action.
4. The product shell states that TradeBot remains separate. No source migration has been performed.
5. Generated visual assets are referenced through managed static URLs. They are design-only and contain no provider or account information. The UI hides an unavailable managed image and falls back to its CSS field treatment when it is run outside the managed preview.

## Verification expected for this change

```sh
pnpm check
pnpm build
```

After copying the scaffold into the local `~/Mynyra-Trade` repository, also run:

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

The local `~/Mynyra-Trade` repository does not yet have dependencies installed, so its own `pnpm check` and `pnpm build` remain a proof gap. The prepared project is the verified equivalent source tree; run those commands after local dependency installation before a commit or any subsequent scope expansion.

## Remaining work

- Decide the future product data and evidence model before attaching any data source.
- Create an explicit migration plan before importing any non-sensitive TradeBot source or documentation.
- Add automated component tests after a stable data model and interaction contract exist.
- Decide whether the product needs a backend. Do not add one by default.

## Prohibited adjacent actions

The completed scope does **not** authorize copying TradeBot source, opening or copying credential-like files, provider traffic, account access, market-data access, orders, risk-limit changes, paper or live trading, Git commit/push, deployment, publication, or any retry of an external action.

## Rollback

The website is a self-contained static foundation. Revert or remove the introduced files only with operator approval. No external state exists to clean up.
