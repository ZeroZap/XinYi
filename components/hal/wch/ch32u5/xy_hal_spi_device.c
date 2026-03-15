/**
 * @file xy_hal_spi_device.c
 * @brief WCH CH32U5 SPI Device Implementation - Unified HAL API
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 实现统一的 SPI 设备 API，基于 WCH CH32U5 HAL 库
 */

#include "../inc/xy_hal_spi_dev.h"
#include "../inc/xy_hal_spi_types.h"
#include <string.h>

/* WCH CH32U5 HAL 头文件 */
#include "ch32u5xx.h"

/* ==================== Private Definitions ==================== */

#define CH32U5_SPI_INSTANCE_COUNT  (4)  /* SPI1-3, I2S2 */

/* ==================== Private Types ==================== */

/**
 * @brief CH32U5 SPI 设备私有数据
 */
typedef struct {
    SPI_HandleTypeDef *hspi;
    uint32_t instance;
    xy_hal_spi_config_t config;
    xy_hal_spi_callback_t callback;
    void *callback_arg;
    xy_hal_spi_stats_t stats;
    uint8_t tx_busy;
    uint8_t rx_busy;
    uint8_t cs_pin;
} ch32u5_spi_data_t;

/* ==================== Private Variables ==================== */

static ch32u5_spi_data_t spi_devices[CH32U5_SPI_INSTANCE_COUNT] = {0};

static const char *const spi_names[CH32U5_SPI_INSTANCE_COUNT] = {
    "SPI1", "SPI2", "SPI3", "I2S2"
};

extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi2;
extern SPI_HandleTypeDef hspi3;
extern SPI_HandleTypeDef hi2s2;

static SPI_HandleTypeDef *const spi_handles[CH32U5_SPI_INSTANCE_COUNT] = {
    &hspi1, &hspi2, &hspi3, &hi2s2
};

/* ==================== Private Functions ==================== */

static int spi_find_index(const char *name)
{
    for (int i = 0; i < CH32U5_SPI_INSTANCE_COUNT; i++) {
        if (strcmp(name, spi_names[i]) == 0) {
            return i;
        }
    }
    return -1;
}

/* ==================== Callback Forwarding ==================== */

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    for (int i = 0; i < CH32U5_SPI_INSTANCE_COUNT; i++) {
        if (hspi == spi_devices[i].hspi && spi_devices[i].callback) {
            spi_devices[i].tx_busy = 0;
            spi_devices[i].callback((xy_hal_spi_t)&spi_devices[i],
                                   XY_HAL_SPI_EVENT_TX_DONE,
                                   spi_devices[i].callback_arg);
            break;
        }
    }
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    for (int i = 0; i < CH32U5_SPI_INSTANCE_COUNT; i++) {
        if (hspi == spi_devices[i].hspi && spi_devices[i].callback) {
            spi_devices[i].rx_busy = 0;
            spi_devices[i].callback((xy_hal_spi_t)&spi_devices[i],
                                   XY_HAL_SPI_EVENT_RX_DONE,
                                   spi_devices[i].callback_arg);
            break;
        }
    }
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    for (int i = 0; i < CH32U5_SPI_INSTANCE_COUNT; i++) {
        if (hspi == spi_devices[i].hspi && spi_devices[i].callback) {
            spi_devices[i].tx_busy = 0;
            spi_devices[i].rx_busy = 0;
            spi_devices[i].callback((xy_hal_spi_t)&spi_devices[i],
                                   XY_HAL_SPI_EVENT_TX_RX_DONE,
                                   spi_devices[i].callback_arg);
            break;
        }
    }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    for (int i = 0; i < CH32U5_SPI_INSTANCE_COUNT; i++) {
        if (hspi == spi_devices[i].hspi) {
            spi_devices[i].stats.tx_errors++;
            if (spi_devices[i].callback) {
                spi_devices[i].callback((xy_hal_spi_t)&spi_devices[i],
                                       XY_HAL_SPI_EVENT_ERROR,
                                       spi_devices[i].callback_arg);
            }
            break;
        }
    }
}

