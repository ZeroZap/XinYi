# System Drivers

# Key
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/key/CMakeLists.txt)
    add_subdirectory(key)
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/key/xy_key/CMakeLists.txt)
    add_subdirectory(key/xy_key)
endif()

# RTC
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/rtc/CMakeLists.txt)
    add_subdirectory(rtc)
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/rtc/xy_rtc/CMakeLists.txt)
    add_subdirectory(rtc/xy_rtc)
endif()

# Watchdog
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/watchdog/CMakeLists.txt)
    add_subdirectory(watchdog)
endif()

# Additional system files in this directory
file(GLOB SYSTEM_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
if(SYSTEM_SOURCES)
    xy_add_srcs(${SYSTEM_SOURCES})
endif()
