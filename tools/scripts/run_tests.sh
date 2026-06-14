#!/bin/bash
# XinYi Framework - Run All Tests Script

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"

echo "======================================"
echo "XinYi Framework - Test Runner"
echo "======================================"
echo ""

# Check if build directory exists
if [ ! -d "$PROJECT_ROOT/build" ]; then
    echo "Build directory not found. Configuring..."
    mkdir -p "$PROJECT_ROOT/build"
    cd "$PROJECT_ROOT/build"
    cmake .. -DBUILD_TESTING=ON
fi

cd "$PROJECT_ROOT/build"

echo "Building tests..."
make -j$(nproc)

echo ""
echo "Running tests..."
echo ""

# Run all tests with CTest
ctest --output-on-failure --verbose

echo ""
echo "======================================"
echo "Test run complete!"
echo "======================================"
