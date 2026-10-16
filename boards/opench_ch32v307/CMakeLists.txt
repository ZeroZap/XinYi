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

add_executable(opench_ch32v307_spi3_w25q128_smoke
    spi3_w25q128_main.c
    ${_wch_startup}
    ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_spi.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_spi.c
)
target_include_directories(opench_ch32v307_spi3_w25q128_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_spi3_w25q128_smoke PRIVATE
    MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_spi3_w25q128_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -Wl,--print-memory-usage -nostartfiles
    --specs=nano.specs --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_spi3_w25q128_smoke.map)
set_target_properties(opench_ch32v307_spi3_w25q128_smoke PROPERTIES SUFFIX ".elf")
add_custom_command(TARGET opench_ch32v307_spi3_w25q128_smoke POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:opench_ch32v307_spi3_w25q128_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_spi3_w25q128_smoke.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:opench_ch32v307_spi3_w25q128_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_spi3_w25q128_smoke.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:opench_ch32v307_spi3_w25q128_smoke>
    COMMENT "Generating openCH SPI3 W25Q128 smoke images")

add_executable(opench_ch32v307_i2c2_scan_smoke
    i2c2_scan_main.c
    ${_wch_startup}
    ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_i2c.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_i2c.c
)
target_include_directories(opench_ch32v307_i2c2_scan_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_i2c2_scan_smoke PRIVATE
    MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_i2c2_scan_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -Wl,--print-memory-usage -nostartfiles
    --specs=nano.specs --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_i2c2_scan_smoke.map)
set_target_properties(opench_ch32v307_i2c2_scan_smoke PROPERTIES SUFFIX ".elf")
add_custom_command(TARGET opench_ch32v307_i2c2_scan_smoke POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:opench_ch32v307_i2c2_scan_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_i2c2_scan_smoke.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:opench_ch32v307_i2c2_scan_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_i2c2_scan_smoke.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:opench_ch32v307_i2c2_scan_smoke>
    COMMENT "Generating openCH I2C2 scan smoke images")

add_executable(opench_ch32v307_uart67_wireless_smoke
    uart67_wireless_main.c
    ${_wch_startup}
    ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
)
target_include_directories(opench_ch32v307_uart67_wireless_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_uart67_wireless_smoke PRIVATE
    MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_uart67_wireless_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -Wl,--print-memory-usage -nostartfiles
    --specs=nano.specs --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_uart67_wireless_smoke.map)
set_target_properties(opench_ch32v307_uart67_wireless_smoke PROPERTIES SUFFIX ".elf")
add_custom_command(TARGET opench_ch32v307_uart67_wireless_smoke POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:opench_ch32v307_uart67_wireless_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_uart67_wireless_smoke.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:opench_ch32v307_uart67_wireless_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_uart67_wireless_smoke.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:opench_ch32v307_uart67_wireless_smoke>
    COMMENT "Generating openCH UART6/UART7 wireless smoke images")

add_executable(opench_ch32v307_uart67_dma_smoke
    uart67_dma_main.c
    ${_wch_startup}
    ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_dma.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_dma.c
)
target_include_directories(opench_ch32v307_uart67_dma_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_uart67_dma_smoke PRIVATE
    MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_uart67_dma_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -Wl,--print-memory-usage -nostartfiles
    --specs=nano.specs --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_uart67_dma_smoke.map)
set_target_properties(opench_ch32v307_uart67_dma_smoke PROPERTIES SUFFIX ".elf")
add_custom_command(TARGET opench_ch32v307_uart67_dma_smoke POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:opench_ch32v307_uart67_dma_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_uart67_dma_smoke.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:opench_ch32v307_uart67_dma_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_uart67_dma_smoke.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:opench_ch32v307_uart67_dma_smoke>
    COMMENT "Generating openCH UART6/UART7 DMA smoke images")

add_executable(opench_ch32v307_adc1_pa0_smoke
    adc1_pa0_main.c
    ${_wch_startup}
    ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_adc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_adc.c)
target_include_directories(opench_ch32v307_adc1_pa0_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_adc1_pa0_smoke PRIVATE MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_adc1_pa0_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -Wl,--print-memory-usage -nostartfiles
    --specs=nano.specs --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_adc1_pa0_smoke.map)
set_target_properties(opench_ch32v307_adc1_pa0_smoke PROPERTIES SUFFIX ".elf")
add_custom_command(TARGET opench_ch32v307_adc1_pa0_smoke POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:opench_ch32v307_adc1_pa0_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_adc1_pa0_smoke.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:opench_ch32v307_adc1_pa0_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_adc1_pa0_smoke.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:opench_ch32v307_adc1_pa0_smoke>
    COMMENT "Generating openCH ADC1 PA0 smoke images")

