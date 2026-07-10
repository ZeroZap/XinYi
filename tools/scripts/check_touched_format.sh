#!/usr/bin/env bash
# Check clang-format only for C/C++ files changed between two git revisions.

set -euo pipefail

BASE_REF="${1:-}"
HEAD_REF="${2:-HEAD}"

if [ -z "$BASE_REF" ]; then
    if git rev-parse --verify origin/main >/dev/null 2>&1; then
        BASE_REF="$(git merge-base origin/main "$HEAD_REF")"
    else
        BASE_REF="$(git rev-parse "$HEAD_REF^")"
    fi
fi

mapfile -d '' changed_files < <(
    git diff --name-only --diff-filter=ACMR -z "$BASE_REF" "$HEAD_REF" -- \
        '*.c' '*.h' '*.cpp' '*.hpp' \
        ':(exclude)third_party/**' \
        ':(exclude)build/**' \
        ':(exclude)MCU/**'
)

if [ "${#changed_files[@]}" -eq 0 ]; then
    echo "No touched C/C++ files require clang-format checking."
    exit 0
fi

if ! command -v clang-format >/dev/null 2>&1; then
    echo "clang-format is required to check touched C/C++ files." >&2
    exit 1
fi

printf 'Checking clang-format for %d touched C/C++ file(s):\n' "${#changed_files[@]}"
printf '  %s\n' "${changed_files[@]}"

clang-format --dry-run --Werror "${changed_files[@]}"
