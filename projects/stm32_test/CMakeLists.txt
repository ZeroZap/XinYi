cmake_minimum_required(VERSION 3.12)

project(stm32_test C)

# STM32F4 系列配置
set(STM32_SERIES "stm32f4" CACHE STRING "STM32 series")
set(STM32_DEVICE "STM32F407xx" CACHE STRING "STM32 device")

# 编译器标志
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D${STM32_DEVICE}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

# 链接脚本 (需要用户根据自己的板子调整)
set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/STM32F407VGTx_FLASH.ld")

# 链接标志
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T${LINKER_SCRIPT}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --specs=nosys.specs")

# 源文件
set(SOURCES
    src/main.c
    src/startup_stm32f407xx.c
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 输出格式
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".elf")

# 生成 bin 文件
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
    COMMENT "Generating ${PROJECT_NAME}.bin"
)

# 生成 hex 文件
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex ${PROJECT_NAME}.elf ${PROJECT_NAME}.hex
    COMMENT "Generating ${PROJECT_NAME}.hex"
)

# 打印大小信息
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}.elf
    COMMENT "Printing size information"
)

message(STATUS "STM32 Test Project configured")
message(STATUS "  Device: ${STM32_DEVICE}")
message(STATUS "  Output: ${PROJECT_NAME}.elf, ${PROJECT_NAME}.bin, ${PROJECT_NAME}.hex")