add_executable(opench_ch32v307_pwm_tim3_pa6_smoke
    pwm_tim3_pa6_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_tim.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_pwm.c)
target_include_directories(opench_ch32v307_pwm_tim3_pa6_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_pwm_tim3_pa6_smoke PRIVATE MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_pwm_tim3_pa6_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -Wl,--print-memory-usage -nostartfiles
    --specs=nano.specs --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_pwm_tim3_pa6_smoke.map)
set_target_properties(opench_ch32v307_pwm_tim3_pa6_smoke PROPERTIES SUFFIX ".elf")
add_custom_command(TARGET opench_ch32v307_pwm_tim3_pa6_smoke POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:opench_ch32v307_pwm_tim3_pa6_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_pwm_tim3_pa6_smoke.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:opench_ch32v307_pwm_tim3_pa6_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_pwm_tim3_pa6_smoke.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:opench_ch32v307_pwm_tim3_pa6_smoke>
    COMMENT "Generating openCH TIM3 CH1 PA6 PWM smoke images")

add_executable(opench_ch32v307_can1_loopback_smoke
    can1_loopback_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_can.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_dbgmcu.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_can.c)
target_include_directories(opench_ch32v307_can1_loopback_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_can1_loopback_smoke PRIVATE MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_can1_loopback_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -Wl,--print-memory-usage -nostartfiles
    --specs=nano.specs --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_can1_loopback_smoke.map)
set_target_properties(opench_ch32v307_can1_loopback_smoke PROPERTIES SUFFIX ".elf")
add_custom_command(TARGET opench_ch32v307_can1_loopback_smoke POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:opench_ch32v307_can1_loopback_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_can1_loopback_smoke.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:opench_ch32v307_can1_loopback_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_can1_loopback_smoke.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:opench_ch32v307_can1_loopback_smoke>
    COMMENT "Generating openCH CAN1 loopback smoke images")

add_executable(opench_ch32v307_tim2_counter_smoke
    tim2_counter_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_tim.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_timer.c)
target_include_directories(opench_ch32v307_tim2_counter_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_tim2_counter_smoke PRIVATE MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_tim2_counter_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -Wl,--print-memory-usage -nostartfiles
    --specs=nano.specs --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_tim2_counter_smoke.map)
set_target_properties(opench_ch32v307_tim2_counter_smoke PROPERTIES SUFFIX ".elf")
add_custom_command(TARGET opench_ch32v307_tim2_counter_smoke POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:opench_ch32v307_tim2_counter_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_tim2_counter_smoke.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:opench_ch32v307_tim2_counter_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_tim2_counter_smoke.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:opench_ch32v307_tim2_counter_smoke>
    COMMENT "Generating openCH TIM2 counter smoke images")

add_executable(opench_ch32v307_rng_smoke
    rng_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rng.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_rng.c)
target_include_directories(opench_ch32v307_rng_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_rng_smoke PRIVATE MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_rng_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -Wl,--print-memory-usage -nostartfiles
    --specs=nano.specs --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_rng_smoke.map)
set_target_properties(opench_ch32v307_rng_smoke PROPERTIES SUFFIX ".elf")
add_custom_command(TARGET opench_ch32v307_rng_smoke POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:opench_ch32v307_rng_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_rng_smoke.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:opench_ch32v307_rng_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_rng_smoke.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:opench_ch32v307_rng_smoke>
    COMMENT "Generating openCH hardware RNG smoke images")

add_executable(opench_ch32v307_rtc_lsi_smoke
    rtc_lsi_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rtc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_bkp.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_pwr.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_rtc.c)
target_include_directories(opench_ch32v307_rtc_lsi_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_rtc_lsi_smoke PRIVATE MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_rtc_lsi_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -Wl,--print-memory-usage -nostartfiles
    --specs=nano.specs --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_rtc_lsi_smoke.map)
set_target_properties(opench_ch32v307_rtc_lsi_smoke PROPERTIES SUFFIX ".elf")
add_custom_command(TARGET opench_ch32v307_rtc_lsi_smoke POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:opench_ch32v307_rtc_lsi_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_rtc_lsi_smoke.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:opench_ch32v307_rtc_lsi_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_rtc_lsi_smoke.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:opench_ch32v307_rtc_lsi_smoke>
    COMMENT "Generating openCH RTC LSI smoke images")

add_executable(opench_ch32v307_delay_smoke
    delay_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_delay.c)
target_include_directories(opench_ch32v307_delay_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_delay_smoke PRIVATE MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_delay_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -Wl,--print-memory-usage -nostartfiles
    --specs=nano.specs --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_delay_smoke.map)
