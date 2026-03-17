# PC Platform Configuration
# Windows/Linux/macOS with standard C library

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Compiler settings for PC
if(CMAKE_HOST_WIN32)
    # Windows (MinGW or MSVC)
    if(MSVC)
        add_compile_options(/W4 /O2)
        add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    else()
        # MinGW
        add_compile_options(-Wall -Wextra -O2)
    endif()
    set(PLATFORM_DEFINES PLATFORM_WINDOWS)
    
elseif(CMAKE_HOST_APPLE)
    # macOS
    add_compile_options(-Wall -Wextra -O2)
    set(PLATFORM_DEFINES PLATFORM_MACOS)
    
else()
    # Linux
    add_compile_options(-Wall -Wextra -O2)
    set(PLATFORM_DEFINES PLATFORM_LINUX)
endif()

# PC HAL sources
set(HAL_PLATFORM_SOURCES
    ${CMAKE_SOURCE_DIR}/components/hal/PC/xy_hal_pc.c
    ${CMAKE_SOURCE_DIR}/components/hal/PC/xy_hal_pin_pc.c
    ${CMAKE_SOURCE_DIR}/components/hal/PC/xy_hal_i2c_pc.c
    ${CMAKE_SOURCE_DIR}/components/hal/PC/xy_hal_spi_pc.c
    ${CMAKE_SOURCE_DIR}/components/hal/PC/xy_hal_uart_pc.c
)

# PC-specific definitions
set(PLATFORM_DEFINES ${PLATFORM_DEFINES}
    HAL_PLATFORM_PC
    XY_HAL_PC_SIM
    HAVE_STDIO_H
    HAVE_STDLIB_H
    HAVE_STRING_H
    HAVE_UNISTD_H
)

# Link libraries for PC
if(CMAKE_HOST_WIN32)
    set(PLATFORM_LIBS ws2_32)
elseif(CMAKE_HOST_APPLE)
    set(PLATFORM_LIBS)
else()
    # Linux
    set(PLATFORM_LIBS pthread)
endif()

message(STATUS "PC Platform: ${CMAKE_HOST_SYSTEM_NAME}")
message(STATUS "  HAL Sources: ${HAL_PLATFORM_SOURCES}")
message(STATUS "  Libraries: ${PLATFORM_LIBS}")
