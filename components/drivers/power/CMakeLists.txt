# Power Drivers

# Charger
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/charger)
    add_subdirectory(charger)
endif()

# Fuel Gauge
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/fuel_gauge)
    add_subdirectory(fuel_gauge)
endif()

# Coulomb Counter
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/coulomb)
    add_subdirectory(coulomb)
endif()

# MAX17043 Battery Fuel Gauge
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/max17043)
    add_subdirectory(max17043)
endif()
