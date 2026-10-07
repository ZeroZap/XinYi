set(_wch_sdk ${CMAKE_SOURCE_DIR}/MCU/wch/CH32V307EVT/EXAM/SRC)
set(_wch_system ${CMAKE_SOURCE_DIR}/MCU/wch/CH32V307EVT/EXAM/RCC/HSE_CLK/User)
set(_wch_startup ${_wch_sdk}/Startup/startup_ch32v30x_D8C.S)
set(_wch_ld ${_wch_sdk}/Ld/Link.ld)

set(XINYI_SOURCE_COMMIT "" CACHE STRING
    "Exact source commit identity for exported/clean openCH builds")
if(XINYI_SOURCE_COMMIT)
    set(_firmware_commit "${XINYI_SOURCE_COMMIT}")
    set(_firmware_commit_result 0)
else()
    execute_process(
        COMMAND git rev-parse HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE _firmware_commit
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _firmware_commit_result)
endif()
string(LENGTH "${_firmware_commit}" _firmware_commit_length)
if(NOT _firmware_commit_result EQUAL 0 OR
   NOT _firmware_commit_length EQUAL 40 OR
   NOT _firmware_commit MATCHES "^[0-9a-f]+$")
    message(FATAL_ERROR "openCH firmware requires an exact Git source commit identity")
endif()

add_executable(opench_ch32v307_uart_smoke
    main.c
    ${_wch_startup}
    ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/kernel/misc/xy_tick.c
    ${CMAKE_SOURCE_DIR}/components/kernel/osal/src/xy_os_tick.c
)

target_include_directories(opench_ch32v307_uart_smoke PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${CMAKE_SOURCE_DIR}/components/kernel/osal/inc
    ${CMAKE_SOURCE_DIR}/components/kernel/misc

    ${_wch_sdk}/Peripheral/inc
    ${_wch_sdk}/Core
    ${_wch_sdk}/Debug
    ${_wch_system}
)

target_compile_definitions(opench_ch32v307_uart_smoke PRIVATE
    MCU_CH32 CH32V30x CH32V30x_D8C
    XINYI_FIRMWARE_COMMIT="${_firmware_commit}"
)
target_link_options(opench_ch32v307_uart_smoke PRIVATE
    -T${_wch_ld}
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    -nostartfiles
    --specs=nano.specs
    --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_uart_smoke.map
)
set_target_properties(opench_ch32v307_uart_smoke PROPERTIES SUFFIX ".elf")
add_custom_command(TARGET opench_ch32v307_uart_smoke POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:opench_ch32v307_uart_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_uart_smoke.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex
            $<TARGET_FILE:opench_ch32v307_uart_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_uart_smoke.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:opench_ch32v307_uart_smoke>
    COMMENT "Generating openCH CH32V307 UART smoke BIN")
