#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
# Manifest discovery and ordering must be locale-stable across verification
# hosts so provenance comparisons remain deterministic.

root="$(cd -P "$(dirname "$0")/.." && pwd -P)"
home_root="$(cd -P "${HOME}" && pwd -P)"
runs=1
fixture_root="${root}/Tests/Fixtures/C0"
output_dir="${root}/.build/c0/references"
check=""

usage() {
    printf 'usage: verify-c0-references.sh [--runs N] [--check ID] [--fixture-root DIR] [--output-dir DIR]\n'
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --runs) [[ $# -ge 2 ]] || exit 2; runs="$2"; shift 2 ;;
        --check) [[ $# -ge 2 ]] || exit 2; check="$2"; shift 2 ;;
        --fixture-root) [[ $# -ge 2 ]] || exit 2; fixture_root="$2"; shift 2 ;;
        --output-dir) [[ $# -ge 2 ]] || exit 2; output_dir="$2"; shift 2 ;;
        --help|-h) usage; exit 0 ;;
        *) printf 'unknown option: %s\n' "$1" >&2; usage >&2; exit 2 ;;
    esac
done

case "${runs}" in *[!0-9]*|0|'') printf 'runs must be a positive integer\n' >&2; exit 2 ;; esac
case "${check}" in
    ""|fixture-provenance|mode7-state|bitmap-reference|mode7-reference) ;;
    *) printf 'unknown reference check: %s\n' "${check}" >&2; exit 2 ;;
esac
case "${output_dir}" in ""|/) printf 'unsafe output directory\n' >&2; exit 2 ;; esac
output_parent="$(cd -P "$(dirname "${output_dir}")" && pwd -P)"
output_target="${output_parent}/$(basename "${output_dir}")"
ownership_marker=".beeb-c0-reference-owned"
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
        printf 'unsafe output directory\n' >&2; exit 2 ;;
    *)
        [[ ! -e "${output_target}" ]] || {
            printf 'refusing to remove an existing unowned directory: %s\n' "${output_target}" >&2
            exit 2
        }
        mkdir "${output_target}"
        ;;
esac
: >"${output_target}/${ownership_marker}"
output_dir="${output_target}"

manifest="${fixture_root}/manifest.txt"
if [[ ! -f "${manifest}" ]]; then
    printf 'missing reference manifest: %s\n' "${manifest}" >&2
    exit 1
fi

manifest_value() {
    local id="$1"
    local key="$2"
    awk -v wanted_id="${id}" -v wanted_key="${key}" '
        BEGIN { RS=""; FS="\n" }
        {
            match_record=0
            for (i=1; i<=NF; ++i) if ($i == "id=" wanted_id) match_record=1
            if (match_record) {
                prefix=wanted_key "="
                for (i=1; i<=NF; ++i) if (index($i, prefix) == 1) {
                    print substr($i, length(prefix) + 1)
                    exit
                }
            }
        }
    ' "${manifest}"
}

required_fields="actual_cycles bytes coverage fixture generation_command generator id kind path redistribution_basis requested_cycles review_note schema sha256"
expected_ids="bitmap-reference mode7-reference mode7-state"
record_count="$(awk 'BEGIN { RS="" } NF { count++ } END { print count+0 }' "${manifest}")"
if [[ "${record_count}" != "3" ]]; then
    printf 'manifest must contain exactly 3 records, found %s\n' "${record_count}" >&2
    exit 1
fi

for id in ${expected_ids}; do
    if [[ "$(manifest_value "${id}" id)" != "${id}" ]]; then
        printf 'missing manifest record: %s\n' "${id}" >&2
        exit 1
    fi
    for field in ${required_fields}; do
        if [[ -z "$(manifest_value "${id}" "${field}")" ]]; then
            printf 'missing manifest field: %s (%s)\n' "${field}" "${id}" >&2
            exit 1
        fi
    done
done

if ! awk '
    BEGIN { RS=""; FS="\n"; bad=0 }
    {
        previous=""
        for (i=1; i<=NF; ++i) {
            split($i, parts, "=")
            if (previous != "" && parts[1] < previous) bad=1
            previous=parts[1]
        }
    }
    END { exit bad }
    ' "${manifest}"; then
    printf 'manifest keys are not sorted\n' >&2
    exit 1
fi

validate_record() {
    local id="$1"
    local expected_fixture="$2"
    local expected_kind="$3"
    local expected_path="$4"
    local path="${fixture_root}/${expected_path}"
    local expected_sha
    local observed_sha

    [[ "$(manifest_value "${id}" schema)" == "beeb-c0-reference-v1" ]] || {
        printf 'invalid manifest schema: %s\n' "${id}" >&2; exit 1;
    }
    [[ "$(manifest_value "${id}" fixture)" == "${expected_fixture}" ]] || {
        printf 'fixture mismatch: %s\n' "${id}" >&2; exit 1;
    }
    [[ "$(manifest_value "${id}" kind)" == "${expected_kind}" ]] || {
        printf 'kind mismatch: %s\n' "${id}" >&2; exit 1;
    }
    [[ "$(manifest_value "${id}" path)" == "${expected_path}" ]] || {
        printf 'path mismatch: %s\n' "${id}" >&2; exit 1;
    }
    [[ -f "${path}" ]] || { printf 'missing approved reference: %s\n' "${expected_path}" >&2; exit 1; }
    expected_sha="$(manifest_value "${id}" sha256)"
    observed_sha="$(shasum -a 256 "${path}" | awk '{print $1}')"
    if [[ "$(wc -c <"${path}" | tr -d ' ')" != "$(manifest_value "${id}" bytes)" || \
          "${observed_sha}" != "${expected_sha}" ]]; then
        printf '%s mismatch expected=%s observed=%s\n' "${id}" "${expected_sha}" "${observed_sha}" >&2
        exit 1
    fi
}

