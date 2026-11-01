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
# - src/xy_device_bus_helpers.c: compatibility bus helpers for I2C/SPI/UART/GPIO child devices
#
# Keep bus helpers separate from src/xy_device.c so the generic device lifecycle
# layer is not confused with bus-specific compatibility wrappers.
set(DEVICE_SOURCES
    src/xy_device.c        # lifecycle + generic device dispatch
    src/xy_device_bus_helpers.c # compatibility bus helpers: i2c/spi/uart/gpio devices
    xy_device_core.c       # global registry + idle PM
    src/xy_device_pm.c     # per-device power management ops
    src/xy_device_async.c  # optional queued async helper state machine
)

set(DEVICE_HEADERS
    inc/xy_adc.h
    inc/xy_dev_api.h
    inc/xy_dev_i2c.h
    inc/xy_device_async.h
    inc/xy_device_error.h
    inc/xy_device.h
    inc/xy_device_pm.h
    inc/xy_device_timing.h
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

# `xy_hal` owns the platform backends, the PC simulation included. This component
# links that library on every platform; it must not compile the backends itself.
# Doing so put a second copy of every backend symbol into `libxy_device.a`
# (59 `xy_hal_*` definitions shared with `libxy_hal.a` on a PC configure with
# `COMPONENT_DEVICE=ON`), which leaves the linked implementation up to archive
# order instead of the build.
set(HAL_INCLUDE_DIRS "")
set(HAL_DEFINES "")
if(HAL_PLATFORM STREQUAL "PC")
    set(HAL_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/../hal/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../hal
    )
    set(HAL_DEFINES
        HAL_PLATFORM_PC
        XY_HAL_PC_SIM
    )
endif()

target_include_directories(xy_device PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${HAL_INCLUDE_DIRS}
)

# Compile definitions
target_compile_definitions(xy_device PUBLIC ${HAL_DEFINES})

# The HAL implementation is owned by exactly one library, on every platform.
target_link_libraries(xy_device PUBLIC xy_hal)

# Installation
install(FILES ${DEVICE_HEADERS} DESTINATION include/xy_device)
install(TARGETS xy_device ARCHIVE DESTINATION lib)
