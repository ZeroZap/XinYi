#ifndef XY_NRF24L01_H
#define XY_NRF24L01_H

#include "xy_hal_error.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XY_NRF24L01_REG_CONFIG      0x00U
#define XY_NRF24L01_REG_EN_AA       0x01U
#define XY_NRF24L01_REG_SETUP_AW    0x03U
#define XY_NRF24L01_REG_RF_CH       0x05U
#define XY_NRF24L01_REG_RF_SETUP    0x06U
#define XY_NRF24L01_REG_STATUS      0x07U
#define XY_NRF24L01_REG_FIFO_STATUS 0x17U

typedef xy_hal_error_t (*xy_nrf24l01_transfer_fn)(void *spi, const uint8_t *tx,
                                                  uint8_t *rx, size_t length,
                                                  uint32_t timeout_ms);
typedef xy_hal_error_t (*xy_nrf24l01_gpio_fn)(void *arg, uint8_t level);
typedef void (*xy_nrf24l01_delay_fn)(uint32_t us);

typedef struct {
    void *spi;
    void *csn_arg;
    void *ce_arg;
    xy_nrf24l01_transfer_fn transfer;
    xy_nrf24l01_gpio_fn set_csn;
    xy_nrf24l01_gpio_fn set_ce;
    xy_nrf24l01_delay_fn delay_us;
    uint32_t timeout_ms;
} xy_nrf24l01_config_t;

typedef struct {
    xy_nrf24l01_config_t config;
    uint8_t status;
    uint8_t config_reg;
    uint8_t en_aa;
    uint8_t setup_aw;
    uint8_t rf_ch;
    uint8_t rf_setup;
    uint8_t fifo_status;
    uint8_t rx_payload_width;
    uint8_t initialized;
} xy_nrf24l01_t;

xy_hal_error_t xy_nrf24l01_probe(xy_nrf24l01_t *radio,
                                  const xy_nrf24l01_config_t *config);
xy_hal_error_t xy_nrf24l01_configure_ptx(xy_nrf24l01_t *radio, uint8_t channel,
                                          const uint8_t address[5], uint8_t data_rate_2mbps,
                                          uint8_t crc16);
xy_hal_error_t xy_nrf24l01_send(xy_nrf24l01_t *radio, const uint8_t *payload,
                                size_t length, uint8_t *retransmit_count);
xy_hal_error_t xy_nrf24l01_configure_prx(xy_nrf24l01_t *radio, uint8_t channel,
                                          const uint8_t address[5], uint8_t payload_width,
                                          uint8_t data_rate_2mbps, uint8_t crc16);
xy_hal_error_t xy_nrf24l01_receive(xy_nrf24l01_t *radio, uint8_t *payload,
                                   size_t capacity, size_t *received_length);

#ifdef __cplusplus
}
#endif

#endif
