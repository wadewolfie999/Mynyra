#!/usr/bin/env bash

set -euo pipefail

failures=0
policy_tmp_dir=$(mktemp -d "${TMPDIR:-/tmp}/tradebot-ci-policy.XXXXXX")
trap 'rm -rf -- "$policy_tmp_dir"' EXIT

report_failure() {
    printf 'policy failure: %s\n' "$1" >&2
    failures=$((failures + 1))
}

while IFS= read -r -d '' path; do
    basename=${path##*/}

    case "$basename" in
        .env.example)
            ;;
        .env|.env.*|*.pem|*.key|*.p12|*.pfx|*.crt)
            report_failure "tracked sensitive-looking path: $path"
            ;;
    esac

    case "$path" in
        handoff/*|handoffs/*|*/handoff/*|*/handoffs/*)
            report_failure "tracked operator evidence path: $path"
            ;;
    esac
done < <(git ls-files --cached --others --exclude-standard -z)

if git grep -nEI -- \
    '-----BEGIN ([A-Z0-9]+ )?PRIVATE KEY-----' \
    > "$policy_tmp_dir/private-key-findings.txt"; then
    report_failure "private-key material appears in tracked content"
fi

for option_name in \
    TRADEBOT_ENABLE_LIVE_RUNTIME \
    TRADEBOT_ENABLE_CTRADER_GATE6 \
    TRADEBOT_ENABLE_CTRADER_GATE7
do
    option_block=$(
        sed -n "/option(${option_name}/,/)/p" CMakeLists.txt
    )
    if ! grep -Eq 'OFF[[:space:]]*\)' <<<"$option_block"; then
        report_failure "${option_name} is not default-disabled"
    fi
done

if ! grep -Eq \
    'SystemMode mode\{SystemMode::BACKTEST\};' \
    include/SystemConfig.hpp; then
    report_failure "SystemConfig no longer defaults to BACKTEST"
fi

if grep -Eq \
    'AuthManager|m_auth|/api/v3/|X-MBX-APIKEY' \
    include/LiveDataAdapter.hpp src/LiveDataAdapter.cpp; then
    report_failure "legacy LiveDataAdapter crossed the credential/provider boundary"
fi

if grep -RIlE \
    'pull_request_target|secrets\.|TRADEBOT_ENABLE_CTRADER_GATE[67]=ON|ctrader_gate[67]_proof' \
    .github/workflows > "$policy_tmp_dir/workflow-findings.txt"; then
    sed -n '1,20p' "$policy_tmp_dir/workflow-findings.txt" >&2
    report_failure "workflow crosses the offline CI boundary"
fi

if (( failures > 0 )); then
    printf 'TradeBot CI policy checks failed: %d finding(s).\n' "$failures" >&2
    exit 1
fi

printf 'TradeBot CI policy checks passed.\n'
