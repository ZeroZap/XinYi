cmake_minimum_required(VERSION 3.10)
project(xy_drivers VERSION 1.0.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler options
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Include directories
include_directories(
    inc
    ../device/inc
    ../device
    ../hal/inc
    ../kernel/osal
    ../kernel/osal/inc
    ../clib/xy_clib
    ../clib/xy_clib/inc
    ../sensor/inc
    ../trace/xy_log/inc
    ../pm/fuel-gauge
    ../pm/charger
    ../pm/inc
    power/charger
    ../gui/inc
    sensor/pressure/bmp390/bosch
    sensor/adc/ina22x
    sensor/adc/ina228
    sensor/adc/ina229
)

# ---------------------------------------------------------------------------
# Canonical Device-model sensor drivers: exactly one owner
# ---------------------------------------------------------------------------
# These implementations are compiled here and nowhere else. Components that
# expose them (the Sensor compatibility layer) link this library instead of
# listing the `.c` files as their own sources. Two copies of the same driver
# produce two archives that both define the same globals, and a static archive
# is only searched, so which copy runs would be decided by archive order rather
# than by the build.
set(XY_SENSOR_DRIVER_CORE_SOURCES
    sensor/temperature/sht30/xy_sht30.c
    sensor/adc/ads1115/xy_ads1115.c
    sensor/motion/mpu6050/xy_mpu6050.c
    sensor/pressure/bmp280/xy_bmp280.c
    sensor/light/bh1750/xy_bh1750.c
    sensor/temperature/aht20/xy_aht20.c
    sensor/temperature/aht10/xy_aht10.c
    sensor/light/ap3216c/xy_ap3216c.c
    sensor/motion/icm20608/xy_icm20608.c
    sensor/motion/qma6100p/xy_qma6100p.c
)

add_library(xy_sensor_drivers_core STATIC ${XY_SENSOR_DRIVER_CORE_SOURCES})

# These drivers reach the bus through the canonical Device helpers
# (`xy_i2c_device_*`, `xy_spi_device_*`). That is a hard requirement, not an
# optional one: without the Device framework the archive is emitted with five to
# seven unresolved references and nothing in the configuration can satisfy them,
# so the breakage is deferred to whatever links the archive first. The root
# Kconfig therefore has every `DRIVER_*` family and both Sensor symbols select
# `COMPONENT_DEVICE`. A Kconfig configure that lost those selects stops here;
# standalone (non-Kconfig) trees never define the `XY_*` symbols and keep the
# previous behaviour, linking the Device target when the tree provides one.
if(DEFINED XY_COMPONENT_DEVICE AND NOT TARGET xy_device)
    message(FATAL_ERROR
        "The canonical sensor drivers require the Device bus helpers, but "
        "target 'xy_device' does not exist in this configuration. "
        "DRIVER_*/COMPONENT_SENSOR/XY_SENSOR_ENABLE select COMPONENT_DEVICE in "
        "the root Kconfig; restore that select rather than dropping the link.")
endif()
if(TARGET xy_device)
    target_link_libraries(xy_sensor_drivers_core PUBLIC xy_device)
endif()

# The aggregate `xy_drivers` archive is only built when a driver family is
# enabled. The canonical sensor drivers above are a shared dependency of the
# Sensor component, so this directory can also be entered with every DRIVER_*
# symbol off; in that case only the owner library above is built. Standalone
# (non-Kconfig) builds never define XY_DRIVER_* and keep the aggregate.
set(_xy_driver_family_enabled FALSE)
foreach(_xy_family SENSOR DISPLAY STORAGE POWER WIRELESS SYSTEM)
    if(XY_DRIVER_${_xy_family})
        set(_xy_driver_family_enabled TRUE)
    endif()
endforeach()
if(DEFINED XY_DRIVER_SENSOR AND NOT _xy_driver_family_enabled)
    message(STATUS "Drivers: no driver family enabled; built only the canonical "
                   "sensor driver owners (xy_sensor_drivers_core)")
    return()
endif()

# Add subdirectories for organized drivers
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/sensor)
    add_subdirectory(sensor)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/display)
    add_subdirectory(display)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/storage)
    add_subdirectory(storage)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/power)
    add_subdirectory(power)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/wireless)
    add_subdirectory(wireless)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/system)
    add_subdirectory(system)
endif()

# Collect source files
file(GLOB_RECURSE COMPONENT_SOURCES "*.c")
list(FILTER COMPONENT_SOURCES EXCLUDE REGEX "test/.*")
list(FILTER COMPONENT_SOURCES EXCLUDE REGEX "samples/.*")
list(FILTER COMPONENT_SOURCES EXCLUDE REGEX "examples/.*")