set_target_properties(opench_ch32v307_delay_smoke PROPERTIES SUFFIX ".elf")
add_custom_command(TARGET opench_ch32v307_delay_smoke POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:opench_ch32v307_delay_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_delay_smoke.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:opench_ch32v307_delay_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_delay_smoke.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:opench_ch32v307_delay_smoke>
    COMMENT "Generating openCH blocking delay smoke images")

add_executable(opench_ch32v307_sys_identity_smoke
    sys_identity_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_dbgmcu.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_sys.c)
target_include_directories(opench_ch32v307_sys_identity_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_sys_identity_smoke PRIVATE
    MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_sys_identity_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -Wl,--print-memory-usage -nostartfiles
    --specs=nano.specs --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_sys_identity_smoke.map)
set_target_properties(opench_ch32v307_sys_identity_smoke PROPERTIES SUFFIX ".elf")
add_custom_command(TARGET opench_ch32v307_sys_identity_smoke POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:opench_ch32v307_sys_identity_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_sys_identity_smoke.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:opench_ch32v307_sys_identity_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_sys_identity_smoke.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:opench_ch32v307_sys_identity_smoke>
    COMMENT "Generating openCH SYS identity smoke images")

add_executable(opench_ch32v307_flash_info_smoke
    flash_info_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_flash.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_flash.c)
target_include_directories(opench_ch32v307_flash_info_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_flash_info_smoke PRIVATE
    MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_flash_info_smoke PRIVATE
    -T${CMAKE_CURRENT_SOURCE_DIR}/flash_test.ld -Wl,--gc-sections -Wl,--print-memory-usage -nostartfiles
    --specs=nano.specs --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_flash_info_smoke.map)
set_target_properties(opench_ch32v307_flash_info_smoke PROPERTIES SUFFIX ".elf")
add_custom_command(TARGET opench_ch32v307_flash_info_smoke POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:opench_ch32v307_flash_info_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_flash_info_smoke.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:opench_ch32v307_flash_info_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_flash_info_smoke.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:opench_ch32v307_flash_info_smoke>
    COMMENT "Generating openCH Flash information smoke images")

add_executable(opench_ch32v307_dac1_adc_smoke
    dac1_adc_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_dac.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_adc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_dac.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_adc.c)
target_include_directories(opench_ch32v307_dac1_adc_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_dac1_adc_smoke PRIVATE MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_dac1_adc_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -Wl,--print-memory-usage -nostartfiles
    --specs=nano.specs --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_dac1_adc_smoke.map)
set_target_properties(opench_ch32v307_dac1_adc_smoke PROPERTIES SUFFIX ".elf")
add_custom_command(TARGET opench_ch32v307_dac1_adc_smoke POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:opench_ch32v307_dac1_adc_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_dac1_adc_smoke.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:opench_ch32v307_dac1_adc_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_dac1_adc_smoke.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:opench_ch32v307_dac1_adc_smoke>
    COMMENT "Generating openCH DAC1-to-ADC smoke images")

add_executable(opench_ch32v307_i2s2_tx_smoke
    i2s2_tx_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_spi.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_i2s.c)
target_include_directories(opench_ch32v307_i2s2_tx_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_i2s2_tx_smoke PRIVATE MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_i2s2_tx_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -Wl,--print-memory-usage -nostartfiles
    --specs=nano.specs --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_i2s2_tx_smoke.map)
set_target_properties(opench_ch32v307_i2s2_tx_smoke PROPERTIES SUFFIX ".elf")
add_custom_command(TARGET opench_ch32v307_i2s2_tx_smoke POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:opench_ch32v307_i2s2_tx_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_i2s2_tx_smoke.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:opench_ch32v307_i2s2_tx_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_i2s2_tx_smoke.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:opench_ch32v307_i2s2_tx_smoke>
    COMMENT "Generating openCH I2S2 master-TX smoke images")

add_executable(opench_ch32v307_exti0_sw_irq_smoke
    exti0_sw_irq_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_exti.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_exti.c)
target_include_directories(opench_ch32v307_exti0_sw_irq_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_exti0_sw_irq_smoke PRIVATE MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_exti0_sw_irq_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -Wl,--print-memory-usage -nostartfiles
    --specs=nano.specs --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_exti0_sw_irq_smoke.map)
set_target_properties(opench_ch32v307_exti0_sw_irq_smoke PROPERTIES SUFFIX ".elf")
add_custom_command(TARGET opench_ch32v307_exti0_sw_irq_smoke POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:opench_ch32v307_exti0_sw_irq_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_exti0_sw_irq_smoke.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:opench_ch32v307_exti0_sw_irq_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_exti0_sw_irq_smoke.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:opench_ch32v307_exti0_sw_irq_smoke>
    COMMENT "Generating openCH EXTI0 software IRQ smoke images")

add_executable(opench_ch32v307_iwdg_smoke
    iwdg_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_iwdg.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_wwdg.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_wdg.c)