validate_record bitmap-reference bitmap bitmap-ppm bitmap.ppm
validate_record mode7-reference mode7 mode7-ppm mode7.ppm
validate_record mode7-state mode7 state approved-state.txt

while IFS= read -r approved_path; do
    name="$(basename "${approved_path}")"
    if ! grep -Fq "path=${name}" "${manifest}"; then
        printf 'orphan approved reference: %s\n' "${name}" >&2
        exit 1
    fi
done < <(find "${fixture_root}" -maxdepth 1 -type f \( -name '*.ppm' -o -name 'approved-state.txt' \) | sort)

if [[ "${check}" == "fixture-provenance" ]]; then
    printf 'fixture-provenance pass\n'
    exit 0
fi

generator="${BEEB_ROM_GENERATOR:-${root}/.build/cpp/make-demo-rom}"
evidence="${BEEB_EVIDENCE_BIN:-${root}/.build/cpp/beeb-evidence}"
if [[ ! -x "${generator}" || ! -x "${evidence}" ]]; then
    make -C "${root}" .build/cpp/make-demo-rom .build/cpp/beeb-evidence >/dev/null
fi
[[ -x "${generator}" && -x "${evidence}" ]] || {
    printf 'required C0 generator or evidence tool is unavailable\n' >&2; exit 1;
}

generate_workload() {
    local workload="$1"
    local workload_dir="${output_dir}/${workload}"
    mkdir -p "${workload_dir}"
    for run in $(seq 1 "${runs}"); do
        "${generator}" --workload "${workload}" "${workload_dir}/${run}.rom" >/dev/null
        "${evidence}" --rom "${workload_dir}/${run}.rom" --workload "${workload}" \
            --cycles 100000 --output "state:${workload_dir}/${run}.state.txt" \
            --output "frame:${workload_dir}/${run}.ppm" >/dev/null
    done
    if [[ ${runs} -gt 1 ]]; then
        for suffix in rom state.txt ppm; do
            for run in $(seq 2 "${runs}"); do
                if ! cmp -s "${workload_dir}/1.${suffix}" "${workload_dir}/${run}.${suffix}"; then
                    printf 'candidate runs are not identical: %s (%s)\n' "${workload}" "${suffix}" >&2
                    exit 1
                fi
            done
        done
    fi
}

check_candidate() {
    local id="$1"
    local workload="$2"
    local suffix="$3"
    local approved_name="$4"
    local state="${output_dir}/${workload}/1.state.txt"
    local candidate="${output_dir}/${workload}/1.${suffix}"
    local approved="${fixture_root}/${approved_name}"
    local expected_actual="$(manifest_value "${id}" actual_cycles)"
    local observed_actual="$(sed -n 's/^actual_cycles=//p' "${state}")"
    local expected_requested="$(manifest_value "${id}" requested_cycles)"
    local observed_requested="$(sed -n 's/^requested_cycles=//p' "${state}")"

    if [[ "${expected_actual}" != "${observed_actual}" ]]; then
        printf 'actual cycle mismatch: %s expected=%s observed=%s\n' \
            "${id}" "${expected_actual}" "${observed_actual}" >&2
        exit 1
    fi
    if [[ "${expected_requested}" != "${observed_requested}" ]]; then
        printf 'requested cycle mismatch: %s expected=%s observed=%s\n' \
            "${id}" "${expected_requested}" "${observed_requested}" >&2
        exit 1
    fi
    if ! cmp -s "${approved}" "${candidate}"; then
        printf '%s mismatch expected=%s observed=%s\n' "${id}" \
            "$(shasum -a 256 "${approved}" | awk '{print $1}')" \
            "$(shasum -a 256 "${candidate}" | awk '{print $1}')" >&2
        exit 1
    fi
    printf '%s pass\n' "${id}"
}

if [[ -z "${check}" || "${check}" == "mode7-state" || "${check}" == "mode7-reference" ]]; then
    generate_workload mode7
fi
if [[ -z "${check}" || "${check}" == "bitmap-reference" ]]; then
    generate_workload bitmap
fi
if [[ -z "${check}" || "${check}" == "mode7-state" ]]; then
    check_candidate mode7-state mode7 state.txt approved-state.txt
fi
if [[ -z "${check}" || "${check}" == "bitmap-reference" ]]; then
    check_candidate bitmap-reference bitmap ppm bitmap.ppm
fi
if [[ -z "${check}" || "${check}" == "mode7-reference" ]]; then
    check_candidate mode7-reference mode7 ppm mode7.ppm
fi
