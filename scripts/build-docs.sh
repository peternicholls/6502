#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source_root="${project_root}"
output_dir="${project_root}/.build/docs"
profile="auto"
check=false
debt_baseline=""
changed_files=""

usage() {
    cat <<'EOF'
usage: scripts/build-docs.sh [--check] [--profile auto|portable|macos]
       [--source-root PATH] [--output-dir PATH]
       [--debt-baseline PATH] [--changed-files PATH]
EOF
}

while (($#)); do
    case "$1" in
        --check)
            check=true
            shift
            ;;
        --profile)
            profile="${2:?missing value for --profile}"
            shift 2
            ;;
        --source-root)
            source_root="${2:?missing value for --source-root}"
            shift 2
            ;;
        --output-dir)
            output_dir="${2:?missing value for --output-dir}"
            shift 2
            ;;
        --debt-baseline)
            debt_baseline="${2:?missing value for --debt-baseline}"
            shift 2
            ;;
        --changed-files)
            changed_files="${2:?missing value for --changed-files}"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            printf 'unknown documentation option: %s\n' "$1" >&2
            usage >&2
            exit 2
            ;;
    esac
done

if [[ "${profile}" == "auto" ]]; then
    if [[ "$(uname -s)" == "Darwin" ]]; then
        profile="macos"
    else
        profile="portable"
    fi
fi
case "${profile}" in
    portable|macos) ;;
    *)
        printf 'unsupported documentation profile: %s\n' "${profile}" >&2
        exit 2
        ;;
esac

if [[ ! -d "${source_root}" ]]; then
    printf 'missing documentation source root: %s\n' "${source_root}" >&2
    exit 2
fi
case "${output_dir}" in
    ""|/)
        printf 'unsafe documentation output directory: %s\n' "${output_dir}" >&2
        exit 2
        ;;
esac

if [[ -z "${debt_baseline}" ]]; then
    debt_baseline="${source_root}/Tests/Fixtures/C0/documentation-debt.txt"
fi
if [[ -z "${changed_files}" ]]; then
    changed_files="$(mktemp "${TMPDIR:-/tmp}/beeb-docs-changed.XXXXXX")"
    trap 'rm -f -- "${changed_files}" "${doxygen_config:-}"' EXIT
    if git -C "${source_root}" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
        git -C "${source_root}" diff --name-only HEAD -- >"${changed_files}"
    fi
fi

if [[ ! -f "${debt_baseline}" ]]; then
    printf 'missing documentation debt baseline: %s\n' "${debt_baseline}" >&2
    exit 2
fi
if [[ ! -f "${changed_files}" ]]; then
    printf 'missing changed-files inventory: %s\n' "${changed_files}" >&2
    exit 2
fi

