# XinYi QEMU 测试示例
# 目标：在 QEMU Cortex-M3/M4 仿真器上运行

cmake_minimum_required(VERSION 3.14)

# 计算项目根目录
get_filename_component(XINYI_ROOT "../../" ABSOLUTE)

# 加载 XinYi 核心配置
include(${XINYI_ROOT}/cmake/xinyi_core.cmake OPTIONAL)

project(qemu_test C ASM)

# 设置编译选项
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O0 -g3")

# 收集源文件
file(GLOB_RECURSE SOURCES "src/*.c")

# 添加 XinYi 核心库
list(APPEND SOURCES
    ${XINYI_ROOT}/core/xy_core.c
    ${XINYI_ROOT}/hal/xy_hal.c
)

# 包含目录
include_directories(
    ${XINYI_ROOT}/include
    ${XINYI_ROOT}/core
    ${XINYI_ROOT}/hal
    src
)

# 创建可执行文件
add_executable(qemu_test.elf ${SOURCES})

# 链接选项
target_link_options(qemu_test.elf PRIVATE
    -T${XINYI_ROOT}/cmake/stm32_flash.ld
    --specs=nosys.specs
    -Wl,--gc-sections
)

# 输出格式
set_target_properties(qemu_test.elf PROPERTIES
    SUFFIX ".elf"
)

# 生成 bin/hex 文件
add_custom_command(TARGET qemu_test.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary qemu_test.elf qemu_test.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex qemu_test.elf qemu_test.hex
    COMMENT "Generating .bin and .hex files"
)
