##
## @file CMakeLists.txt
## @brief CMake configuration for XY OSAL (OS Abstraction Layer)
## @version 2.0
## @date 2026-02-28
##
## Note: Third party RTOS sources are managed in third_party/ directory.
##       This CMakeLists.txt only builds the OSAL adaptation layer.
##

cmake_minimum_required(VERSION 3.12)

project(xy_osal C)

# ==================== Backend Selection ====================

set(OSAL_BACKEND "baremetal" CACHE STRING "OSAL backend selection")
set_property(CACHE OSAL_BACKEND PROPERTY STRINGS baremetal freertos rtthread cmsis_rtx)

# Validate backend selection. Canonical backend names are:
# baremetal, freertos, rtthread, cmsis_rtx.
set(OSAL_BACKEND_OPTIONS baremetal freertos rtthread cmsis_rtx)
if(NOT OSAL_BACKEND IN_LIST OSAL_BACKEND_OPTIONS)
    message(FATAL_ERROR "Invalid OSAL backend: ${OSAL_BACKEND}. Valid values: ${OSAL_BACKEND_OPTIONS}")
endif()

# ==================== Source Files ====================

# Include directories
set(OSAL_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../misc
)

# Common sources - backend implementations include all necessary functionality
# xy_tick and xy_timer_sw are provided by components/kernel/misc module
set(OSAL_COMMON_SOURCES
)

# Backend-specific sources
if(OSAL_BACKEND STREQUAL "baremetal")
    set(OSAL_BACKEND_SOURCES
        backend/baremetal/xy_os_baremetal.c
    )
    set(OSAL_BACKEND_DEFINITIONS
        XY_OS_BACKEND_BAREMETAL
    )
    set(OSAL_BACKEND_INCLUDES
        ${OSAL_INCLUDE_DIRS}
    )
    message(STATUS "XY OSAL: Using Bare-metal backend")

