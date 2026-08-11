cmake_minimum_required(VERSION 3.12)
project(crypto_root_target_smoke_probe LANGUAGES C)

if(NOT DEFINED ROOT)
    message(FATAL_ERROR "ROOT is required")
endif()

add_subdirectory("${ROOT}/components/crypto" "${CMAKE_BINARY_DIR}/crypto")

add_executable(crypto_root_target_smoke main.c)
target_include_directories(crypto_root_target_smoke PRIVATE
    "${ROOT}/components/crypto/inc"
)
target_link_libraries(crypto_root_target_smoke PRIVATE xy_tiny_crypto)

add_custom_target(run_crypto_root_target_smoke
    COMMAND crypto_root_target_smoke
    DEPENDS crypto_root_target_smoke
)
