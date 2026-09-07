set(_cube ${CMAKE_SOURCE_DIR}/MCU/ST/STM32L4/STM32CubeL4)
set(_hal ${_cube}/Drivers/STM32L4xx_HAL_Driver/Src)
set(_cmsis ${_cube}/Drivers/CMSIS/Device/ST/STM32L4xx)

execute_process(
    COMMAND git rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE _firmware_commit
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _firmware_commit_result)
string(LENGTH "${_firmware_commit}" _firmware_commit_length)
if(NOT _firmware_commit_result EQUAL 0 OR
   NOT _firmware_commit_length EQUAL 40 OR
   NOT _firmware_commit MATCHES "^[0-9a-f]+$")
    message(FATAL_ERROR "Pandora firmware requires an exact Git source commit identity")
endif()

add_executable(pandora_stm32l475_smoke
    main.c
    pandora_fota_flash.c
    pandora_soft_i2c.c
    pandora_sys.c
    ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32l4/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/fota/xy_fota.c
    ${CMAKE_SOURCE_DIR}/components/fota/src/xy_fota_metadata.c
    ${CMAKE_SOURCE_DIR}/components/device/src/xy_device_bus_helpers.c
    ${_cmsis}/Source/Templates/gcc/startup_stm32l475xx.s
    ${_cmsis}/Source/Templates/system_stm32l4xx.c
    ${_hal}/stm32l4xx_hal.c
    ${_hal}/stm32l4xx_hal_cortex.c
    ${_hal}/stm32l4xx_hal_dma.c
    ${_hal}/stm32l4xx_hal_flash.c
    ${_hal}/stm32l4xx_hal_flash_ex.c
    ${_hal}/stm32l4xx_hal_gpio.c
    ${_hal}/stm32l4xx_hal_iwdg.c
    ${_hal}/stm32l4xx_hal_pwr.c
    ${_hal}/stm32l4xx_hal_pwr_ex.c
    ${_hal}/stm32l4xx_hal_rcc.c
    ${_hal}/stm32l4xx_hal_rcc_ex.c
    ${_hal}/stm32l4xx_hal_uart.c
)

target_include_directories(pandora_stm32l475_smoke BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/components/device/inc
    ${CMAKE_SOURCE_DIR}/components/device
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${CMAKE_SOURCE_DIR}/components/fota/inc
    ${CMAKE_SOURCE_DIR}/components/sys/xy_sys
    ${CMAKE_SOURCE_DIR}/components/clib/xy_clib)
target_compile_definitions(pandora_stm32l475_smoke PRIVATE
    STM32L475xx
    USE_HAL_DRIVER
    XINYI_FIRMWARE_COMMIT="${_firmware_commit}")
target_link_options(pandora_stm32l475_smoke PRIVATE
    -T${CMAKE_CURRENT_SOURCE_DIR}/STM32L475VETX_FLASH.ld
    --specs=nano.specs
    --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/pandora_stm32l475_smoke.map
    -Wl,-u,xy_sys_reset
    -Wl,-u,xy_sys_reboot_reason
    -Wl,-u,xy_sys_get_chip_id
)
set_target_properties(pandora_stm32l475_smoke PROPERTIES SUFFIX ".elf")

add_custom_command(TARGET pandora_stm32l475_smoke POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:pandora_stm32l475_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/pandora_stm32l475_smoke.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:pandora_stm32l475_smoke>
    COMMENT "Generating Pandora STM32L475VE smoke BIN")

add_executable(pandora_stm32l475_icm20608
    sensor_icm20608_main.c
    pandora_soft_i2c.c
    ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32l4/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32l4/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/device/src/xy_device_bus_helpers.c
    ${CMAKE_SOURCE_DIR}/components/sensor/sensors/sensor_icm20608.c
    ${_cmsis}/Source/Templates/gcc/startup_stm32l475xx.s
    ${_cmsis}/Source/Templates/system_stm32l4xx.c
    ${_hal}/stm32l4xx_hal.c
    ${_hal}/stm32l4xx_hal_cortex.c
    ${_hal}/stm32l4xx_hal_gpio.c
    ${_hal}/stm32l4xx_hal_pwr.c
    ${_hal}/stm32l4xx_hal_pwr_ex.c
    ${_hal}/stm32l4xx_hal_rcc.c
    ${_hal}/stm32l4xx_hal_rcc_ex.c
    ${_hal}/stm32l4xx_hal_uart.c)
