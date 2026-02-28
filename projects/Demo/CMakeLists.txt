cmake_minimum_required(VERSION 3.10)
project(xy_demo C)

set(CMAKE_C_STANDARD 99)

# Find XinYi components
find_package(xy_osal REQUIRED)
find_package(xy_device REQUIRED)
find_package(xy_crypto REQUIRED)

# Demo executable
add_executable(xy_demo main.c)

target_include_directories(xy_demo PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(xy_demo PRIVATE
    xy_osal
    xy_device
    xy_crypto
    xy_hal
)
