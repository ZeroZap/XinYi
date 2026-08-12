cmake_minimum_required(VERSION 3.10)
project(xy_tiny_crypto VERSION 1.0.0 LANGUAGES C)

# 设置 C 标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 编译选项
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 包含目录
include_directories(
    xy_crc
    xy_blake
    xy_25519
    inc
    ../hal/inc
    ../kernel/osal
    ../trace/xy_log/inc
    ../clib/xy_clib
)

# 添加子模块
set(XY_CRYPTO_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
add_subdirectory(xy_sm3)
add_subdirectory(xy_sm4)
add_subdirectory(xy_sm2)

# 收集源文件 - 核心算法在 src/
file(GLOB CRYPTO_SOURCES "src/*.c")
list(FILTER CRYPTO_SOURCES EXCLUDE REGEX ".*/src/xy_sha256\\.c$")
file(GLOB CRYPTO_HEADERS "inc/*.h")

# 创建静态库
add_library(xy_tiny_crypto STATIC ${CRYPTO_SOURCES})
# Compatibility target for examples/consumers that use the component-style xy_crypto name.
# Keep xy_tiny_crypto as the installed/runtime target until a separate rename proposal lands.
if(NOT TARGET xy_crypto)
    add_library(xy_crypto ALIAS xy_tiny_crypto)
endif()

# 链接子模块
target_link_libraries(xy_tiny_crypto PRIVATE
    xy_sm3
    xy_sm4
    xy_sm2
)

# 设置库属性
set_target_properties(xy_tiny_crypto PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "${CRYPTO_HEADERS}"
)

# Windows 系统需要链接额外库
if(WIN32)
    target_link_libraries(xy_tiny_crypto PRIVATE advapi32)
endif()

# 安装规则
install(TARGETS xy_tiny_crypto
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)

# Tests live in the repository-level Unity/CTest suite. Use `make test-unit`
# or focused CTest names such as `crypto_hash`, `crypto_encode`,
# `crypto_cipher_hmac`, `crypto_crc`, and `crypto_sm2`.

# 打包
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
