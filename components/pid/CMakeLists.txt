# XinYi PID Controller

if(NOT XY_COMPONENT_PID AND NOT COMPONENT_PID)
    return()
endif()

set(PID_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/xy_pid.c
    ${CMAKE_CURRENT_LIST_DIR}/src/xy_pid_auto.c
)

set(PID_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/xy_pid.h
    ${CMAKE_CURRENT_LIST_DIR}/inc/xy_pid.h
    ${CMAKE_CURRENT_LIST_DIR}/inc/xy_pid_auto.h
    ${CMAKE_CURRENT_LIST_DIR}/inc/xy_pid_types.h
)

add_library(xy_pid STATIC ${PID_SOURCES})

target_include_directories(xy_pid PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/inc
    ${CMAKE_SOURCE_DIR}/components/kernel/osal
    ${CMAKE_SOURCE_DIR}/components/trace/xy_log/inc
)

target_link_libraries(xy_pid PRIVATE xy_osal xy_trace m)
target_compile_options(xy_pid PRIVATE -Wall -Wextra)

set(PID_SOURCES ${PID_SOURCES} PARENT_SCOPE)
set(PID_HEADERS ${PID_HEADERS} PARENT_SCOPE)
set(PID_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/inc PARENT_SCOPE)
