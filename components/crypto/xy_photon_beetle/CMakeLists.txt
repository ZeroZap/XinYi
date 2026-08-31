# xy_photon_beetle CMakeLists.txt
# Photon Beetle Lightweight Cryptography

add_library(xy_photon_beetle STATIC
    xy_photon_beetle.c
)

target_include_directories(xy_photon_beetle PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(xy_photon_beetle PRIVATE
    ${XY_CRYPTO_CFLAGS}
)

set_target_properties(xy_photon_beetle PROPERTIES
    OUTPUT_NAME "xy_photon_beetle"
    POSITION_INDEPENDENT_CODE 0
)
