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
    ../trace/xy_log/inc
    ../pm/fuel-gauge
    ../pm/charger
    ../pm/inc
)

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

# Create static library
add_library(xy_drivers STATIC ${COMPONENT_SOURCES})

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
