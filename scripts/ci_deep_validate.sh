#!/usr/bin/env bash

set -euo pipefail

build_dir="${1:-build/ci-deep-validation}"
build_jobs="${TRADEBOT_BUILD_JOBS:-2}"
sanitizer_flags="-fsanitize=address,undefined -fno-omit-frame-pointer"
asan_options="detect_leaks=1:halt_on_error=1"

if [[ "$(uname -s)" == "Darwin" ]]; then
    asan_options="detect_leaks=0:halt_on_error=1"
fi

if [[ -z "${build_dir}" || "${build_dir}" == "/" || "${build_dir}" == "." ]]; then
    echo "Refusing unsafe build directory: ${build_dir}" >&2
    exit 2
fi

python3 scripts/validate_automation.py

cmake -S . -B "${build_dir}" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_TESTING=ON \
    -DTRADEBOT_ENABLE_CTRADER_GATE6=OFF \
    -DTRADEBOT_ENABLE_CTRADER_GATE7=OFF \
    -DCMAKE_CXX_FLAGS="${sanitizer_flags}" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build "${build_dir}" --parallel "${build_jobs}"
ASAN_OPTIONS="${asan_options}" \
UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1" \
ctest --test-dir "${build_dir}" --output-on-failure --parallel 1
