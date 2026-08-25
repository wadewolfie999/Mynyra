# Mynyra Trade Control Room Architecture

## Current system

The present system is a **static control-room plus offline engine foundation**.
The UI is intentionally local, offline, and non-trading. A small Node
entrypoint serves built static files and client-side routes, but exposes no
product API. `engine/` is an imported C++ source lineage with no enabled
provider runtime; its first product-facing interface is a hash-pinned local
`BACKTEST` replay.

```text
Browser
  └── Static React interface
        ├── local tab state
        ├── static boundary copy
        ├── static product-transition map
        └── static evidence register

Mynyra Engine (`engine/`)
  └── hash-pinned `BACKTEST` replay only
        ├── providerAllowed=false
        ├── ordersAllowed=false
        └── no UI/API integration

External providers / credentials / accounts / orders
  └── intentionally absent
```

## Ownership boundary

Mynyra-Trade is the software-level product repository. It contains the frozen
TradeBot M1 source lineage under `engine/`, imported without ignored outputs,
provider evidence, credentials, or runtime artifacts. The separate TradeBot
checkout is retained read-only during the recovery window. The imported engine
owns strategy, risk, accounting, broker translation, lifecycle, and
reconciliation; no infrastructure component may become an order channel or
risk authority.

The current website source entered this repository as a Manus AI-generated export. It is a local source snapshot, not a live Manus project or a production infrastructure baseline. Retained Manus template helpers must be audited before use in a production path.

Radicle is the repository's sole software forge. The local Git repository is used beneath Radicle for version storage and working-tree operations; no GitHub or other centralized forge is part of the intended collaboration topology. See `RADICLE.md` for the operational contract.

## Future backend placement

The future primary runtime node is `asus-node`. No Mynyra backend is yet
implemented, configured, or deployed there. The first ASUS proof must be the
immutable offline replay described in `engine/docs/MYNYRA_OFFLINE_REPLAY.md`,
not cTrader.

The enabling plan must define the service boundary, Radicle source revision, configuration/secrets ownership, private network exposure, persistence, observability, health checks, deployment process, rollback, and the authorization for each live operation. It must also preserve the current ban on provider traffic, credentials, accounts, orders, and risk-limit changes until those capabilities receive their own explicit authority.

## UI structure

| Area | File | Responsibility |
| --- | --- | --- |
| Route composition | `client/src/App.tsx` | Dark application shell and top-level routing. |
| Control-room page | `client/src/pages/Home.tsx` | Static content model, local reading-frame tabs, boundary-aware placeholder actions. |
| Design system | `client/src/index.css` | Night Operations Manual tokens, layout, responsive design, motion preferences. |
| Visual contract | `ideas.md` | Aesthetic direction and copy/interaction principles. |

## Extension seams

Future work should introduce data only through a reviewed, read-only evidence model. Before adding any integration, define:

1. the evidence source and provenance;
2. the distinction between observed, inferred, and unavailable values;
3. the exact authority for connection, retention, and display;
4. the failure and no-data state;
5. the verification command and rollback path.

No future seam implies authorization to add provider traffic, credentials, accounts, order capability, or risk-limit controls.

## Non-goals

- Providing financial advice, performance analysis, or profitability claims.
- Simulating a live provider, account, quote, or order workflow.
- Adding a UI engine-evidence API or operational controls during this cutover.
- Publishing or deploying the site.
- Adding GitHub, a centralized forge, or forge-specific automation outside Radicle.
- Treating `asus-node` as a deployed backend without current service evidence and explicit authorization.
