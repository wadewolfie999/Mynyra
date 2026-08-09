# TradeBot Configuration

## Purpose And Authority

- Purpose: document verified runtime configuration, modes, flags, env vars, and output paths.
- Authority level: configuration reference below architecture and risk policy.
- Audience: operator, Codex, maintainers, testers, and contributors.

## Verified Runtime Modes

`SystemConfig` defines:

- `BACKTEST`: deterministic CSV replay; default.
- `PAPER`: live-data-like path with simulated broker execution.
- `LIVE`: live-capable data and broker execution path.

`parseModeFlag` accepts `backtest`, `paper`, and `live` case variants shown in code and returns `BACKTEST` for unrecognized strings.

## Verified CLI Flags

`src/main.cpp` handles:

```sh
build/tradebot_core --mode backtest <csv-files>
build/tradebot_core --mode paper <csv-files>
build/tradebot_core --mode live <csv-files>
build/tradebot_core --resume <snapshot-file> <csv-files>
```

If no file arguments are supplied, the program falls back to `data/BTCUSDT-15.csv`. That file was not present in the verified tracked tree.

## Credential Configuration

`SystemConfig` defaults:

- API key env name: `AIIO_API_KEY`
- API secret env name: `AIIO_API_SECRET`

Credentials may also be held in `SystemConfig` fields. Do not hardcode or document values.

### cTrader Open API Gate 5 Names

- Client ID name: `TRADEBOT_CTRADER_CLIENT_ID` (identifier, not a secret).
- Client secret: macOS Keychain service
  `TradeBot.cTraderOpenApi.client-secret`.
- Gate 6 `trading` token envelope: macOS Keychain service
  `TradeBot.cTraderOpenApi.tokens.trading`.

There are no client-secret, authorization-code, access-token, refresh-token,
account-ID, endpoint, port, or scope environment variables. `.env.example`
contains one clearly invalid client-ID placeholder only.

Gate 5/6 fixed values, which the opt-in proof rejects attempts to override:

- Redirect URI: `http://127.0.0.1:18080/ctrader/oauth/callback`.
- OAuth scope: `trading`, explicitly authorized by Wade for Gate 6 on
  2026-08-10. The fixed outbound message allowlist still excludes every
  trading, order, position, symbol, and market-data request.
- Open API host: `demo.ctraderapi.com`.
- Open API port/transport: `5035`, Protobuf over strict TLS/TCP.

A live hostname and runtime endpoint or scope selection are not valid Gate 6
configuration. The `trading` scope is fixed rather than configurable.

### Gate 6 Opt-In Proof Target

The Gate 6 proof is excluded from normal builds unless explicitly enabled:

```sh
cmake -S . -B build/gate6 -DTRADEBOT_ENABLE_CTRADER_GATE6=ON
cmake --build build/gate6 --target ctrader_gate6_proof
```

`ctrader_gate6_proof --preflight-only` checks only that the client-ID name and
Keychain client-secret item are available, emits fixed categories, and exits
before opening a browser or contacting a provider. It accepts no endpoint,
scope, account, credential, token, or identifier argument. The normal proof
target is authorized only under the active Gate 6 directive and must not be run
until the credential and redirect prerequisites are remediated.

## Network Defaults

`SystemConfig` contains default endpoint strings:

- WSS endpoint: `wss://stream.example.com/ws`
- REST endpoint: `https://api.example.com`

These defaults are not live authorization.

## Risk Configuration

Verified defaults include:

- `latencyMaxMs`: `500`
- `errorRateThresh`: `5`
- `atrScaleUpThreshold`: `1.5`
- `varScaleLowVolFactor`: `1.0`
- `varScaleHighVolFactor`: `0.5`

Financial limit changes require operator approval.

## Output Paths

- Analytics default: `data/results`.
- Snapshot default: `data/results/snapshot.json`.
- Throughput report: `data/results/latency_report.csv`.
- Phase 18 burn-in report: `data/results/phase18_burnin_latency.csv`.
- Phase 18 default replay path: `data/historical/BTCUSDT-L2-1M.bin`.

Generated outputs are ignored by Git unless intentionally versioned.

## Configuration Change Rules

Configuration changes require documentation updates when they affect:

- Runtime modes.
- CLI flags.
- Env var names.
- Output paths.
- Risk thresholds.
- Credential behavior.
- Live-capable network behavior.