target_include_directories(opench_ch32v307_iwdg_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_iwdg_smoke PRIVATE MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_iwdg_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -Wl,--print-memory-usage -nostartfiles
    --specs=nano.specs --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_iwdg_smoke.map)
set_target_properties(opench_ch32v307_iwdg_smoke PROPERTIES SUFFIX ".elf")
add_custom_command(TARGET opench_ch32v307_iwdg_smoke POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:opench_ch32v307_iwdg_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_iwdg_smoke.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:opench_ch32v307_iwdg_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_iwdg_smoke.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:opench_ch32v307_iwdg_smoke>
    COMMENT "Generating openCH IWDG smoke images")

add_executable(opench_ch32v307_sys_reset_reason_smoke
    sys_reset_reason_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_dbgmcu.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_pwr.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_bkp.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_sys.c)
target_include_directories(opench_ch32v307_sys_reset_reason_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_sys_reset_reason_smoke PRIVATE MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_sys_reset_reason_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -Wl,--print-memory-usage -nostartfiles
    --specs=nano.specs --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_sys_reset_reason_smoke.map)
set_target_properties(opench_ch32v307_sys_reset_reason_smoke PROPERTIES SUFFIX ".elf")
add_custom_command(TARGET opench_ch32v307_sys_reset_reason_smoke POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:opench_ch32v307_sys_reset_reason_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_sys_reset_reason_smoke.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:opench_ch32v307_sys_reset_reason_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_sys_reset_reason_smoke.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:opench_ch32v307_sys_reset_reason_smoke>
    COMMENT "Generating openCH System reset-reason smoke images")

add_executable(opench_ch32v307_w25q128_rw_restore_smoke
    w25q128_rw_restore_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_spi.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_spi.c)
target_include_directories(opench_ch32v307_w25q128_rw_restore_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_w25q128_rw_restore_smoke PRIVATE
    MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_w25q128_rw_restore_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_w25q128_rw_restore_smoke.map)
set_target_properties(opench_ch32v307_w25q128_rw_restore_smoke PROPERTIES SUFFIX ".elf")
add_custom_command(TARGET opench_ch32v307_w25q128_rw_restore_smoke POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:opench_ch32v307_w25q128_rw_restore_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_w25q128_rw_restore_smoke.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:opench_ch32v307_w25q128_rw_restore_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_w25q128_rw_restore_smoke.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:opench_ch32v307_w25q128_rw_restore_smoke>
    COMMENT "Generating openCH W25Q128 reversible R/W smoke images")

add_executable(opench_ch32v307_uart67_peer_smoke
    uart67_peer_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c)
target_include_directories(opench_ch32v307_uart67_peer_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_uart67_peer_smoke PRIVATE MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_uart67_peer_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_uart67_peer_smoke.map)
set_target_properties(opench_ch32v307_uart67_peer_smoke PROPERTIES SUFFIX ".elf")
add_custom_command(TARGET opench_ch32v307_uart67_peer_smoke POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:opench_ch32v307_uart67_peer_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_uart67_peer_smoke.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:opench_ch32v307_uart67_peer_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_uart67_peer_smoke.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:opench_ch32v307_uart67_peer_smoke>)

add_executable(opench_ch32v307_i2c2_hal_devices_smoke
    i2c2_hal_devices_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_i2c.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_i2c.c)
target_include_directories(opench_ch32v307_i2c2_hal_devices_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_i2c2_hal_devices_smoke PRIVATE MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_i2c2_hal_devices_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_i2c2_hal_devices_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_aht10_protocol_smoke
    aht10_protocol_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_i2c.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_i2c.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_delay.c)
target_include_directories(opench_ch32v307_aht10_protocol_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_aht10_protocol_smoke PRIVATE
    MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_aht10_protocol_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_aht10_protocol_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_ap3216c_protocol_smoke
    ap3216c_protocol_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_i2c.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_i2c.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_delay.c)
target_include_directories(opench_ch32v307_ap3216c_protocol_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_ap3216c_protocol_smoke PRIVATE
    MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_ap3216c_protocol_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_ap3216c_protocol_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_mpu6050_sample_smoke
    mpu6050_sample_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_i2c.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_i2c.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_delay.c)
target_include_directories(opench_ch32v307_mpu6050_sample_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_mpu6050_sample_smoke PRIVATE
    MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_mpu6050_sample_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_mpu6050_sample_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_es8388_registers_smoke
    es8388_registers_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_i2c.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_i2c.c)
target_include_directories(opench_ch32v307_es8388_registers_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_es8388_registers_smoke PRIVATE
    MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_es8388_registers_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_es8388_registers_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_i2c2_recovery_smoke
    i2c2_recovery_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_i2c.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_i2c.c)