elseif(OSAL_BACKEND STREQUAL "freertos")
    if(HAL_PLATFORM STREQUAL "STM32L4")
        set(OSAL_FREERTOS_CONFIG_DIR
            ${CMAKE_CURRENT_SOURCE_DIR}/config/freertos/stm32l4)
        set(OSAL_FREERTOS_PORT_DIR
            ${CMAKE_CURRENT_SOURCE_DIR}/portable/freertos/GCC/ARM_CM4F)
    elseif(HAL_PLATFORM STREQUAL "WCH")
        set(OSAL_FREERTOS_CONFIG_DIR
            ${CMAKE_SOURCE_DIR}/boards/opench_ch32v307)
        set(OSAL_FREERTOS_PORT_DIR
            ${CMAKE_SOURCE_DIR}/MCU/wch/CH32V307EVT/EXAM/FreeRTOS/FreeRTOS_Core/FreeRTOS/portable/GCC/RISC-V)
    else()
        set(OSAL_FREERTOS_CONFIG_DIR
            ${CMAKE_CURRENT_SOURCE_DIR}/config/freertos)
        set(OSAL_FREERTOS_PORT_DIR
            ${CMAKE_CURRENT_SOURCE_DIR}/portable/freertos/GCC/ARM_CM33_NTZ/non_secure)
    endif()
    set(OSAL_BACKEND_SOURCES
        backend/freertos/xy_os_freertos.c
    )
    set(OSAL_BACKEND_DEFINITIONS
        XY_OS_BACKEND_FREERTOS
        USE_FREERTOS
    )
    set(OSAL_BACKEND_INCLUDES
        ${OSAL_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/third_party/freertos/FreeRTOS/include
        ${OSAL_FREERTOS_CONFIG_DIR}
        ${OSAL_FREERTOS_PORT_DIR}
    )
    message(STATUS "XY OSAL: Using FreeRTOS backend")

elseif(OSAL_BACKEND STREQUAL "rtthread")
    set(OSAL_BACKEND_SOURCES
        backend/rtthread/xy_os_rtthread.c
    )
    set(OSAL_BACKEND_DEFINITIONS
        XY_OS_BACKEND_RTTHREAD
        USE_RTTHREAD
    )
    # The vendored RT-Thread tree sits one level deeper than the other backends,
    # and the backend needs a rtconfig.h this repository does not ship. Both facts
    # used to be invisible: the include path named a directory that does not exist,
    # which gcc skips silently, so the backend compiled to an empty object rather
    # than failing.
    set(XY_RTTHREAD_ROOT ${CMAKE_SOURCE_DIR}/third_party/rt-thread/rt-thread)
    set(OSAL_BACKEND_INCLUDES
        ${OSAL_INCLUDE_DIRS}
        ${XY_RTTHREAD_ROOT}/include
    )
    if(NOT EXISTS ${XY_RTTHREAD_ROOT}/include/rtthread.h)
        message(FATAL_ERROR
            "RT-Thread backend selected but ${XY_RTTHREAD_ROOT}/include/rtthread.h is missing")
    endif()
    if(NOT EXISTS ${XY_RTTHREAD_ROOT}/rtconfig.h AND NOT EXISTS ${CMAKE_SOURCE_DIR}/third_party/rt-config/rtconfig.h)
        message(FATAL_ERROR
            "RT-Thread backend selected but there is no rtconfig.h: rtthread.h includes it "
            "unconditionally. Ship one for this project before enabling this backend.")
    endif()
    message(STATUS "XY OSAL: Using RT-Thread backend")

elseif(OSAL_BACKEND STREQUAL "cmsis_rtx")
    set(OSAL_BACKEND_SOURCES
        backend/cmsis_rtx/xy_os_cmsis_rtx.c
    )
    set(OSAL_BACKEND_DEFINITIONS
        XY_OS_BACKEND_CMSIS_RTX
        USE_CMSIS_RTX
    )
    # The vendored directory is third_party/CMSIS-RTX/CMSIS-RTX/Include: upper case
    # and nested. The old path was wrong in both respects, and Linux is case
    # sensitive, so it could never have resolved.
    set(XY_CMSIS_RTX_ROOT ${CMAKE_SOURCE_DIR}/third_party/CMSIS-RTX/CMSIS-RTX)
    set(OSAL_BACKEND_INCLUDES
        ${OSAL_INCLUDE_DIRS}
        ${XY_CMSIS_RTX_ROOT}/Include
    )
    if(NOT EXISTS ${XY_CMSIS_RTX_ROOT}/Include/rtx_os.h)
        message(FATAL_ERROR
            "CMSIS-RTX backend selected but ${XY_CMSIS_RTX_ROOT}/Include/rtx_os.h is missing")
    endif()
    message(STATUS "XY OSAL: Using CMSIS-RTX backend")

else()
    message(FATAL_ERROR "Invalid OSAL backend: ${OSAL_BACKEND}")
endif()

# ==================== Baremetal Pool Configuration ====================

if(OSAL_BACKEND STREQUAL "baremetal")
    set(XY_OS_BM_MAX_MUTEX   8   CACHE STRING "Baremetal max mutex objects")
    set(XY_OS_BM_MAX_SEM     8   CACHE STRING "Baremetal max semaphore objects")
    set(XY_OS_BM_MAX_EVENT   8   CACHE STRING "Baremetal max event flag objects")
    set(XY_OS_BM_MAX_QUEUE   8   CACHE STRING "Baremetal max message queue objects")
    set(XY_OS_BM_MAX_POOL    4   CACHE STRING "Baremetal max memory pool objects")
    set(XY_OS_BM_MQ_BUF_SIZE   256 CACHE STRING "Baremetal MQ embedded buffer size (bytes)")
    set(XY_OS_BM_POOL_BUF_SIZE 128 CACHE STRING "Baremetal pool embedded block storage (bytes)")
endif()

# ==================== Feature Configuration ====================

option(XY_OS_FEATURE_THREAD "Enable thread management" ON)
option(XY_OS_FEATURE_THREAD_FLAGS "Enable thread flags" ON)
option(XY_OS_FEATURE_MUTEX "Enable mutex support" ON)
option(XY_OS_FEATURE_SEMAPHORE "Enable semaphore support" ON)
option(XY_OS_FEATURE_EVENT_FLAGS "Enable event flags" ON)
option(XY_OS_FEATURE_MESSAGE_QUEUE "Enable message queues" ON)
option(XY_OS_FEATURE_MEMORY_POOL "Enable memory pools" ON)
option(XY_OS_FEATURE_TIMER "Enable software timers" ON)
option(XY_OS_FEATURE_DELAY "Enable delay functions" ON)

# ==================== Debug Configuration ====================

option(XY_OS_PARAM_CHECK "Enable parameter validation" ON)
option(XY_OS_ISR_CHECK "Enable ISR context detection" ON)
option(XY_OS_NULL_CHECK "Enable NULL pointer checks" ON)
option(XY_OS_ENABLE_STATS "Enable statistics collection" OFF)

# ==================== Build Library ====================

set(OSAL_SOURCES
    src/xy_os_tick.c
    ${OSAL_COMMON_SOURCES}
    ${OSAL_BACKEND_SOURCES}
)

add_library(xy_osal STATIC ${OSAL_SOURCES})

# Link against xy_misc for xy_tick and xy_timer_sw
target_link_libraries(xy_osal PUBLIC xy_misc)

# Include directories
target_include_directories(xy_osal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../misc
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OSAL_BACKEND_INCLUDES}
    # xy_hal_time.h is the platform-neutral HAL tick hook. The FreeRTOS backend
    # includes it to hand the kernel tick to the HAL; without this root on the
    # backend's own include path the include fails silently and the binding never
    # compiles (which is exactly what happened on the STM32 reference board).
    ${CMAKE_SOURCE_DIR}/components/hal/inc
)

# Compile definitions
target_compile_definitions(xy_osal PUBLIC
    ${OSAL_BACKEND_DEFINITIONS}
)

