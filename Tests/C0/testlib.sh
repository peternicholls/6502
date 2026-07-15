#!/usr/bin/env bash

# Shared assertions for C0 shell contract tests. Test scripts must enable
# `set -euo pipefail` before sourcing this file.

C0_TEST_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
C0_TEST_TMP=""

c0_test_setup() {
    if [[ -n "${C0_TEST_TMP}" ]]; then
        printf 'C0 test temporary directory already exists: %s\n' "${C0_TEST_TMP}" >&2
        return 1
    fi

    C0_TEST_TMP="$(mktemp -d "${TMPDIR:-/tmp}/beeb-c0-test.XXXXXX")"
    trap c0_test_cleanup EXIT
    trap 'exit 130' INT
    trap 'exit 143' TERM
}

c0_test_cleanup() {
    if [[ -z "${C0_TEST_TMP}" ]]; then
        return
    fi

    case "${C0_TEST_TMP}" in
        "${TMPDIR:-/tmp}"/beeb-c0-test.*)
            rm -rf -- "${C0_TEST_TMP}"
            C0_TEST_TMP=""
            ;;
        *)
            printf 'refusing to remove unsafe C0 test path: %s\n' "${C0_TEST_TMP}" >&2
            return 1
            ;;
    esac
}

c0_capture() {
    local name="$1"
    shift
    local stdout_path="${C0_TEST_TMP}/${name}.stdout"
    local stderr_path="${C0_TEST_TMP}/${name}.stderr"
    local status_path="${C0_TEST_TMP}/${name}.status"
    local status

    set +e
    "$@" >"${stdout_path}" 2>"${stderr_path}"
    status=$?
    set -e
    printf '%s\n' "${status}" >"${status_path}"
}

c0_status() {
    local name="$1"
    tr -d '[:space:]' <"${C0_TEST_TMP}/${name}.status"
}

c0_expect_status() {
    local expected="$1"
    local name="$2"
    local actual
    actual="$(c0_status "${name}")"
    if [[ "${actual}" != "${expected}" ]]; then
        printf '%s: expected status %s, got %s\n' "${name}" "${expected}" "${actual}" >&2
        return 1
    fi
}

c0_expect_failure() {
    local name="$1"
    local actual
    actual="$(c0_status "${name}")"
    if [[ "${actual}" == "0" ]]; then
        printf '%s: expected failure, got status 0\n' "${name}" >&2
        return 1
    fi
}

c0_assert_contains() {
    local path="$1"
    local expected="$2"
    if ! grep -Fq -- "${expected}" "${path}"; then
        printf '%s: expected text not found: %s\n' "${path}" "${expected}" >&2
        return 1
    fi
}

c0_snapshot_tree() {
    local directory="$1"
    local destination="$2"
    if [[ ! -d "${directory}" ]]; then
        printf 'missing tree for snapshot: %s\n' "${directory}" >&2
        return 1
    fi

    (
        cd "${directory}"
        find . -type f -print0 | LC_ALL=C sort -z | xargs -0 shasum -a 256
    ) >"${destination}"
}

c0_assert_tree_unchanged() {
    local directory="$1"
    local expected_snapshot="$2"
    local actual_snapshot="${C0_TEST_TMP}/tree-actual.sha256"
    c0_snapshot_tree "${directory}" "${actual_snapshot}"
    if ! cmp -s "${expected_snapshot}" "${actual_snapshot}"; then
        printf 'tree changed unexpectedly: %s\n' "${directory}" >&2
        diff -u "${expected_snapshot}" "${actual_snapshot}" >&2 || true
        return 1
    fi
}

c0_pass() {
    printf 'PASS  %s\n' "$1"
}
