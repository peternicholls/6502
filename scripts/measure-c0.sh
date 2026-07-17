#!/usr/bin/env bash
set -euo pipefail

root="$(cd -P "$(dirname "$0")/.." && pwd -P)"
home_root="$(cd -P "${HOME}" && pwd -P)"
samples=5
output="${root}/.build/c0/measurements/latest.txt"
sample_command=""
environment_file=""
requested_cycles=100000
workload="cleanroom-mode7-100k"

usage() {
    printf 'usage: measure-c0.sh [--samples N] [--output FILE] [--sample-command FILE] [--environment-file FILE]\n'
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --samples) [[ $# -ge 2 ]] || exit 2; samples="$2"; shift 2 ;;
        --output) [[ $# -ge 2 ]] || exit 2; output="$2"; shift 2 ;;
        --sample-command) [[ $# -ge 2 ]] || exit 2; sample_command="$2"; shift 2 ;;
        --environment-file) [[ $# -ge 2 ]] || exit 2; environment_file="$2"; shift 2 ;;
        --help|-h) usage; exit 0 ;;
        *) printf 'unknown option: %s\n' "$1" >&2; usage >&2; exit 2 ;;
    esac
done
case "${samples}" in *[!0-9]*|0|'') printf 'samples must be a positive integer\n' >&2; exit 2 ;; esac

case "${output}" in
    ""|/)
        printf 'unsafe measurement output: %s\n' "${output}" >&2
        exit 2
        ;;
esac
if [[ -d "${output}" ]]; then
    printf 'measurement output must be a file: %s\n' "${output}" >&2
    exit 2
fi

mkdir -p "$(dirname "${output}")"
work_dir="$(dirname "${output}")/.measurement-work"
work_parent="$(cd -P "$(dirname "${work_dir}")" && pwd -P)"
work_target="${work_parent}/$(basename "${work_dir}")"
ownership_marker=".beeb-c0-measure-owned"
case "${work_target}" in
    "${root}/.build"/*)
        if [[ -e "${work_target}" && ! -f "${work_target}/${ownership_marker}" ]]; then
            printf 'refusing to remove an existing unowned directory: %s\n' "${work_target}" >&2
            exit 2
        fi
        [[ ! -e "${work_target}" ]] || rm -rf -- "${work_target}"
        mkdir -p "${work_target}"
        ;;
    /|"${root}"|"${root}"/*|"${home_root}"|"${home_root}"/*)
        printf 'unsafe measurement work directory: %s\n' "${work_target}" >&2; exit 2 ;;
    *)
        [[ ! -e "${work_target}" ]] || {
            printf 'refusing to remove an existing unowned directory: %s\n' "${work_target}" >&2
            exit 2
        }
        mkdir "${work_target}"
        ;;
esac
work_dir="${work_target}"
: >"${work_dir}/${ownership_marker}"
# The directory was either created atomically above or recreated inside the
# tool-owned build tree, so this trap cannot remove a caller-owned path.
trap 'rm -rf -- "${work_dir}"' EXIT
sample_records="${work_dir}/samples.txt"
rates="${work_dir}/rates.txt"
: >"${sample_records}"
: >"${rates}"

revision="$(git -C "${root}" rev-parse HEAD 2>/dev/null || printf unknown)"
if [[ -n "$(git -C "${root}" status --porcelain 2>/dev/null)" ]]; then
    revision="${revision}-dirty"
fi

environment_value() {
    local key="$1"
    sed -n "s/^${key}=//p" "${environment_file}" | sed -n '1p'
}

if [[ -n "${environment_file}" ]]; then
    os="$(environment_value os)"
    architecture="$(environment_value architecture)"
    cpu="$(environment_value cpu)"
    compiler="$(environment_value compiler)"
else
    os="$(uname -s) $(uname -r)"
    architecture="$(uname -m)"
    if [[ "$(uname -s)" == "Darwin" ]]; then
        cpu="$(sysctl -n machdep.cpu.brand_string 2>/dev/null || printf unknown)"
    elif [[ -r /proc/cpuinfo ]]; then
        cpu="$(sed -n 's/^model name[[:space:]]*:[[:space:]]*//p' /proc/cpuinfo | sed -n '1p')"
        [[ -n "${cpu}" ]] || cpu="unknown"
    else
        cpu="unknown"
    fi
    compiler="$(${CXX:-c++} --version 2>/dev/null | sed -n '1p')"
fi

invalid_reason=""
for field in os architecture cpu compiler; do
    case "${field}" in
        os) value="${os}" ;;
        architecture) value="${architecture}" ;;
        cpu) value="${cpu}" ;;
        compiler) value="${compiler}" ;;
    esac
    if [[ -z "${value}" ]]; then
        invalid_reason="missing environment field: ${field}"
        break
    fi
done

now_ns() {
    local value
    value="$(date +%s%N 2>/dev/null || true)"
    if [[ "${value}" =~ ^[0-9]+$ ]]; then
        printf '%s\n' "${value}"
    elif command -v perl >/dev/null 2>&1; then
        perl -MTime::HiRes=time -e 'printf "%.0f\n", time() * 1_000_000_000'
    elif command -v python3 >/dev/null 2>&1; then
        python3 -c 'import time; print(time.monotonic_ns())'
    else
        return 1
    fi
}

