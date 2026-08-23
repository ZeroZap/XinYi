cmake_minimum_required(VERSION 3.12)

# =============================================================================
# Platform selection — must happen before project() for cross-compilation
# =============================================================================
set(HAL_PLATFORM "PC" CACHE STRING "Target platform: PC | STM32F4 | STM32U5 | STM32L4 | WCH | HC32")
set_property(CACHE HAL_PLATFORM PROPERTY STRINGS PC STM32F4 STM32U5 STM32L4 WCH HC32)

if(NOT HAL_PLATFORM STREQUAL "PC")
    # Bare-metal: suppress compiler link-test (no startup/libc yet)
    set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

    set(CMAKE_SYSTEM_NAME Generic)

    if(HAL_PLATFORM STREQUAL "WCH")
        # WCH CH32V — RISC-V 32-bit, MRS-bundled toolchain (riscv32-wch-elf-).
        set(CMAKE_SYSTEM_PROCESSOR riscv)
        set(TOOLCHAIN_PATH
            "/usr/share/MRS2/MRS-linux-x64/resources/app/resources/linux/components/WCH/Toolchain/RISC-V Embedded GCC15/bin"
            CACHE PATH "WCH RISC-V toolchain bin directory")
        set(_TC_PREFIX "riscv32-wch-elf")
    else()
        # STM32 / HC32 — ARM Cortex-M family, arm-none-eabi-gcc.
        set(CMAKE_SYSTEM_PROCESSOR arm)
        set(TOOLCHAIN_PATH "/home/eugene/Tools/arm-gnu-toolchain/bin"
            CACHE PATH "arm-none-eabi toolchain bin directory")
        set(_TC_PREFIX "arm-none-eabi")
    endif()

    set(CMAKE_C_COMPILER   "${TOOLCHAIN_PATH}/${_TC_PREFIX}-gcc"     CACHE FILEPATH "" FORCE)
    set(CMAKE_ASM_COMPILER "${TOOLCHAIN_PATH}/${_TC_PREFIX}-gcc"     CACHE FILEPATH "" FORCE)
    set(CMAKE_AR           "${TOOLCHAIN_PATH}/${_TC_PREFIX}-ar"      CACHE FILEPATH "" FORCE)
    set(CMAKE_RANLIB       "${TOOLCHAIN_PATH}/${_TC_PREFIX}-ranlib"  CACHE FILEPATH "" FORCE)
    set(CMAKE_SIZE         "${TOOLCHAIN_PATH}/${_TC_PREFIX}-size"    CACHE FILEPATH "" FORCE)
    set(CMAKE_OBJCOPY      "${TOOLCHAIN_PATH}/${_TC_PREFIX}-objcopy" CACHE FILEPATH "" FORCE)
    set(CMAKE_OBJDUMP      "${TOOLCHAIN_PATH}/${_TC_PREFIX}-objdump" CACHE FILEPATH "" FORCE)

    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
endif()

# =============================================================================
# Project
# =============================================================================
project(XY_Framework VERSION 1.0.0 LANGUAGES C ASM)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# =============================================================================
# Common compile flags
# =============================================================================
add_compile_options(-Wall -Wextra -Wno-unused-parameter)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-O0 -g3)
    add_compile_definitions(DEBUG XY_DEBUG)
else()
    add_compile_options(-Os)
    add_compile_definitions(NDEBUG)
endif()

if(NOT HAL_PLATFORM STREQUAL "PC")
    add_compile_options(-ffunction-sections -fdata-sections -fno-builtin -ffreestanding)
endif()

# =============================================================================
# Load platform-specific settings (CPU flags / defines / SDK include paths)
# =============================================================================
set(_platform_cmake "${CMAKE_SOURCE_DIR}/cmake/platform/${HAL_PLATFORM}.cmake")
if(NOT EXISTS "${_platform_cmake}")
    message(FATAL_ERROR
        "Unknown HAL_PLATFORM='${HAL_PLATFORM}'.\n"
        "Available: PC  STM32F4  STM32U5  STM32L4  WCH  HC32\n"
        "Pass -DHAL_PLATFORM=<value> to cmake.")
endif()
include("${_platform_cmake}")

# Apply CPU flags (set by platform cmake)
if(CPU_FLAGS)
    add_compile_options(${CPU_FLAGS})
    add_link_options(${CPU_FLAGS})
endif()

# Apply platform-level preprocessor defines
if(PLATFORM_DEFINES)
    add_compile_definitions(${PLATFORM_DEFINES})
endif()

# Apply SDK/HAL include directories
if(PLATFORM_INCLUDE_DIRS)
    include_directories(${PLATFORM_INCLUDE_DIRS})
endif()

# Apply platform linker flags
if(PLATFORM_LINKER_FLAGS)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${PLATFORM_LINKER_FLAGS}")
endif()

# =============================================================================
# Kconfig integration
# =============================================================================
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/Kconfig"
   OR NOT EXISTS "${CMAKE_SOURCE_DIR}/cmake/Kconfig.cmake")
    message(FATAL_ERROR "Kconfig integration is required for this build")
endif()
include("${CMAKE_SOURCE_DIR}/cmake/Kconfig.cmake")

if(CONFIG_FOTA_ENABLED)
    add_compile_definitions(XY_FOTA_ENABLED)
endif()

