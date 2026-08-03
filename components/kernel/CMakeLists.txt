# XinYi Kernel Components
# OSAL + Kernel Services
#
# NOTE: The active OSAL build entrypoint is components/kernel/osal/CMakeLists.txt,
# added from the repository root as the nested kernel/osal component. The legacy
# aggregate variables below are kept for older standalone consumers only and point
# at the same canonical CMSIS-like OSAL API surface (`osal/xy_os.h`).

set(CMAKE_C_STANDARD 99)

# ==================== OSAL Core ====================

set(OSAL_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/osal/src/xy_os_tick.c
)

set(OSAL_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/osal/xy_os.h
    ${CMAKE_CURRENT_LIST_DIR}/osal/xy_os_cfg.h
    ${CMAKE_CURRENT_LIST_DIR}/osal/inc/xy_os.h
    ${CMAKE_CURRENT_LIST_DIR}/osal/inc/xy_os_delay.h
    ${CMAKE_CURRENT_LIST_DIR}/osal/inc/xy_os_error.h
    ${CMAKE_CURRENT_LIST_DIR}/osal/inc/xy_os_sys.h
    ${CMAKE_CURRENT_LIST_DIR}/osal/inc/xy_os_tick.h
    ${CMAKE_CURRENT_LIST_DIR}/osal/inc/xy_os_types.h
)

# Backend selection uses the same canonical names as components/kernel/osal/CMakeLists.txt.
# Kconfig compatibility symbols map to those names here: baremetal, freertos,
# rtthread, cmsis_rtx.
if(CONFIG_OSAL_BACKEND_FREERTOS)
    list(APPEND OSAL_SOURCES ${CMAKE_CURRENT_LIST_DIR}/osal/backend/freertos/xy_os_freertos.c)
elseif(CONFIG_OSAL_BACKEND_RTTHREAD)
    list(APPEND OSAL_SOURCES ${CMAKE_CURRENT_LIST_DIR}/osal/backend/rtthread/xy_os_rtthread.c)
elseif(CONFIG_OSAL_BACKEND_RTX5 OR CONFIG_OSAL_BACKEND_CMSIS_RTX)
    list(APPEND OSAL_SOURCES ${CMAKE_CURRENT_LIST_DIR}/osal/backend/cmsis_rtx/xy_os_cmsis_rtx.c)
elseif(CONFIG_OSAL_BACKEND_BAREMETAL)
    list(APPEND OSAL_SOURCES ${CMAKE_CURRENT_LIST_DIR}/osal/backend/baremetal/xy_os_baremetal.c)
endif()

# ==================== Misc Services ====================

set(MISC_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/misc/src/xy_sysmon.c
)

set(MISC_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/misc/inc/xy_sysmon.h
)

# ==================== Services ====================

set(SERVICE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/service/src/xy_scheduler.c
)

set(SERVICE_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/service/inc/xy_scheduler.h
)

# ==================== Aggregate ====================

set(KERNEL_SOURCES
    ${OSAL_SOURCES}
    ${MISC_SOURCES}
    ${SERVICE_SOURCES}
    PARENT_SCOPE
)

set(KERNEL_HEADERS
    ${OSAL_HEADERS}
    ${MISC_HEADERS}
    ${SERVICE_HEADERS}
    PARENT_SCOPE
)

set(KERNEL_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/osal/inc
    ${CMAKE_CURRENT_LIST_DIR}/misc/inc
    ${CMAKE_CURRENT_LIST_DIR}/service/inc
    PARENT_SCOPE
)

# ==================== Tests ====================

if(BUILD_TESTING)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/osal/tests)
endif()

# ==================== Summary ====================

message(STATUS "Kernel Components:")
message(STATUS "  OSAL Backend: ${CONFIG_OSAL_BACKEND}")
message(STATUS "  Sources: ${OSAL_SOURCES}")
