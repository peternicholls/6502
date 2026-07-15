#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/testlib.sh"

c1_tsan_supported() {
    local probe_source="${c1_build_dir}/tsan-probe.cpp"
    local probe_binary="${c1_build_dir}/tsan-probe"
    mkdir -p "${c1_build_dir}"
    printf '%s\n' \
        '#include <thread>' \
        'int main() { int value = 0; std::thread worker([&] { value = 1; }); worker.join(); return value == 1 ? 0 : 1; }' \
        > "${probe_source}"
    if ! "${c1_cxx}" -std=c++20 -fsanitize=thread -pthread \
        "${probe_source}" -o "${probe_binary}" >/dev/null 2>&1; then
        return 1
    fi
    sh -c 'TSAN_OPTIONS="halt_on_error=1:exitcode=66" "$1" >/dev/null 2>&1' \
        c1-tsan-probe "${probe_binary}" >/dev/null 2>&1
}

if [[ "${C1_ONLY_TSAN:-0}" != "1" ]]; then
    c1_run_tests --filter "C1 race:"
fi

if c1_tsan_supported; then
    TSAN_OPTIONS="halt_on_error=1:exitcode=66" \
        c1_run_tsan_tests --filter "C1 race:"
else
    printf 'N/A: ThreadSanitizer is not supported by %s on this host\n' "${c1_cxx}"
fi
