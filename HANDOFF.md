# Codex Handoff — Mynyra Trade Control Room Foundation

## 2026-08-26 provider-free release surface (active)

- `MYNYRA_OFFLINE_ONLY=ON` is a structural CMake boundary. The
  `mynyra-offline-release` and `mynyra-offline-asan` presets expose only
  `tradebot_core_lib`, `mynyra_offline_replay`, and offline tests; they reject
  all live/cTrader enablement and do not declare cTrader OAuth/provider targets
  or `tradebot_core`.
- `mynyra_offline_replay` is the release-only CLI. It self-hashes its resolved
  executable, accepts only strict hash-pinned BACKTEST manifest/input/config
  paths, enforces input and configuration limits, and atomically persists a
  redacted deterministic result. It has no provider, broker, credential,
  shell, process-launch, SSH, or network option.
- Offline preset verification passed: release `2/2`; ASan/UBSan `2/2`. The
  release binary SHA-256 was
  `cab4c2dfa0cdbe90ca2de9a2e312ab79ff91dcb0934a79a3620e87bd7e5cdf02`.
  Its only observed dynamic dependencies were macOS system C++/System libraries;
  no cTrader/OAuth strings were present.
- Full default validation passed `20/20`; full ASan/UBSan validation passed
  `20/20`. `pnpm check` and the new loopback server test passed. Existing
  warnings remain limited to unused `SSL_ERROR_NONE`, unused `totalPositioned`,
  and the test-only `private` macro.
- Source revision: `2774c121025d9e8ccbc82e8d6aaec1be3a477dff`; source-tree
  SHA-256: `6c42adc35521bdba6a34d8fad961f55377865e3f`. The offline CLI test
  covers repeatability, manifest/input tampering, malformed fields, resource
  exhaustion, and refusal to overwrite an output location. Cancellation,
  timeout, and incomplete-event-evidence are exercised through the engine
  contract test because the release CLI intentionally exposes no test or
  event-injection interface.
- `server/index.ts` now binds only to `127.0.0.1`. The repository includes an
  ASUS APPLY envelope at `ops/asus-cutover-apply.md`; it does not itself alter
  ASUS, Node Control, Radicle, or any provider state.
- No identity update, publication, sync, seed, ASUS filesystem change, service
  installation, provider traffic, credential access, market-data request, or
  order attempt has occurred in this source step.

## 2026-08-25 Mynyra Engine consolidation (active)

- Branch: `codex/mynyra-engine-cutover`.
- The history-preserving import commit is `6d07a04b30a7e260d8d40ee65750a0b505d3748c`; its parents are the frozen Mynyra baseline `268a2548379b54742c7315fac0bec23f108e0f71` and frozen TradeBot M1 candidate `4d715d53953836d99bb1d69cafa23ca3252fd4d7`.
- `engine/` exactly matched the frozen TradeBot tree before cutover edits. The current migration removes `engine/.github/`, retains local CMake/CTest guardrails, and documents Radicle patch review. Historical forge records remain reachable through the imported parent history.
- The new engine-owned `RunManifestV1`, `EvidenceEnvelopeV1`, `CapabilityReportV1`, and `OfflineRunResultV1` implement only hash-pinned local `BACKTEST` replay. The runner accepts neither provider nor order permission, has no SSH/shell/container/network authority, returns redacted memory evidence, and fails closed for cancellation, timeout, malformed input, resource exhaustion, and incomplete evidence.
- Current candidate verification passed: `./scripts/ci_validate.sh ../build/mynyra-engine-validation` (19/19) and `./scripts/ci_deep_validate.sh ../build/mynyra-engine-deep-validation` (19/19, ASan/UBSan). Existing warnings are unchanged: unused `SSL_ERROR_NONE`, unused `totalPositioned`, and the test-only `private` macro.
- Exact-commit verification also passed from clean build trees: `./scripts/ci_validate.sh ../build/mynyra-engine-exact` (19/19) and `./scripts/ci_deep_validate.sh ../build/mynyra-engine-exact-deep` (19/19, ASan/UBSan). The exact source tree is `600c3a7bda2de51948705f572d2a958f03809a57`; default-off `tradebot_core` SHA-256 is `f9e7a0317801042532db0455046fc686afbdfe9c7773a81e052c4743ddd38634` and `mynyra_offline_run_tests` SHA-256 is `8e3b96f866514434795c630f6824f867301b6ffeb67d49d2b92fa3bfc327ad54`.
- Recovery bundle: `/Users/vaheedgorgeen/Archives/Mynyra-Trade/mynyra-engine-cutover-a6a9d42.bundle`, SHA-256 `bbc1f1719e92303e3b0323c9ffbaa129a30dcf9c123e65b5c6c29c43d2d13b28`; `git bundle verify` reports complete history and the cutover branch ref.
- No provider process, OAuth flow, Keychain access, account access, market-data request, order attempt, commissioning flag, deployment, publication, or Radicle network action occurred during this source work. Stage 2 remains operator-accepted historical evidence; Stage 3 is unstarted and excluded.
- ASUS is not yet changed. The prior `asus-remote` route must be repaired under a separate Node Control envelope before the immutable offline release and loopback-only control-room service can be installed.

## Next exact action

Commit the reviewed local source transition, create its recovery bundle and
exact-commit verification record, then diagnose the existing `asus-remote`
reverse-tunnel owner without modifying protected workloads.

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