target_include_directories(opench_ch32v307_i2c2_recovery_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_i2c2_recovery_smoke PRIVATE
    MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_i2c2_recovery_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_i2c2_recovery_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_i2c2_probe_diag
    i2c2_probe_diag_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_i2c.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_i2c.c)
target_include_directories(opench_ch32v307_i2c2_probe_diag PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_i2c2_probe_diag PRIVATE MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_i2c2_probe_diag PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_i2c2_probe_diag PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_spi3_systick8_smoke
    spi3_systick8_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_spi.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_spi.c)
target_include_directories(opench_ch32v307_spi3_systick8_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_spi3_systick8_smoke PRIVATE
    MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_spi3_systick8_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_spi3_systick8_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_uart67_systick8_smoke
    uart67_systick8_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c)
target_include_directories(opench_ch32v307_uart67_systick8_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_uart67_systick8_smoke PRIVATE
    MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_uart67_systick8_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_uart67_systick8_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_adc1_systick8_smoke
    adc1_systick8_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_adc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_dac.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_adc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_dac.c)
target_include_directories(opench_ch32v307_adc1_systick8_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_adc1_systick8_smoke PRIVATE
    MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_adc1_systick8_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_adc1_systick8_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_can1_systick8_smoke
    can1_systick8_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_can.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_dbgmcu.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_can.c)
target_include_directories(opench_ch32v307_can1_systick8_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_can1_systick8_smoke PRIVATE
    MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_can1_systick8_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_can1_systick8_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_dma1_systick8_smoke
    dma1_systick8_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_dma.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_dma.c)
target_include_directories(opench_ch32v307_dma1_systick8_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_dma1_systick8_smoke PRIVATE
    MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_dma1_systick8_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_dma1_systick8_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_i2s2_systick_source_smoke
    i2s2_systick_source_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_spi.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_i2s.c)
target_include_directories(opench_ch32v307_i2s2_systick_source_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_i2s2_systick_source_smoke PRIVATE
    MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_i2s2_systick_source_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_i2s2_systick_source_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_system_systick_source_smoke
    system_systick_source_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_dbgmcu.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_sys.c)
target_include_directories(opench_ch32v307_system_systick_source_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_system_systick_source_smoke PRIVATE
    MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_system_systick_source_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_system_systick_source_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_delay_systick_source_smoke
    delay_systick_source_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_delay.c)
target_include_directories(opench_ch32v307_delay_systick_source_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_delay_systick_source_smoke PRIVATE
    MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_delay_systick_source_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_delay_systick_source_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_rng_systick_source_smoke
    rng_systick_source_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rng.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_rng.c)
target_include_directories(opench_ch32v307_rng_systick_source_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_rng_systick_source_smoke PRIVATE
    MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_rng_systick_source_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_rng_systick_source_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_adc1_prescaler_smoke
    adc1_prescaler_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_adc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_dac.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_adc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_dac.c)
target_include_directories(opench_ch32v307_adc1_prescaler_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_adc1_prescaler_smoke PRIVATE
    MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_adc1_prescaler_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_adc1_prescaler_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_dac1_8bit_alignment_smoke
    dac1_8bit_alignment_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_adc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_dac.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_adc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_dac.c)
target_include_directories(opench_ch32v307_dac1_8bit_alignment_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_dac1_8bit_alignment_smoke PRIVATE
    MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_dac1_8bit_alignment_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_dac1_8bit_alignment_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_dac_wave_register_smoke
    dac_wave_register_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_dac.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_dac.c)
target_include_directories(opench_ch32v307_dac_wave_register_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_dac_wave_register_smoke PRIVATE
    MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_dac_wave_register_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_dac_wave_register_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_dac_trigger_boundary_smoke
    dac_trigger_boundary_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_dac.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_dac.c)
target_include_directories(opench_ch32v307_dac_trigger_boundary_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_dac_trigger_boundary_smoke PRIVATE
    MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_dac_trigger_boundary_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_dac_trigger_boundary_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_exti_grouped_irq_smoke
    exti_grouped_irq_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_exti.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_exti.c)
target_include_directories(opench_ch32v307_exti_grouped_irq_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_exti_grouped_irq_smoke PRIVATE
    MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_exti_grouped_irq_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_exti_grouped_irq_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_exti_pending_state_smoke
    exti_pending_state_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_exti.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_exti.c)
target_include_directories(opench_ch32v307_exti_pending_state_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_exti_pending_state_smoke PRIVATE
    MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_exti_pending_state_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_exti_pending_state_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_exti_lifecycle_smoke
    exti_lifecycle_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_exti.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_exti.c)
target_include_directories(opench_ch32v307_exti_lifecycle_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_exti_lifecycle_smoke PRIVATE MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_exti_lifecycle_smoke PRIVATE
    -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_exti_lifecycle_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_gpio_write_state_smoke
    gpio_write_state_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c)
