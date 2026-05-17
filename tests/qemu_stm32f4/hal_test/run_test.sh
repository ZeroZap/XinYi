#!/bin/bash
# XinYi HAL 统一 API 测试

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "========================================"
echo "XinYi HAL Unified API Test"
echo "========================================"
echo ""

source /home/eugene/zerozap/scripts/env.sh 2>/dev/null || true

echo "[1/3] Compiling..."
arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=softfp \
    -mfpu=fpv4-sp-d16 -O0 -g3 \
    -o hal_test.elf src/main.c src/startup.c \
    -nostdlib -T ../stm32f405rg.ld

echo "      ✓ Compiled: hal_test.elf"
arm-none-eabi-size hal_test.elf | tail -1

echo ""
echo "[2/3] Running on QEMU..."
echo "----------------------------------------"

timeout 10 qemu-system-arm -M olimex-stm32-h405 -nographic \
    -kernel hal_test.elf -semihosting 2>&1 | tee test_output.log

echo "----------------------------------------"

echo ""
echo "[3/3] Test Results:"
echo ""

if grep -q "\[RESULT\] PASS=" test_output.log; then
    RESULT=$(grep "\[RESULT\]" test_output.log)
    PASS=$(echo "$RESULT" | sed 's/.*PASS=\([0-9]*\).*/\1/')
    FAIL=$(echo "$RESULT" | sed 's/.*FAIL=\([0-9]*\).*/\1/')
    
    echo "  PASS: $PASS"
    echo "  FAIL: $FAIL"
    echo ""
    
    if [ "$FAIL" -eq 0 ]; then
        echo "✅ ALL TESTS PASSED"
        exit 0
    else
        echo "❌ SOME TESTS FAILED"
        exit 1
    fi
else
    echo "❌ Failed to parse test results"
    exit 1
fi
