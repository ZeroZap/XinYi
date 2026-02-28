cmake_minimum_required(VERSION 3.10)
project(xy_device C)

set(CMAKE_C_STANDARD 99)

# Collect source files
set(DEVICE_SOURCES
    xy_device.c
    xy_eeprom_24xx.c
    xy_oled_ssd1306.c
    xy_mpu6050.c
    xy_bmp280.c
    xy_sht30.c
    xy_ads1115.c
)

set(DEVICE_HEADERS
    xy_device.h
    xy_eeprom_24xx.h
    xy_oled_ssd1306.h
    xy_mpu6050.h
    xy_bmp280.h
    xy_sht30.h
    xy_ads1115.h
)

# Create library
add_library(xy_device STATIC ${DEVICE_SOURCES})

target_include_directories(xy_device PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(xy_device PUBLIC
    xy_hal
)

# Installation
install(FILES ${DEVICE_HEADERS} DESTINATION include/xy_device)
install(TARGETS xy_device ARCHIVE DESTINATION lib)
