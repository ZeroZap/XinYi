# WCH HAL CMakeLists.txt

cmake_minimum_required(VERSION 3.10)

# WCH HAL 源文件 (通用驱动)
set(WCH_HAL_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/xy_hal_gpio.c
    ${CMAKE_CURRENT_LIST_DIR}/src/xy_hal_uart.c
    ${CMAKE_CURRENT_LIST_DIR}/src/xy_hal_spi.c
    ${CMAKE_CURRENT_LIST_DIR}/src/xy_hal_i2c.c
    ${CMAKE_CURRENT_LIST_DIR}/src/xy_hal_adc.c
    ${CMAKE_CURRENT_LIST_DIR}/src/xy_hal_pwm.c
    ${CMAKE_CURRENT_LIST_DIR}/src/xy_hal_can.c
)

# 添加 WCH HAL 库
add_library(xy_hal_wch STATIC ${WCH_HAL_SOURCES})

target_include_directories(xy_hal_wch PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/../inc
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_compile_definitions(xy_hal_wch PRIVATE
    XY_HAL_PLATFORM_WCH=1
)

# 编译优化：使用 O2 优化
target_compile_options(xy_hal_wch PRIVATE
    -O2
    -Wall
    -Wextra
)