if(CONFIG_FS_LITTLEFS)
    list(APPEND COMPONENT_SOURCES
        ${CMAKE_SOURCE_DIR}/third_party/littlefs/lfs.c
        ${CMAKE_SOURCE_DIR}/third_party/littlefs/lfs_util.c
    )
endif()

if(NOT XY_DRIVER_DISPLAY)
    list(FILTER COMPONENT_SOURCES EXCLUDE REGEX ".*/display/.*")
else()
    if(NOT XY_DRIVER_DISPLAY_OLED)
        list(FILTER COMPONENT_SOURCES EXCLUDE REGEX ".*/display/oled/.*")
    elseif(NOT XY_DRIVER_DISPLAY_SSD1306)
        list(FILTER COMPONENT_SOURCES EXCLUDE REGEX ".*/display/oled/ssd1306/.*")
    endif()

    if(NOT XY_DRIVER_DISPLAY_LCD)
        list(FILTER COMPONENT_SOURCES EXCLUDE REGEX ".*/display/lcd/.*")
        list(FILTER COMPONENT_SOURCES EXCLUDE REGEX ".*/display/lcd_drivers/.*")
    else()
        if(NOT XY_DRIVER_DISPLAY_LCD_SPI)
            list(FILTER COMPONENT_SOURCES EXCLUDE REGEX ".*/display/lcd/xy_lcd_spi\\.c$")
        endif()
        if(NOT XY_DRIVER_DISPLAY_LCD_I8080)
            list(FILTER COMPONENT_SOURCES EXCLUDE REGEX ".*/display/lcd/xy_lcd_i8080\\.c$")
        endif()
        if(NOT XY_DRIVER_DISPLAY_LCD_ST7789)
            list(FILTER COMPONENT_SOURCES EXCLUDE REGEX ".*/display/lcd/xy_lcd_st7789\\.c$")
        endif()
    endif()

    if(NOT XY_DRIVER_DISPLAY_LED)
        list(FILTER COMPONENT_SOURCES EXCLUDE REGEX ".*/display/led/.*")
        list(FILTER COMPONENT_SOURCES EXCLUDE REGEX ".*/display/led_drivers/.*")
    elseif(NOT XY_DRIVER_DISPLAY_LED_SERIAL_RGB)
        list(FILTER COMPONENT_SOURCES EXCLUDE REGEX ".*/display/led_drivers/serial_rgb/.*")
    endif()
endif()

if(NOT XY_DRIVER_STORAGE)
    list(FILTER COMPONENT_SOURCES EXCLUDE REGEX ".*/storage/.*")
endif()

file(GLOB_RECURSE COMPONENT_HEADERS "inc/*.h")

# The canonical sensor drivers are compiled once, by `xy_sensor_drivers_core`
# above. Take them back out of the aggregate source list, and fail the configure
# if the list has drifted: a stale entry would silently be compiled twice again,
# which is exactly the state this split exists to prevent.
foreach(_xy_core IN LISTS XY_SENSOR_DRIVER_CORE_SOURCES)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_xy_core}")
        message(FATAL_ERROR
            "xy_sensor_drivers_core lists a missing source: ${_xy_core}")
    endif()
    list(FIND COMPONENT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${_xy_core}" _xy_core_index)
    if(_xy_core_index EQUAL -1)
        message(FATAL_ERROR
            "xy_sensor_drivers_core owner ${_xy_core} is absent from the driver "
            "glob; the ownership split is stale and the file would be compiled "
            "twice")
    endif()
    list(REMOVE_AT COMPONENT_SOURCES ${_xy_core_index})
endforeach()

# Create static library
add_library(xy_drivers STATIC ${COMPONENT_SOURCES})
target_link_libraries(xy_drivers PUBLIC xy_sensor_drivers_core)

# The aggregate also carries the power/wireless/system/storage drivers, which
# reach the bus through the same Device helpers, so Device is a public
# dependency of everything that links this archive.
if(TARGET xy_device)
    target_link_libraries(xy_drivers PUBLIC xy_device)
endif()

if(CONFIG_FS_LITTLEFS)
    target_include_directories(xy_drivers PUBLIC
        ${CMAKE_SOURCE_DIR}/third_party/littlefs
    )
    target_compile_definitions(xy_drivers PUBLIC XY_LITTLEFS_W25Q128=1)
endif()

# Set library properties
set_target_properties(xy_drivers PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "${COMPONENT_HEADERS}"
)

# Windows system needs additional libraries
if(WIN32)
    target_link_libraries(xy_drivers PRIVATE advapi32)
endif()

# Tests live in the repository-level Unity/CTest suite. Use `make test-unit`
# or focused driver tests under sensor, display, storage, fuel gauge, and actuator.

# Installation rules
install(TARGETS xy_drivers
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)

# Packaging
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
