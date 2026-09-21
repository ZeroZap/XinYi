add_subdirectory(bme680)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/sgp40)
    add_subdirectory(sgp40)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ccs811)
    add_subdirectory(ccs811)
endif()
