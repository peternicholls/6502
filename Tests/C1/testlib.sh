#!/usr/bin/env bash

set -euo pipefail

c1_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${c1_dir}/../.." && pwd)"
c1_build_dir="${repo_root}/.build/c1"
c1_test_binary="${c1_build_dir}/beeb-tests"
c1_tsan_binary="${c1_build_dir}/beeb-tests-tsan"

c1_cxx="${CXX:-c++}"
c1_common_flags=(
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
c1_core_sources=("${repo_root}"/Sources/BeebCore/src/*.cpp)
c1_test_source="${repo_root}/Tests/test_main.cpp"

c1_build_tests() {
    mkdir -p "${c1_build_dir}"
    "${c1_cxx}" "${c1_common_flags[@]}" "$@" \
        "${c1_core_sources[@]}" "${c1_test_source}" -o "${c1_test_binary}"
}

c1_build_tsan_tests() {
    mkdir -p "${c1_build_dir}"
    "${c1_cxx}" "${c1_common_flags[@]}" \
        -fsanitize=thread -fno-omit-frame-pointer \
        "${c1_core_sources[@]}" "${c1_test_source}" -o "${c1_tsan_binary}"
}

c1_run_tests() {
    c1_build_tests
    "${c1_test_binary}" "$@"
}

c1_run_tsan_tests() {
    c1_build_tsan_tests
    "${c1_tsan_binary}" "$@"
}

c1_assert_untracked_build_dir() {
    if git -C "${repo_root}" check-ignore -q "${c1_build_dir}/probe"; then
        return 0
    fi
    printf 'C1 build directory is not ignored: %s\n' "${c1_build_dir}" >&2
    return 1
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
    c1_assert_untracked_build_dir
    c1_run_tests --quick
fi
