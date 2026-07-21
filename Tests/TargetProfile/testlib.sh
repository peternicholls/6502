#!/usr/bin/env bash

set -euo pipefail

target_profile_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${target_profile_dir}/../.." && pwd)"
target_profile_build_dir="${repo_root}/.build/target-profile"

target_profile_cxx="${CXX:-c++}"
target_profile_cc="${CC:-cc}"
target_profile_common_cxx_flags=(
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
target_profile_common_c_flags=(
    -std=c11
    -O1
    -g
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    "-I${repo_root}/Sources/BeebCore/include"
)
target_profile_core_sources=("${repo_root}"/Sources/BeebCore/src/*.cpp)

target_profile_prepare_build_dir() {
    mkdir -p "${target_profile_build_dir}"
}

target_profile_assert_untracked_build_dir() {
    if git -C "${repo_root}" check-ignore -q "${target_profile_build_dir}/probe"; then
        return 0
    fi
    printf 'Target-profile build directory is not ignored: %s\n' \
        "${target_profile_build_dir}" >&2
    return 1
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
    target_profile_assert_untracked_build_dir
fi