/* ==================== Device Model API Implementation ==================== */

xy_hal_spi_t xy_hal_spi_bind(const char *name)
{
    int index = spi_find_index(name);
    if (index < 0) return NULL;
    
    ch32u5_spi_data_t *dev = &spi_devices[index];
    dev->hspi = (SPI_HandleTypeDef *)spi_handles[index];
    dev->instance = (uint32_t)index;
    dev->callback = NULL;
    dev->callback_arg = NULL;
    dev->tx_busy = 0;
    dev->rx_busy = 0;
    dev->cs_pin = 0xFF;
    memset(&dev->stats, 0, sizeof(dev->stats));
    
    return (xy_hal_spi_t)dev;
}

xy_hal_error_t xy_hal_spi_unbind(xy_hal_spi_t spi)
{
    if (!spi) return XY_HAL_ERROR_INVALID_PARAM;
    
    ch32u5_spi_data_t *dev = (ch32u5_spi_data_t *)spi;
    HAL_SPI_Abort(dev->hspi);
    HAL_SPI_DeInit(dev->hspi);
    memset(dev, 0, sizeof(*dev));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_configure(xy_hal_spi_t spi,
                                    const xy_hal_spi_config_t *config)
{
    if (!spi || !config) return XY_HAL_ERROR_INVALID_PARAM;
    
    ch32u5_spi_data_t *dev = (ch32u5_spi_data_t *)spi;
    
    dev->hspi->Init.Mode = config->is_master ? SPI_MODE_MASTER : SPI_MODE_SLAVE;
    dev->hspi->Init.Direction = (config->direction == XY_HAL_SPI_DIR_1LINE) ?
                                SPI_DIRECTION_1LINE : SPI_DIRECTION_2LINES;
    dev->hspi->Init.DataSize = (config->datasize == XY_HAL_SPI_DATASIZE_16BIT) ?
                               SPI_DATASIZE_16BIT : SPI_DATASIZE_8BIT;
    dev->hspi->Init.CLKPolarity = (config->mode == XY_HAL_SPI_MODE_2 || 
                                   config->mode == XY_HAL_SPI_MODE_3) ?
                                  SPI_POLARITY_HIGH : SPI_POLARITY_LOW;
    dev->hspi->Init.CLKPhase = (config->mode == XY_HAL_SPI_MODE_1 || 
                                config->mode == XY_HAL_SPI_MODE_3) ?
                               SPI_PHASE_2EDGE : SPI_PHASE_1EDGE;
    dev->hspi->Init.NSS = (config->nss == XY_HAL_SPI_NSS_SOFT) ?
                          SPI_NSS_SOFT : SPI_NSS_HARD_INPUT;
    dev->hspi->Init.BaudRatePrescaler = config->baudrate_prescaler;
    dev->hspi->Init.FirstBit = (config->firstbit == XY_HAL_SPI_FIRSTBIT_LSB) ?
                               SPI_FIRSTBIT_LSB : SPI_FIRSTBIT_MSB;
    
    if (HAL_SPI_Init(dev->hspi) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
    
    dev->config = *config;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_get_config(xy_hal_spi_t spi,
                                     xy_hal_spi_config_t *config)
{
    if (!spi || !config) return XY_HAL_ERROR_INVALID_PARAM;
    
    ch32u5_spi_data_t *dev = (ch32u5_spi_data_t *)spi;
    *config = dev->config;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_set_baudrate(xy_hal_spi_t spi, uint32_t prescaler)
{
    if (!spi) return XY_HAL_ERROR_INVALID_PARAM;
    
    ch32u5_spi_data_t *dev = (ch32u5_spi_data_t *)spi;
    dev->config.baudrate_prescaler = prescaler;
    dev->hspi->Init.BaudRatePrescaler = prescaler;
    
    if (HAL_SPI_Init(dev->hspi) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
    return XY_HAL_OK;
}

int32_t xy_hal_spi_send(xy_hal_spi_t spi, const uint8_t *tx_data,
                        size_t length, uint32_t timeout)
{
    if (!spi || !tx_data || length == 0) return XY_HAL_ERROR_INVALID_PARAM;
    
    ch32u5_spi_data_t *dev = (ch32u5_spi_data_t *)spi;
    
    HAL_StatusTypeDef status = HAL_SPI_Transmit(dev->hspi, (uint8_t *)tx_data,
                                                 length, timeout);
    
    if (status == HAL_OK) {
        dev->stats.tx_bytes += length;
        return (int32_t)length;
    } else if (status == HAL_TIMEOUT) {
        return XY_HAL_ERROR_TIMEOUT;
    } else {
        dev->stats.tx_errors++;
        return XY_HAL_ERROR_FAIL;
    }
}

int32_t xy_hal_spi_receive(xy_hal_spi_t spi, uint8_t *rx_data,
                           size_t length, uint32_t timeout)
{
    if (!spi || !rx_data || length == 0) return XY_HAL_ERROR_INVALID_PARAM;
    
    ch32u5_spi_data_t *dev = (ch32u5_spi_data_t *)spi;
    
    HAL_StatusTypeDef status = HAL_SPI_Receive(dev->hspi, rx_data, length, timeout);
    
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

int32_t xy_hal_spi_transfer(xy_hal_spi_t spi, const uint8_t *tx_data,
                            uint8_t *rx_data, size_t length, uint32_t timeout)
{
    if (!spi || !rx_data || length == 0) return XY_HAL_ERROR_INVALID_PARAM;
    
    ch32u5_spi_data_t *dev = (ch32u5_spi_data_t *)spi;
    uint8_t dummy_tx = 0x00;
    const uint8_t *tx_buf = tx_data ? tx_data : &dummy_tx;
    
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(dev->hspi, 
                                                       (uint8_t *)tx_buf,
                                                       rx_data, 
                                                       length, 
                                                       timeout);
    
    if (status == HAL_OK) {
        dev->stats.tx_bytes += length;
        dev->stats.rx_bytes += length;
        return (int32_t)length;
    } else if (status == HAL_TIMEOUT) {
        return XY_HAL_ERROR_TIMEOUT;
    } else {
        dev->stats.tx_errors++;
        dev->stats.rx_errors++;
        return XY_HAL_ERROR_FAIL;
    }
}

int32_t xy_hal_spi_send_byte(xy_hal_spi_t spi, uint8_t data, uint32_t timeout)
{
    uint8_t rx_data;
    int32_t ret = xy_hal_spi_transfer(spi, &data, &rx_data, 1, timeout);
    return (ret > 0) ? (int32_t)rx_data : ret;
}

int32_t xy_hal_spi_receive_byte(xy_hal_spi_t spi, uint32_t timeout)
{
    uint8_t tx_data = 0x00;
    uint8_t rx_data;
    int32_t ret = xy_hal_spi_transfer(spi, &tx_data, &rx_data, 1, timeout);
    return (ret > 0) ? (int32_t)rx_data : ret;
}

int32_t xy_hal_spi_send_nb(xy_hal_spi_t spi, const uint8_t *tx_data,
                           size_t length)
{
    if (!spi || !tx_data || length == 0) return XY_HAL_ERROR_INVALID_PARAM;
    
    ch32u5_spi_data_t *dev = (ch32u5_spi_data_t *)spi;
    
    if (__HAL_SPI_GET_FLAG(dev->hspi, SPI_FLAG_TXE) == RESET) {
        return 0;
    }
    
    if (dev->hspi->Init.DataSize == SPI_DATASIZE_8BIT) {
        *((__IO uint8_t *)&dev->hspi->Instance->DR) = *tx_data;
    } else {
        *((__IO uint16_t *)&dev->hspi->Instance->DR) = *(uint16_t *)tx_data;
    }
    
    dev->stats.tx_bytes++;
    return 1;
}

int32_t xy_hal_spi_receive_nb(xy_hal_spi_t spi, uint8_t *rx_data,
                              size_t length)
{
    if (!spi || !rx_data || length == 0) return XY_HAL_ERROR_INVALID_PARAM;
    
    ch32u5_spi_data_t *dev = (ch32u5_spi_data_t *)spi;
    
    if (__HAL_SPI_GET_FLAG(dev->hspi, SPI_FLAG_RXNE) == RESET) {
        return 0;
    }
    
    if (dev->hspi->Init.DataSize == SPI_DATASIZE_8BIT) {
        *rx_data = *((__IO uint8_t *)&dev->hspi->Instance->DR);
    } else {
        *(uint16_t *)rx_data = *((__IO uint16_t *)&dev->hspi->Instance->DR);
    }
    
    dev->stats.rx_bytes++;
    return 1;
}

int32_t xy_hal_spi_tx_ready(xy_hal_spi_t spi)
{
    if (!spi) return 0;
    ch32u5_spi_data_t *dev = (ch32u5_spi_data_t *)spi;
    return (__HAL_SPI_GET_FLAG(dev->hspi, SPI_FLAG_TXE) != RESET) ? 1 : 0;
}

int32_t xy_hal_spi_rx_available(xy_hal_spi_t spi)
{
    if (!spi) return 0;
    ch32u5_spi_data_t *dev = (ch32u5_spi_data_t *)spi;
    return (__HAL_SPI_GET_FLAG(dev->hspi, SPI_FLAG_RXNE) != RESET) ? 1 : 0;
}

xy_hal_error_t xy_hal_spi_send_async(xy_hal_spi_t spi, const uint8_t *tx_data,
                                     size_t length, xy_hal_spi_callback_t callback,
                                     void *arg)
{
    if (!spi || !tx_data || length == 0) return XY_HAL_ERROR_INVALID_PARAM;
    
    ch32u5_spi_data_t *dev = (ch32u5_spi_data_t *)spi;
    
    if (dev->tx_busy) return XY_HAL_ERROR_BUSY;
    
    dev->callback = callback;
    dev->callback_arg = arg;
    dev->tx_busy = 1;
    
    HAL_StatusTypeDef status = HAL_SPI_Transmit_IT(dev->hspi, (uint8_t *)tx_data, length);
    
    if (status != HAL_OK) {
        dev->tx_busy = 0;
        dev->stats.tx_errors++;
        return XY_HAL_ERROR_FAIL;
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_receive_async(xy_hal_spi_t spi, uint8_t *rx_data,
                                        size_t length, xy_hal_spi_callback_t callback,
                                        void *arg)
{
    if (!spi || !rx_data || length == 0) return XY_HAL_ERROR_INVALID_PARAM;
    
    ch32u5_spi_data_t *dev = (ch32u5_spi_data_t *)spi;
    
    if (dev->rx_busy) return XY_HAL_ERROR_BUSY;
    
    dev->callback = callback;
    dev->callback_arg = arg;
    dev->rx_busy = 1;
    
    HAL_StatusTypeDef status = HAL_SPI_Receive_IT(dev->hspi, rx_data, length);
    
    if (status != HAL_OK) {
        dev->rx_busy = 0;
        dev->stats.rx_errors++;
        return XY_HAL_ERROR_FAIL;
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_transfer_async(xy_hal_spi_t spi, const uint8_t *tx_data,
                                         uint8_t *rx_data, size_t length,
                                         xy_hal_spi_callback_t callback, void *arg)
{
    if (!spi || !rx_data || length == 0) return XY_HAL_ERROR_INVALID_PARAM;
    
    ch32u5_spi_data_t *dev = (ch32u5_spi_data_t *)spi;
    
    if (dev->tx_busy || dev->rx_busy) return XY_HAL_ERROR_BUSY;
    
    dev->callback = callback;
    dev->callback_arg = arg;
    dev->tx_busy = 1;
    dev->rx_busy = 1;
    
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive_IT(dev->hspi,
                                                          (uint8_t *)tx_data,
                                                          rx_data,
                                                          length);
    
    if (status != HAL_OK) {
        dev->tx_busy = 0;
        dev->rx_busy = 0;
        dev->stats.tx_errors++;
        dev->stats.rx_errors++;
        return XY_HAL_ERROR_FAIL;
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_stop_async(xy_hal_spi_t spi)
{
    if (!spi) return XY_HAL_ERROR_INVALID_PARAM;
    
    ch32u5_spi_data_t *dev = (ch32u5_spi_data_t *)spi;
    HAL_SPI_Abort(dev->hspi);
    dev->tx_busy = 0;
    dev->rx_busy = 0;
    dev->callback = NULL;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_get_status(xy_hal_spi_t spi,
                                     xy_hal_spi_status_t *status)
{
    if (!spi || !status) return XY_HAL_ERROR_INVALID_PARAM;
    
    ch32u5_spi_data_t *dev = (ch32u5_spi_data_t *)spi;
    status->tx_busy = dev->tx_busy;
    status->rx_busy = dev->rx_busy;
    status->tx_complete = !dev->tx_busy;
    status->rx_available = xy_hal_spi_rx_available(spi);
    status->errors = dev->hspi->ErrorCode;
    return XY_HAL_OK;
}

xy_hal_spi_error_t xy_hal_spi_get_error(xy_hal_spi_t spi)
{
    if (!spi) return XY_HAL_SPI_ERROR_NONE;
    ch32u5_spi_data_t *dev = (ch32u5_spi_data_t *)spi;
    return (xy_hal_spi_error_t)dev->hspi->ErrorCode;
}

xy_hal_error_t xy_hal_spi_clear_error(xy_hal_spi_t spi)
{
    if (!spi) return XY_HAL_ERROR_INVALID_PARAM;
    ch32u5_spi_data_t *dev = (ch32u5_spi_data_t *)spi;
    dev->hspi->ErrorCode = HAL_SPI_ERROR_NONE;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_get_stats(xy_hal_spi_t spi,
                                    xy_hal_spi_stats_t *stats)
{
    if (!spi || !stats) return XY_HAL_ERROR_INVALID_PARAM;
    ch32u5_spi_data_t *dev = (ch32u5_spi_data_t *)spi;
    *stats = dev->stats;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_reset_stats(xy_hal_spi_t spi)
{
    if (!spi) return XY_HAL_ERROR_INVALID_PARAM;
    ch32u5_spi_data_t *dev = (ch32u5_spi_data_t *)spi;
    memset(&dev->stats, 0, sizeof(dev->stats));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_enable(xy_hal_spi_t spi, int enable)
{
    if (!spi) return XY_HAL_ERROR_INVALID_PARAM;
    ch32u5_spi_data_t *dev = (ch32u5_spi_data_t *)spi;
    if (enable) {
        __HAL_SPI_ENABLE(dev->hspi);
    } else {
        __HAL_SPI_DISABLE(dev->hspi);
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_set_cs(xy_hal_spi_t spi, uint8_t cs_pin)
{
    if (!spi) return XY_HAL_ERROR_INVALID_PARAM;
    ch32u5_spi_data_t *dev = (ch32u5_spi_data_t *)spi;
    dev->cs_pin = cs_pin;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_cs_assert(xy_hal_spi_t spi, int assert)
{
    (void)spi;
    (void)assert;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_spi_control(xy_hal_spi_t spi, int cmd, void *arg)
{
    (void)spi;
    (void)cmd;
    (void)arg;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

/* ==================== End of File ==================== */
