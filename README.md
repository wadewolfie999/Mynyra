# Mynyra

Mynyra is an evidence-aware control room and default-off trading engine. The
operator accepted the bounded Demo milestone on 2026-08-30; LIVE support and
real-money authority remain absent.

## Repository layout

| Path | Purpose |
| --- | --- |
| `client/` | Static React control room. It displays repository posture and does not expose an execution control. |
| `server/` | Loopback-only static-serving compatibility entrypoint. |
| `engine/` | History-preserving Mynyra Engine C++ source, including the compile-time gated cTrader Demo adapter. |
| `.github/` | Protected validation, CodeQL, dependency updates, and offline evidence workflows. |
| `docs/archive/` | Historical handoffs, forge records, and operational envelopes. |

The default control-room and engine builds do not initiate provider traffic,
read credentials, access accounts, or place orders. Demo-capable code is
compile-time gated and the repository contains no LIVE endpoint path.

## Frontend

```sh
pnpm install --frozen-lockfile
pnpm check
pnpm test:server
```

Development and production serving bind to loopback. Imported Manus runtime,
storage proxy, debug collector, and managed-asset dependencies were removed at
the Demo cleanup boundary.

## Engine

```sh
cmake -S engine -B engine/build -DBUILD_TESTING=ON
cmake --build engine/build --parallel 2
ctest --test-dir engine/build --output-on-failure
```

See `engine/docs/PROJECT_STATE.md`, `engine/docs/ARCHITECTURE.md`, and
`engine/docs/ACTORS.md` for current state, boundaries, and collaboration roles.

## Collaboration

GitHub is the system of record. Changes land through pull requests and the
protected `validate` check. Bigi is registered as a task-scoped human
operator-contributor; identity mapping and any expanded authority must be
confirmed by Wade before access is interpreted as approval.

## Governing principle

> Observe evidence. Do not infer current market or account state.
