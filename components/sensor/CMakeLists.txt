# Sensor Component CMakeLists.txt

cmake_minimum_required(VERSION 3.10)

# Sensor component - now properly organized
# All driver implementations are in .c files, .h files contain only declarations

if(NOT XY_XY_SENSOR_ENABLE AND NOT XY_COMPONENT_SENSOR)
    return()
endif()

# Add sources (only include files that exist)
set(SENSOR_SOURCES
    sensor_bus.c
    sensor_core.c
    sensor_calibration.c
    sensor_dma.c
    sensor_fifo.c
    sensor_filter.c
    sensor_fusion.c
    sensor_interrupt.c
    sensor_moton.c
    sensor_power.c
    sensor_self_test.c
    sensor_threshold.c
    sensor_mem_pool.c
    sensor_adt7420.c
)

# Add driver sources from sensors/ directory
file(GLOB SENSOR_DRIVERS "${CMAKE_CURRENT_LIST_DIR}/sensors/sensor_*.c")
# The canonical Device-model drivers these compatibility paths use (MPU6050,
# BMP280, AHT20, AP3216C, ICM20608) are owned by components/drivers and compiled
# by `xy_sensor_drivers_core`.
# They are linked below, not compiled here: a second copy would put the same
# globals in two archives, and archive order - not the build - would decide
# which implementation runs.
list(APPEND SENSOR_SOURCES ${SENSOR_DRIVERS})

add_library(sensor_component STATIC ${SENSOR_SOURCES})
target_include_directories(sensor_component PUBLIC ${CMAKE_CURRENT_LIST_DIR}/inc)
target_include_directories(sensor_component PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(sensor_component PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../device/inc
    ${CMAKE_CURRENT_LIST_DIR}/../drivers/sensor/motion/mpu6050
    ${CMAKE_CURRENT_LIST_DIR}/../drivers/sensor/pressure/bmp280
    ${CMAKE_CURRENT_LIST_DIR}/../drivers/sensor/temperature/aht20
    ${CMAKE_CURRENT_LIST_DIR}/../drivers/sensor/light/ap3216c
    ${CMAKE_CURRENT_LIST_DIR}/../drivers/sensor/motion/icm20608
    ${CMAKE_CURRENT_LIST_DIR}/../hal/inc
    ${CMAKE_CURRENT_LIST_DIR}/../kernel/osal
    ${CMAKE_CURRENT_LIST_DIR}/../trace/xy_log/inc
    ${CMAKE_CURRENT_LIST_DIR}/../clib/xy_clib
)
target_link_libraries(sensor_component PUBLIC hardware_abstraction xy_device
    xy_sensor_drivers_core)

# `xy_device` above is a target, not a library name: if the Device framework is
# absent, CMake would quietly turn it into `-lxy_device` and the archive would
# ship with unresolved `xy_i2c_device_*` references. `COMPONENT_SENSOR` and
# `XY_SENSOR_ENABLE` both select `COMPONENT_DEVICE` in the root Kconfig, so this
# branch must never be taken; if it is, the configure lost that select.
if(NOT TARGET xy_device)
    message(FATAL_ERROR
        "sensor_component requires the canonical Device bus helpers, but target "
        "'xy_device' does not exist in this configuration. COMPONENT_SENSOR and "
        "XY_SENSOR_ENABLE select COMPONENT_DEVICE in the root Kconfig; restore "
        "that select rather than dropping the link.")
endif()
