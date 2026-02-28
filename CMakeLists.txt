cmake_minimum_required(VERSION 3.12)
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

option(BUILD_TESTING "Build tests" OFF)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

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
            add_subdirectory(components/${component_name})
        endif()
    endif()
endforeach()

# Add kernel sub-components
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/components/kernel/osal/CMakeLists.txt)
    add_subdirectory(components/kernel/osal)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/components/kernel/misc/CMakeLists.txt)
    add_subdirectory(components/kernel/misc)
endif()

# Add HAL sub-components
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/components/hal/stm32/CMakeLists.txt)
    add_subdirectory(components/hal/stm32)
endif()

# Add clib sub-components
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/components/clib/xy_clib/CMakeLists.txt)
    add_subdirectory(components/clib/xy_clib)
endif()

# Add crypto sub-components
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/components/crypto/CMakeLists.txt)
    add_subdirectory(components/crypto)
endif()

# Add dm sub-components
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/components/dm/CMakeLists.txt)
    add_subdirectory(components/dm)
endif()

# Add net sub-components
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/components/net/CMakeLists.txt)
    add_subdirectory(components/net)
endif()

# Add trace sub-components
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/components/trace/CMakeLists.txt)
    add_subdirectory(components/trace)
endif()

# Add tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
    add_subdirectory(tests)
endif()

# ==================== Testing ====================

if(BUILD_TESTING)
    enable_testing()
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
        add_subdirectory(tests)
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
