#!/bin/bash
# XinYi Framework - Generate Coverage Report Script

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"

echo "======================================"
echo "XinYi Framework - Coverage Report"
echo "======================================"
echo ""

# Check if gcovr is installed
if ! command -v gcovr &> /dev/null; then
    echo "Error: gcovr is not installed."
    echo "Please install it first:"
    echo "  Ubuntu: sudo apt-get install gcovr"
    echo "  macOS:  brew install gcovr"
    echo "  pip:    pip install gcovr"
    exit 1
fi

echo "Using gcovr: $(gcovr --version)"
echo ""

# Check if build directory exists
if [ ! -d "$PROJECT_ROOT/build" ]; then
    echo "Error: Build directory not found."
    echo "Please run: mkdir build && cd build && cmake .. -DBUILD_TESTING=ON -DTEST_COVERAGE=ON"
    exit 1
fi

cd "$PROJECT_ROOT/build"

echo "Building with coverage..."
make clean
cmake .. -DBUILD_TESTING=ON -DTEST_COVERAGE=ON
make -j$(nproc)

echo ""
echo "Running tests for coverage..."
ctest --output-on-failure

echo ""
echo "Generating coverage report..."

# Generate HTML report
gcovr -r .. \
    --html --html-details \
    -o coverage-report.html \
    --exclude '.*tests/.*' \
    --exclude '.*third_party/.*' \
    --exclude '.*build/.*'

echo ""
echo "======================================"
echo "Coverage report generated: coverage-report.html"
echo "======================================"

# Try to open the report
if command -v xdg-open &> /dev/null; then
    xdg-open coverage-report.html
elif command -v open &> /dev/null; then
    open coverage-report.html
elif command -v start &> /dev/null; then
    start coverage-report.html
fi
