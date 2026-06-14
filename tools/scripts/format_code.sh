#!/bin/bash
# XinYi Framework - Code Format Script
# Formats all C/C++ source files using clang-format

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"

echo "======================================"
echo "XinYi Framework - Code Formatter"
echo "======================================"
echo ""

# Check if clang-format is installed
if ! command -v clang-format &> /dev/null; then
    echo "Error: clang-format is not installed."
    echo "Please install it first:"
    echo "  Ubuntu: sudo apt-get install clang-format"
    echo "  macOS:  brew install clang-format"
    exit 1
fi

echo "Using clang-format: $(clang-format --version)"
echo ""

# Find all C/C++ files, excluding third_party and build directories
find "$PROJECT_ROOT" \
    -type f \
    \( -name "*.c" -o -name "*.h" -o -name "*.cpp" -o -name "*.hpp" \) \
    -not -path "*/third_party/*" \
    -not -path "*/build/*" \
    -not -path "*/.git/*" \
    | while read -r file; do
        echo "Formatting: $file"
        clang-format -i "$file"
    done

echo ""
echo "======================================"
echo "Code formatting complete!"
echo "======================================"
