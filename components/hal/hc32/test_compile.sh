#!/bin/bash
# HC32 HAL Syntax Check (No Linking)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
XINYI_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
XHYSC_ROOT="$(cd "$SCRIPT_DIR/../../../../xhsc" && pwd)"

echo "========================================"
echo "XinYi HAL - HC32 Syntax Check"
echo "========================================"
echo "Source: $SCRIPT_DIR/hc32l021/xy_hal_gpio_device.c"
echo ""

# ARM GCC Path
ARM_GCC="/usr/share/MRS2/MRS-linux-x64/resources/app/resources/linux/components/WCH/Toolchain/arm-none-eabi-gcc/bin/arm-none-eabi-gcc"

echo "Using: $ARM_GCC"
echo ""

# Compile (no link, just syntax check)
"$ARM_GCC" \
    -c \
    -mcpu=cortex-m0plus \
    -mthumb \
    -std=c99 \
    -Wall \
    -Wextra \
    -Wno-unused-parameter \
    -I"$XINYI_ROOT/components/hal/inc" \
    -I"$XHYSC_ROOT/HC32L021/mcu/common" \
    -DHC32L021 \
    -D__ARM_ARCH=6M \
    "$SCRIPT_DIR/hc32l021/xy_hal_gpio_device.c" \
    -o /tmp/xy_hal_gpio_device.o

echo "✅ Syntax check passed!"
echo "Output: /tmp/xy_hal_gpio_device.o"
