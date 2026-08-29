##
## @file CMakeLists.txt
## @brief Third Party Libraries CMake Configuration
## @version 1.0
## @date 2026-02-28
##

cmake_minimum_required(VERSION 3.12)

project(third_party_libs C)

# ==================== RTOS Selection ====================

set(RTOS_BACKEND "none" CACHE STRING "RTOS backend selection")
set_property(CACHE RTOS_BACKEND PROPERTY STRINGS none freertos rtthread cmsis_rtx)

# ==================== FreeRTOS ====================

if(RTOS_BACKEND STREQUAL "freertos" OR RTOS_FREERTOS)
    set(USE_FREERTOS ON)

    # FreeRTOS source directory
    set(FREERTOS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/freertos/FreeRTOS)

    if(EXISTS ${FREERTOS_DIR})
        # FreeRTOS source files
        set(FREERTOS_SOURCES
            ${FREERTOS_DIR}/tasks.c
            ${FREERTOS_DIR}/queue.c
            ${FREERTOS_DIR}/list.c
            ${FREERTOS_DIR}/timers.c
            ${FREERTOS_DIR}/event_groups.c
            ${FREERTOS_DIR}/portable/MemMang/heap_4.c
        )

        # Find portable layer based on MCU
        if(HAL_PLATFORM STREQUAL "STM32U5")
            set(FREERTOS_PORT_DIR
                ${CMAKE_SOURCE_DIR}/components/kernel/osal/portable/freertos/GCC/ARM_CM33_NTZ/non_secure)
            list(APPEND FREERTOS_SOURCES
                ${FREERTOS_PORT_DIR}/port.c
                ${FREERTOS_PORT_DIR}/portasm.c
            )
            set(FREERTOS_PORT "ARM_CM33_NTZ/non_secure")
        else()
            message(FATAL_ERROR "The guarded FreeRTOS reference kernel currently supports STM32U5 only")
        endif()

        # Create FreeRTOS library
        add_library(freertos_kernel STATIC ${FREERTOS_SOURCES})

        target_include_directories(freertos_kernel PUBLIC
            ${FREERTOS_DIR}/include
            ${FREERTOS_PORT_DIR}
            ${CMAKE_SOURCE_DIR}/components/kernel/osal/config/freertos
        )

        target_compile_definitions(freertos_kernel PUBLIC
            FREERTOS
            USE_FREERTOS
        )

        message(STATUS "Third Party: FreeRTOS enabled (port: ${FREERTOS_PORT})")
    else()
        message(FATAL_ERROR "FreeRTOS source not found at ${FREERTOS_DIR}")
    endif()
endif()

# ==================== RT-Thread ====================

if(RTOS_BACKEND STREQUAL "rtthread" OR RTOS_RTTHREAD)
    set(USE_RTTHREAD ON)

    # RT-Thread source directory
    set(RTTHREAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/rt-thread)

    if(EXISTS ${RTTHREAD_DIR})
        # RT-Thread source files
        set(RTTHREAD_SOURCES
            ${RTTHREAD_DIR}/src/ipc.c
            ${RTTHREAD_DIR}/src/kservice.c
            ${RTTHREAD_DIR}/src/mem.c
            ${RTTHREAD_DIR}/src/mempool.c
            ${RTTHREAD_DIR}/src/object.c
            ${RTTHREAD_DIR}/src/scheduler.c
            ${RTTHREAD_DIR}/src/signal.c
            ${RTTHREAD_DIR}/src/thread.c
            ${RTTHREAD_DIR}/src/timer.c
        )

        # Add libcpu based on MCU
        if(MCU_FAMILY MATCHES "STM32.*")
            list(APPEND RTTHREAD_SOURCES
                ${RTTHREAD_DIR}/libcpu/ARM/Cortex-M4/context_gcc.S
                ${RTTHREAD_DIR}/libcpu/ARM/Cortex-M4/cpuport.c
            )
            set(RTTHREAD_CPU "Cortex-M4")
        else()
            message(WARNING "Unknown MCU family, using default Cortex-M4")
            list(APPEND RTTHREAD_SOURCES
                ${RTTHREAD_DIR}/libcpu/ARM/Cortex-M4/context_gcc.S
                ${RTTHREAD_DIR}/libcpu/ARM/Cortex-M4/cpuport.c
            )
            set(RTTHREAD_CPU "Cortex-M4")
        endif()

        # Create RT-Thread library
        add_library(rtthread STATIC ${RTTHREAD_SOURCES})

        target_include_directories(rtthread PUBLIC
            ${RTTHREAD_DIR}/include
            ${RTTHREAD_DIR}/libcpu/ARM/${RTTHREAD_CPU}
        )

        target_compile_definitions(rtthread PUBLIC
            RTTHREAD
            USE_RTTHREAD
        )

        message(STATUS "Third Party: RT-Thread enabled (CPU: ${RTTHREAD_CPU})")
    else()
        message(FATAL_ERROR "RT-Thread source not found at ${RTTHREAD_DIR}")
    endif()
endif()

# ==================== CMSIS-RTX ====================

if(RTOS_BACKEND STREQUAL "cmsis_rtx" OR RTOS_CMSIS_RTX)
    set(USE_CMSIS_RTX ON)

    # CMSIS-RTX source directory
    set(RTX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cmsis-rtx)

    if(EXISTS ${RTX_DIR})
        # RTX source files
        set(RTX_SOURCES
            ${RTX_DIR}/Source/rtx_kernel.c
            ${RTX_DIR}/Source/rtx_thread.c
            ${RTX_DIR}/Source/rtx_timer.c
            ${RTX_DIR}/Source/rtx_mutex.c
            ${RTX_DIR}/Source/rtx_semaphore.c
            ${RTX_DIR}/Source/rtx_msgqueue.c
            ${RTX_DIR}/Source/rtx_memory.c
            ${RTX_DIR}/Source/rtx_mempool.c
            ${RTX_DIR}/Source/rtx_evflags.c
            ${RTX_DIR}/Source/rtx_delay.c
            ${RTX_DIR}/Source/rtx_system.c
        )

        # Create RTX library
        add_library(rtx5 STATIC ${RTX_SOURCES})

        target_include_directories(rtx5 PUBLIC
            ${RTX_DIR}/Include
            ${RTX_DIR}/Config
        )

        target_compile_definitions(rtx5 PUBLIC
            CMSIS_RTOS2
            USE_CMSIS_RTX
            RTX_CORE_CM4
        )

        message(STATUS "Third Party: CMSIS-RTX enabled")
    else()
        message(FATAL_ERROR "CMSIS-RTX source not found at ${RTX_DIR}")
    endif()
endif()

# ==================== Summary ====================

message(STATUS "Third Party Configuration:")
message(STATUS "  RTOS Backend: ${RTOS_BACKEND}")
if(USE_FREERTOS)
message(STATUS "  FreeRTOS: Enabled")
endif()
if(USE_RTTHREAD)
message(STATUS "  RT-Thread: Enabled")
endif()
if(USE_CMSIS_RTX)
message(STATUS "  CMSIS-RTX: Enabled")
endif()
if(NOT USE_FREERTOS AND NOT USE_RTTHREAD AND NOT USE_CMSIS_RTX)
message(STATUS "  No RTOS selected (Bare-metal mode)")
endif()