# Baremetal pool size definitions
if(OSAL_BACKEND STREQUAL "baremetal")
    target_compile_definitions(xy_osal PUBLIC
        XY_OS_BM_MAX_MUTEX=${XY_OS_BM_MAX_MUTEX}
        XY_OS_BM_MAX_SEM=${XY_OS_BM_MAX_SEM}
        XY_OS_BM_MAX_EVENT=${XY_OS_BM_MAX_EVENT}
        XY_OS_BM_MAX_QUEUE=${XY_OS_BM_MAX_QUEUE}
        XY_OS_BM_MAX_POOL=${XY_OS_BM_MAX_POOL}
        XY_OS_BM_MQ_BUF_SIZE=${XY_OS_BM_MQ_BUF_SIZE}
        XY_OS_BM_POOL_BUF_SIZE=${XY_OS_BM_POOL_BUF_SIZE}
    )
endif()

# Feature definitions
if(XY_OS_FEATURE_THREAD)
    target_compile_definitions(xy_osal PUBLIC XY_OS_FEATURE_THREAD=1)
endif()
if(XY_OS_FEATURE_THREAD_FLAGS)
    target_compile_definitions(xy_osal PUBLIC XY_OS_FEATURE_THREAD_FLAGS=1)
endif()
if(XY_OS_FEATURE_MUTEX)
    target_compile_definitions(xy_osal PUBLIC XY_OS_FEATURE_MUTEX=1)
endif()
if(XY_OS_FEATURE_SEMAPHORE)
    target_compile_definitions(xy_osal PUBLIC XY_OS_FEATURE_SEMAPHORE=1)
endif()
if(XY_OS_FEATURE_EVENT_FLAGS)
    target_compile_definitions(xy_osal PUBLIC XY_OS_FEATURE_EVENT_FLAGS=1)
endif()
if(XY_OS_FEATURE_MESSAGE_QUEUE)
    target_compile_definitions(xy_osal PUBLIC XY_OS_FEATURE_MESSAGE_QUEUE=1)
endif()
if(XY_OS_FEATURE_MEMORY_POOL)
    target_compile_definitions(xy_osal PUBLIC XY_OS_FEATURE_MEMORY_POOL=1)
endif()
if(XY_OS_FEATURE_TIMER)
    target_compile_definitions(xy_osal PUBLIC XY_OS_FEATURE_TIMER=1)
endif()
if(XY_OS_FEATURE_DELAY)
    target_compile_definitions(xy_osal PUBLIC XY_OS_FEATURE_DELAY=1)
endif()

# Debug definitions
if(XY_OS_PARAM_CHECK)
    target_compile_definitions(xy_osal PUBLIC XY_OS_PARAM_CHECK=1)
endif()
if(XY_OS_ISR_CHECK)
    target_compile_definitions(xy_osal PUBLIC XY_OS_ISR_CHECK=1)
endif()
if(XY_OS_NULL_CHECK)
    target_compile_definitions(xy_osal PUBLIC XY_OS_NULL_CHECK=1)
endif()
if(XY_OS_ENABLE_STATS)
    target_compile_definitions(xy_osal PUBLIC XY_OS_ENABLE_STATS=1)
endif()

# ==================== Link Libraries ====================

# Link with backend RTOS if needed
if(OSAL_BACKEND STREQUAL "freertos")
    if(NOT TARGET freertos_kernel)
        message(FATAL_ERROR "FreeRTOS OSAL selected without the matching freertos_kernel target")
    endif()
    target_link_libraries(xy_osal PUBLIC freertos_kernel)
elseif(OSAL_BACKEND STREQUAL "rtthread")
    if(TARGET rtthread)
        target_link_libraries(xy_osal PUBLIC rtthread)
    endif()
elseif(OSAL_BACKEND STREQUAL "cmsis_rtx")
    if(TARGET rtx5)
        target_link_libraries(xy_osal PUBLIC rtx5)
    endif()
endif()

# ==================== Installation ====================

install(TARGETS xy_osal
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES
    xy_os.h
    xy_os_cfg.h
    DESTINATION include/xy_osal
)

# ==================== Examples ====================

option(BUILD_OSAL_EXAMPLES "Build OSAL examples" OFF)

if(BUILD_OSAL_EXAMPLES)
    add_executable(xy_osal_example examples/osal_example.c)
    target_link_libraries(xy_osal_example xy_osal)
endif()

# ==================== Summary ====================

message(STATUS "XY OSAL Configuration:")
message(STATUS "  Backend: ${OSAL_BACKEND}")
message(STATUS "  Thread Support: ${XY_OS_FEATURE_THREAD}")
message(STATUS "  Mutex Support: ${XY_OS_FEATURE_MUTEX}")
message(STATUS "  Semaphore Support: ${XY_OS_FEATURE_SEMAPHORE}")
message(STATUS "  Timer Support: ${XY_OS_FEATURE_TIMER}")
message(STATUS "  Parameter Checks: ${XY_OS_PARAM_CHECK}")
message(STATUS "  Build Examples: ${BUILD_OSAL_EXAMPLES}")