target_include_directories(pandora_stm32l475_icm20608 BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/components/device/inc
    ${CMAKE_SOURCE_DIR}/components/device
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${CMAKE_SOURCE_DIR}/components/sensor
    ${CMAKE_SOURCE_DIR}/components/sensor/sensors)
target_compile_definitions(pandora_stm32l475_icm20608 PRIVATE
    STM32L475xx USE_HAL_DRIVER SENSOR_USE_FLOAT=0
    XINYI_FIRMWARE_COMMIT="${_firmware_commit}")
target_link_options(pandora_stm32l475_icm20608 PRIVATE
    -T${CMAKE_CURRENT_SOURCE_DIR}/STM32L475VETX_FLASH.ld
    --specs=nano.specs --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/pandora_stm32l475_icm20608.map)
set_target_properties(pandora_stm32l475_icm20608 PROPERTIES SUFFIX ".elf")
add_custom_command(TARGET pandora_stm32l475_icm20608 POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:pandora_stm32l475_icm20608>
            ${CMAKE_CURRENT_BINARY_DIR}/pandora_stm32l475_icm20608.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:pandora_stm32l475_icm20608>
    COMMENT "Generating Pandora ICM20608 validation BIN")

add_executable(pandora_stm32l475_ap3216c
    sensor_ap3216c_main.c
    pandora_hw_i2c.c
    ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32l4/xy_hal_i2c.c
    ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32l4/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32l4/xy_hal_uart.c
    ${CMAKE_SOURCE_DIR}/components/device/src/xy_device_bus_helpers.c
    ${CMAKE_SOURCE_DIR}/components/sensor/sensors/sensor_ap3216c.c
    ${_cmsis}/Source/Templates/gcc/startup_stm32l475xx.s
    ${_cmsis}/Source/Templates/system_stm32l4xx.c
    ${_hal}/stm32l4xx_hal.c
    ${_hal}/stm32l4xx_hal_cortex.c
    ${_hal}/stm32l4xx_hal_gpio.c
    ${_hal}/stm32l4xx_hal_i2c.c
    ${_hal}/stm32l4xx_hal_i2c_ex.c
    ${_hal}/stm32l4xx_hal_pwr.c
    ${_hal}/stm32l4xx_hal_pwr_ex.c
    ${_hal}/stm32l4xx_hal_rcc.c
    ${_hal}/stm32l4xx_hal_rcc_ex.c
    ${_hal}/stm32l4xx_hal_uart.c)
target_include_directories(pandora_stm32l475_ap3216c BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/components/device/inc
    ${CMAKE_SOURCE_DIR}/components/device
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${CMAKE_SOURCE_DIR}/components/sensor
    ${CMAKE_SOURCE_DIR}/components/sensor/sensors)
target_compile_definitions(pandora_stm32l475_ap3216c PRIVATE
    STM32L475xx USE_HAL_DRIVER SENSOR_USE_FLOAT=0
    XINYI_FIRMWARE_COMMIT="${_firmware_commit}")
target_link_options(pandora_stm32l475_ap3216c PRIVATE
    -T${CMAKE_CURRENT_SOURCE_DIR}/STM32L475VETX_FLASH.ld
    --specs=nano.specs --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/pandora_stm32l475_ap3216c.map)
set_target_properties(pandora_stm32l475_ap3216c PROPERTIES SUFFIX ".elf")
add_custom_command(TARGET pandora_stm32l475_ap3216c POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:pandora_stm32l475_ap3216c>
            ${CMAKE_CURRENT_BINARY_DIR}/pandora_stm32l475_ap3216c.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:pandora_stm32l475_ap3216c>
    COMMENT "Generating Pandora AP3216C validation BIN")

add_executable(pandora_stm32l475_buzzer
    buzzer_main.c
    ${CMAKE_SOURCE_DIR}/components/actuator/xy_actuator_buzzer.c
    ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32l4/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32l4/xy_hal_uart.c
    ${_cmsis}/Source/Templates/gcc/startup_stm32l475xx.s
    ${_cmsis}/Source/Templates/system_stm32l4xx.c
    ${_hal}/stm32l4xx_hal.c
    ${_hal}/stm32l4xx_hal_cortex.c
    ${_hal}/stm32l4xx_hal_gpio.c
    ${_hal}/stm32l4xx_hal_pwr.c
    ${_hal}/stm32l4xx_hal_pwr_ex.c
    ${_hal}/stm32l4xx_hal_rcc.c
    ${_hal}/stm32l4xx_hal_rcc_ex.c
    ${_hal}/stm32l4xx_hal_uart.c)
