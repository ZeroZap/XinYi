# Motion Sensor Drivers

# MPU6050
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/mpu6050)
    add_subdirectory(mpu6050)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/l3g4200d)
    add_subdirectory(l3g4200d)
endif()
