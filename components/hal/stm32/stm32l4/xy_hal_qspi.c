#include "../../inc/xy_hal_qspi.h"

#if defined(STM32L4) || defined(STM32L4xx)

#include "stm32l4xx_hal.h"

static uint32_t map_lines(xy_hal_qspi_lines_t lines, uint32_t none, uint32_t one, uint32_t two,
                          uint32_t four)
{
    switch (lines) {
    case XY_HAL_QSPI_LINES_1:
        return one;
    case XY_HAL_QSPI_LINES_2:
        return two;
    case XY_HAL_QSPI_LINES_4:
        return four;
    case XY_HAL_QSPI_LINES_NONE:
    default:
        return none;
    }
}

static xy_hal_error_t map_status(HAL_StatusTypeDef status)
{
    if (status == HAL_OK) {
        return XY_HAL_OK;
    }
    if (status == HAL_TIMEOUT) {
        return XY_HAL_ERROR_TIMEOUT;
    }
    if (status == HAL_BUSY) {
        return XY_HAL_ERROR_BUSY;
    }
    return XY_HAL_ERROR_IO;
}

xy_hal_error_t xy_hal_qspi_init(void *qspi, const xy_hal_qspi_config_t *config)
{
    QSPI_HandleTypeDef *handle = qspi;
    if (handle == NULL || config == NULL || config->flash_size_bits == 0U ||
        config->chip_select_high_cycles == 0U || config->chip_select_high_cycles > 8U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    handle->Init.ClockPrescaler = config->clock_prescaler;
    handle->Init.FifoThreshold = config->fifo_threshold;
    handle->Init.SampleShifting = config->sample_shift_half_cycle != 0U
                                      ? QSPI_SAMPLE_SHIFTING_HALFCYCLE
                                      : QSPI_SAMPLE_SHIFTING_NONE;
    handle->Init.FlashSize = config->flash_size_bits - 1U;
    handle->Init.ChipSelectHighTime = (config->chip_select_high_cycles - 1U) << QUADSPI_DCR_CSHT_Pos;
    handle->Init.ClockMode = config->clock_mode != 0U ? QSPI_CLOCK_MODE_3 : QSPI_CLOCK_MODE_0;
    return map_status(HAL_QSPI_Init(handle));
}

xy_hal_error_t xy_hal_qspi_deinit(void *qspi)
{
    return qspi == NULL ? XY_HAL_ERROR_INVALID_PARAM
                        : map_status(HAL_QSPI_DeInit((QSPI_HandleTypeDef *)qspi));
}

xy_hal_error_t xy_hal_qspi_command(void *qspi, const xy_hal_qspi_command_t *operation,
                                   uint8_t *data, uint32_t timeout)
{
    QSPI_CommandTypeDef command = {0};
    HAL_StatusTypeDef status;
    if (qspi == NULL || operation == NULL || operation->instruction_lines == XY_HAL_QSPI_LINES_NONE ||
        (operation->data_length != 0U && data == NULL) ||
        (operation->has_address != 0U && operation->address_bits != 8U &&
         operation->address_bits != 16U && operation->address_bits != 24U &&
         operation->address_bits != 32U)) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    command.Instruction = operation->instruction;
    command.InstructionMode = map_lines(operation->instruction_lines, QSPI_INSTRUCTION_NONE,
                                        QSPI_INSTRUCTION_1_LINE, QSPI_INSTRUCTION_2_LINES,
                                        QSPI_INSTRUCTION_4_LINES);
    command.AddressMode = map_lines(operation->address_lines, QSPI_ADDRESS_NONE, QSPI_ADDRESS_1_LINE,
                                    QSPI_ADDRESS_2_LINES, QSPI_ADDRESS_4_LINES);
    command.AddressSize = (operation->address_bits / 8U - 1U) << QUADSPI_CCR_ADSIZE_Pos;
    command.Address = operation->address;
    command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    command.DataMode = map_lines(operation->data_lines, QSPI_DATA_NONE, QSPI_DATA_1_LINE,
                                 QSPI_DATA_2_LINES, QSPI_DATA_4_LINES);
    command.DummyCycles = operation->dummy_cycles;
    command.NbData = operation->data_length;
    command.DdrMode = QSPI_DDR_MODE_DISABLE;
    command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
    status = HAL_QSPI_Command((QSPI_HandleTypeDef *)qspi, &command, timeout);
    if (status != HAL_OK || operation->data_length == 0U) {
        return map_status(status);
    }
    status = operation->write ? HAL_QSPI_Transmit((QSPI_HandleTypeDef *)qspi, data, timeout)
                              : HAL_QSPI_Receive((QSPI_HandleTypeDef *)qspi, data, timeout);
    return map_status(status);
}

uint32_t xy_hal_qspi_tick_ms(void)
{
    return HAL_GetTick();
}

#endif
