# Sensor Component CMakeLists.txt

cmake_minimum_required(VERSION 3.10)

if(NOT XY_SENSOR_ENABLE AND NOT XY_COMPONENT_SENSOR)
    return()
endif()

# Sensor sources
set(SENSOR_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/sensor_core.c
    ${CMAKE_CURRENT_SOURCE_DIR}/
    ${CMAKE_CURRENT_SOURCE_DIR}/sensor_calibration.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sensor_filter.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sensor_fifo.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sensor_interrupt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sensor_threshold.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sensor_power.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sensor_mem_pool.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sensor_self_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sensor_fusion.c
)

# Platform-specific sources
if(PLATFORM_PC)
    list(APPEND SENSOR_SOURCES 
        ${CMAKE_CURRENT_SOURCE_DIR}/sensor_moton.c
    )
endif()

add_library(xy_sensor STATIC ${SENSOR_SOURCES})

target_include_directories(xy_sensor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(xy_sensor PUBLIC xy_trace xy_hal)

target_compile_definitions(xy_sensor PRIVATE XY_COMPONENT_SENSOR=1)
target_compile_options(xy_sensor PRIVATE -Wall -Wextra)
