#!/bin/bash
# Build script for XY Framework

# Function to show usage
show_usage() {
    echo "Usage: $0 [make|cmake] [target]"
    echo "  make  - Use Make build system"
    echo "  cmake - Use CMake build system"
    echo ""
    echo "Targets:"
    echo "  all     - Build all components (default)"
    echo "  clean   - Clean build artifacts"
    echo "  test    - Run tests"
    echo "  install - Install components"
    echo ""
    echo "Examples:"
    echo "  $0 make all"
    echo "  $0 cmake test"
}

# Check arguments
if [ $# -lt 1 ]; then
    show_usage
    exit 1
fi

BUILD_SYSTEM=$1
TARGET=${2:-all}

# Build based on system
case $BUILD_SYSTEM in
    make)
        echo "Building with Make..."
        make $TARGET
        ;;
    cmake)
        echo "Building with CMake..."
        mkdir -p build
        cd build
        cmake ..
        make $TARGET
        ;;
    *)
        echo "Unknown build system: $BUILD_SYSTEM"
        show_usage
        exit 1
        ;;
esac