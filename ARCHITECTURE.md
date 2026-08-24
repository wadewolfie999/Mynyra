# Mynyra Trade Control Room Architecture

## Current system

The present system is a **static frontend-only control-room foundation**. It is intentionally local, offline, and non-trading. The implementation has no server endpoints, database, provider client, credential loader, market-data source, account model, risk engine, or execution path.

```text
Browser
  └── Static React interface
        ├── local tab state
        ├── static boundary copy
        ├── static product-transition map
        └── static evidence register

External providers / credentials / accounts / orders / TradeBot source
  └── intentionally absent
```

## Ownership boundary

Mynyra-Trade is intended to become the software-level product repository. The existing TradeBot repository remains separate and authoritative for its current source and governance until an approved migration plan says otherwise. This foundation imports neither source code nor runtime artifacts from TradeBot.

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
- Migrating TradeBot source or governance by implication.
- Publishing or deploying the site.
