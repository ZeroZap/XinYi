# Pressure Sensor Drivers

# BMP280
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bmp280)
    add_subdirectory(bmp280)
endif()

# BMP390
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bmp390)
    add_subdirectory(bmp390)
endif()

# LPS22HB
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/lps22hb)
    add_subdirectory(lps22hb)
endif()
