# LCD Display CMakeLists.txt

cmake_minimum_required(VERSION 3.10)

if(NOT DRIVER_DISPLAY_LCD)
    return()
endif()

message(STATUS "Display: Building LCD drivers")
