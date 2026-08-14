cmake_minimum_required(VERSION 3.10)
project(xy_tiny_crypto_examples LANGUAGES C)

# Enable C99
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT TARGET xy_tiny_crypto)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/crypto)
endif()

# Add individual example executables
set(EXAMPLES
    example_aes
    example_hash
    example_hmac
    example_base64
    example_hex
    example_random
)

# SM2/SM3/SM4 examples are historical broad demos with placeholder/review-pending
# API drift; keep them out of this host-safe build guard until they get their own
# path-limited security/ownership cleanup slice.

foreach(example ${EXAMPLES})
    add_executable(${example} ${example}.c)

    # Include the crypto headers
    target_include_directories(${example} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../inc
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../xy_sm2
        ${CMAKE_CURRENT_SOURCE_DIR}/../xy_sm3
        ${CMAKE_CURRENT_SOURCE_DIR}/../xy_sm4
    )

    # Link the canonical Crypto runtime target used by the root build.
    target_link_libraries(${example} xy_tiny_crypto)
endforeach()

# Optional: Group examples in IDE
foreach(example ${EXAMPLES})
    set_target_properties(${example} PROPERTIES FOLDER "crypto/examples")
endforeach()
