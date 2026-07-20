#!/usr/bin/env bash
set -euo pipefail

root="$(cd -P "$(dirname "$0")/.." && pwd -P)"
home_root="$(cd -P "${HOME}" && pwd -P)"
profile=""
groups_file=""
default_output_dir="${root}/.build/c0/run"
output_dir="${default_output_dir}"

usage() {
    printf 'usage: verify-c0.sh [--profile portable|macos|all] [--groups FILE] [--output-dir DIR]\n'
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --profile)
            [[ $# -ge 2 ]] || { printf 'missing value for --profile\n' >&2; exit 2; }
            profile="$2"
            shift 2
            ;;
        --groups)
            [[ $# -ge 2 ]] || { printf 'missing value for --groups\n' >&2; exit 2; }
            groups_file="$2"
            shift 2
            ;;
        --output-dir)
            [[ $# -ge 2 ]] || { printf 'missing value for --output-dir\n' >&2; exit 2; }
            output_dir="$2"
            shift 2
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        *)
            printf 'unknown option: %s\n' "$1" >&2
            usage >&2
            exit 2
            ;;
    esac
done

if [[ -z "${profile}" ]]; then
    if [[ "$(uname -s)" == "Darwin" ]]; then profile="macos"; else profile="portable"; fi
fi
case "${profile}" in
    portable|macos|all) ;;
    *) printf 'unsupported profile: %s\n' "${profile}" >&2; exit 2 ;;
esac
case "${output_dir}" in
    ""|/) printf 'unsafe output directory: %s\n' "${output_dir}" >&2; exit 2 ;;
esac
if [[ "${output_dir}" == "${default_output_dir}" ]]; then
    mkdir -p "$(dirname "${default_output_dir}")"
