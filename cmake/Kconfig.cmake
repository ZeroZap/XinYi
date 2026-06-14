# Kconfig CMake Integration
# Parses Kconfig and generates build configuration

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(KCONFIG_OVERRIDES "" CACHE STRING "Semicolon-separated Kconfig overrides, e.g. BUILD_TESTING=ON;FOTA_ENABLED=ON")

# Kconfig parser script
set(KCONFIG_PARSER ${CMAKE_SOURCE_DIR}/cmake/kconfig_parser.py)
set(KCONFIG_FILE ${CMAKE_SOURCE_DIR}/Kconfig)
set(CONFIG_FILE ${CMAKE_BINARY_DIR}/.config)
set(AUTOCONF_FILE ${CMAKE_BINARY_DIR}/include/autoconf.h)
set(CONFIG_CMAKE_FILE ${CMAKE_BINARY_DIR}/config.cmake)

# Create output directories
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/include)

# Parse Kconfig and generate configuration files
set(_kconfig_override_args)
foreach(_override IN LISTS KCONFIG_OVERRIDES)
    if(_override)
        list(APPEND _kconfig_override_args --set "${_override}")
    endif()
endforeach()

message(STATUS "Parsing Kconfig...")
execute_process(
    COMMAND ${Python3_EXECUTABLE} ${KCONFIG_PARSER}
            --kconfig ${KCONFIG_FILE}
            --output ${CONFIG_FILE}
            --autoconf ${AUTOCONF_FILE}
            --cmake ${CONFIG_CMAKE_FILE}
            --platform ${HAL_PLATFORM}
            ${_kconfig_override_args}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE KCONFIG_RESULT
)

if(NOT KCONFIG_RESULT EQUAL 0)
    message(FATAL_ERROR "Kconfig parsing failed")
endif()

# Include generated CMake configuration
if(EXISTS ${CONFIG_CMAKE_FILE})
    include(${CONFIG_CMAKE_FILE})
endif()

include_directories(${CMAKE_BINARY_DIR}/include)

set(BUILD_TESTING ${CONFIG_BUILD_TESTING} CACHE BOOL "Build tests" FORCE)
set(BUILD_SHARED_LIBS ${CONFIG_BUILD_SHARED_LIBS} CACHE BOOL "Build shared libraries" FORCE)

# Print configuration summary
message(STATUS "")
message(STATUS "=== XinYi Configuration Summary ===")
message(STATUS "Platform:        ${HAL_PLATFORM}")
message(STATUS "GUI Enabled:     ${CONFIG_GUI_ENABLED}")
message(STATUS "FOTA Enabled:    ${CONFIG_FOTA_ENABLED}")
message(STATUS "Tests Enabled:   ${CONFIG_BUILD_TESTING}")
message(STATUS "Log Level:       ${CONFIG_LOG_LEVEL}")
message(STATUS "Build Type:      ${CMAKE_BUILD_TYPE}")
message(STATUS "")
