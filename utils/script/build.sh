#!/bin/bash
# XinYi Framework - Build Script for Linux/macOS
# Builds the entire project using CMake

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"

echo "======================================"
echo "XinYi Framework - Build Script"
echo "======================================"
echo ""

# Check if CMake is installed
if ! command -v cmake &> /dev/null; then
    echo "Error: CMake is not installed or not in PATH."
    echo "Please install CMake first:"
    echo "  Ubuntu: sudo apt-get install cmake"
    echo "  macOS:  brew install cmake"
    exit 1
fi

echo "Using CMake: $(cmake --version | head -n1)"
echo ""

# Create build directory if not exists
if [ ! -d "$PROJECT_ROOT/build" ]; then
    echo "Creating build directory..."
    mkdir -p "$PROJECT_ROOT/build"
fi

cd "$PROJECT_ROOT/build"

# Configure
echo "Configuring project..."
cmake .. \
    -DBUILD_TESTING=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -DTEST_COVERAGE=ON

# Build
echo ""
echo "Building project..."
make -j$(nproc)

echo ""
echo "======================================"
echo "Build complete!"
echo "======================================"
echo ""
echo "To run tests: ./utils/script/run_tests.sh"
echo "To run coverage: ./utils/script/coverage.sh"
