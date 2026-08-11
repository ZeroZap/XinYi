cmake_minimum_required(VERSION 3.12)
project(crypto_alias_target_probe LANGUAGES C)

if(NOT DEFINED ROOT)
    message(FATAL_ERROR "ROOT is required")
endif()

add_subdirectory("${ROOT}/components/crypto" "${CMAKE_BINARY_DIR}/crypto")

if(NOT TARGET xy_tiny_crypto)
    message(FATAL_ERROR "xy_tiny_crypto target is missing")
endif()

if(NOT TARGET xy_crypto)
    message(FATAL_ERROR "xy_crypto compatibility alias target is missing")
endif()

message(STATUS "crypto_alias_target_ok real=xy_tiny_crypto alias=xy_crypto")
