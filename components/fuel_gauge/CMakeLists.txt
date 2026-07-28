# Fuel Gauge Component
cmake_minimum_required(VERSION 3.10)

if(NOT XY_COMPONENT_FUEL_GAUGE)
    return()
endif()

set(FUEL_GAUGE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/core/fuel_gauge_core.c
    ${CMAKE_CURRENT_SOURCE_DIR}/core/xy_fuel_gauge_safety.c
    ${CMAKE_CURRENT_SOURCE_DIR}/core/xy_fuel_gauge_security.c
    ${CMAKE_CURRENT_SOURCE_DIR}/core/xy_fuel_gauge_status.c
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/xy_fg_bq27z561.c
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/xy_fg_bq27z746.c
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/xy_fg_bq40z50.c
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/xy_fg_max17043.c
)

add_library(xy_fuel_gauge STATIC ${FUEL_GAUGE_SOURCES})
add_library(fuel_gauge_component ALIAS xy_fuel_gauge)

target_include_directories(xy_fuel_gauge PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers
    ${CMAKE_CURRENT_SOURCE_DIR}/../sensor/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../trace/xy_log/inc
)

target_link_libraries(xy_fuel_gauge PUBLIC sensor_component xy_trace)
target_compile_definitions(xy_fuel_gauge PUBLIC XY_COMPONENT_FUEL_GAUGE=1)
