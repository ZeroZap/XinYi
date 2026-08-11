# xy_sm2 CMakeLists.txt - SM2 Elliptic Curve

add_library(xy_sm2 STATIC
    xy_sm2.c
)

target_include_directories(xy_sm2 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${XY_CRYPTO_ROOT}
    ${XY_CRYPTO_ROOT}/xy_sm3
    ${XY_CRYPTO_ROOT}/xy_sm4
    ${XY_CRYPTO_ROOT}/xy_rng
)

target_compile_options(xy_sm2 PRIVATE
    ${XY_CRYPTO_CFLAGS}
)

set_target_properties(xy_sm2 PROPERTIES
    OUTPUT_NAME "xy_sm2"
    POSITION_INDEPENDENT_CODE 0
)
