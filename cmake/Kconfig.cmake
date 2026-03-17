# Kconfig CMake Integration
# Parses Kconfig and generates build configuration

find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Kconfig parser script
set(KCONFIG_PARSER ${CMAKE_SOURCE_DIR}/cmake/kconfig_parser.py)
set(KCONFIG_FILE ${CMAKE_SOURCE_DIR}/Kconfig)
set(CONFIG_FILE ${CMAKE_BINARY_DIR}/.config)
set(AUTOCONF_FILE ${CMAKE_BINARY_DIR}/include/autoconf.h)
set(CONFIG_CMAKE_FILE ${CMAKE_BINARY_DIR}/config.cmake)

# Create output directories
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/include)

# Parse Kconfig and generate configuration files
message(STATUS "Parsing Kconfig...")
execute_process(
    COMMAND ${Python3_EXECUTABLE} ${KCONFIG_PARSER}
            --kconfig ${KCONFIG_FILE}
            --output ${CONFIG_FILE}
            --autoconf ${AUTOCONF_FILE}
            --cmake ${CONFIG_CMAKE_FILE}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE KCONFIG_RESULT
)

if(NOT KCONFIG_RESULT EQUAL 0)
    message(WARNING "Kconfig parsing failed, using defaults")
endif()

# Include generated CMake configuration
if(EXISTS ${CONFIG_CMAKE_FILE})
    include(${CONFIG_CMAKE_FILE})
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "=== XinYi Configuration Summary ===")
message(STATUS "Platform:        $ENV{XY_PLATFORM}")
message(STATUS "GUI Enabled:     $ENV{XY_GUI_ENABLED}")
message(STATUS "Log Level:       $ENV{XY_LOG_LEVEL}")
message(STATUS "Build Type:      ${CMAKE_BUILD_TYPE}")
message(STATUS "")
