set(LITTLEFS_SOURCES
    xy_littlefs_w25q128.c
    ${CMAKE_SOURCE_DIR}/third_party/littlefs/lfs.c
    ${CMAKE_SOURCE_DIR}/third_party/littlefs/lfs_util.c
)
xy_add_srcs(${LITTLEFS_SOURCES})
