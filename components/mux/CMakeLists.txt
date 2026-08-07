# MUX Component

if(NOT XY_COMPONENT_MUX)
    return()
endif()

set(MUX_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/xy_mux.c
    ${CMAKE_CURRENT_LIST_DIR}/src/xy_mux_gpio.c
    ${CMAKE_CURRENT_LIST_DIR}/src/xy_mux_i2c.c
    ${CMAKE_CURRENT_LIST_DIR}/src/xy_mux_spi.c
    ${CMAKE_CURRENT_LIST_DIR}/src/xy_mux_uart.c
)

set(MUX_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/inc/xy_mux.h
    ${CMAKE_CURRENT_LIST_DIR}/inc/xy_mux_gpio.h
    ${CMAKE_CURRENT_LIST_DIR}/inc/xy_mux_i2c.h
    ${CMAKE_CURRENT_LIST_DIR}/inc/xy_mux_log.h
    ${CMAKE_CURRENT_LIST_DIR}/inc/xy_mux_spi.h
    ${CMAKE_CURRENT_LIST_DIR}/inc/xy_mux_uart.h
)

add_library(xy_mux STATIC ${MUX_SOURCES})
add_library(mux_component ALIAS xy_mux)

target_include_directories(xy_mux PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/inc
    ${CMAKE_SOURCE_DIR}/components/trace/xy_log/inc
)

target_link_libraries(xy_mux PUBLIC xy_trace)
target_compile_definitions(xy_mux PUBLIC XY_COMPONENT_MUX=1)

set(MUX_SOURCES ${MUX_SOURCES} PARENT_SCOPE)
set(MUX_HEADERS ${MUX_HEADERS} PARENT_SCOPE)
set(MUX_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/inc PARENT_SCOPE)