# =============================================================================
# CMake helper functions (xy_add_srcs etc.)
# =============================================================================
include("${CMAKE_SOURCE_DIR}/cmake/xy_helpers.cmake")

# =============================================================================
# Third-party libraries
# =============================================================================
if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/CMakeLists.txt")
    add_subdirectory(third_party)
endif()

# =============================================================================
# Components — auto-discover top-level; handle nested manually
# =============================================================================

# These directories have no top-level CMakeLists.txt (nested structure)
set(_NESTED_COMPONENTS kernel clib)

file(GLOB _comp_entries LIST_DIRECTORIES true
    "${CMAKE_SOURCE_DIR}/components/*")

foreach(_entry ${_comp_entries})
    if(NOT IS_DIRECTORY "${_entry}")
        continue()
    endif()
    get_filename_component(_name "${_entry}" NAME)

    # Skip third_party (handled above) and nested-only components
    if(_name STREQUAL "third_party" OR _name IN_LIST _NESTED_COMPONENTS)
        continue()
    endif()

    # Keep root target discovery aligned with Kconfig. Component-local guards remain
    # useful for standalone builds, but cannot prevent unconditional subdirectories
    # (for example Crypto's algorithm libraries) from leaking into an all-off build.
    set(_component_enabled TRUE)
    if(_name STREQUAL "device" AND NOT XY_COMPONENT_DEVICE)
        set(_component_enabled FALSE)
    elseif(_name STREQUAL "charger" AND NOT XY_COMPONENT_CHARGER)
        set(_component_enabled FALSE)
    elseif(_name STREQUAL "crypto" AND NOT XY_COMPONENT_CRYPTO)
        set(_component_enabled FALSE)
    elseif(_name STREQUAL "dm" AND NOT XY_COMPONENT_DM)
        set(_component_enabled FALSE)
    elseif(_name STREQUAL "sensor" AND NOT XY_XY_SENSOR_ENABLE AND NOT XY_COMPONENT_SENSOR)
        set(_component_enabled FALSE)
    elseif(_name STREQUAL "actuator" AND NOT XY_XY_ACTUATOR_ENABLE AND NOT XY_COMPONENT_ACTUATOR)
        set(_component_enabled FALSE)
    elseif(_name STREQUAL "drivers"
           AND NOT XY_DRIVER_SENSOR
           AND NOT XY_DRIVER_DISPLAY
           AND NOT XY_DRIVER_STORAGE
           AND NOT XY_DRIVER_POWER
           AND NOT XY_DRIVER_WIRELESS
           AND NOT XY_DRIVER_SYSTEM)
        set(_component_enabled FALSE)
    elseif(_name STREQUAL "gui" AND NOT XY_GUI_ENABLED)
        set(_component_enabled FALSE)
    elseif(_name STREQUAL "net" AND NOT XY_NETWORK)
        set(_component_enabled FALSE)
    elseif(_name STREQUAL "fota" AND NOT XY_FOTA_ENABLED)
        set(_component_enabled FALSE)
    elseif(_name STREQUAL "pm" AND NOT XY_PM_ENABLED)
        set(_component_enabled FALSE)
    elseif(_name STREQUAL "pid" AND NOT XY_COMPONENT_PID)
        set(_component_enabled FALSE)
    elseif(_name STREQUAL "mux" AND NOT XY_COMPONENT_MUX)
        set(_component_enabled FALSE)
    elseif(_name STREQUAL "sys" AND NOT XY_COMPONENT_SYS)
        set(_component_enabled FALSE)
    elseif(_name STREQUAL "fuel_gauge" AND NOT XY_COMPONENT_FUEL_GAUGE)
        set(_component_enabled FALSE)
    endif()

    if(_component_enabled AND EXISTS "${_entry}/CMakeLists.txt")
        add_subdirectory("${_entry}" "${CMAKE_BINARY_DIR}/components/${_name}")
    endif()
endforeach()

# kernel sub-components
foreach(_sub osal misc)
    set(_dir "${CMAKE_SOURCE_DIR}/components/kernel/${_sub}")
    if(EXISTS "${_dir}/CMakeLists.txt")
        add_subdirectory("${_dir}" "${CMAKE_BINARY_DIR}/components/kernel/${_sub}")
    endif()
endforeach()

# clib sub-component
set(_dir "${CMAKE_SOURCE_DIR}/components/clib/xy_clib")
if(EXISTS "${_dir}/CMakeLists.txt")
    add_subdirectory("${_dir}" "${CMAKE_BINARY_DIR}/components/clib/xy_clib")
endif()

# =============================================================================
# Tests
# =============================================================================
if(BUILD_TESTING)
    enable_testing()
    if(EXISTS "${CMAKE_SOURCE_DIR}/tests/CMakeLists.txt")
        add_subdirectory(tests)
    endif()
endif()

# =============================================================================
# Configuration summary
# =============================================================================
message(STATUS "")
message(STATUS "┌─ XinYi Framework v${PROJECT_VERSION} ───────────────────")
message(STATUS "│  Platform  : ${HAL_PLATFORM}")
message(STATUS "│  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "│  Testing   : ${BUILD_TESTING}")
message(STATUS "│  FOTA      : ${CONFIG_FOTA_ENABLED}")
message(STATUS "└────────────────────────────────────────────────")
message(STATUS "")
