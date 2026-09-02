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
    ${_hal}/stm32l4xx_hal_rcc.c
    ${_hal}/stm32l4xx_hal_rcc_ex.c
    ${_hal}/stm32l4xx_hal_uart.c
)

target_include_directories(pandora_stm32l475_smoke BEFORE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(pandora_stm32l475_smoke PRIVATE
    STM32L475xx
    USE_HAL_DRIVER
    XINYI_FIRMWARE_COMMIT="${_firmware_commit}")
target_link_options(pandora_stm32l475_smoke PRIVATE
    -T${CMAKE_CURRENT_SOURCE_DIR}/STM32L475VETX_FLASH.ld
    --specs=nano.specs
    --specs=nosys.specs
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/pandora_stm32l475_smoke.map
)
set_target_properties(pandora_stm32l475_smoke PROPERTIES SUFFIX ".elf")

add_custom_command(TARGET pandora_stm32l475_smoke POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:pandora_stm32l475_smoke>
            ${CMAKE_CURRENT_BINARY_DIR}/pandora_stm32l475_smoke.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:pandora_stm32l475_smoke>
    COMMENT "Generating Pandora STM32L475VE smoke BIN")
