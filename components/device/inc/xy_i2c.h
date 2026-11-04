/**
 * @file xy_i2c.h
 * @brief I2C Interface Placeholder
 */

#ifndef XY_I2C_H
#define XY_I2C_H

#include <stdint.h>

typedef void xy_i2c_t;

int xy_i2c_master_transmit(xy_i2c_t *i2c, uint8_t addr, const uint8_t *tx, uint16_t tx_len,
                           const void *data, uint16_t data_len, uint32_t timeout_ms);

#endif /* XY_I2C_H */
