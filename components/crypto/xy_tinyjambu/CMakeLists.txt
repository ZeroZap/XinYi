# xy_tinyjambu CMakeLists.txt
# TinyJambu Lightweight Cryptography

add_library(xy_tinyjambu STATIC
    xy_tinyjambu.c
)

target_include_directories(xy_tinyjambu PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(xy_tinyjambu PRIVATE
    ${XY_CRYPTO_CFLAGS}
)

set_target_properties(xy_tinyjambu PROPERTIES
    OUTPUT_NAME "xy_tinyjambu"
    POSITION_INDEPENDENT_CODE 0
)
