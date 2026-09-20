if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bh1750)
    add_subdirectory(bh1750)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ap3216c)
    add_subdirectory(ap3216c)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tsl2561)
    add_subdirectory(tsl2561)
endif()
