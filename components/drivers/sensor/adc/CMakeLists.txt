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

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/acs712)
    add_subdirectory(acs712)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/fsr)
    add_subdirectory(fsr)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/mg811)
    add_subdirectory(mg811)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/guvas12sd)
    add_subdirectory(guvas12sd)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/mq135)
    add_subdirectory(mq135)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/mq3)
    add_subdirectory(mq3)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/mq7)
    add_subdirectory(mq7)
endif()
