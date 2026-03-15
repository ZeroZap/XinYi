# WCH HAL CMakeLists.txt

cmake_minimum_required(VERSION 3.10)

# WCH CH32U5 HAL 源文件
set(WCH_HAL_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/ch32u5/xy_hal_gpio_device.c
    ${CMAKE_CURRENT_LIST_DIR}/ch32u5/xy_hal_uart_device.c
    ${CMAKE_CURRENT_LIST_DIR}/ch32u5/xy_hal_spi_device.c
    ${CMAKE_CURRENT_LIST_DIR}/ch32u5/xy_hal_i2c_device.c
)

# 添加 WCH HAL 库
add_library(xy_hal_wch STATIC ${WCH_HAL_SOURCES})

target_include_directories(xy_hal_wch PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/../inc
    ${CMAKE_CURRENT_LIST_DIR}/ch32u5
    ${WCH_HAL_PATH}/Peripheral/CH32U5xx/Inc
)

target_compile_definitions(xy_hal_wch PRIVATE
    USE_HAL_DRIVER
    CH32U575
    XY_HAL_PLATFORM_WCH=1
)

# 编译优化：使用 O2 优化
target_compile_options(xy_hal_wch PRIVATE
    -O2
    -Wall
    -Wextra
)
