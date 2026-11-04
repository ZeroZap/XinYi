if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/w25q128)
    add_subdirectory(w25q128)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/w25qxx)
    add_subdirectory(w25qxx)
endif()