target_include_directories(opench_ch32v307_gpio_write_state_smoke PRIVATE ${CMAKE_SOURCE_DIR}/components/hal/inc ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_gpio_write_state_smoke PRIVATE MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_gpio_write_state_smoke PRIVATE -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_gpio_write_state_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_gpio_batch_atomic_smoke
    gpio_batch_atomic_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c)
target_include_directories(opench_ch32v307_gpio_batch_atomic_smoke PRIVATE ${CMAKE_SOURCE_DIR}/components/hal/inc ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_gpio_batch_atomic_smoke PRIVATE MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_gpio_batch_atomic_smoke PRIVATE -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_gpio_batch_atomic_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_gpio_batch_read_smoke
    gpio_batch_read_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c)
target_include_directories(opench_ch32v307_gpio_batch_read_smoke PRIVATE ${CMAKE_SOURCE_DIR}/components/hal/inc ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_gpio_batch_read_smoke PRIVATE MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_gpio_batch_read_smoke PRIVATE -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_gpio_batch_read_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_gpio_read_state_smoke
    gpio_read_state_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c)
target_include_directories(opench_ch32v307_gpio_read_state_smoke PRIVATE ${CMAKE_SOURCE_DIR}/components/hal/inc ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_gpio_read_state_smoke PRIVATE MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_gpio_read_state_smoke PRIVATE -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_gpio_read_state_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_gpio_deinit_state_smoke
    gpio_deinit_state_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c)
target_include_directories(opench_ch32v307_gpio_deinit_state_smoke PRIVATE ${CMAKE_SOURCE_DIR}/components/hal/inc ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_gpio_deinit_state_smoke PRIVATE MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_gpio_deinit_state_smoke PRIVATE -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_gpio_deinit_state_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_gpio_alternate_boundary_smoke
    gpio_alternate_boundary_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c)
target_include_directories(opench_ch32v307_gpio_alternate_boundary_smoke PRIVATE ${CMAKE_SOURCE_DIR}/components/hal/inc ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_gpio_alternate_boundary_smoke PRIVATE MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_gpio_alternate_boundary_smoke PRIVATE -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_gpio_alternate_boundary_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_gpio_reinit_boundary_smoke
    gpio_reinit_boundary_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c)
target_include_directories(opench_ch32v307_gpio_reinit_boundary_smoke PRIVATE ${CMAKE_SOURCE_DIR}/components/hal/inc ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_gpio_reinit_boundary_smoke PRIVATE MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_gpio_reinit_boundary_smoke PRIVATE -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_gpio_reinit_boundary_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_gpio_init_validation_smoke
    gpio_init_validation_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c)
target_include_directories(opench_ch32v307_gpio_init_validation_smoke PRIVATE ${CMAKE_SOURCE_DIR}/components/hal/inc ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_gpio_init_validation_smoke PRIVATE MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_gpio_init_validation_smoke PRIVATE -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_gpio_init_validation_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_gpio_setter_validation_smoke
    gpio_setter_validation_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c)
target_include_directories(opench_ch32v307_gpio_setter_validation_smoke PRIVATE ${CMAKE_SOURCE_DIR}/components/hal/inc ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_gpio_setter_validation_smoke PRIVATE MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_gpio_setter_validation_smoke PRIVATE -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_gpio_setter_validation_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_uart_lifecycle_smoke
    uart_lifecycle_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c)
target_include_directories(opench_ch32v307_uart_lifecycle_smoke PRIVATE ${CMAKE_SOURCE_DIR}/components/hal/inc ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_uart_lifecycle_smoke PRIVATE MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_uart_lifecycle_smoke PRIVATE -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_uart_lifecycle_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_uart_callback_isolation_smoke
    uart_callback_isolation_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c)
target_include_directories(opench_ch32v307_uart_callback_isolation_smoke PRIVATE ${CMAKE_SOURCE_DIR}/components/hal/inc ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_uart_callback_isolation_smoke PRIVATE MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_uart_callback_isolation_smoke PRIVATE -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_uart_callback_isolation_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_uart_aux_state_smoke
    uart_aux_state_main.c ${_wch_startup} ${_wch_system}/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src/xy_hal_gpio.c)
target_include_directories(opench_ch32v307_uart_aux_state_smoke PRIVATE ${CMAKE_SOURCE_DIR}/components/hal/inc ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_system})
target_compile_definitions(opench_ch32v307_uart_aux_state_smoke PRIVATE MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_uart_aux_state_smoke PRIVATE -T${_wch_ld} -Wl,--gc-sections -nostartfiles --specs=nano.specs --specs=nosys.specs)
set_target_properties(opench_ch32v307_uart_aux_state_smoke PROPERTIES SUFFIX ".elf")

