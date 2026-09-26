# ADC Sensor Drivers

# ADS1115
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ads1115)
    add_subdirectory(ads1115)
endif()

# INA219
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ina219)
    add_subdirectory(ina219)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ina226)
    add_subdirectory(ina226)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ina22x)
    add_subdirectory(ina22x)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ina228)
    add_subdirectory(ina228)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ina229)
    add_subdirectory(ina229)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ltc2945)
    add_subdirectory(ltc2945)
endif()