if [[ -z "${sample_command}" && -z "${invalid_reason}" ]]; then
    make -C "${root}" .build/cpp/make-demo-rom .build/cpp/beeb-evidence >/dev/null
    rom="${work_dir}/mode7.rom"
    "${root}/.build/cpp/make-demo-rom" --workload mode7 "${rom}" >/dev/null
fi

valid_samples=0
if [[ -z "${invalid_reason}" ]]; then
    for sample in $(seq 1 "${samples}"); do
        sample_output="${work_dir}/sample-${sample}.txt"
        # Preserve an output record for interrupted or incomplete samples;
        # statuses 130/143 are classified below instead of losing evidence.
        set +e
        if [[ -n "${sample_command}" ]]; then
            "${sample_command}" "${sample}" >"${sample_output}" 2>"${work_dir}/sample-${sample}.stderr"
            sample_status=$?
        else
            start="$(now_ns)" || { sample_status=1; start=0; }
            state="${work_dir}/sample-${sample}.state.txt"
            "${root}/.build/cpp/beeb-evidence" --rom "${rom}" --workload mode7 \
                --cycles "${requested_cycles}" --output "state:${state}" \
                >"${work_dir}/sample-${sample}.stdout" 2>"${work_dir}/sample-${sample}.stderr"
            sample_status=$?
            end="$(now_ns)" || { sample_status=1; end=0; }
            if [[ ${sample_status} -eq 0 ]]; then
                actual="$(sed -n 's/^actual_cycles=//p' "${state}")"
                printf 'actual_cycles=%s\nelapsed_ns=%s\n' "${actual}" "$((end - start))" >"${sample_output}"
            fi
        fi
        set -e

        if [[ ${sample_status} -eq 130 || ${sample_status} -eq 143 ]]; then
            invalid_reason="measurement interrupted at sample ${sample}"
            break
        elif [[ ${sample_status} -ne 0 ]]; then
            invalid_reason="workload incomplete at sample ${sample}"
            break
        fi

        actual_cycles="$(sed -n 's/^actual_cycles=//p' "${sample_output}")"
        elapsed_ns="$(sed -n 's/^elapsed_ns=//p' "${sample_output}")"
        if [[ ! "${actual_cycles}" =~ ^[0-9]+$ || ${actual_cycles} -le 0 ]]; then
            invalid_reason="sample ${sample} has invalid cycle count"
            break
        fi
        if [[ ! "${elapsed_ns}" =~ ^-?[0-9]+$ || ${elapsed_ns} -le 0 ]]; then
            invalid_reason="sample ${sample} has non-positive duration"
            break
        fi
        rate="$(awk -v cycles="${actual_cycles}" -v duration="${elapsed_ns}" \
            'BEGIN { printf "%.0f", cycles * 1000000000 / duration }')"
        valid_samples=$((valid_samples + 1))
        {
            printf 'sample.%s.actual_cycles=%s\n' "${valid_samples}" "${actual_cycles}"
            printf 'sample.%s.elapsed_ns=%s\n' "${valid_samples}" "${elapsed_ns}"
            printf 'sample.%s.cycles_per_second=%s\n' "${valid_samples}" "${rate}"
        } >>"${sample_records}"
        printf '%s\n' "${rate}" >>"${rates}"
    done
fi

if [[ -z "${invalid_reason}" && ${valid_samples} -lt 5 ]]; then
    invalid_reason="fewer than five valid samples"
fi

median=0
minimum=0
maximum=0
if [[ ${valid_samples} -gt 0 ]]; then
    sorted="${work_dir}/rates-sorted.txt"
    sort -n "${rates}" >"${sorted}"
    median_index=$(((valid_samples + 1) / 2))
    median="$(sed -n "${median_index}p" "${sorted}")"
    minimum="$(sed -n '1p' "${sorted}")"
    maximum="$(tail -n 1 "${sorted}")"
fi

if [[ -z "${invalid_reason}" ]]; then valid=true; else valid=false; fi
{
    printf 'schema=beeb-c0-measurement-v1\n'
    printf 'label=descriptive comparison baseline; not a product guarantee\n'
    printf 'workload=%s\n' "${workload}"
    printf 'source_revision=%s\n' "${revision}"
    printf 'os=%s\n' "${os}"
    printf 'architecture=%s\n' "${architecture}"
    printf 'cpu=%s\n' "${cpu}"
    printf 'compiler=%s\n' "${compiler}"
    printf 'build_mode=release\n'
    printf 'requested_cycles=%s\n' "${requested_cycles}"
    printf 'sample_count=%s\n' "${valid_samples}"
    cat "${sample_records}"
    printf 'median_cycles_per_second=%s\n' "${median}"
    printf 'min_cycles_per_second=%s\n' "${minimum}"
    printf 'max_cycles_per_second=%s\n' "${maximum}"
    printf 'valid=%s\n' "${valid}"
    printf 'invalid_reason=%s\n' "${invalid_reason}"
} >"${output}"

printf 'measurement=%s valid=%s samples=%s\n' "${output}" "${valid}" "${valid_samples}"
[[ "${valid}" == "true" ]]
