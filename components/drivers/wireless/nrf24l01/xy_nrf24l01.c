#include "xy_nrf24l01.h"

#include <string.h>

#define NRF24_CMD_R_REGISTER 0x00U
#define NRF24_CMD_W_REGISTER 0x20U
#define NRF24_CMD_REGISTER_MASK 0x1FU
#define NRF24_DUMMY 0xFFU
#define NRF24_CMD_FLUSH_TX 0xE1U
#define NRF24_CMD_W_TX_PAYLOAD 0xA0U
#define NRF24_CMD_R_RX_PAYLOAD 0x61U
#define NRF24_CMD_FLUSH_RX 0xE2U
#define NRF24_CMD_NOP 0xFFU
#define NRF24_REG_SETUP_RETR 0x04U
#define NRF24_REG_OBSERVE_TX 0x08U
#define NRF24_REG_RX_ADDR_P0 0x0AU
#define NRF24_REG_RX_PW_P0 0x11U
#define NRF24_REG_TX_ADDR 0x10U
#define NRF24_STATUS_RX_DR 0x40U
#define NRF24_STATUS_TX_DS 0x20U
#define NRF24_STATUS_MAX_RT 0x10U
#define NRF24_FIFO_RX_EMPTY 0x01U

static xy_hal_error_t nrf24_frame(xy_nrf24l01_t *radio, const uint8_t *tx, uint8_t *rx,
                                  size_t length)
{
    xy_hal_error_t result;
    xy_hal_error_t release_result;

    result = radio->config.set_csn(radio->config.csn_arg, 0U);
    if (result != XY_HAL_OK) return result;
    result = radio->config.transfer(radio->config.spi, tx, rx, length,
                                    radio->config.timeout_ms);
    release_result = radio->config.set_csn(radio->config.csn_arg, 1U);
    if (result != XY_HAL_OK) return result;
    return release_result;
}