target_include_directories(pandora_stm32l475_buzzer BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/components/actuator
    ${CMAKE_SOURCE_DIR}/components/hal/inc)
target_compile_definitions(pandora_stm32l475_buzzer PRIVATE
    STM32L4 STM32L475xx USE_HAL_DRIVER
    XINYI_FIRMWARE_COMMIT="${_firmware_commit}")
target_link_options(pandora_stm32l475_buzzer PRIVATE
    -T${CMAKE_CURRENT_SOURCE_DIR}/STM32L475VETX_FLASH.ld
    --specs=nano.specs --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/pandora_stm32l475_buzzer.map)
set_target_properties(pandora_stm32l475_buzzer PROPERTIES SUFFIX ".elf")
add_custom_command(TARGET pandora_stm32l475_buzzer POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:pandora_stm32l475_buzzer>
            ${CMAKE_CURRENT_BINARY_DIR}/pandora_stm32l475_buzzer.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:pandora_stm32l475_buzzer>
    COMMENT "Generating Pandora PB2 buzzer validation BIN")

add_executable(pandora_stm32l475_motor
    motor_main.c
    ${CMAKE_SOURCE_DIR}/components/actuator/xy_actuator_motor.c
    ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32l4/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32l4/xy_hal_uart.c
    ${_cmsis}/Source/Templates/gcc/startup_stm32l475xx.s
    ${_cmsis}/Source/Templates/system_stm32l4xx.c
    ${_hal}/stm32l4xx_hal.c
    ${_hal}/stm32l4xx_hal_cortex.c
    ${_hal}/stm32l4xx_hal_gpio.c
    ${_hal}/stm32l4xx_hal_pwr.c
    ${_hal}/stm32l4xx_hal_pwr_ex.c
    ${_hal}/stm32l4xx_hal_rcc.c
    ${_hal}/stm32l4xx_hal_rcc_ex.c
    ${_hal}/stm32l4xx_hal_uart.c)
target_include_directories(pandora_stm32l475_motor BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/components/actuator
    ${CMAKE_SOURCE_DIR}/components/hal/inc)
target_compile_definitions(pandora_stm32l475_motor PRIVATE
    STM32L4 STM32L475xx USE_HAL_DRIVER
    XINYI_FIRMWARE_COMMIT="${_firmware_commit}")
target_link_options(pandora_stm32l475_motor PRIVATE
    -T${CMAKE_CURRENT_SOURCE_DIR}/STM32L475VETX_FLASH.ld
    --specs=nano.specs --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/pandora_stm32l475_motor.map)
set_target_properties(pandora_stm32l475_motor PROPERTIES SUFFIX ".elf")
add_custom_command(TARGET pandora_stm32l475_motor POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:pandora_stm32l475_motor>
            ${CMAKE_CURRENT_BINARY_DIR}/pandora_stm32l475_motor.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:pandora_stm32l475_motor>
    COMMENT "Generating Pandora PA1/PA0 motor validation BIN")

add_executable(pandora_stm32l475_rgb_led
    rgb_led_main.c
    ${CMAKE_SOURCE_DIR}/components/actuator/xy_actuator_rgb_led.c
    ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32l4/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32l4/xy_hal_uart.c
    ${_cmsis}/Source/Templates/gcc/startup_stm32l475xx.s
    ${_cmsis}/Source/Templates/system_stm32l4xx.c
    ${_hal}/stm32l4xx_hal.c
    ${_hal}/stm32l4xx_hal_cortex.c
    ${_hal}/stm32l4xx_hal_gpio.c
    ${_hal}/stm32l4xx_hal_pwr.c
    ${_hal}/stm32l4xx_hal_pwr_ex.c
    ${_hal}/stm32l4xx_hal_rcc.c
    ${_hal}/stm32l4xx_hal_rcc_ex.c
    ${_hal}/stm32l4xx_hal_uart.c)
target_include_directories(pandora_stm32l475_rgb_led BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/components/actuator
    ${CMAKE_SOURCE_DIR}/components/hal/inc)
