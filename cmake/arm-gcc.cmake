# CMake ARM GCC 工具链文件
# 用于 XinYi 嵌入式框架的交叉编译

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# 工具链路径
set(TOOLCHAIN_PATH "/home/eugene/Tools/arm-gnu-toolchain/bin")

# 编译器
set(CMAKE_C_COMPILER "${TOOLCHAIN_PATH}/arm-none-eabi-gcc")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PATH}/arm-none-eabi-g++")
set(CMAKE_ASM_COMPILER "${TOOLCHAIN_PATH}/arm-none-eabi-gcc")

# 工具
set(CMAKE_AR     "${TOOLCHAIN_PATH}/arm-none-eabi-ar"      CACHE FILEPATH "Archiver")
set(CMAKE_RANLIB "${TOOLCHAIN_PATH}/arm-none-eabi-ranlib"  CACHE FILEPATH "Ranlib")
set(CMAKE_SIZE   "${TOOLCHAIN_PATH}/arm-none-eabi-size"    CACHE FILEPATH "Size tool")
set(CMAKE_OBJCOPY  "${TOOLCHAIN_PATH}/arm-none-eabi-objcopy"  CACHE FILEPATH "Objcopy tool")
set(CMAKE_OBJDUMP  "${TOOLCHAIN_PATH}/arm-none-eabi-objdump"  CACHE FILEPATH "Objdump tool")
set(CMAKE_GDB      "${TOOLCHAIN_PATH}/arm-none-eabi-gdb"      CACHE FILEPATH "Debugger")

# 编译标志
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror=implicit-function-declaration -fno-builtin -ffreestanding -nostdlib")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fno-builtin -ffreestanding -nostdlib")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -Wall -Wextra")

# 链接标志 (裸机环境)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nostdlib -nostartfiles -nodefaultlibs")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -nostdlib -nostartfiles -nodefaultlibs")

# 优化级别 (可配置)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g3")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g3")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -DNDEBUG")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -DNDEBUG")
endif()

# 查找包含文件
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# 打印工具链信息
message(STATUS "ARM GCC Toolchain loaded")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
