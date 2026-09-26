#include "xy_nrf24l01.h"

#include <string.h>

#define NRF24_CMD_R_REGISTER 0x00U
#define NRF24_CMD_W_REGISTER 0x20U
#define NRF24_CMD_REGISTER_MASK 0x1FU
#define NRF24_DUMMY 0xFFU

static xy_hal_error_t nrf24_command(xy_nrf24l01_t *radio, uint8_t command,
                                    uint8_t tx_value, uint8_t *status,
                                    uint8_t *rx_value)
{
    uint8_t tx[2] = {command, tx_value};
    uint8_t rx[2] = {0U, 0U};
    xy_hal_error_t result;
    xy_hal_error_t release_result;

    result = radio->config.set_csn(radio->config.csn_arg, 0U);
    if (result != XY_HAL_OK) return result;
    result = radio->config.transfer(radio->config.spi, tx, rx, sizeof(tx),
                                    radio->config.timeout_ms);
    release_result = radio->config.set_csn(radio->config.csn_arg, 1U);
    if (result != XY_HAL_OK) return result;
    if (release_result != XY_HAL_OK) return release_result;
    if (status != NULL) *status = rx[0];
    if (rx_value != NULL) *rx_value = rx[1];
    return XY_HAL_OK;
}

static xy_hal_error_t nrf24_read_register(xy_nrf24l01_t *radio, uint8_t reg,
                                          uint8_t *status, uint8_t *value)
{
    if (value == NULL || reg > NRF24_CMD_REGISTER_MASK) return XY_HAL_ERROR_INVALID_PARAM;
    return nrf24_command(radio, NRF24_CMD_R_REGISTER | reg, NRF24_DUMMY, status, value);
}

static xy_hal_error_t nrf24_write_register(xy_nrf24l01_t *radio, uint8_t reg,
                                           uint8_t value, uint8_t *status)
{
    if (reg > NRF24_CMD_REGISTER_MASK) return XY_HAL_ERROR_INVALID_PARAM;
    return nrf24_command(radio, NRF24_CMD_W_REGISTER | reg, value, status, NULL);
}

static xy_hal_error_t nrf24_restore_channel(xy_nrf24l01_t *radio, uint8_t original)
{
    uint8_t restored;
    xy_hal_error_t result;

    result = nrf24_write_register(radio, XY_NRF24L01_REG_RF_CH, original, NULL);
    if (result != XY_HAL_OK) return result;
    result = nrf24_read_register(radio, XY_NRF24L01_REG_RF_CH, NULL, &restored);
    if (result != XY_HAL_OK) return result;
    return restored == original ? XY_HAL_OK : XY_HAL_ERROR_IO;
}

xy_hal_error_t xy_nrf24l01_probe(xy_nrf24l01_t *radio,
                                  const xy_nrf24l01_config_t *config)
{
    xy_nrf24l01_t next;
    uint8_t trial;
    uint8_t readback;
    uint8_t original;
    uint8_t status;
    xy_hal_error_t result;
    xy_hal_error_t restore_result;

    if (radio == NULL || config == NULL || config->spi == NULL || config->transfer == NULL ||
        config->set_csn == NULL || config->set_ce == NULL || config->timeout_ms == 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    memset(&next, 0, sizeof(next));
    next.config = *config;
    result = next.config.set_ce(next.config.ce_arg, 0U);
    if (result != XY_HAL_OK) return result;
    result = next.config.set_csn(next.config.csn_arg, 1U);
    if (result != XY_HAL_OK) return result;

    result = nrf24_read_register(&next, XY_NRF24L01_REG_RF_CH, &status, &original);
    if (result != XY_HAL_OK) return result;
    if ((original & 0x80U) != 0U || status == 0xFFU) return XY_HAL_ERROR_NOT_FOUND;
    trial = (uint8_t)((original ^ 0x2AU) & 0x7FU);
    if (trial == original) trial = (uint8_t)((original + 1U) & 0x7FU);

    result = nrf24_write_register(&next, XY_NRF24L01_REG_RF_CH, trial, NULL);
    if (result != XY_HAL_OK) return result;
    result = nrf24_read_register(&next, XY_NRF24L01_REG_RF_CH, NULL, &readback);
    restore_result = nrf24_restore_channel(&next, original);
    if (restore_result != XY_HAL_OK) return restore_result;
    if (result != XY_HAL_OK) return result;
    if (readback != trial) return XY_HAL_ERROR_NOT_FOUND;

    next.status = status;
    next.rf_ch = original;
#define READ_FIELD(reg, field)                                                         \
    do {                                                                               \
        result = nrf24_read_register(&next, (reg), &next.status, &next.field);         \
        if (result != XY_HAL_OK) return result;                                         \
    } while (0)
    READ_FIELD(XY_NRF24L01_REG_CONFIG, config_reg);
    READ_FIELD(XY_NRF24L01_REG_EN_AA, en_aa);
    READ_FIELD(XY_NRF24L01_REG_SETUP_AW, setup_aw);
    READ_FIELD(XY_NRF24L01_REG_RF_SETUP, rf_setup);
    READ_FIELD(XY_NRF24L01_REG_FIFO_STATUS, fifo_status);
#undef READ_FIELD
    if (next.status == 0xFFU || (next.setup_aw & 0xFCU) != 0U ||
        (next.setup_aw & 0x03U) == 0U) {
        return XY_HAL_ERROR_NOT_FOUND;
    }
    next.initialized = 1U;
    *radio = next;
    return XY_HAL_OK;
}
