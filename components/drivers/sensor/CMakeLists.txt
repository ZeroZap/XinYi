# Sensor Drivers

# Temperature Sensors
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/temperature)
    add_subdirectory(temperature)
endif()

# Pressure Sensors
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/pressure)
    add_subdirectory(pressure)
endif()

# Light Sensors
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/angle)
    add_subdirectory(angle)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/light)
    add_subdirectory(light)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/proximity)
    add_subdirectory(proximity)
endif()

# Motion Sensors
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/motion)
    add_subdirectory(motion)
endif()

# Magnetic Sensors
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/magnetic)
    add_subdirectory(magnetic)
endif()

# Environment Sensors
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/environment)
    add_subdirectory(environment)
endif()

# ADC Sensors
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/adc)
    add_subdirectory(adc)
endif()

# Distance Sensors
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/distance)
    add_subdirectory(distance)
endif()

# Additional sensor files in this directory
file(GLOB SENSOR_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
if(SENSOR_SOURCES)
    xy_add_srcs(${SENSOR_SOURCES})
endif()
