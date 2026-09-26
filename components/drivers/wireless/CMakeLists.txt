# Wireless Drivers

# NRF24L01
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/nrf24l01)
    add_subdirectory(nrf24l01)
endif()

# RFID
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/rfid)
    add_subdirectory(rfid)
endif()

# WiFi
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/wifi)
    add_subdirectory(wifi)
endif()

# Bluetooth
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bluetooth)
    add_subdirectory(bluetooth)
endif()