# FreeRTOS bring-up intentionally consumes the unmodified kernel, RISC-V port,
# startup and linker script from the pinned WCH SDK example.
set(_wch_freertos ${CMAKE_SOURCE_DIR}/MCU/wch/CH32V307EVT/EXAM/FreeRTOS/FreeRTOS_Core)
set(_wch_freertos_kernel ${_wch_freertos}/FreeRTOS)
set(_wch_freertos_port ${_wch_freertos_kernel}/portable/GCC/RISC-V)
add_executable(opench_ch32v307_freertos_sdk_port_smoke
    freertos_sdk_port_main.c
    ${_wch_freertos}/Startup/startup_ch32v30x_D8C.S
    ${_wch_freertos}/User/system_ch32v30x.c
    ${_wch_freertos_kernel}/tasks.c
    ${_wch_freertos_kernel}/queue.c
    ${_wch_freertos_kernel}/list.c
    ${_wch_freertos_kernel}/timers.c
    ${_wch_freertos_kernel}/portable/MemMang/heap_4.c
    ${_wch_freertos_port}/port.c
    ${_wch_freertos_port}/portASM.S
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_tim.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c)
target_include_directories(opench_ch32v307_freertos_sdk_port_smoke PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${_wch_freertos_kernel}/include
    ${_wch_freertos_port}
    ${_wch_freertos_port}/chip_specific_extensions/RV32I_PFIC_no_extensions
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_freertos}/User)
target_compile_definitions(opench_ch32v307_freertos_sdk_port_smoke PRIVATE
    MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_options(opench_ch32v307_freertos_sdk_port_smoke PRIVATE
    -T${_wch_freertos}/Ld/Link.ld -Wl,--gc-sections -Wl,--print-memory-usage
    -nostartfiles --specs=nano.specs --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_freertos_sdk_port_smoke.map)
set_target_properties(opench_ch32v307_freertos_sdk_port_smoke PROPERTIES SUFFIX ".elf")

if(CONFIG_OSAL_BACKEND_FREERTOS)
add_executable(opench_ch32v307_osal_freertos_sdk_smoke
    osal_freertos_sdk_main.c
    ${_wch_freertos}/Startup/startup_ch32v30x_D8C.S
    ${_wch_freertos}/User/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_tim.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c)
target_include_directories(opench_ch32v307_osal_freertos_sdk_smoke PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/components/kernel/osal
    ${CMAKE_SOURCE_DIR}/components/kernel/osal/inc
    ${_wch_freertos_kernel}/include
    ${_wch_freertos_port}
    ${_wch_freertos_port}/chip_specific_extensions/RV32I_PFIC_no_extensions
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_freertos}/User)
target_compile_definitions(opench_ch32v307_osal_freertos_sdk_smoke PRIVATE
    MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_libraries(opench_ch32v307_osal_freertos_sdk_smoke PRIVATE
    xy_osal freertos_kernel)
target_link_options(opench_ch32v307_osal_freertos_sdk_smoke PRIVATE
    -T${_wch_freertos}/Ld/Link.ld -Wl,--gc-sections -Wl,--print-memory-usage
    -nostartfiles --specs=nano.specs --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_osal_freertos_sdk_smoke.map)
set_target_properties(opench_ch32v307_osal_freertos_sdk_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_osal_freertos_primitives_smoke
        osal_freertos_primitives_main.c
        ${_wch_freertos}/Startup/startup_ch32v30x_D8C.S
        ${_wch_freertos}/User/system_ch32v30x.c
        ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
        ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
        ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
        ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c)
    target_include_directories(opench_ch32v307_osal_freertos_primitives_smoke PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/components/kernel/osal
        ${CMAKE_SOURCE_DIR}/components/kernel/osal/inc
        ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_freertos}/User)
    target_compile_definitions(opench_ch32v307_osal_freertos_primitives_smoke PRIVATE
        MCU_CH32 CH32V30x CH32V30x_D8C)
    target_link_libraries(opench_ch32v307_osal_freertos_primitives_smoke PRIVATE
        xy_osal freertos_kernel)
    target_link_options(opench_ch32v307_osal_freertos_primitives_smoke PRIVATE
        -T${_wch_freertos}/Ld/Link.ld -Wl,--gc-sections -Wl,--print-memory-usage
        -nostartfiles --specs=nano.specs --specs=nosys.specs
        -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_osal_freertos_primitives_smoke.map)
set_target_properties(opench_ch32v307_osal_freertos_primitives_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_osal_freertos_isr_stress_smoke
    osal_freertos_isr_stress_main.c
    ${_wch_freertos}/Startup/startup_ch32v30x_D8C.S
    ${_wch_freertos}/User/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_tim.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c)