static xy_hal_error_t nrf24_command(xy_nrf24l01_t *radio, uint8_t command,
                                    uint8_t tx_value, uint8_t *status,
                                    uint8_t *rx_value)
{
    uint8_t tx[2] = {command, tx_value};
    uint8_t rx[2] = {0U, 0U};
    xy_hal_error_t result;

    result = nrf24_frame(radio, tx, rx, sizeof(tx));
    if (result != XY_HAL_OK) return result;
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

static xy_hal_error_t nrf24_write_buffer(xy_nrf24l01_t *radio, uint8_t command,
                                          const uint8_t *data, size_t length)
{
    uint8_t tx[33];
    uint8_t rx[33];

    if (data == NULL || length == 0U || length > 32U) return XY_HAL_ERROR_INVALID_PARAM;
    tx[0] = command;
    memcpy(&tx[1], data, length);
    return nrf24_frame(radio, tx, rx, length + 1U);
}

static xy_hal_error_t nrf24_read_buffer(xy_nrf24l01_t *radio, uint8_t command,
                                         uint8_t *data, size_t length)
{
    uint8_t tx[33];
    uint8_t rx[33];
    xy_hal_error_t result;

    if (data == NULL || length == 0U || length > 32U) return XY_HAL_ERROR_INVALID_PARAM;
    memset(tx, NRF24_DUMMY, length + 1U);
    tx[0] = command;
    result = nrf24_frame(radio, tx, rx, length + 1U);
    if (result != XY_HAL_OK) return result;
    memcpy(data, &rx[1], length);
    return XY_HAL_OK;
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

xy_hal_error_t xy_nrf24l01_configure_ptx(xy_nrf24l01_t *radio, uint8_t channel,
                                          const uint8_t address[5], uint8_t data_rate_2mbps,
                                          uint8_t crc16)
{
    xy_hal_error_t result;
    uint8_t value;

    if (radio == NULL || radio->initialized == 0U || address == NULL || channel > 125U ||
        data_rate_2mbps > 1U || crc16 > 1U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
#define WRITE_OR_RETURN(reg, data)                                                      \
    do {                                                                                \
        result = nrf24_write_register(radio, (reg), (data), NULL);                     \
        if (result != XY_HAL_OK) return result;                                         \
    } while (0)
    result = radio->config.set_ce(radio->config.ce_arg, 0U);
    if (result != XY_HAL_OK) return result;
    value = (uint8_t)(0x0AU | (crc16 != 0U ? 0x04U : 0U));
    WRITE_OR_RETURN(XY_NRF24L01_REG_CONFIG, value);
    WRITE_OR_RETURN(XY_NRF24L01_REG_EN_AA, 0x01U);
    WRITE_OR_RETURN(XY_NRF24L01_REG_SETUP_AW, 0x03U);
    WRITE_OR_RETURN(NRF24_REG_SETUP_RETR, 0x5FU);
    WRITE_OR_RETURN(XY_NRF24L01_REG_RF_CH, channel);
    WRITE_OR_RETURN(XY_NRF24L01_REG_RF_SETUP, data_rate_2mbps != 0U ? 0x0FU : 0x07U);
    WRITE_OR_RETURN(XY_NRF24L01_REG_STATUS,
                    NRF24_STATUS_RX_DR | NRF24_STATUS_TX_DS | NRF24_STATUS_MAX_RT);
#undef WRITE_OR_RETURN
    result = nrf24_write_buffer(radio, NRF24_CMD_W_REGISTER | NRF24_REG_RX_ADDR_P0,
                                address, 5U);
    if (result != XY_HAL_OK) return result;
    return nrf24_write_buffer(radio, NRF24_CMD_W_REGISTER | NRF24_REG_TX_ADDR,
                              address, 5U);
}

xy_hal_error_t xy_nrf24l01_send(xy_nrf24l01_t *radio, const uint8_t *payload,
                                size_t length, uint8_t *retransmit_count)
{
    uint8_t status = 0U;
    uint8_t observe = 0U;
    uint32_t poll;
    xy_hal_error_t result;

    if (radio == NULL || radio->initialized == 0U || payload == NULL || length == 0U ||
        length > 32U || radio->config.delay_us == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    result = nrf24_command(radio, NRF24_CMD_FLUSH_TX, NRF24_DUMMY, NULL, NULL);
    if (result != XY_HAL_OK) return result;
    result = nrf24_write_buffer(radio, NRF24_CMD_W_TX_PAYLOAD, payload, length);
    if (result != XY_HAL_OK) return result;
    result = radio->config.set_ce(radio->config.ce_arg, 1U);
    if (result != XY_HAL_OK) return result;
    radio->config.delay_us(20U);
    result = radio->config.set_ce(radio->config.ce_arg, 0U);
    if (result != XY_HAL_OK) return result;

    for (poll = 0U; poll < 200U; ++poll) {
        result = nrf24_command(radio, NRF24_CMD_NOP, NRF24_DUMMY, &status, NULL);
        if (result != XY_HAL_OK) return result;
        if ((status & (NRF24_STATUS_TX_DS | NRF24_STATUS_MAX_RT)) != 0U) break;
        radio->config.delay_us(100U);
    }
    if (poll == 200U) return XY_HAL_ERROR_TIMEOUT;
    result = nrf24_read_register(radio, NRF24_REG_OBSERVE_TX, NULL, &observe);
    if (result != XY_HAL_OK) return result;
    if (retransmit_count != NULL) *retransmit_count = observe & 0x0FU;
    result = nrf24_write_register(radio, XY_NRF24L01_REG_STATUS,
                                  status & (NRF24_STATUS_TX_DS | NRF24_STATUS_MAX_RT), NULL);
    if (result != XY_HAL_OK) return result;
    if ((status & NRF24_STATUS_MAX_RT) != 0U) {
        result = nrf24_command(radio, NRF24_CMD_FLUSH_TX, NRF24_DUMMY, NULL, NULL);
        if (result != XY_HAL_OK) return result;
        return XY_HAL_ERROR_NOT_FOUND;
    }
    return (status & NRF24_STATUS_TX_DS) != 0U ? XY_HAL_OK : XY_HAL_ERROR_IO;
}

xy_hal_error_t xy_nrf24l01_configure_prx(xy_nrf24l01_t *radio, uint8_t channel,
                                          const uint8_t address[5], uint8_t payload_width,
                                          uint8_t data_rate_2mbps, uint8_t crc16)
{
    xy_hal_error_t result;
    uint8_t config_value;

    if (radio == NULL || radio->initialized == 0U || address == NULL || channel > 125U ||
        payload_width == 0U || payload_width > 32U || data_rate_2mbps > 1U || crc16 > 1U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    result = radio->config.set_ce(radio->config.ce_arg, 0U);
    if (result != XY_HAL_OK) return result;
#define WRITE_RX_OR_RETURN(reg, data)                                                   \
    do {                                                                                \
        result = nrf24_write_register(radio, (reg), (data), NULL);                     \
        if (result != XY_HAL_OK) return result;                                         \
    } while (0)
    config_value = (uint8_t)(0x0BU | (crc16 != 0U ? 0x04U : 0U));
    WRITE_RX_OR_RETURN(XY_NRF24L01_REG_CONFIG, config_value);
    WRITE_RX_OR_RETURN(XY_NRF24L01_REG_EN_AA, 0x01U);
    WRITE_RX_OR_RETURN(XY_NRF24L01_REG_SETUP_AW, 0x03U);
    WRITE_RX_OR_RETURN(XY_NRF24L01_REG_RF_CH, channel);
    WRITE_RX_OR_RETURN(XY_NRF24L01_REG_RF_SETUP, data_rate_2mbps != 0U ? 0x0FU : 0x07U);
    WRITE_RX_OR_RETURN(NRF24_REG_RX_PW_P0, payload_width);
    WRITE_RX_OR_RETURN(XY_NRF24L01_REG_STATUS,
                       NRF24_STATUS_RX_DR | NRF24_STATUS_TX_DS | NRF24_STATUS_MAX_RT);
#undef WRITE_RX_OR_RETURN
    result = nrf24_write_buffer(radio, NRF24_CMD_W_REGISTER | NRF24_REG_RX_ADDR_P0,
                                address, 5U);
    if (result != XY_HAL_OK) return result;
    result = nrf24_command(radio, NRF24_CMD_FLUSH_RX, NRF24_DUMMY, NULL, NULL);
    if (result != XY_HAL_OK) return result;
    radio->rx_payload_width = payload_width;
    result = radio->config.set_ce(radio->config.ce_arg, 1U);
    if (result != XY_HAL_OK) {
        radio->rx_payload_width = 0U;
        return result;
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_nrf24l01_receive(xy_nrf24l01_t *radio, uint8_t *payload,
                                   size_t capacity, size_t *received_length)
{
    uint8_t status;
    uint8_t fifo_status;
    xy_hal_error_t result;

    if (radio == NULL || radio->initialized == 0U || payload == NULL ||
        received_length == NULL || radio->rx_payload_width == 0U ||
        capacity < radio->rx_payload_width) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    *received_length = 0U;
    result = nrf24_read_register(radio, XY_NRF24L01_REG_FIFO_STATUS, &status, &fifo_status);
    if (result != XY_HAL_OK) return result;
    if ((fifo_status & NRF24_FIFO_RX_EMPTY) != 0U) return XY_HAL_ERROR_NOT_FOUND;
    result = nrf24_read_buffer(radio, NRF24_CMD_R_RX_PAYLOAD, payload,
                               radio->rx_payload_width);
    if (result != XY_HAL_OK) return result;
    result = nrf24_write_register(radio, XY_NRF24L01_REG_STATUS, NRF24_STATUS_RX_DR, NULL);
    if (result != XY_HAL_OK) return result;
    *received_length = radio->rx_payload_width;
    return XY_HAL_OK;
}
