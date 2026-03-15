/**
 * @file xy_hal_i2c_device.c
 * @brief WCH CH32U5 I2C Device Implementation - Unified HAL API
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 实现统一的 I2C 设备 API，基于 WCH CH32U5 HAL 库
 */

#include "../inc/xy_hal_i2c_dev.h"
#include "../inc/xy_hal_i2c_types.h"
#include <string.h>

/* WCH CH32U5 HAL 头文件 */
#include "ch32u5xx.h"

/* ==================== Private Definitions ==================== */

#define CH32U5_I2C_INSTANCE_COUNT  (4)  /* I2C1-4 */

/* ==================== Private Types ==================== */

typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint32_t instance;
    xy_hal_i2c_config_t config;
    xy_hal_i2c_callback_t callback;
    void *callback_arg;
    xy_hal_i2c_stats_t stats;
    uint8_t tx_busy;
    uint8_t rx_busy;
} ch32u5_i2c_data_t;

/* ==================== Private Variables ==================== */

static ch32u5_i2c_data_t i2c_devices[CH32U5_I2C_INSTANCE_COUNT] = {0};

static const char *const i2c_names[CH32U5_I2C_INSTANCE_COUNT] = {
    "I2C1", "I2C2", "I2C3", "I2C4"
};

extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;
extern I2C_HandleTypeDef hi2c3;
extern I2C_HandleTypeDef hi2c4;

static I2C_HandleTypeDef *const i2c_handles[CH32U5_I2C_INSTANCE_COUNT] = {
    &hi2c1, &hi2c2, &hi2c3, &hi2c4
};

/* ==================== Private Functions ==================== */

static int i2c_find_index(const char *name)
{
    for (int i = 0; i < CH32U5_I2C_INSTANCE_COUNT; i++) {
        if (strcmp(name, i2c_names[i]) == 0) {
            return i;
        }
    }
    return -1;
}

/* ==================== Callback Forwarding ==================== */

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    for (int i = 0; i < CH32U5_I2C_INSTANCE_COUNT; i++) {
        if (hi2c == i2c_devices[i].hi2c && i2c_devices[i].callback) {
            i2c_devices[i].tx_busy = 0;
            i2c_devices[i].callback((xy_hal_i2c_t)&i2c_devices[i],
                                   XY_HAL_I2C_EVENT_TX_DONE,
                                   i2c_devices[i].callback_arg);
            break;
        }
    }
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    for (int i = 0; i < CH32U5_I2C_INSTANCE_COUNT; i++) {
        if (hi2c == i2c_devices[i].hi2c && i2c_devices[i].callback) {
            i2c_devices[i].rx_busy = 0;
            i2c_devices[i].callback((xy_hal_i2c_t)&i2c_devices[i],
                                   XY_HAL_I2C_EVENT_RX_DONE,
                                   i2c_devices[i].callback_arg);
            break;
        }
    }
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    for (int i = 0; i < CH32U5_I2C_INSTANCE_COUNT; i++) {
        if (hi2c == i2c_devices[i].hi2c) {
            i2c_devices[i].stats.tx_errors++;
            if (hi2c->ErrorCode & HAL_I2C_ERROR_AF) {
                i2c_devices[i].stats.nack_count++;
            }
            if (i2c_devices[i].callback) {
                i2c_devices[i].callback((xy_hal_i2c_t)&i2c_devices[i],
                                       XY_HAL_I2C_EVENT_ERROR,
                                       i2c_devices[i].callback_arg);
            }
            break;
        }
    }
}

/* ==================== Device Model API Implementation ==================== */

xy_hal_i2c_t xy_hal_i2c_bind(const char *name)
{
    int index = i2c_find_index(name);
    if (index < 0) return NULL;
    
    ch32u5_i2c_data_t *dev = &i2c_devices[index];
    dev->hi2c = (I2C_HandleTypeDef *)i2c_handles[index];
    dev->instance = (uint32_t)index;
    dev->callback = NULL;
    dev->callback_arg = NULL;
    dev->tx_busy = 0;
    dev->rx_busy = 0;
    memset(&dev->stats, 0, sizeof(dev->stats));
    
    return (xy_hal_i2c_t)dev;
}

