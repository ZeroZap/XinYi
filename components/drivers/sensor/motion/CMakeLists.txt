# Motion Sensor Drivers

# MPU6050
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/mpu6050)
    add_subdirectory(mpu6050)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/l3g4200d)
    add_subdirectory(l3g4200d)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/sc7a22h)
    add_subdirectory(sc7a22h)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/icm20608)
    add_subdirectory(icm20608)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bmi088)
    add_subdirectory(bmi088)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bmi270)
    add_subdirectory(bmi270)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bno055)
    add_subdirectory(bno055)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bma400)
    add_subdirectory(bma400)
endif()
