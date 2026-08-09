# TradeBot Security Policy

## Purpose And Authority

- Purpose: govern credential handling, secret storage, dependency review, logging, shell safety, network services, and incident response.
- Authority level: security policy below risk policy where financial risk is involved, above workflow and contributor guidance.
- Audience: operator, maintainers, Codex, contributors, reviewers, testers, and AI agents.

## Credential Storage

- Do not commit credentials.
- Do not write credentials into docs, logs, tests, generated outputs, or chat.
- Prefer environment variables, OS keychain, or a local secret manager.
- `.env` files are ignored and must remain local.
- Any `.env.example` file must contain names and safe instructions only, never values.

Verified credential env names:

- `AIIO_API_KEY`
- `AIIO_API_SECRET`
- `TRADEBOT_CTRADER_CLIENT_ID` (identifier only; no cTrader secret/token env vars)

## `.env` Handling

- Agents must not open or inspect `.env` values unless explicitly authorized.
- Agents must not modify `.env` files unless explicitly authorized.
- If a task requires checking whether an env file exists, report only existence and path sensitivity, not contents.

## Least Privilege

- Exchange or broker keys must not have withdrawal permissions for TradeBot testing.
- Use read-only or sandbox credentials when possible.
- Scope keys by venue, environment, and task.
- Rotate keys after exposure or suspected exposure.

## Log Redaction

- Logs must not contain secret values, raw auth headers, private keys, account IDs, or private balances.
- Use redacted representations such as first/last characters only when necessary.
- Generated logs under `build/` or `data/results/` must be reviewed before sharing.

## API Keys And Exchange Keys

- Documentation may mention variable names but not values.
- Do not paste keys into prompts.
- Do not use live exchange keys for tests.
- Do not infer authorization from credentials being configured.

## cTrader Open API Gate 5

- Fixed callback:
  `http://127.0.0.1:18080/ctrader/oauth/callback`; loopback-only, one-shot,
  exact path/host/method, and secure request-to-callback correlation are
  mandatory. cTrader `state` round-trip support is undocumented; one controlled
  callback matched exactly on 2026-08-10. Every fresh request must still use a
  new value and match exactly before token exchange. The accepted
  `CTraderOAuthCorrelationGuard` controls do not establish a provider guarantee.
- The accepted Gate 5 baseline used `accounts`. Wade explicitly superseded
  that scope for Gate 6 on 2026-08-10 and authorized `trading` solely for the
  demo account proof. The executable's fixed payload allowlist still prohibits
  every trading operation and all Gates 7-9 requests.
- Store the client secret in macOS Keychain service
  `TradeBot.cTraderOpenApi.client-secret`.
- Store the Gate 6 access token, refresh token, scope, token type, and expiry
  atomically in Keychain service `TradeBot.cTraderOpenApi.tokens.trading`.
  Never load or reuse a Playground or differently scoped token.
- Authorization codes are memory-only and must be discarded after one exchange
  attempt or timeout. `ctidTraderAccountId`, `traderLogin`, account numbers,
  visible logins, and equivalent account-identifying values are
  response-derived and volatile process-memory-only: never write, display,
  hash, encode, or otherwise place them in logs, evidence, reports, checkpoint
  artifacts, configuration, reusable labels, or tracked/untracked files.
- Fully redact client IDs/secrets, authorization URLs and callback queries,
  codes, state, tokens, headers, account IDs/logins, balances, and raw payloads.
  Prefix/suffix or hash redaction is not allowed for these fields.
- Never ask an operator to paste a cTrader credential, code, token, or account
  identifier into Codex, ChatGPT, a tracked file, fixture, log, or report.
- The authorized Gate 6A checkpoint may contain only exact `brokerTitleShort`
  presence/value, exact `isLive` presence/value, and a bounded non-identifying
  outcome category. No candidate label, per-account ordinal, visible login, or
  other account-identifying value is permitted. If present `isLive == false`
  plus exact `brokerTitleShort` cannot distinguish exactly one intended demo
  account, stop without producing a persistent selection predicate.
- Gate 6B must retrieve a fresh account list in the same volatile process,
  reproduce the approved `isLive == false` plus exact `brokerTitleShort`
  selection deterministically, keep the
  fresh identifier only in volatile process memory, and use it solely for that
  session's account-authentication request.
- The only Gate 6 runtime message endpoint is
  `demo.ctraderapi.com:5035`. Live account entries are excluded candidates;
  attempting to authenticate one or reaching a live endpoint is a terminal
  security failure.

## Source-Control Exclusions

`.gitignore` excludes:

- `.env`
- `.env.*`
- `*.pem`
- `*.key`
- `*.crt`
- `build/`
- `data/results/`
- `data/archive/`
- `config/local/`

Certificate files are ignored by default. If test certificates need to be versioned in the future, operator approval and clear test-only labeling are required.

## Dependency Review

New dependencies must be reviewed for:

- Source and maintainer trust.
- License.
- Network behavior.
- Native code or post-install scripts.
- Reproducibility.
- Credential access.

See `DEPENDENCY_POLICY.md`.

## Shell-Script Safety

- Tracked validation shell scripts exist under `scripts/`, with `.githooks/pre-push`.
- Do not add or materially change shell scripts without review.
- Avoid destructive commands.
- Avoid printing environment variables that may contain secrets.
- Prefer explicit paths and quoted variables when scripts are introduced.

## Network Services

- Network-capable code must be treated as sensitive.
- Do not start live services, exchange connections, or broker sessions unless explicitly authorized.
- Tests should use local simulation hooks rather than external services.
- Intermittent global connectivity is documented as an operational constraint.

## SSH Considerations

- Do not inspect private SSH keys.
- Do not add remote hosts or modify SSH config without operator approval.
- Do not push to remotes unless explicitly instructed.

## Incident Response

If a secret is exposed:

1. Stop work that might propagate it.
2. Preserve enough evidence to identify scope without copying the secret further.
3. Notify the operator.
4. Revoke or rotate affected credentials.
5. Audit Git status, logs, generated outputs, and remote publication risk.
6. Remove or redact exposed values with operator-approved remediation.
7. Document the incident without secret values.

## Compromised-Key Response

- Revoke the key immediately.
- Rotate dependent keys or tokens.
- Review account permissions and recent activity.
- Confirm no withdrawal permissions were present.
- Re-run relevant tests with safe credentials or no credentials.

## AI-Agent Restrictions

AI agents must:

- Avoid reading secrets.
- Avoid broad filesystem searches that expose sensitive files unless necessary.
- Report credential-related uncertainty.
- Refuse to enable live behavior without explicit authorization.
- Not commit, push, or publish security-sensitive material without approval.

## Security Review Requirements

Security review is required for changes to:

- `AuthManager`.
- `AsyncNetworkClient`.
- `LiveDataAdapter`.
- `BrokerGateway`.
- TLS/cert handling.
- `.gitignore` secret exclusions.
- Dependency manifests or build scripts.
- Logging or analytics that may include sensitive data.