xy_hal_error_t xy_hal_i2c_unbind(xy_hal_i2c_t i2c)
{
    if (!i2c) return XY_HAL_ERROR_INVALID_PARAM;
    
    ch32u5_i2c_data_t *dev = (ch32u5_i2c_data_t *)i2c;
    HAL_I2C_Abort(dev->hi2c);
    HAL_I2C_DeInit(dev->hi2c);
    memset(dev, 0, sizeof(*dev));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_configure(xy_hal_i2c_t i2c,
                                    const xy_hal_i2c_config_t *config)
{
    if (!i2c || !config) return XY_HAL_ERROR_INVALID_PARAM;
    
    ch32u5_i2c_data_t *dev = (ch32u5_i2c_data_t *)i2c;
    
    dev->hi2c->Init.ClockSpeed = config->clock_speed;
    dev->hi2c->Init.OwnAddress1 = config->own_address;
    dev->hi2c->Init.AddressingMode = (config->addr_mode == XY_HAL_I2C_ADDR_10BIT) ?
                                     I2C_ADDRESSINGMODE_10BIT : I2C_ADDRESSINGMODE_7BIT;
    dev->hi2c->Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    dev->hi2c->Init.GeneralCallMode = config->general_call_mode ?
                                      I2C_GENERALCALL_ENABLE : I2C_GENERALCALL_DISABLE;
    dev->hi2c->Init.NoStretchMode = config->stretch_clock ?
                                    I2C_NOSTRETCH_DISABLE : I2C_NOSTRETCH_ENABLE;
    
    if (HAL_I2C_Init(dev->hi2c) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
    
    dev->config = *config;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_get_config(xy_hal_i2c_t i2c,
                                     xy_hal_i2c_config_t *config)
{
    if (!i2c || !config) return XY_HAL_ERROR_INVALID_PARAM;
    
    ch32u5_i2c_data_t *dev = (ch32u5_i2c_data_t *)i2c;
    *config = dev->config;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_set_clock_speed(xy_hal_i2c_t i2c, uint32_t clock_speed)
{
    if (!i2c) return XY_HAL_ERROR_INVALID_PARAM;
    
    ch32u5_i2c_data_t *dev = (ch32u5_i2c_data_t *)i2c;
    dev->config.clock_speed = clock_speed;
    dev->hi2c->Init.ClockSpeed = clock_speed;
    
    if (HAL_I2C_Init(dev->hi2c) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
    return XY_HAL_OK;
}

int32_t xy_hal_i2c_master_transmit(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                   const uint8_t *data, size_t length,
                                   uint32_t timeout)
{
    if (!i2c || !data || length == 0) return XY_HAL_ERROR_INVALID_PARAM;
    
    ch32u5_i2c_data_t *dev = (ch32u5_i2c_data_t *)i2c;
    
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(dev->hi2c, dev_addr << 1,
                                                        (uint8_t *)data, length, timeout);
    
    if (status == HAL_OK) {
        dev->stats.tx_bytes += length;
        return (int32_t)length;
    } else if (status == HAL_TIMEOUT) {
        return XY_HAL_ERROR_TIMEOUT;
    } else {
        dev->stats.tx_errors++;
        if (dev->hi2c->ErrorCode & HAL_I2C_ERROR_AF) {
            dev->stats.nack_count++;
        }
        return XY_HAL_ERROR_FAIL;
    }
}

int32_t xy_hal_i2c_master_receive(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                  uint8_t *data, size_t length,
                                  uint32_t timeout)
{
    if (!i2c || !data || length == 0) return XY_HAL_ERROR_INVALID_PARAM;
    
    ch32u5_i2c_data_t *dev = (ch32u5_i2c_data_t *)i2c;
    
    HAL_StatusTypeDef status = HAL_I2C_Master_Receive(dev->hi2c, dev_addr << 1,
                                                       data, length, timeout);
    
    if (status == HAL_OK) {
        dev->stats.rx_bytes += length;
        return (int32_t)length;
    } else if (status == HAL_TIMEOUT) {
        return XY_HAL_ERROR_TIMEOUT;
    } else {
        dev->stats.rx_errors++;
        return XY_HAL_ERROR_FAIL;
    }
}

xy_hal_error_t xy_hal_i2c_reg_write(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                    const uint8_t *reg_addr, size_t reg_size,
                                    const uint8_t *data, size_t length,
                                    uint32_t timeout)
{
    if (!i2c || !reg_addr || !data) return XY_HAL_ERROR_INVALID_PARAM;
    
    ch32u5_i2c_data_t *dev = (ch32u5_i2c_data_t *)i2c;
    
    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(dev->hi2c, dev_addr << 1,
                                                  *(uint16_t *)reg_addr,
                                                  (reg_size == 2) ? I2C_MEMADD_SIZE_16BIT : I2C_MEMADD_SIZE_8BIT,
                                                  (uint8_t *)data, length, timeout);
    
    if (status == HAL_OK) {
        dev->stats.tx_bytes += length + reg_size;
        return XY_HAL_OK;
    } else {
        dev->stats.tx_errors++;
        return XY_HAL_ERROR_FAIL;
    }
}

xy_hal_error_t xy_hal_i2c_reg_read(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                   const uint8_t *reg_addr, size_t reg_size,
                                   uint8_t *data, size_t length,
                                   uint32_t timeout)
{
    if (!i2c || !reg_addr || !data) return XY_HAL_ERROR_INVALID_PARAM;
    
    ch32u5_i2c_data_t *dev = (ch32u5_i2c_data_t *)i2c;
    
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(dev->hi2c, dev_addr << 1,
                                                 *(uint16_t *)reg_addr,
                                                 (reg_size == 2) ? I2C_MEMADD_SIZE_16BIT : I2C_MEMADD_SIZE_8BIT,
                                                 data, length, timeout);
    
    if (status == HAL_OK) {
        dev->stats.rx_bytes += length;
        return XY_HAL_OK;
    } else {
        dev->stats.rx_errors++;
        return XY_HAL_ERROR_FAIL;
    }
}

int32_t xy_hal_i2c_scan(xy_hal_i2c_t i2c, uint8_t *addrs, size_t max_count,
                        uint32_t timeout)
{
    if (!i2c || !addrs || max_count == 0) return XY_HAL_ERROR_INVALID_PARAM;
    
    int count = 0;
    
    for (uint16_t addr = 0x08; addr <= 0x77 && count < (int)max_count; addr++) {
        if (xy_hal_i2c_probe(i2c, addr, timeout) == XY_HAL_OK) {
            addrs[count++] = (uint8_t)addr;
        }
    }
    
    return count;
}

xy_hal_error_t xy_hal_i2c_probe(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                uint32_t timeout)
{
    if (!i2c) return XY_HAL_ERROR_INVALID_PARAM;
    
    ch32u5_i2c_data_t *dev = (ch32u5_i2c_data_t *)i2c;
    
    HAL_StatusTypeDef status = HAL_I2C_IsDeviceReady(dev->hi2c, dev_addr << 1, 1, timeout);
    
    if (status == HAL_OK) {
        return XY_HAL_OK;
    } else {
        return XY_HAL_ERROR_FAIL;
    }
}

int32_t xy_hal_i2c_master_transmit_nb(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                      const uint8_t *data, size_t length)
{
    (void)i2c;
    (void)dev_addr;
    (void)data;
    (void)length;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int32_t xy_hal_i2c_master_receive_nb(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                     uint8_t *data, size_t length)
{
    (void)i2c;
    (void)dev_addr;
    (void)data;
    (void)length;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_i2c_master_transmit_async(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                                const uint8_t *data, size_t length,
                                                xy_hal_i2c_callback_t callback, void *arg)
{
    if (!i2c || !data || length == 0) return XY_HAL_ERROR_INVALID_PARAM;
    
    ch32u5_i2c_data_t *dev = (ch32u5_i2c_data_t *)i2c;
    
    if (dev->tx_busy) return XY_HAL_ERROR_BUSY;
    
    dev->callback = callback;
    dev->callback_arg = arg;
    dev->tx_busy = 1;
    
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit_IT(dev->hi2c, dev_addr << 1,
                                                          (uint8_t *)data, length);
    
    if (status != HAL_OK) {
        dev->tx_busy = 0;
        dev->stats.tx_errors++;
        return XY_HAL_ERROR_FAIL;
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_master_receive_async(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                               uint8_t *data, size_t length,
                                               xy_hal_i2c_callback_t callback, void *arg)
{
    if (!i2c || !data || length == 0) return XY_HAL_ERROR_INVALID_PARAM;
    
    ch32u5_i2c_data_t *dev = (ch32u5_i2c_data_t *)i2c;
    
    if (dev->rx_busy) return XY_HAL_ERROR_BUSY;
    
    dev->callback = callback;
    dev->callback_arg = arg;
    dev->rx_busy = 1;
    
    HAL_StatusTypeDef status = HAL_I2C_Master_Receive_IT(dev->hi2c, dev_addr << 1,
                                                         data, length);
    
    if (status != HAL_OK) {
        dev->rx_busy = 0;
        dev->stats.rx_errors++;
        return XY_HAL_ERROR_FAIL;
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_stop_async(xy_hal_i2c_t i2c)
{
    if (!i2c) return XY_HAL_ERROR_INVALID_PARAM;
    
    ch32u5_i2c_data_t *dev = (ch32u5_i2c_data_t *)i2c;
    HAL_I2C_Abort(dev->hi2c);
    dev->tx_busy = 0;
    dev->rx_busy = 0;
    dev->callback = NULL;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_get_status(xy_hal_i2c_t i2c,
                                     xy_hal_i2c_status_t *status)
{
    if (!i2c || !status) return XY_HAL_ERROR_INVALID_PARAM;
    
    ch32u5_i2c_data_t *dev = (ch32u5_i2c_data_t *)i2c;
    status->tx_busy = dev->tx_busy;
    status->rx_busy = dev->rx_busy;
    status->bus_busy = (__HAL_I2C_GET_FLAG(dev->hi2c, I2C_FLAG_BUSY) != RESET);
    status->tx_complete = !dev->tx_busy;
    status->rx_available = 0;
    status->errors = dev->hi2c->ErrorCode;
    return XY_HAL_OK;
}

xy_hal_i2c_error_t xy_hal_i2c_get_error(xy_hal_i2c_t i2c)
{
    if (!i2c) return XY_HAL_I2C_ERROR_NONE;
    ch32u5_i2c_data_t *dev = (ch32u5_i2c_data_t *)i2c;
    return (xy_hal_i2c_error_t)dev->hi2c->ErrorCode;
}

xy_hal_error_t xy_hal_i2c_clear_error(xy_hal_i2c_t i2c)
{
    if (!i2c) return XY_HAL_ERROR_INVALID_PARAM;
    ch32u5_i2c_data_t *dev = (ch32u5_i2c_data_t *)i2c;
    dev->hi2c->ErrorCode = HAL_I2C_ERROR_NONE;
    __HAL_I2C_CLEAR_FLAG(dev->hi2c, I2C_FLAG_AF);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_get_stats(xy_hal_i2c_t i2c,
                                    xy_hal_i2c_stats_t *stats)
{
    if (!i2c || !stats) return XY_HAL_ERROR_INVALID_PARAM;
    ch32u5_i2c_data_t *dev = (ch32u5_i2c_data_t *)i2c;
    *stats = dev->stats;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_reset_stats(xy_hal_i2c_t i2c)
{
    if (!i2c) return XY_HAL_ERROR_INVALID_PARAM;
    ch32u5_i2c_data_t *dev = (ch32u5_i2c_data_t *)i2c;
    memset(&dev->stats, 0, sizeof(dev->stats));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_enable(xy_hal_i2c_t i2c, int enable)
{
    if (!i2c) return XY_HAL_ERROR_INVALID_PARAM;
    ch32u5_i2c_data_t *dev = (ch32u5_i2c_data_t *)i2c;
    if (enable) {
        __HAL_I2C_ENABLE(dev->hi2c);
    } else {
        __HAL_I2C_DISABLE(dev->hi2c);
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_reset(xy_hal_i2c_t i2c)
{
    if (!i2c) return XY_HAL_ERROR_INVALID_PARAM;
    ch32u5_i2c_data_t *dev = (ch32u5_i2c_data_t *)i2c;
    HAL_I2C_DeInit(dev->hi2c);
    HAL_I2C_Init(dev->hi2c);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_control(xy_hal_i2c_t i2c, int cmd, void *arg)
{
    switch (cmd) {
        case XY_HAL_I2C_CMD_RESET:
            return xy_hal_i2c_reset(i2c);
        case XY_HAL_I2C_CMD_PROBE:
            return xy_hal_i2c_probe(i2c, (uint16_t)(uintptr_t)arg, 100);
        default:
            return XY_HAL_ERROR_NOT_SUPPORTED;
    }
}

/* ==================== End of File ==================== */
