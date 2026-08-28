cmake_minimum_required(VERSION 3.10)
project(SmartHygrometer C)

# 设置 C 标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# XinYi 项目根目录
set(XINYI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

# 包含目录
include_directories(
    ${XINYI_ROOT}/components/trace/xy_log/inc
    ${XINYI_ROOT}/components/kernel/osal/inc
    ${XINYI_ROOT}/components/hal/inc
    ${XINYI_ROOT}/components/device/inc
    ${XINYI_ROOT}/components/drivers/sensor/temperature/sht30
    ${XINYI_ROOT}/components/sensor/inc
)

# 源文件
set(SOURCES
    main.c
    
    # XinYi 组件
    ${XINYI_ROOT}/components/trace/xy_log/src/xy_log.c
    ${XINYI_ROOT}/components/device/src/xy_device.c
    ${XINYI_ROOT}/components/drivers/sensor/temperature/sht30/xy_sht30.c
    ${XINYI_ROOT}/components/sensor/src/xy_bmp280.c
    ${XINYI_ROOT}/components/sensor/src/xy_oled.c
)

# 创建可执行文件
add_executable(smart_hygrometer ${SOURCES})

# 编译选项
target_compile_definitions(smart_hygrometer PRIVATE
    XY_LOG_LEVEL=XY_LOG_LEVEL_INFO
)

# 链接库
# target_link_libraries(smart_hygrometer PRIVATE ...)

# 输出信息
message(STATUS "Building Smart Hygrometer example")
message(STATUS "XinYi root: ${XINYI_ROOT}")
