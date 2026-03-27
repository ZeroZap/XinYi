#!/bin/bash
# AT Client Test 独立构建脚本

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build_atc_test"

echo "=== AT Client Test 构建脚本 ==="

# 清理并创建目录
rm -rf $BUILD_DIR
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# 创建简化 CMakeLists
cat > CMakeLists.txt << CMAKE
cmake_minimum_required(VERSION 3.10)
project(atc_test C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "\${CMAKE_C_FLAGS} -Wall -Wextra -DXY_PLATFORM_PC=1 -DXY_LOG_LEVEL=3")

include_directories(
    $SCRIPT_DIR/components/inc
    $SCRIPT_DIR/components/hal/inc
    $SCRIPT_DIR/components/kernel/misc/inc
    $SCRIPT_DIR/components/kernel/misc
    $SCRIPT_DIR/components/kernel/osal/inc
    $SCRIPT_DIR/components/kernel/osal/src
    $SCRIPT_DIR/components/kernel/osal/backend/baremetal
    $SCRIPT_DIR/components/net/at/atc/Modules/cellular/common
    $SCRIPT_DIR/components/trace/xy_log/inc
    $SCRIPT_DIR/components/clib/xy_clib/inc
    $SCRIPT_DIR/components/clib/xy_clib/src
)

set(AT_SRC
    $SCRIPT_DIR/components/net/at/atc/Modules/cellular/common/at_client.c
)

set(OSAL_SRC
    $SCRIPT_DIR/components/kernel/osal/backend/baremetal/xy_os_baremetal.c
    $SCRIPT_DIR/components/kernel/osal/src/osal_baremetal.c
    $SCRIPT_DIR/components/kernel/osal/src/xy_os_tick.c
    $SCRIPT_DIR/components/kernel/osal/src/xy_os_timer_sw.c
)

set(MISC_SRC
    $SCRIPT_DIR/components/kernel/misc/xy_tick.c
    $SCRIPT_DIR/components/kernel/misc/xy_timer_sw.c
    $SCRIPT_DIR/components/kernel/misc/src/xy_autotask.c
    $SCRIPT_DIR/components/kernel/misc/src/xy_sysmon.c
)

set(CLIB_SRC
    $SCRIPT_DIR/components/clib/xy_clib/src/xy_stdio.c
    $SCRIPT_DIR/components/clib/xy_clib/src/xy_string.c
    $SCRIPT_DIR/components/clib/xy_clib/src/xy_stdlib.c
    $SCRIPT_DIR/components/clib/xy_clib/src/xy_math.c
    $SCRIPT_DIR/components/clib/xy_clib/src/xy_common.c
    $SCRIPT_DIR/components/clib/xy_clib/src/xy_assert.c
    $SCRIPT_DIR/components/clib/xy_clib/src/xy_ctype.c
    $SCRIPT_DIR/components/clib/xy_clib/src/xy_rb.c
    $SCRIPT_DIR/components/clib/xy_clib/src/xy_sort.c
    $SCRIPT_DIR/components/clib/xy_clib/src/xy_filter.c
)

set(LOG_SRC
    $SCRIPT_DIR/components/trace/xy_log/src/xy_log.c
)

add_executable(test_at_client 
    $SCRIPT_DIR/tests/at_client/test_at_client.c
    \${AT_SRC}
    \${OSAL_SRC}
    \${MISC_SRC}
    \${CLIB_SRC}
    \${LOG_SRC}
)

target_link_libraries(test_at_client m pthread dl)
CMAKE

# 构建
cmake .
make -j\$(nproc)

echo ""
echo "=== 构建完成 ==="
echo "运行测试: ./test_at_client"