target_compile_definitions(pandora_stm32l475_rgb_led PRIVATE
    STM32L4 STM32L475xx USE_HAL_DRIVER
    XINYI_FIRMWARE_COMMIT="${_firmware_commit}")
target_link_options(pandora_stm32l475_rgb_led PRIVATE
    -T${CMAKE_CURRENT_SOURCE_DIR}/STM32L475VETX_FLASH.ld
    --specs=nano.specs --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/pandora_stm32l475_rgb_led.map)
set_target_properties(pandora_stm32l475_rgb_led PROPERTIES SUFFIX ".elf")
add_custom_command(TARGET pandora_stm32l475_rgb_led POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:pandora_stm32l475_rgb_led>
            ${CMAKE_CURRENT_BINARY_DIR}/pandora_stm32l475_rgb_led.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:pandora_stm32l475_rgb_led>
    COMMENT "Generating Pandora PE7/PE8/PE9 RGB validation BIN")

add_executable(pandora_stm32l475_st7789
    st7789_main.c
    ${CMAKE_SOURCE_DIR}/components/drivers/display/lcd/xy_lcd.c
    ${CMAKE_SOURCE_DIR}/components/drivers/display/lcd/xy_lcd_spi.c
    ${CMAKE_SOURCE_DIR}/components/drivers/display/lcd/xy_lcd_st7789.c
    ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32l4/xy_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32l4/xy_hal_spi.c
    ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32l4/xy_hal_uart.c
    ${_cmsis}/Source/Templates/gcc/startup_stm32l475xx.s
    ${_cmsis}/Source/Templates/system_stm32l4xx.c
    ${_hal}/stm32l4xx_hal.c ${_hal}/stm32l4xx_hal_cortex.c ${_hal}/stm32l4xx_hal_dma.c
    ${_hal}/stm32l4xx_hal_gpio.c
    ${_hal}/stm32l4xx_hal_pwr.c ${_hal}/stm32l4xx_hal_pwr_ex.c
    ${_hal}/stm32l4xx_hal_rcc.c ${_hal}/stm32l4xx_hal_rcc_ex.c
    ${_hal}/stm32l4xx_hal_spi.c ${_hal}/stm32l4xx_hal_uart.c)
target_include_directories(pandora_stm32l475_st7789 BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/components/device/inc
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${CMAKE_SOURCE_DIR}/components/drivers/display/lcd)
target_compile_definitions(pandora_stm32l475_st7789 PRIVATE
    STM32L4 STM32L475xx USE_HAL_DRIVER XINYI_FIRMWARE_COMMIT="${_firmware_commit}")
target_link_options(pandora_stm32l475_st7789 PRIVATE
    -T${CMAKE_CURRENT_SOURCE_DIR}/STM32L475VETX_FLASH.ld --specs=nano.specs --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/pandora_stm32l475_st7789.map)
set_target_properties(pandora_stm32l475_st7789 PROPERTIES SUFFIX ".elf")
add_custom_command(TARGET pandora_stm32l475_st7789 POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:pandora_stm32l475_st7789>
            ${CMAKE_CURRENT_BINARY_DIR}/pandora_stm32l475_st7789.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:pandora_stm32l475_st7789>
    COMMENT "Generating Pandora onboard ST7789 validation BIN")

