#!/usr/bin/env bash

set -euo pipefail

build_dir="${1:-build/ci-validation}"
build_jobs="${TRADEBOT_BUILD_JOBS:-2}"

if [[ -z "${build_dir}" || "${build_dir}" == "/" || "${build_dir}" == "." ]]; then
    echo "Refusing unsafe build directory: ${build_dir}" >&2
    exit 2
fi

python3 scripts/validate_automation.py

cmake -S . -B "${build_dir}" \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DBUILD_TESTING=ON \
    -DTRADEBOT_ENABLE_CTRADER_GATE6=OFF \
    -DTRADEBOT_ENABLE_CTRADER_GATE7=OFF
cmake --build "${build_dir}" --parallel "${build_jobs}"
ctest --test-dir "${build_dir}" --output-on-failure --parallel 1

commit_sha="$(git rev-parse HEAD)"
printf '%s\n' \
    'format_version=1' \
    "commit_sha=${commit_sha}" \
    'build_type=RelWithDebInfo' \
    'ctrader_gate6=OFF' \
    'ctrader_gate7=OFF' \
    'ctest=passed_sequentially' \
    > "${build_dir}/tradebot-ci-validation-passed"
