cmake_minimum_required(VERSION 3.10)
project(WeatherStation C)

set(CMAKE_C_STANDARD 99)

# XinYi 根目录
set(XINYI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")

# 包含目录
include_directories(
    ${XINYI_ROOT}/components/trace/xy_log/inc
    ${XINYI_ROOT}/components/kernel/osal/inc
    ${XINYI_ROOT}/components/hal/inc
    ${XINYI_ROOT}/components/device/inc
    ${XINYI_ROOT}/components/drivers/sensor/temperature/sht30
    ${XINYI_ROOT}/components/sensor/inc
    ${XINYI_ROOT}/components/pid/inc
    ${XINYI_ROOT}/components/dm/inc
    ${XINYI_ROOT}/components/net/inc
    ${XINYI_ROOT}/components/fota/inc
)

# 源文件
set(SOURCES
    main.c
    
    # XinYi 组件
    ${XINYI_ROOT}/components/trace/xy_log/src/xy_log.c
    ${XINYI_ROOT}/components/drivers/sensor/temperature/sht30/xy_sht30.c
    ${XINYI_ROOT}/components/sensor/src/xy_mpu6050.c
    ${XINYI_ROOT}/components/sensor/src/xy_oled_ssd1306.c
    ${XINYI_ROOT}/components/pid/src/xy_pid.c
    ${XINYI_ROOT}/components/dm/src/xy_mq.c
    ${XINYI_ROOT}/components/fota/src/xy_fota.c
)

# 创建可执行文件
add_executable(weather_station ${SOURCES})

# 编译选项
target_compile_definitions(weather_station PRIVATE
    XY_LOG_LEVEL=XY_LOG_LEVEL_INFO
)

message(STATUS "Building Weather Station Example")
