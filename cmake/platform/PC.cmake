# PC Platform Configuration — variables only, no direct flag manipulation

# OS-specific defines and link libs
if(CMAKE_HOST_WIN32)
    set(PLATFORM_DEFINES PLATFORM_WINDOWS)
    set(PLATFORM_LIBS ws2_32)
elseif(CMAKE_HOST_APPLE)
    set(PLATFORM_DEFINES PLATFORM_MACOS)
    set(PLATFORM_LIBS "")
else()
    set(PLATFORM_DEFINES PLATFORM_LINUX)
    set(PLATFORM_LIBS pthread)
endif()

list(APPEND PLATFORM_DEFINES
    HAL_PLATFORM_PC
    XY_HAL_PC_SIM
    HAVE_STDIO_H
    HAVE_STDLIB_H
    HAVE_STRING_H
    HAVE_UNISTD_H
)

set(PLATFORM_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/components/hal/PC
)

# No CPU_FLAGS for native build
set(CPU_FLAGS "")

message(STATUS "Platform: PC (${CMAKE_HOST_SYSTEM_NAME})")