validate_debt() {
    local baseline_count current_count
    baseline_count="$(sed -n 's/^baseline_count=//p' "${debt_baseline}")"
    if [[ ! "${baseline_count}" =~ ^[0-9]+$ ]]; then
        printf 'invalid documentation debt baseline_count: %s\n' "${debt_baseline}" >&2
        return 1
    fi
    current_count="$(awk '
        /^[[:space:]]*($|#)/ { next }
        /^baseline_count=/ { next }
        { count++ }
        END { print count + 0 }
    ' "${debt_baseline}")"
    if ((current_count > baseline_count)); then
        printf 'documentation debt exceeds baseline: %d > %d\n' \
            "${current_count}" "${baseline_count}" >&2
        return 1
    fi
}

validate_public_headers() {
    local header relative reference symbol occurrences
    local -a public_roots=()
    for public_root in "${source_root}/include" "${source_root}/Sources/BeebCore/include"; do
        [[ -d "${public_root}" ]] && public_roots+=("${public_root}")
    done
    while IFS= read -r -d '' header; do
        relative="${header#"${source_root}/"}"
        awk -v path="${relative}" '
            BEGIN { public_surface = 1 }
            function reset_doc() {
                documented = 0
                in_block = 0
                split("", param_docs)
            }
            /^[[:space:]]*\/\/\// {
                documented = 1
                if (match($0, /@param[[:space:]]+[[:alnum:]_]+/)) {
                    parameter = substr($0, RSTART, RLENGTH)
                    sub(/^@param[[:space:]]+/, "", parameter)
                    param_docs[parameter] = 1
                }
                next
            }
            /^[[:space:]]*\/\*\*/ { documented = 1; in_block = 1; next }
            in_block {
                if ($0 ~ /\*\//) in_block = 0
                next
            }
            /^[[:space:]]*#/ { reset_doc(); next }
            /^[[:space:]]*$/ { next }
            /^[[:space:]]*template/ { next }
            /^[[:space:]]*\[\[/ { reset_doc(); next }
            /^[[:space:]]*class[[:space:]].*\{/ {
                public_surface = 0
                reset_doc()
                next
            }
            /^[[:space:]]*struct[[:space:]].*\{/ {
                public_surface = 1
                reset_doc()
                next
            }
            /^[[:space:]]*public[[:space:]]*:/ {
                public_surface = 1
                reset_doc()
                next
            }
            /^[[:space:]]*(private|protected)[[:space:]]*:/ {
                public_surface = 0
                reset_doc()
                next
            }
            /^[[:space:]]*[[:alnum:]_~].*\([^;{}]*\)[^;{}]*;[[:space:]]*$/ {
                if (public_surface && !documented) {
                    printf "missing public documentation: %s:%d\n", path, NR > "/dev/stderr"
                    failed = 1
                }
                arguments = $0
                sub(/^[^(]*\(/, "", arguments)
                sub(/\).*/, "", arguments)
                for (parameter in param_docs) {
                    pattern = "(^|[^[:alnum:]_])" parameter "([^[:alnum:]_]|$)"
                    if (public_surface && arguments !~ pattern) {
                        printf "argument '\''%s'\'' of command @param is not found in the argument list\n", parameter > "/dev/stderr"
                        failed = 1
                    }
                }
                reset_doc()
                next
            }
            { reset_doc() }
            END { exit failed }
        ' "${header}" || return 1
    done < <(find "${public_roots[@]}" -type f \
        \( -name '*.h' -o -name '*.hpp' \) -print0 2>/dev/null || true)

    if ((${#public_roots[@]})); then
        while IFS= read -r reference; do
            symbol="${reference##* }"
            occurrences="$(rg --no-filename -w -o "${symbol}" "${public_roots[@]}" \
                2>/dev/null | wc -l | tr -d ' ')"
            if ((occurrences < 2)); then
                printf "unable to resolve reference to '%s'\n" "${symbol}" >&2
                return 1
            fi
        done < <(rg --no-filename -o '@ref[[:space:]]+[[:alnum:]_]+' \
            "${public_roots[@]}" 2>/dev/null || true)
    fi
}

validate_changed_complex_code() {
    local entry path rationale diff_text
    while IFS= read -r entry || [[ -n "${entry}" ]]; do
        [[ -z "${entry}" || "${entry}" == \#* ]] && continue
        path="${entry%%|*}"
        rationale=""
        if [[ "${entry}" == *"|"* ]]; then
            rationale="${entry#*|}"
        fi
        case "${path}" in
            *.c|*.cc|*.cpp|*.h|*.hpp|*.swift) ;;
            *) continue ;;
        esac
        if [[ "${rationale}" == N/A:* ]]; then
            continue
        fi
        diff_text=""
        if git -C "${source_root}" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
            diff_text="$(git -C "${source_root}" diff --unified=0 HEAD -- "${path}")"
        fi
        if [[ -n "${diff_text}" ]] && awk '
            /^\+\+\+|^---/ { next }
            /^[+-]/ {
                line = substr($0, 2)
                sub(/^[[:space:]]*/, "", line)
                if (line == "" || line ~ /^(\/\/|\/\*|\*|\*\/)/) next
                code_changed = 1
            }
            END { exit code_changed }
        ' <<<"${diff_text}"; then
            continue
        fi
        if [[ -f "${source_root}/${path}" ]] && \
            rg -q 'C0-DOC-RATIONALE:|Documentation rationale:' "${source_root}/${path}"; then
            continue
        fi
        printf 'complex documentation missing: %s\n' "${path}" >&2
        return 1
    done <"${changed_files}"
}

validate_debt
validate_public_headers
validate_changed_complex_code

command -v doxygen >/dev/null 2>&1 || {
    printf 'missing required documentation generator: doxygen\n' >&2
    exit 2
}

rm -rf -- "${output_dir}"
mkdir -p "${output_dir}"
doxygen_config="$(mktemp "${TMPDIR:-/tmp}/beeb-doxygen.XXXXXX")"
if [[ -z "${changed_files:-}" || "${changed_files}" != "${TMPDIR:-/tmp}"/beeb-docs-changed.* ]]; then
    trap 'rm -f -- "${doxygen_config:-}"' EXIT
fi

inputs=("${source_root}/include" "${source_root}/Sources/BeebCore/include" \
    "${source_root}/src" "${source_root}/Sources/BeebCore/src")
if [[ -d "${source_root}/docs/code" ]]; then
    inputs+=("${source_root}/docs/code")
else
    inputs+=("${source_root}/docs")
fi
input_value=""
for input in "${inputs[@]}"; do
    if [[ -d "${input}" ]]; then
        input_value+=" \"${input}\""
    fi
done

{
    sed -e '/^[[:space:]]*OUTPUT_DIRECTORY[[:space:]]*=/d' \
        -e '/^[[:space:]]*HTML_OUTPUT[[:space:]]*=/d' \
        "${project_root}/Doxyfile"
    printf '\nOUTPUT_DIRECTORY = "%s"\n' "${output_dir}"
    printf 'HTML_OUTPUT = cpp\n'
    printf 'STRIP_FROM_PATH = "%s"\n' "${source_root}"
    printf 'INPUT =%s\n' "${input_value}"
    if [[ -f "${source_root}/docs/code/architecture.md" ]]; then
        printf 'USE_MDFILE_AS_MAINPAGE = "%s"\n' \
            "${source_root}/docs/code/architecture.md"
    fi
} >"${doxygen_config}"

doxygen "${doxygen_config}"

if [[ "${profile}" == "macos" ]]; then
    if [[ -n "${C0_DOCC_COMMAND:-}" ]]; then
        "${C0_DOCC_COMMAND}" "${output_dir}/swift"
    else
        mkdir -p "${output_dir}/swift"
        (
            cd "${source_root}"
            swift package --allow-writing-to-directory "${output_dir}/swift" \
                generate-documentation --target BeebKit \
                --output-path "${output_dir}/swift" \
                --transform-for-static-hosting \
                --hosting-base-path swift \
                --warnings-as-errors
        )
    fi
fi

{
    printf '%s\n' '<!doctype html>' '<html lang="en"><head>' \
        '<meta charset="utf-8"><title>Beeb 6502 code documentation</title>' \
        '</head><body><main>' '<h1>Beeb 6502 code documentation</h1>' \
        '<ul><li><a href="cpp/index.html">C, C ABI, and C++ reference</a></li>'
    if [[ "${profile}" == "macos" ]]; then
        printf '%s\n' '<li><a href="swift/index.html">Swift reference and guides</a></li>'
    else
        printf '%s\n' '<li>Swift documentation requires the macOS profile.</li>'
    fi
    printf '%s\n' '</ul></main></body></html>'
} >"${output_dir}/index.html"

check_internal_links() {
    local html href clean target
    while IFS= read -r -d '' html; do
        while IFS= read -r href; do
            case "${href}" in
                ""|\#*|/favicon.*|http://*|https://*|mailto:*|javascript:*|data:*) continue ;;
            esac
            clean="${href%%\#*}"
            clean="${clean%%\?*}"
            [[ -z "${clean}" ]] && continue
            if [[ "${clean}" == /* ]]; then
                target="${output_dir}/${clean#/}"
            else
                target="$(dirname "${html}")/${clean}"
            fi
            if [[ ! -e "${target}" ]]; then
                printf 'broken generated documentation link: %s -> %s\n' \
                    "${html#"${output_dir}/"}" "${href}" >&2
                return 1
            fi
        done < <(perl -ne 'while (/href="([^"]+)"/g) { print "$1\n" }' "${html}")
    done < <(find "${output_dir}" -type f -name '*.html' -print0)
}

if [[ "${check}" == true ]]; then
    check_internal_links
fi

printf 'documentation profile %s generated at %s\n' "${profile}" "${output_dir}"
