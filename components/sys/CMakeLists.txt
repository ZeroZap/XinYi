# XinYi System Components

cmake_minimum_required(VERSION 3.10)
project(xy_sys_component C)

set(SYS_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/xy_state_machine/xy_st.c
    ${CMAKE_CURRENT_LIST_DIR}/xy_sys/xy_sys.c
    ${CMAKE_CURRENT_LIST_DIR}/xy_timer/xy_timer.c
)

set(SYS_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/xy_state_machine/xy_st.h
    ${CMAKE_CURRENT_LIST_DIR}/xy_sys/xy_sys.h
    ${CMAKE_CURRENT_LIST_DIR}/xy_timer/xy_timer.h
)

add_library(xy_sys STATIC ${SYS_SOURCES})
add_library(sys_component ALIAS xy_sys)

target_include_directories(xy_sys PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/xy_state_machine
    ${CMAKE_CURRENT_LIST_DIR}/xy_sys
    ${CMAKE_CURRENT_LIST_DIR}/xy_timer
    ${CMAKE_CURRENT_LIST_DIR}/../clib/xy_clib
    ${CMAKE_CURRENT_LIST_DIR}/../kernel/misc
    ${CMAKE_CURRENT_LIST_DIR}/../kernel/osal
)

target_compile_definitions(xy_sys PUBLIC XY_COMPONENT_SYS=1)

install(FILES ${SYS_HEADERS} DESTINATION include/xy_sys)
install(TARGETS xy_sys ARCHIVE DESTINATION lib)