fi
output_parent="$(cd -P "$(dirname "${output_dir}")" && pwd -P)"
output_target="${output_parent}/$(basename "${output_dir}")"
ownership_marker=".beeb-c0-verify-owned"
case "${output_target}" in
    "${root}/.build"/*)
        if [[ -e "${output_target}" && ! -f "${output_target}/${ownership_marker}" ]]; then
            printf 'refusing to remove an existing unowned directory: %s\n' "${output_target}" >&2
            exit 2
        fi
        [[ ! -e "${output_target}" ]] || rm -rf -- "${output_target}"
        mkdir -p "${output_target}"
        ;;
    "${root}"|"${root}"/*|"${home_root}"|"${home_root}"/*|/)
        printf 'unsafe output directory: %s\n' "${output_target}" >&2; exit 2 ;;
    *)
        [[ ! -e "${output_target}" ]] || {
            printf 'refusing to remove an existing unowned directory: %s\n' "${output_target}" >&2
            exit 2
        }
        mkdir "${output_target}"
        ;;
esac
: >"${output_target}/${ownership_marker}"
if [[ -n "${groups_file}" && ! -f "${groups_file}" ]]; then
    printf 'groups file does not exist: %s\n' "${groups_file}" >&2
    exit 2
fi

output_dir="${output_target}"
results="${output_dir}/results.txt"
: >"${results}"

revision="$(git -C "${root}" rev-parse HEAD 2>/dev/null || printf unknown)"
if [[ -n "$(git -C "${root}" status --porcelain 2>/dev/null)" ]]; then
    revision="${revision}-dirty"
fi
{
    printf 'schema=beeb-c0-baseline-v1\n'
    printf 'source_revision=%s\n' "${revision}"
    printf 'environment=%s/%s\n' "$(uname -s)" "$(uname -m)"
    printf 'profile=%s\n' "${profile}"
    printf 'started_at=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
} >"${output_dir}/run.txt"

default_groups() {
    printf 'cpp-behavior\tall\t@internal\n'
    printf 'sanitizers\tall\t@internal\n'
    printf 'version-sync\tall\t@internal\n'
    printf 'c-boundary\tall\t@internal\n'
    printf 'swift-boundary\tmacos\t@internal\n'
    printf 'fixture-provenance\tall\t@internal\n'
    printf 'cleanroom-boot\tall\t@internal\n'
    printf 'bitmap-reference\tall\t@internal\n'
    printf 'mode7-reference\tall\t@internal\n'
    printf 'cpp-documentation\tall\t@internal\n'
    printf 'swift-documentation\tmacos\t@internal\n'
}

applicable() {
    local requirement="$1"
    case "${requirement}" in
        all) return 0 ;;
        macos) [[ "${profile}" == "macos" || "${profile}" == "all" ]] ;;
        portable) [[ "${profile}" == "portable" || "${profile}" == "all" ]] ;;
        *) return 1 ;;
    esac
}

run_internal() {
    local id="$1"
    case "${id}" in
        cpp-behavior) make -C "${root}" test ;;
        sanitizers) make -C "${root}" sanitize ;;
        version-sync) make -C "${root}" check-version ;;
        c-boundary)
            make -C "${root}" .build/cpp/beeb-tests
            "${root}/.build/cpp/beeb-tests" --quick
            ;;
        swift-boundary)
            (cd "${root}" && swift test && swift build)
            ;;
        fixture-provenance)
            "${root}/scripts/verify-c0-references.sh" --check fixture-provenance \
                --output-dir "${output_dir}/reference-provenance"
            ;;
        cleanroom-boot)
            "${root}/scripts/verify-c0-references.sh" --check mode7-state \
                --output-dir "${output_dir}/reference-state"
            ;;
        bitmap-reference)
            "${root}/scripts/verify-c0-references.sh" --check bitmap-reference \
                --output-dir "${output_dir}/reference-bitmap"
            ;;
        mode7-reference)
            "${root}/scripts/verify-c0-references.sh" --check mode7-reference \
                --output-dir "${output_dir}/reference-mode7"
            ;;
        cpp-documentation)
            "${root}/scripts/build-docs.sh" --profile portable --check \
                --output-dir "${root}/.build/docs"
            ;;
        swift-documentation)
            "${root}/scripts/build-docs.sh" --profile macos --check \
                --output-dir "${root}/.build/docs"
            ;;
        *) printf 'unknown internal group: %s\n' "${id}" >&2; return 127 ;;
    esac
}

run_group() {
    local id="$1"
    local requirement="$2"
    local command_path="$3"
    local log_path="${output_dir}/${id}.log"
    local result_status
    local suffix=""
    local command_status

    if ! applicable "${requirement}"; then
        printf '%s\tnot-applicable\t%s\n' "${id}" "${log_path}" >>"${results}"
        return
    fi

    if [[ "${command_path}" != "@internal" && ! -x "${command_path}" ]]; then
        printf 'required command is unavailable: %s\n' "${command_path}" >"${log_path}"
        printf '%s\tunexpected-skip\t%s\n' "${id}" "${log_path}" >>"${results}"
        return
    fi

    # Keep running later groups and retain every failure/interruption in the
    # aggregate result instead of aborting at the first non-zero command.
    set +e
    if [[ "${command_path}" == "@internal" ]]; then
        run_internal "${id}" >"${log_path}" 2>&1
    else
        "${command_path}" >"${log_path}" 2>&1
    fi
    command_status=$?
    set -e

    if [[ ${command_status} -eq 0 ]]; then
        result_status="pass"
    else
        result_status="fail"
        if [[ ${command_status} -eq 130 || ${command_status} -eq 143 ]]; then
            suffix=" (interrupted)"
        fi
    fi
    printf '%s\t%s%s\t%s\n' "${id}" "${result_status}" "${suffix}" "${log_path}" >>"${results}"
}

group_source="${groups_file}"
if [[ -z "${group_source}" ]]; then
    group_source="${output_dir}/groups.txt"
    default_groups >"${group_source}"
fi

while IFS=$'\t' read -r id requirement command_path extra; do
    [[ -z "${id}" ]] && continue
    if [[ -n "${extra:-}" || -z "${requirement}" || -z "${command_path}" ]]; then
        printf 'invalid group definition for %s\n' "${id}" >&2
        exit 2
    fi
    run_group "${id}" "${requirement}" "${command_path}"
done <"${group_source}"

overall="pass"
printf 'C0 baseline profile=%s revision=%s\n' "${profile}" "${revision}"
while IFS=$'\t' read -r id result log_path; do
    diagnosis=""
    case "${result}" in
        fail*|unexpected-skip*)
            diagnosis="$(sed -n '/[^[:space:]]/ { s/[[:space:]]\+/ /g; p; q; }' "${log_path}")"
            ;;
    esac
    if [[ -n "${diagnosis}" ]]; then
        printf '%s %s detail=%s diagnosis=%s\n' \
            "${id}" "${result}" "${log_path}" "${diagnosis}"
    else
        printf '%s %s detail=%s\n' "${id}" "${result}" "${log_path}"
    fi
    case "${result}" in
        pass|not-applicable) ;;
        *) overall="fail" ;;
    esac
done <"${results}"
printf 'overall %s\n' "${overall}"
printf 'overall=%s\n' "${overall}" >>"${output_dir}/run.txt"

[[ "${overall}" == "pass" ]]
