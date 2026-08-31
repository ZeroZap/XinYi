# xy_ascon CMakeLists.txt
# Ascon Lightweight Cryptography

add_library(xy_ascon STATIC
    xy_ascon.c
)

target_include_directories(xy_ascon PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(xy_ascon PRIVATE
    ${XY_CRYPTO_CFLAGS}
)

set_target_properties(xy_ascon PROPERTIES
    OUTPUT_NAME "xy_ascon"
    POSITION_INDEPENDENT_CODE 0
)
