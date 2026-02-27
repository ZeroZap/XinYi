#!/bin/bash
# XinYi Framework - Check Code Style Script
# Checks if all C/C++ files are properly formatted

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"

echo "======================================"
echo "XinYi Framework - Code Style Checker"
echo "======================================"
echo ""

# Check if clang-format is installed
if ! command -v clang-format &> /dev/null; then
    echo "Error: clang-format is not installed."
    exit 1
fi

echo "Checking code style..."
echo ""

# Find all C/C++ files and check formatting
FAILED=0
TOTAL=0

while IFS= read -r file; do
    TOTAL=$((TOTAL + 1))
    if ! clang-format --dry-run --Werror "$file" 2>/dev/null; then
        echo "❌ $file"
        FAILED=$((FAILED + 1))
    else
        echo "✓ $file"
    fi
done < <(find "$PROJECT_ROOT" \
    -type f \
    \( -name "*.c" -o -name "*.h" -o -name "*.cpp" -o -name "*.hpp" \) \
    -not -path "*/third_party/*" \
    -not -path "*/build/*" \
    -not -path "*/.git/*")

echo ""
echo "======================================"
echo "Total files: $TOTAL"
echo "Passed: $((TOTAL - FAILED))"
echo "Failed: $FAILED"
echo "======================================"

if [ $FAILED -gt 0 ]; then
    echo ""
    echo "❌ Code style check failed!"
    echo "Run './utils/script/format_code.sh' to fix formatting."
    exit 1
else
    echo ""
    echo "✅ All files pass code style check!"
    exit 0
fi
