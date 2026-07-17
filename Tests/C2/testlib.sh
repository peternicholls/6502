#!/usr/bin/env bash

# Shared paths and build helpers for C2 shell contract tests. Test scripts must
# enable `set -euo pipefail` before sourcing this file.

c2_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${c2_dir}/../.." && pwd)"
c2_build_dir="${repo_root}/.build/c2"
c2_fixture_dir="${c2_build_dir}/fixtures"
c2_evidence_dir="${c2_build_dir}/evidence"
c2_measurement_dir="${c2_build_dir}/measurements"
c2_test_binary="${c2_build_dir}/beeb-tests"
c2_tsan_binary="${c2_build_dir}/beeb-tests-tsan"

c2_cxx="${CXX:-c++}"
c2_common_flags=(
    -std=c++20
    -O1
    -g
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -pthread
    "-I${repo_root}/Sources/BeebCore/include"
)
c2_core_sources=("${repo_root}"/Sources/BeebCore/src/*.cpp)
c2_test_source="${repo_root}/Tests/test_main.cpp"

c2_prepare_paths() {
    mkdir -p "${c2_fixture_dir}" "${c2_evidence_dir}" "${c2_measurement_dir}"
}

c2_build_tests() {
    c2_prepare_paths
    "${c2_cxx}" "${c2_common_flags[@]}" "$@" \
        "${c2_core_sources[@]}" "${c2_test_source}" -o "${c2_test_binary}"
}

c2_build_tsan_tests() {
    c2_prepare_paths
    "${c2_cxx}" "${c2_common_flags[@]}" \
        -fsanitize=thread -fno-omit-frame-pointer \
        "${c2_core_sources[@]}" "${c2_test_source}" -o "${c2_tsan_binary}"
}

c2_run_tests() {
    c2_build_tests
    "${c2_test_binary}" "$@"
}

c2_run_tsan_tests() {
    c2_build_tsan_tests
    "${c2_tsan_binary}" "$@"
}

c2_assert_untracked_build_dir() {
    if git -C "${repo_root}" check-ignore -q "${c2_build_dir}/probe"; then
        return 0
    fi
    printf 'C2 build directory is not ignored: %s\n' "${c2_build_dir}" >&2
    return 1
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
    c2_assert_untracked_build_dir
    c2_prepare_paths
fi
