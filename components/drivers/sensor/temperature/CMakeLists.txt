# Temperature Sensor Drivers

# SHT30
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/sht30)
    add_subdirectory(sht30)
endif()

# AHT20
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/aht20)
    add_subdirectory(aht20)
endif()

# SHT40
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/sht40)
    add_subdirectory(sht40)
endif()

# AHT30
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/aht30)
    add_subdirectory(aht30)
endif()

# DHT11
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/dht11)
    add_subdirectory(dht11)
endif()
