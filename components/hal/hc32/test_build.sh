#!/bin/bash
# HC32 HAL Test Build Script

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
XINYI_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
XHYSC_ROOT="$(cd "$SCRIPT_DIR/../../../../xhsc" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build_test"

echo "========================================"
echo "XinYi HAL - HC32 Test Build"
echo "========================================"
echo "XinYi Root: $XINYI_ROOT"
echo "xhsc Root:  $XHYSC_ROOT"
echo "Build Dir:  $BUILD_DIR"
echo ""

# Check xhsc library
if [ ! -d "$XHYSC_ROOT/HC32L021/driver/inc" ]; then
    echo "❌ Error: xhsc library not found!"
    echo "   Please ensure xhsc is at: $XHYSC_ROOT"
    exit 1
fi
echo "✅ xhsc library found"

# Create build directory
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo ""
echo "Configuring with CMake..."
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../../../cmake/arm-gcc.cmake \
    -DHC32_SERIES=HC32L021 \
    -DXHYSC_ROOT="$XHYSC_ROOT" \
    -DCMAKE_BUILD_TYPE=Debug

# Build
echo ""
echo "Building..."
make VERBOSE=1

echo ""
echo "========================================"
echo "✅ Build completed successfully!"
echo "========================================"
