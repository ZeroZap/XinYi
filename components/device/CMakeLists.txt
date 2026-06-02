cmake_minimum_required(VERSION 3.10)
project(xy_device C)

set(CMAKE_C_STANDARD 99)

# Include directories
include_directories(
    inc
    ../hal/inc
    ../kernel/osal
    ../trace/xy_log/inc
)

# Device framework sources.
#
# Keep the model close to RT-Thread/Zephyr layering:
# - src/xy_device.c: public device lifecycle and generic I/O dispatch
# - xy_device_core.c: registry, reference counting, and idle PM state
# - xy_device.c: compatibility bus helpers for I2C/SPI/UART/GPIO child devices
#
# Do not collapse the two xy_device.c files without first renaming the helper
# layer and updating unit-test source lists; they intentionally provide different
# parts of the current compatibility model.
set(DEVICE_SOURCES
    src/xy_device.c        # lifecycle + generic device dispatch
    xy_device.c            # compatibility bus helpers: i2c/spi/uart/gpio devices
    xy_device_core.c       # global registry + idle PM
    src/xy_device_pm.c     # per-device power management ops
)

set(DEVICE_HEADERS
    inc/xy_adc.h
    inc/xy_dev_api.h
    inc/xy_dev_i2c.h
    inc/xy_device_async.h
    inc/xy_device_error.h
    inc/xy_device.h
    inc/xy_device_pm.h
    inc/xy_dev_spi.h
    inc/xy_dev_uart.h
    inc/xy_gpio.h
    inc/xy_pwm.h
    xy_device_core.h
)

# Create library
add_library(xy_device STATIC ${DEVICE_SOURCES})

# HAL Platform Selection (same as xy_hal component)
set(HAL_PLATFORM "PC" CACHE STRING "HAL Platform for device component")

# Include HAL sources based on platform
if(HAL_PLATFORM STREQUAL "PC")
    set(HAL_SOURCES
        ../hal/PC/xy_hal_pc.c
        ../hal/PC/xy_hal_gpio_pc.c
        ../hal/PC/xy_hal_i2c_pc.c
        ../hal/PC/xy_hal_spi_pc.c
        ../hal/PC/xy_hal_uart_pc.c
    )
    set(HAL_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/../hal/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../hal
    )
    set(HAL_DEFINES
        HAL_PLATFORM_PC
        XY_HAL_PC_SIM
    )
else()
    # For embedded platforms, link with external xy_hal
    message(STATUS "Device: Using external xy_hal for ${HAL_PLATFORM}")
    set(HAL_SOURCES "")
    set(HAL_INCLUDE_DIRS "")
    set(HAL_DEFINES "")
endif()

target_include_directories(xy_device PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${HAL_INCLUDE_DIRS}
)

# Add HAL sources if building for PC
if(HAL_SOURCES)
    target_sources(xy_device PRIVATE ${HAL_SOURCES})
endif()

# Compile definitions
target_compile_definitions(xy_device PUBLIC ${HAL_DEFINES})

# For non-PC platforms, link with xy_hal library
if(NOT HAL_PLATFORM STREQUAL "PC")
    target_link_libraries(xy_device PUBLIC xy_hal)
endif()

# Installation
install(FILES ${DEVICE_HEADERS} DESTINATION include/xy_device)
install(TARGETS xy_device ARCHIVE DESTINATION lib)
