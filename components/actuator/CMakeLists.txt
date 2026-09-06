# Actuator Component CMakeLists.txt
# 执行器框架 - 继电器、舵机、电机等

cmake_minimum_required(VERSION 3.10)

if(NOT XY_XY_ACTUATOR_ENABLE AND NOT XY_COMPONENT_ACTUATOR)
    return()
endif()

# Actuator sources
set(ACTUATOR_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/xy_actuator.c
    ${CMAKE_CURRENT_SOURCE_DIR}/xy_actuator_buzzer.c
)

# Add library
add_library(xy_actuator STATIC ${ACTUATOR_SOURCES})

# Backward-compatible component target name used by top-level builds.
add_library(actuator_component ALIAS xy_actuator)

target_include_directories(xy_actuator PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(xy_actuator PRIVATE xy_hal)

target_compile_definitions(xy_actuator PRIVATE XY_COMPONENT_ACTUATOR=1)
target_compile_options(xy_actuator PRIVATE -Wall -Wextra)
