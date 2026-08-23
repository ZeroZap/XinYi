# LED Drivers CMakeLists.txt

cmake_minimum_required(VERSION 3.10)

if(NOT XY_DRIVER_DISPLAY_LED)
    return()
endif()

message(STATUS "Display: Building LED driver backends")

xy_add_srcs(${CMAKE_CURRENT_SOURCE_DIR}/xy_led_driver.c)

# Charlieplex driver
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/charlieplex/CMakeLists.txt)
    add_subdirectory(charlieplex)
endif()

# Matrix scan driver
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/matrix_scan/CMakeLists.txt)
    add_subdirectory(matrix_scan)
endif()

# Serial RGB driver
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/serial_rgb/CMakeLists.txt)
    add_subdirectory(serial_rgb)
endif()