if(OSAL_BACKEND STREQUAL "freertos")
    option(PANDORA_IPC_ISR_STRESS_ONLY
           "Keep the Pandora runtime in one boot for bounded IPC ISR stress capture" OFF)
    option(PANDORA_FOTA_AUTHORIZE_RESTAGE
           "Build a one-shot reviewed restage-authorizing bootloader" OFF)
    set(PANDORA_FOTA_RESTAGE_SOURCE_COMMIT "" CACHE STRING
        "Exact source commit approved for one-shot candidate restage")
    string(LENGTH "${PANDORA_FOTA_RESTAGE_SOURCE_COMMIT}" _restage_commit_length)
    set(PANDORA_FOTA_CANDIDATE "" CACHE FILEPATH
        "Validated candidate envelope to embed in the one-shot W25Q128 programmer")
    add_executable(pandora_stm32l475_fota_bootloader
        fota_bootloader_main.c
        pandora_fota_flash.c
        pandora_fota_install_flash.c
        ${CMAKE_SOURCE_DIR}/components/device/src/xy_device.c
        ${CMAKE_SOURCE_DIR}/components/device/xy_device_core.c
        ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32l4/xy_hal_gpio.c
        ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32l4/xy_hal_qspi.c
        ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32l4/xy_hal_uart.c
        ${CMAKE_SOURCE_DIR}/components/drivers/storage/flash/w25q128/xy_w25q128.c
        ${CMAKE_SOURCE_DIR}/components/fota/src/xy_fota_boot.c
        ${CMAKE_SOURCE_DIR}/components/fota/src/xy_fota_w25q128.c
        ${CMAKE_SOURCE_DIR}/components/fota/xy_fota.c
        ${_cmsis}/Source/Templates/gcc/startup_stm32l475xx.s
        ${_cmsis}/Source/Templates/system_stm32l4xx.c
        ${_hal}/stm32l4xx_hal.c
        ${_hal}/stm32l4xx_hal_cortex.c
        ${_hal}/stm32l4xx_hal_dma.c
        ${_hal}/stm32l4xx_hal_flash.c
        ${_hal}/stm32l4xx_hal_flash_ex.c
        ${_hal}/stm32l4xx_hal_gpio.c
        ${_hal}/stm32l4xx_hal_pwr.c
        ${_hal}/stm32l4xx_hal_pwr_ex.c
        ${_hal}/stm32l4xx_hal_qspi.c
        ${_hal}/stm32l4xx_hal_rcc.c
        ${_hal}/stm32l4xx_hal_rcc_ex.c
        ${_hal}/stm32l4xx_hal_uart.c)
    target_include_directories(pandora_stm32l475_fota_bootloader BEFORE PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/components/device/inc
        ${CMAKE_SOURCE_DIR}/components/device
        ${CMAKE_SOURCE_DIR}/components/hal/inc
        ${CMAKE_SOURCE_DIR}/components/drivers/storage/flash/w25q128
        ${CMAKE_SOURCE_DIR}/components/fota/inc
        ${CMAKE_SOURCE_DIR}/components/clib/xy_clib)
    target_compile_definitions(pandora_stm32l475_fota_bootloader PRIVATE
        STM32L475xx USE_HAL_DRIVER XINYI_FIRMWARE_COMMIT="${_firmware_commit}")
    if(PANDORA_FOTA_AUTHORIZE_RESTAGE)
        if(NOT _restage_commit_length EQUAL 40 OR
           NOT PANDORA_FOTA_RESTAGE_SOURCE_COMMIT MATCHES "^[0-9a-f]+$")
            message(FATAL_ERROR
                "PANDORA_FOTA_AUTHORIZE_RESTAGE requires exact PANDORA_FOTA_RESTAGE_SOURCE_COMMIT")
        endif()
        target_compile_definitions(pandora_stm32l475_fota_bootloader PRIVATE
            PANDORA_FOTA_AUTHORIZE_RESTAGE=1
            PANDORA_FOTA_RESTAGE_SOURCE_COMMIT="${PANDORA_FOTA_RESTAGE_SOURCE_COMMIT}")
    endif()
    target_link_libraries(pandora_stm32l475_fota_bootloader PRIVATE xy_xy_clib)
    target_link_options(pandora_stm32l475_fota_bootloader PRIVATE
        -T${CMAKE_CURRENT_SOURCE_DIR}/STM32L475VETX_BOOTLOADER.ld
        --specs=nano.specs --specs=nosys.specs
        -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/pandora_stm32l475_fota_bootloader.map)
    set_target_properties(pandora_stm32l475_fota_bootloader PROPERTIES SUFFIX ".elf")
    add_custom_command(TARGET pandora_stm32l475_fota_bootloader POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:pandora_stm32l475_fota_bootloader>
                ${CMAKE_CURRENT_BINARY_DIR}/pandora_stm32l475_fota_bootloader.bin
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:pandora_stm32l475_fota_bootloader>
        COMMENT "Generating resident Pandora FOTA bootloader BIN")

    if(PANDORA_FOTA_CANDIDATE)
        if(NOT EXISTS "${PANDORA_FOTA_CANDIDATE}")
            message(FATAL_ERROR "PANDORA_FOTA_CANDIDATE does not exist")
        endif()
        if(NOT _restage_commit_length EQUAL 40 OR
           NOT PANDORA_FOTA_RESTAGE_SOURCE_COMMIT MATCHES "^[0-9a-f]+$")
            message(FATAL_ERROR
                "PANDORA_FOTA_CANDIDATE requires exact PANDORA_FOTA_RESTAGE_SOURCE_COMMIT")
        endif()
        set(_candidate_header ${CMAKE_CURRENT_BINARY_DIR}/pandora_fota_candidate_blob.h)
        add_custom_command(
            OUTPUT ${_candidate_header}
            COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/make_fota_candidate.py
                    --candidate ${PANDORA_FOTA_CANDIDATE}
                    --source-commit ${PANDORA_FOTA_RESTAGE_SOURCE_COMMIT}
                    --c-header ${_candidate_header}
            DEPENDS ${PANDORA_FOTA_CANDIDATE} ${CMAKE_CURRENT_SOURCE_DIR}/make_fota_candidate.py
            VERBATIM)
        add_executable(pandora_stm32l475_fota_candidate_programmer
            fota_candidate_programmer_main.c
            pandora_fota_flash.c
            ${_candidate_header}
            ${CMAKE_SOURCE_DIR}/components/device/src/xy_device.c
            ${CMAKE_SOURCE_DIR}/components/device/xy_device_core.c
            ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32l4/xy_hal_gpio.c
            ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32l4/xy_hal_qspi.c
            ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32l4/xy_hal_uart.c
            ${CMAKE_SOURCE_DIR}/components/drivers/storage/flash/w25q128/xy_w25q128.c
            ${CMAKE_SOURCE_DIR}/components/fota/src/xy_fota_w25q128.c
            ${_cmsis}/Source/Templates/gcc/startup_stm32l475xx.s
            ${_cmsis}/Source/Templates/system_stm32l4xx.c
            ${_hal}/stm32l4xx_hal.c ${_hal}/stm32l4xx_hal_cortex.c ${_hal}/stm32l4xx_hal_dma.c
            ${_hal}/stm32l4xx_hal_gpio.c
            ${_hal}/stm32l4xx_hal_pwr.c ${_hal}/stm32l4xx_hal_pwr_ex.c ${_hal}/stm32l4xx_hal_qspi.c
            ${_hal}/stm32l4xx_hal_rcc.c ${_hal}/stm32l4xx_hal_rcc_ex.c ${_hal}/stm32l4xx_hal_uart.c)
        target_include_directories(pandora_stm32l475_fota_candidate_programmer BEFORE PRIVATE
            ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_SOURCE_DIR}/components/device/inc ${CMAKE_SOURCE_DIR}/components/device
            ${CMAKE_SOURCE_DIR}/components/hal/inc
            ${CMAKE_SOURCE_DIR}/components/drivers/storage/flash/w25q128
            ${CMAKE_SOURCE_DIR}/components/fota/inc ${CMAKE_SOURCE_DIR}/components/clib/xy_clib)
        target_compile_definitions(pandora_stm32l475_fota_candidate_programmer PRIVATE
            STM32L475xx USE_HAL_DRIVER XINYI_FIRMWARE_COMMIT="${_firmware_commit}")
        target_link_libraries(pandora_stm32l475_fota_candidate_programmer PRIVATE xy_xy_clib)
        target_link_options(pandora_stm32l475_fota_candidate_programmer PRIVATE
            -T${CMAKE_CURRENT_SOURCE_DIR}/STM32L475VETX_FLASH.ld --specs=nano.specs --specs=nosys.specs)
        set_target_properties(pandora_stm32l475_fota_candidate_programmer PROPERTIES SUFFIX ".elf")
        add_custom_command(TARGET pandora_stm32l475_fota_candidate_programmer POST_BUILD
            COMMAND ${CMAKE_OBJCOPY} -O binary
                    $<TARGET_FILE:pandora_stm32l475_fota_candidate_programmer>
                    ${CMAKE_CURRENT_BINARY_DIR}/pandora_stm32l475_fota_candidate_programmer.bin
            COMMAND ${CMAKE_SIZE} $<TARGET_FILE:pandora_stm32l475_fota_candidate_programmer>)
    endif()

    add_executable(pandora_stm32l475_rtos
        rtos_main.c
        rtos_handlers.c
        ${CMAKE_SOURCE_DIR}/components/device/src/xy_device.c
        ${CMAKE_SOURCE_DIR}/components/device/xy_device_core.c
        ${CMAKE_SOURCE_DIR}/components/ipc/xy_broker/xy_broker.c
        ${CMAKE_SOURCE_DIR}/components/ipc/xy_broker/xy_broker_isr_ingress.c
        ${CMAKE_SOURCE_DIR}/components/pm/src/xy_pm_platform.c
        ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32l4/xy_hal_dma.c
        ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32l4/xy_hal_spi.c
        ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32l4/xy_hal_qspi.c
        ${CMAKE_SOURCE_DIR}/components/drivers/storage/flash/w25q128/xy_w25q128.c
        ${CMAKE_SOURCE_DIR}/components/fota/xy_fota.c
        ${CMAKE_SOURCE_DIR}/components/fota/src/xy_fota_boot.c
        ${CMAKE_SOURCE_DIR}/components/fota/src/xy_fota_w25q128.c
        ${_cmsis}/Source/Templates/gcc/startup_stm32l475xx.s
        ${_cmsis}/Source/Templates/system_stm32l4xx.c
        ${_hal}/stm32l4xx_hal.c
        ${_hal}/stm32l4xx_hal_cortex.c
        ${_hal}/stm32l4xx_hal_dma.c
        ${_hal}/stm32l4xx_hal_flash.c
        ${_hal}/stm32l4xx_hal_flash_ex.c
        ${_hal}/stm32l4xx_hal_gpio.c
        ${_hal}/stm32l4xx_hal_pwr.c
        ${_hal}/stm32l4xx_hal_pwr_ex.c
        ${_hal}/stm32l4xx_hal_qspi.c
        ${_hal}/stm32l4xx_hal_rcc.c
        ${_hal}/stm32l4xx_hal_rcc_ex.c
        ${_hal}/stm32l4xx_hal_spi.c
        ${_hal}/stm32l4xx_hal_tim.c
        ${_hal}/stm32l4xx_hal_tim_ex.c
        ${_hal}/stm32l4xx_hal_uart.c
    )
    target_include_directories(pandora_stm32l475_rtos BEFORE PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/components/sys/xy_sys
        ${CMAKE_SOURCE_DIR}/components/device/inc
        ${CMAKE_SOURCE_DIR}/components/device
        ${CMAKE_SOURCE_DIR}/components/hal/inc
        ${CMAKE_SOURCE_DIR}/components/drivers/storage/flash/w25q128
        ${CMAKE_SOURCE_DIR}/components/fota/inc
        ${CMAKE_SOURCE_DIR}/components/ipc/xy_broker
        ${CMAKE_SOURCE_DIR}/components/pm/inc
        ${CMAKE_SOURCE_DIR}/components/pm/charger
        ${CMAKE_SOURCE_DIR}/components/pm/fuel-gauge
        ${CMAKE_SOURCE_DIR}/components/clib/xy_clib)
    target_compile_definitions(pandora_stm32l475_rtos PRIVATE
        STM32L475xx
        USE_HAL_DRIVER
        XY_BROKER_MAX_SERVERS=2
        XY_BROKER_MAX_SUBSCRIBERS=2
        XY_BROKER_MAX_TOPICS=2
        XY_BROKER_MSG_QUEUE_SIZE=2
        XY_OSAL_AVAILABLE
        XINYI_FIRMWARE_COMMIT="${_firmware_commit}")
    if(PANDORA_IPC_ISR_STRESS_ONLY)
        target_compile_definitions(pandora_stm32l475_rtos PRIVATE PANDORA_IPC_ISR_STRESS_ONLY=1)
    endif()
    target_link_libraries(pandora_stm32l475_rtos PRIVATE
        xy_osal
        freertos_kernel
        xy_xy_clib)
    target_link_options(pandora_stm32l475_rtos PRIVATE
        -T${CMAKE_CURRENT_SOURCE_DIR}/STM32L475VETX_FLASH.ld
        --specs=nano.specs
        --specs=nosys.specs
        -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/pandora_stm32l475_rtos.map)
    set_target_properties(pandora_stm32l475_rtos PROPERTIES SUFFIX ".elf")
    add_custom_command(TARGET pandora_stm32l475_rtos POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary
                $<TARGET_FILE:pandora_stm32l475_rtos>
                ${CMAKE_CURRENT_BINARY_DIR}/pandora_stm32l475_rtos.bin
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:pandora_stm32l475_rtos>
        COMMENT "Generating Pandora STM32L475VE OSAL FreeRTOS BIN")

    add_executable(pandora_stm32l475_fota_app
        rtos_main.c
        rtos_handlers.c
        ${CMAKE_SOURCE_DIR}/components/device/src/xy_device.c
        ${CMAKE_SOURCE_DIR}/components/device/xy_device_core.c
        ${CMAKE_SOURCE_DIR}/components/ipc/xy_broker/xy_broker.c
        ${CMAKE_SOURCE_DIR}/components/ipc/xy_broker/xy_broker_isr_ingress.c
        ${CMAKE_SOURCE_DIR}/components/pm/src/xy_pm_platform.c
        ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32l4/xy_hal_dma.c
        ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32l4/xy_hal_spi.c
        ${CMAKE_SOURCE_DIR}/components/hal/stm32/stm32l4/xy_hal_qspi.c
        ${CMAKE_SOURCE_DIR}/components/drivers/storage/flash/w25q128/xy_w25q128.c
        ${CMAKE_SOURCE_DIR}/components/fota/xy_fota.c
        ${CMAKE_SOURCE_DIR}/components/fota/src/xy_fota_boot.c
        ${CMAKE_SOURCE_DIR}/components/fota/src/xy_fota_w25q128.c
        ${_cmsis}/Source/Templates/gcc/startup_stm32l475xx.s
        ${_cmsis}/Source/Templates/system_stm32l4xx.c
        ${_hal}/stm32l4xx_hal.c
        ${_hal}/stm32l4xx_hal_cortex.c
        ${_hal}/stm32l4xx_hal_dma.c
        ${_hal}/stm32l4xx_hal_flash.c
        ${_hal}/stm32l4xx_hal_flash_ex.c
        ${_hal}/stm32l4xx_hal_gpio.c
        ${_hal}/stm32l4xx_hal_pwr.c
        ${_hal}/stm32l4xx_hal_pwr_ex.c
        ${_hal}/stm32l4xx_hal_qspi.c
        ${_hal}/stm32l4xx_hal_rcc.c
        ${_hal}/stm32l4xx_hal_rcc_ex.c
        ${_hal}/stm32l4xx_hal_spi.c
        ${_hal}/stm32l4xx_hal_tim.c
        ${_hal}/stm32l4xx_hal_tim_ex.c
        ${_hal}/stm32l4xx_hal_uart.c)
    target_include_directories(pandora_stm32l475_fota_app BEFORE PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/components/sys/xy_sys
        ${CMAKE_SOURCE_DIR}/components/device/inc
        ${CMAKE_SOURCE_DIR}/components/device
        ${CMAKE_SOURCE_DIR}/components/hal/inc
        ${CMAKE_SOURCE_DIR}/components/drivers/storage/flash/w25q128
        ${CMAKE_SOURCE_DIR}/components/fota/inc
        ${CMAKE_SOURCE_DIR}/components/ipc/xy_broker
        ${CMAKE_SOURCE_DIR}/components/pm/inc
        ${CMAKE_SOURCE_DIR}/components/pm/charger
        ${CMAKE_SOURCE_DIR}/components/pm/fuel-gauge
        ${CMAKE_SOURCE_DIR}/components/clib/xy_clib)
    target_compile_definitions(pandora_stm32l475_fota_app PRIVATE
        STM32L475xx USE_HAL_DRIVER XY_BROKER_MAX_SERVERS=2 XY_BROKER_MAX_SUBSCRIBERS=2
        XY_BROKER_MAX_TOPICS=2 XY_BROKER_MSG_QUEUE_SIZE=2 XY_OSAL_AVAILABLE
        PANDORA_FOTA_APPLICATION
        XINYI_FIRMWARE_COMMIT="${_firmware_commit}")
    target_link_libraries(pandora_stm32l475_fota_app PRIVATE xy_osal freertos_kernel xy_xy_clib)
    target_link_options(pandora_stm32l475_fota_app PRIVATE
        -T${CMAKE_CURRENT_SOURCE_DIR}/STM32L475VETX_FOTA_APP.ld
        --specs=nano.specs --specs=nosys.specs
        -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/pandora_stm32l475_fota_app.map)
    set_target_properties(pandora_stm32l475_fota_app PROPERTIES SUFFIX ".elf")
    add_custom_command(TARGET pandora_stm32l475_fota_app POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:pandora_stm32l475_fota_app>
                ${CMAKE_CURRENT_BINARY_DIR}/pandora_stm32l475_fota_app.bin
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:pandora_stm32l475_fota_app>
        COMMENT "Generating Pandora relocatable FOTA application BIN")
endif()
