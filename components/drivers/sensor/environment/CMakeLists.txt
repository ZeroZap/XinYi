add_subdirectory(bme680)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/sgp40)
    add_subdirectory(sgp40)
endif()
