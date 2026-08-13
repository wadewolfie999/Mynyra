#!/usr/bin/env bash

set -euo pipefail

if [[ "$#" -ne 2 ]]; then
    echo "Usage: $0 <build-dir> <new-output-dir>" >&2
    exit 2
fi

build_dir="$1"
output_dir="$2"
binary_path="${build_dir}/tradebot_core"
test_log_path="${build_dir}/Testing/Temporary/LastTest.log"
validation_marker="${build_dir}/tradebot-ci-validation-passed"

if [[ ! -x "${binary_path}" ]]; then
    echo "Missing executable build artifact: ${binary_path}" >&2
    exit 1
fi
if [[ ! -f "${test_log_path}" ]]; then
    echo "Missing full CTest result log: ${test_log_path}" >&2
    exit 1
fi
if [[ ! -f "${validation_marker}" ]]; then
    echo "Missing successful CI validation marker: ${validation_marker}" >&2
    exit 1
fi
if [[ -e "${output_dir}" ]]; then
    echo "Refusing to overwrite existing output: ${output_dir}" >&2
    exit 1
fi
if [[ -n "$(git status --porcelain --untracked-files=no)" ]]; then
    echo "Tracked worktree/index must be clean for exact-commit packaging" >&2
    exit 1
fi

commit_sha="$(git rev-parse HEAD)"
for required_marker in \
    "commit_sha=${commit_sha}" \
    'build_type=RelWithDebInfo' \
    'ctrader_gate6=OFF' \
    'ctrader_gate7=OFF' \
    'ctest=passed_sequentially'; do
    if ! grep -Fqx "${required_marker}" "${validation_marker}"; then
        echo "Validation marker does not match this exact build/ref: ${required_marker}" >&2
        exit 1
    fi
done

hash_file() {
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$1" | awk '{print $1}'
    else
        shasum -a 256 "$1" | awk '{print $1}'
    fi
}

mkdir -p "${output_dir}"
cp LICENSE "${output_dir}/LICENSE"
cp "${test_log_path}" "${output_dir}/CTEST_RESULTS.log"

compiler_path="$(sed -n 's/^CMAKE_CXX_COMPILER:FILEPATH=//p' "${build_dir}/CMakeCache.txt" | head -n 1)"
cmake_version="$(cmake --version | head -n 1)"
compiler_version="$("${compiler_path}" --version | head -n 1)"
binary_sha="$(hash_file "${binary_path}")"

printf '%s\n' \
    'format_version=1' \
    'project=TradeBot' \
    'artifact_class=offline_validation_evidence' \
    "commit_sha=${commit_sha}" \
    'tracked_tree=clean' \
    'build_type=RelWithDebInfo' \
    'ctrader_gate6=OFF' \
    'ctrader_gate7=OFF' \
    'ctest=passed_sequentially' \
    "cmake=${cmake_version}" \
    "compiler=${compiler_version}" \
    "tradebot_core_sha256=${binary_sha}" \
    'packaged_executable=false' \
    'release_authorized=false' \
    'deployment_authorized=false' \
    'live_trading_authorized=false' \
    > "${output_dir}/MANIFEST.txt"

for artifact in CTEST_RESULTS.log LICENSE MANIFEST.txt; do
    printf '%s  %s\n' "$(hash_file "${output_dir}/${artifact}")" "${artifact}"
done > "${output_dir}/SHA256SUMS"

echo "Prepared non-executable offline evidence artifact: ${output_dir}"
