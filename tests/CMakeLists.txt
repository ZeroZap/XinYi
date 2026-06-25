# XinYi Framework Tests CMake Configuration
cmake_minimum_required(VERSION 3.10)

set(NET_UNIT_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/unit/net)

# AT Client Test
add_executable(test_at_client
    ${NET_UNIT_TEST_DIR}/test_at_client.c
    ${CMAKE_CURRENT_SOURCE_DIR}/unity/unity.c
)
target_link_libraries(test_at_client xy_net xy_misc xy_osal xy_trace xy_xy_clib m pthread)
target_include_directories(test_at_client PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/hal/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/kernel/misc/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/kernel/misc
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/kernel/osal/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/kernel/osal/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/kernel/osal/backend/baremetal
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/net/at/atc/Modules/cellular/common
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/trace/xy_log/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/clib/xy_clib/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/clib/xy_clib
    ${CMAKE_CURRENT_SOURCE_DIR}/unity
)
target_compile_definitions(test_at_client PRIVATE XY_PLATFORM_PC=1 XY_LOG_LEVEL=3)

# AT Server Test
add_executable(test_at_server
    ${NET_UNIT_TEST_DIR}/at_server_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/unity/unity.c
)
target_link_libraries(test_at_server xy_net xy_misc xy_osal xy_trace xy_xy_clib m pthread)
target_include_directories(test_at_server PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/hal/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/kernel/misc/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/kernel/misc
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/kernel/osal/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/net/at/ats
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/trace/xy_log/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/unity
)
target_compile_definitions(test_at_server PRIVATE XY_PLATFORM_PC=1 XY_LOG_LEVEL=3)
