#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
reference=""
reason=""
fixture_root="${root}/Tests/Fixtures/C0"

usage() {
    printf 'usage: update-c0-reference.sh --reference ID --reason TEXT [--fixture-root DIR]\n'
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --reference)
            [[ $# -ge 2 ]] || { printf 'missing value for --reference\n' >&2; exit 2; }
            reference="$2"
            shift 2
            ;;
        --reason)
            [[ $# -ge 2 ]] || { printf 'missing value for --reason\n' >&2; exit 2; }
            reason="$2"
            shift 2
            ;;
        --fixture-root)
            [[ $# -ge 2 ]] || { printf 'missing value for --fixture-root\n' >&2; exit 2; }
            fixture_root="$2"
            shift 2
            ;;
        --help|-h) usage; exit 0 ;;
        *) printf 'unknown option: %s\n' "$1" >&2; usage >&2; exit 2 ;;
    esac
done

if [[ -n "${CI:-}" ]]; then
    printf 'refusing reference update in CI\n' >&2
    exit 1
fi
if [[ -z "${reference}" ]]; then
    printf 'reference is required\n' >&2
    exit 2
fi
if [[ -z "${reason//[[:space:]]/}" ]]; then
    printf 'reason is required\n' >&2
    exit 2
fi

case "${reference}" in
    bitmap-reference)
        workload="bitmap"
        selected_kind="frame"
        approved_name="bitmap.ppm"
        ;;
    mode7-reference)
        workload="mode7"
        selected_kind="frame"
        approved_name="mode7.ppm"
        ;;
    mode7-state)
        workload="mode7"
        selected_kind="state"
        approved_name="approved-state.txt"
        ;;
    *)
        printf 'unknown reference: %s\n' "${reference}" >&2
        exit 2
        ;;
esac

manifest="${fixture_root}/manifest.txt"
approved_path="${fixture_root}/${approved_name}"
if [[ ! -f "${manifest}" || ! -f "${approved_path}" ]]; then
    printf 'approved reference is incomplete: %s\n' "${reference}" >&2
    exit 1
fi

generator="${BEEB_ROM_GENERATOR:-${root}/.build/cpp/make-demo-rom}"
evidence="${BEEB_EVIDENCE_BIN:-${root}/.build/cpp/beeb-evidence}"
if [[ ! -x "${generator}" || ! -x "${evidence}" ]]; then
    make -C "${root}" .build/cpp/make-demo-rom .build/cpp/beeb-evidence >/dev/null
fi
if [[ ! -x "${generator}" || ! -x "${evidence}" ]]; then
    printf 'required C0 generator or evidence tool is unavailable\n' >&2
    exit 1
fi

candidate_root="${root}/.build/c0/update/${reference}"
rm -rf -- "${candidate_root}"
mkdir -p "${candidate_root}"

for run in $(seq 1 10); do
    rom="${candidate_root}/${run}.rom"
    state="${candidate_root}/${run}.state.txt"
    frame="${candidate_root}/${run}.ppm"
    "${generator}" --workload "${workload}" "${rom}" >/dev/null
    "${evidence}" --rom "${rom}" --workload "${workload}" --cycles 100000 \
        --output "state:${state}" --output "frame:${frame}" >/dev/null
done

if [[ "${selected_kind}" == "state" ]]; then
    selected_suffix="state.txt"
else
    selected_suffix="ppm"
fi
candidate="${candidate_root}/1.${selected_suffix}"
for run in $(seq 2 10); do
    if ! cmp -s "${candidate}" "${candidate_root}/${run}.${selected_suffix}"; then
        printf 'candidate runs are not identical: %s\n' "${reference}" >&2
        exit 1
    fi
done

actual_cycles="$(sed -n 's/^actual_cycles=//p' "${candidate_root}/1.state.txt")"
if [[ -z "${actual_cycles}" ]]; then
    printf 'candidate has no actual cycle count: %s\n' "${reference}" >&2
    exit 1
fi
bytes="$(wc -c <"${candidate}" | tr -d ' ')"
sha256="$(shasum -a 256 "${candidate}" | awk '{print $1}')"

manifest_new="${candidate_root}/manifest.txt"
awk -v target="${reference}" -v actual="${actual_cycles}" -v bytes="${bytes}" \
    -v sha="${sha256}" -v note="${reason}" '
    BEGIN { RS=""; ORS=""; found=0 }
    {
        count=split($0, lines, "\n")
        match_record=0
        for (i=1; i<=count; ++i) if (lines[i] == "id=" target) match_record=1
        if (match_record) {
            found=1
            for (i=1; i<=count; ++i) {
                if (lines[i] ~ /^actual_cycles=/) lines[i]="actual_cycles=" actual
                else if (lines[i] ~ /^bytes=/) lines[i]="bytes=" bytes
                else if (lines[i] ~ /^review_note=/) lines[i]="review_note=" note
                else if (lines[i] ~ /^sha256=/) lines[i]="sha256=" sha
            }
        }
        for (i=1; i<=count; ++i) printf "%s\n", lines[i]
        printf "\n"
    }
    END { if (!found) exit 3 }
    ' "${manifest}" >"${manifest_new}" || {
        printf 'reference is missing from manifest: %s\n' "${reference}" >&2
        exit 1
    }

cp "${candidate}" "${approved_path}"
mv "${manifest_new}" "${manifest}"

printf 'updated %s from ten identical candidates\n' "${reference}"
printf 'reason: %s\n' "${reason}"
printf 'Review with: git diff -- %s\n' "${fixture_root}"
