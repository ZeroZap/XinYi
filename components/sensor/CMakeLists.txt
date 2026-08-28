# Sensor Component CMakeLists.txt

cmake_minimum_required(VERSION 3.10)

# Sensor component - now properly organized
# All driver implementations are in .c files, .h files contain only declarations

if(NOT XY_XY_SENSOR_ENABLE AND NOT XY_COMPONENT_SENSOR)
    return()
endif()

# Add sources (only include files that exist)
set(SENSOR_SOURCES
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
    sensor_apds9960.c
)

# Add driver sources from sensors/ directory
file(GLOB SENSOR_DRIVERS "${CMAKE_CURRENT_LIST_DIR}/sensors/sensor_*.c")
# Canonical Device-model implementations used by root Sensor compatibility
# paths are linked here so each migrated chip has one implementation owner.
list(APPEND SENSOR_DRIVERS
    "${CMAKE_CURRENT_LIST_DIR}/../drivers/sensor/temperature/sht30/xy_sht30.c"
    "${CMAKE_CURRENT_LIST_DIR}/../drivers/sensor/adc/ads1115/xy_ads1115.c"
    "${CMAKE_CURRENT_LIST_DIR}/../drivers/sensor/motion/mpu6050/xy_mpu6050.c"
)
list(APPEND SENSOR_SOURCES ${SENSOR_DRIVERS})

add_library(sensor_component STATIC ${SENSOR_SOURCES})
target_include_directories(sensor_component PUBLIC ${CMAKE_CURRENT_LIST_DIR}/inc)
target_include_directories(sensor_component PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(sensor_component PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../device/inc
    ${CMAKE_CURRENT_LIST_DIR}/../drivers/sensor/temperature/sht30
    ${CMAKE_CURRENT_LIST_DIR}/../drivers/sensor/adc/ads1115
    ${CMAKE_CURRENT_LIST_DIR}/../drivers/sensor/motion/mpu6050
    ${CMAKE_CURRENT_LIST_DIR}/../hal/inc
    ${CMAKE_CURRENT_LIST_DIR}/../kernel/osal
    ${CMAKE_CURRENT_LIST_DIR}/../trace/xy_log/inc
    ${CMAKE_CURRENT_LIST_DIR}/../clib/xy_clib
)
target_link_libraries(sensor_component PUBLIC hardware_abstraction xy_device)
