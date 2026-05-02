cmake_minimum_required(VERSION 3.12)

# 禁用编译器检查 (裸机环境不需要)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# ==================== Kconfig Integration ====================

# Include Kconfig parser before project() to configure build
if(EXISTS ${CMAKE_SOURCE_DIR}/Kconfig)
    include(${CMAKE_SOURCE_DIR}/cmake/Kconfig.cmake)
endif()

project(XY_Framework VERSION 1.0.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler options
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ==================== Build Options ====================

# Override from Kconfig if available
if(DEFINED XY_BUILD_TESTING)
    set(BUILD_TESTING ${XY_BUILD_TESTING})
endif()
if(DEFINED XY_BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS ${XY_BUILD_SHARED_LIBS})
endif()

# Don't redefine BUILD_TESTING as an option - it should come from Kconfig
# option(BUILD_TESTING "Build tests" ${BUILD_TESTING})
# option(BUILD_SHARED_LIBS "Build shared libraries" ${BUILD_SHARED_LIBS})

# ==================== Third Party Libraries ====================

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/CMakeLists.txt)
    add_subdirectory(third_party)
endif()

# ==================== Components ====================

# Auto-detect and add all component subdirectories
file(GLOB COMPONENT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/components/*)
foreach(component_dir ${COMPONENT_DIRS})
    if(IS_DIRECTORY ${component_dir})
        get_filename_component(component_name ${component_dir} NAME)
        if(EXISTS ${component_dir}/CMakeLists.txt)
            add_subdirectory(${component_dir} ${CMAKE_CURRENT_BINARY_DIR}/components/${component_name})
        endif()
    endif()
endforeach()

# Add kernel sub-components (nested under components/kernel)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/components/kernel/osal/CMakeLists.txt)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/components/kernel/osal ${CMAKE_CURRENT_BINARY_DIR}/components/kernel/osal)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/components/kernel/misc/CMakeLists.txt)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/components/kernel/misc ${CMAKE_CURRENT_BINARY_DIR}/components/kernel/misc)
endif()

# Add HAL sub-components (nested under components/hal)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/components/hal/stm32/CMakeLists.txt)
    #add_subdirectory hal/stm32)
endif()

# Add clib sub-components (nested under components/clib)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/components/clib/xy_clib/CMakeLists.txt)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/components/clib/xy_clib ${CMAKE_CURRENT_BINARY_DIR}/components/clib/xy_clib)
endif()

# Add tests - RE-ENABLED FOR STM32F4 VALIDATION
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
    # Only build AT Server test manually
    add_executable(at_server_test tests/at_server_test.c)
    target_link_libraries(at_server_test xy_net xy_misc xy_osal xy_trace xy_xy_clib)
    target_include_directories(at_server_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/components/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/components/hal/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/components/kernel/misc/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/components/kernel/misc
        ${CMAKE_CURRENT_SOURCE_DIR}/components/kernel/osal/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/components/net/at/ats
        ${CMAKE_CURRENT_SOURCE_DIR}/components/trace/xy_log/inc
    )
    target_compile_definitions(at_server_test PRIVATE XY_PLATFORM_PC XY_LOG_LEVEL=3)
endif()

# Add examples (optional) - RE-ENABLED FOR STM32F4 VALIDATION
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/examples/at_server_example/CMakeLists.txt)
    add_subdirectory(examples/at_server_example ${CMAKE_CURRENT_BINARY_DIR}/examples/at_server_example)
endif()

# MQTT Example
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/examples/net_mqtt_example/CMakeLists.txt)
    add_subdirectory(examples/net_mqtt_example ${CMAKE_CURRENT_BINARY_DIR}/examples/net_mqtt_example)
endif()

# ==================== Testing ====================

if(BUILD_TESTING)
    enable_testing()
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
    endif()
endif()

# ==================== Top-level Targets ====================

add_custom_target(build_all
    COMMENT "Building all components"
)

add_custom_target(clean_all
    COMMAND ${CMAKE_BUILD_TOOL} clean
    COMMENT "Cleaning all components"
)

if(BUILD_TESTING)
    add_custom_target(test_all
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        DEPENDS build_all
        COMMENT "Testing all components"
    )
endif()

# ==================== Installation ====================

install(DIRECTORY components/
    DESTINATION include/xy_framework
    FILES_MATCHING PATTERN "*.h"
    PATTERN "test*" EXCLUDE
    PATTERN "tests" EXCLUDE
)

install(DIRECTORY third_party/unity/
    DESTINATION include/xy_framework/unity
    FILES_MATCHING PATTERN "*.h"
)

# ==================== Packaging ====================

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "XinYi Embedded Framework")
set(CPACK_PACKAGE_VENDOR "XinYi Team")
set(CPACK_PACKAGE_CONTACT "xinyi@example.com")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
include(CPack)

# ==================== Summary ====================

message(STATUS "")
message(STATUS "XinYi Framework Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build Testing: ${BUILD_TESTING}")
message(STATUS "  Shared Libraries: ${BUILD_SHARED_LIBS}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "")

# STM32F4 SDK 包含路径 (skip for PC platform)
if(NOT CONFIG_PLATFORM_PC)
if(EXISTS ${CMAKE_SOURCE_DIR}/MCU/ST/STM32F4/Drivers/STM32F4xx_HAL_Driver/Inc)
    include_directories(
        ${CMAKE_SOURCE_DIR}/MCU/ST/STM32F4/Drivers/CMSIS/Device/ST/STM32F4xx/Include
        ${CMAKE_SOURCE_DIR}/MCU/ST/STM32F4/Drivers/STM32F4xx_HAL_Driver/Inc
    )
    add_definitions(-DSTM32F407xx -DUSE_HAL_DRIVER)
    message(STATUS "STM32F4 SDK found - HAL Driver enabled")
endif()
endif()

# Simple GPIO test for PC platform
add_executable(simple_gpio_test tests/simple_gpio_test.c)
target_link_libraries(simple_gpio_test xy_hal)
target_include_directories(simple_gpio_test PRIVATE components/hal/inc components/hal/PC)
target_include_directories(simple_gpio_test PRIVATE components/hal/PC)

# Add tests subdirectory if enabled
if(BUILD_TESTING)
    add_subdirectory(tests ${CMAKE_CURRENT_BINARY_DIR}/tests)
endif()