target_include_directories(opench_ch32v307_osal_freertos_isr_stress_smoke PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/components/kernel/osal
    ${CMAKE_SOURCE_DIR}/components/kernel/osal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_freertos}/User)
target_compile_definitions(opench_ch32v307_osal_freertos_isr_stress_smoke PRIVATE
    MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_libraries(opench_ch32v307_osal_freertos_isr_stress_smoke PRIVATE
    xy_osal freertos_kernel)
target_link_options(opench_ch32v307_osal_freertos_isr_stress_smoke PRIVATE
    -T${_wch_freertos}/Ld/Link.ld -Wl,--gc-sections -Wl,--print-memory-usage
    -nostartfiles --specs=nano.specs --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_osal_freertos_isr_stress_smoke.map)
set_target_properties(opench_ch32v307_osal_freertos_isr_stress_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_osal_freertos_timer_flags_smoke
    osal_freertos_timer_flags_main.c
    ${_wch_freertos}/Startup/startup_ch32v30x_D8C.S
    ${_wch_freertos}/User/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c)
target_include_directories(opench_ch32v307_osal_freertos_timer_flags_smoke PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/components/kernel/osal
    ${CMAKE_SOURCE_DIR}/components/kernel/osal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_freertos}/User)
target_compile_definitions(opench_ch32v307_osal_freertos_timer_flags_smoke PRIVATE
    MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_libraries(opench_ch32v307_osal_freertos_timer_flags_smoke PRIVATE
    xy_osal freertos_kernel)
target_link_options(opench_ch32v307_osal_freertos_timer_flags_smoke PRIVATE
    -T${_wch_freertos}/Ld/Link.ld -Wl,--gc-sections -Wl,--print-memory-usage
    -nostartfiles --specs=nano.specs --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_osal_freertos_timer_flags_smoke.map)
set_target_properties(opench_ch32v307_osal_freertos_timer_flags_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_osal_freertos_oneshot_timer_smoke
    osal_freertos_oneshot_timer_main.c
    ${_wch_freertos}/Startup/startup_ch32v30x_D8C.S
    ${_wch_freertos}/User/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c)
target_include_directories(opench_ch32v307_osal_freertos_oneshot_timer_smoke PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/components/kernel/osal
    ${CMAKE_SOURCE_DIR}/components/kernel/osal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_freertos}/User)
target_compile_definitions(opench_ch32v307_osal_freertos_oneshot_timer_smoke PRIVATE
    MCU_CH32 CH32V30x CH32V30x_D8C)
target_link_libraries(opench_ch32v307_osal_freertos_oneshot_timer_smoke PRIVATE
    xy_osal freertos_kernel)
target_link_options(opench_ch32v307_osal_freertos_oneshot_timer_smoke PRIVATE
    -T${_wch_freertos}/Ld/Link.ld -Wl,--gc-sections -Wl,--print-memory-usage
    -nostartfiles --specs=nano.specs --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_osal_freertos_oneshot_timer_smoke.map)
set_target_properties(opench_ch32v307_osal_freertos_oneshot_timer_smoke PROPERTIES SUFFIX ".elf")

add_executable(opench_ch32v307_osal_freertos_queue_lifecycle_smoke
    osal_freertos_queue_lifecycle_main.c
    ${_wch_freertos}/Startup/startup_ch32v30x_D8C.S
    ${_wch_freertos}/User/system_ch32v30x.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_rcc.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_usart.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_gpio.c
    ${_wch_sdk}/Peripheral/src/ch32v30x_misc.c)
target_include_directories(opench_ch32v307_osal_freertos_queue_lifecycle_smoke PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/components/kernel/osal
    ${CMAKE_SOURCE_DIR}/components/kernel/osal/inc
    ${_wch_sdk}/Peripheral/inc ${_wch_sdk}/Core ${_wch_sdk}/Debug ${_wch_freertos}/User)
target_compile_definitions(opench_ch32v307_osal_freertos_queue_lifecycle_smoke PRIVATE
    MCU_CH32 CH32V30x CH32V30x_D8C
    XINYI_FIRMWARE_COMMIT="${_firmware_commit}")
target_link_libraries(opench_ch32v307_osal_freertos_queue_lifecycle_smoke PRIVATE
    xy_osal freertos_kernel)
target_link_options(opench_ch32v307_osal_freertos_queue_lifecycle_smoke PRIVATE
    -T${_wch_freertos}/Ld/Link.ld -Wl,--gc-sections -Wl,--print-memory-usage
    -nostartfiles --specs=nano.specs --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/opench_ch32v307_osal_freertos_queue_lifecycle_smoke.map)
set_target_properties(opench_ch32v307_osal_freertos_queue_lifecycle_smoke PROPERTIES SUFFIX ".elf")
endif()
