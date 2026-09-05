/**
 * @file xy_hal_spi.c
 * @brief SPI HAL STM32L4 implementation
 */

#include "../../inc/xy_hal_spi.h"

#if defined(STM32L4) || defined(STM32L4xx)

#include "stm32l4xx_hal.h"
#include <string.h>

typedef struct {
    SPI_HandleTypeDef *hspi;
    xy_hal_spi_callback_t callback;
    void *arg;
    uint8_t initialized;
} spi_ctx_t;

#define MAX_SPI_INSTANCES 4U
static spi_ctx_t spi_contexts[MAX_SPI_INSTANCES];

static spi_ctx_t *find_context(const void *spi)
{
    for (size_t index = 0U; index < MAX_SPI_INSTANCES; ++index) {
        if (spi_contexts[index].hspi == spi) {
            return &spi_contexts[index];
        }
    }
    return NULL;
}

static spi_ctx_t *allocate_context(void)
{
    for (size_t index = 0U; index < MAX_SPI_INSTANCES; ++index) {
        if (spi_contexts[index].hspi == NULL) {
            return &spi_contexts[index];
        }
    }
    return NULL;
}

static uint32_t to_direction(xy_hal_spi_direction_t direction)
{
    switch (direction) {
    case XY_HAL_SPI_DIR_2LINES_RXONLY:
        return SPI_DIRECTION_2LINES_RXONLY;
    case XY_HAL_SPI_DIR_1LINE:
        return SPI_DIRECTION_1LINE;
    case XY_HAL_SPI_DIR_2LINES:
    default:
        return SPI_DIRECTION_2LINES;
    }
}

static uint32_t to_nss(xy_hal_spi_nss_t nss)
{
    switch (nss) {
    case XY_HAL_SPI_NSS_HARD_INPUT:
        return SPI_NSS_HARD_INPUT;
    case XY_HAL_SPI_NSS_HARD_OUTPUT:
        return SPI_NSS_HARD_OUTPUT;
    case XY_HAL_SPI_NSS_SOFT:
    default:
        return SPI_NSS_SOFT;
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

xy_hal_error_t xy_hal_spi_init(void *spi, const xy_hal_spi_config_t *config)
{
    SPI_HandleTypeDef *hspi;
    spi_ctx_t *context;

    if (spi == NULL || config == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    hspi = (SPI_HandleTypeDef *)spi;
    context = find_context(spi);
    if (context != NULL && context->initialized != 0U) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }
    if (context == NULL) {
        context = allocate_context();
        if (context == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
    }

    hspi->Init.Mode = config->is_master != 0U ? SPI_MODE_MASTER : SPI_MODE_SLAVE;
    hspi->Init.Direction = to_direction(config->direction);
    hspi->Init.DataSize = config->datasize == XY_HAL_SPI_DATASIZE_16BIT ? SPI_DATASIZE_16BIT
                                                                        : SPI_DATASIZE_8BIT;
    hspi->Init.CLKPolarity = config->mode >= XY_HAL_SPI_MODE_2 ? SPI_POLARITY_HIGH
                                                               : SPI_POLARITY_LOW;
    hspi->Init.CLKPhase = (config->mode == XY_HAL_SPI_MODE_1 ||
                           config->mode == XY_HAL_SPI_MODE_3)
                              ? SPI_PHASE_2EDGE
                              : SPI_PHASE_1EDGE;
    hspi->Init.NSS = to_nss(config->nss);
    hspi->Init.BaudRatePrescaler = config->baudrate_prescaler;
    hspi->Init.FirstBit = config->firstbit == XY_HAL_SPI_FIRSTBIT_LSB ? SPI_FIRSTBIT_LSB
                                                                      : SPI_FIRSTBIT_MSB;
    hspi->Init.TIMode = SPI_TIMODE_DISABLE;
    hspi->Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi->Init.CRCPolynomial = 7U;

    if (HAL_SPI_Init(hspi) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
    context->hspi = hspi;
    context->callback = NULL;
    context->arg = NULL;
    context->initialized = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_deinit(void *spi)
{
    spi_ctx_t *context;

    if (spi == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    context = find_context(spi);
    if (context == NULL || context->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (HAL_SPI_DeInit((SPI_HandleTypeDef *)spi) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
    memset(context, 0, sizeof(*context));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_transmit(void *spi, const uint8_t *data, size_t len, uint32_t timeout)
{
    spi_ctx_t *context;

    if (spi == NULL || data == NULL || len == 0U || len > UINT16_MAX) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    context = find_context(spi);
    if (context == NULL || context->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return map_status(HAL_SPI_Transmit((SPI_HandleTypeDef *)spi, (uint8_t *)data, (uint16_t)len,
                                       timeout));
}

xy_hal_error_t xy_hal_spi_receive(void *spi, uint8_t *data, size_t len, uint32_t timeout)
{
    spi_ctx_t *context;

    if (spi == NULL || data == NULL || len == 0U || len > UINT16_MAX) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    context = find_context(spi);
    if (context == NULL || context->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return map_status(
        HAL_SPI_Receive((SPI_HandleTypeDef *)spi, data, (uint16_t)len, timeout));
}

xy_hal_error_t xy_hal_spi_transmit_receive(void *spi, const uint8_t *tx_data, uint8_t *rx_data,
                                           size_t len, uint32_t timeout)
{
    spi_ctx_t *context;

    if (spi == NULL || tx_data == NULL || rx_data == NULL || len == 0U || len > UINT16_MAX) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    context = find_context(spi);
    if (context == NULL || context->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return map_status(HAL_SPI_TransmitReceive((SPI_HandleTypeDef *)spi, (uint8_t *)tx_data, rx_data,
                                              (uint16_t)len, timeout));
}

xy_hal_error_t xy_hal_spi_transmit_dma(void *spi, const uint8_t *data, size_t len)
{
    spi_ctx_t *context;

    if (spi == NULL || data == NULL || len == 0U || len > UINT16_MAX) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    context = find_context(spi);
    if (context == NULL || context->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return map_status(HAL_SPI_Transmit_DMA((SPI_HandleTypeDef *)spi, (uint8_t *)data, (uint16_t)len));
}

xy_hal_error_t xy_hal_spi_receive_dma(void *spi, uint8_t *data, size_t len)
{
    spi_ctx_t *context;

    if (spi == NULL || data == NULL || len == 0U || len > UINT16_MAX) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    context = find_context(spi);
    if (context == NULL || context->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return map_status(HAL_SPI_Receive_DMA((SPI_HandleTypeDef *)spi, data, (uint16_t)len));
}

xy_hal_error_t xy_hal_spi_transmit_receive_dma(void *spi, const uint8_t *tx_data, uint8_t *rx_data,
                                               size_t len)
{
    spi_ctx_t *context;

    if (spi == NULL || tx_data == NULL || rx_data == NULL || len == 0U || len > UINT16_MAX) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    context = find_context(spi);
    if (context == NULL || context->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return map_status(HAL_SPI_TransmitReceive_DMA((SPI_HandleTypeDef *)spi, (uint8_t *)tx_data,
                                                  rx_data, (uint16_t)len));
}

xy_hal_error_t xy_hal_spi_register_callback(void *spi, xy_hal_spi_callback_t callback, void *arg)
{
    spi_ctx_t *context;

    if (spi == NULL || callback == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    context = find_context(spi);
    if (context == NULL || context->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    context->callback = callback;
    context->arg = arg;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_set_cs(void *spi, uint8_t level)
{
    XY_UNUSED(level);
    return spi == NULL ? XY_HAL_ERROR_INVALID_PARAM : XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_spi_error(void *spi, uint8_t level)
{
    XY_UNUSED(level);
    return spi == NULL ? XY_HAL_ERROR_INVALID_PARAM : XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_spi_set_error_cb(void *spi, xy_hal_spi_callback_t callback, void *arg)
{
    return xy_hal_spi_register_callback(spi, callback, arg);
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    spi_ctx_t *context = find_context(hspi);
    if (context != NULL && context->callback != NULL) {
        context->callback(hspi, XY_HAL_SPI_EVENT_TX_DONE, context->arg);
    }
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    spi_ctx_t *context = find_context(hspi);
    if (context != NULL && context->callback != NULL) {
        context->callback(hspi, XY_HAL_SPI_EVENT_RX_DONE, context->arg);
    }
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    spi_ctx_t *context = find_context(hspi);
    if (context != NULL && context->callback != NULL) {
        context->callback(hspi, XY_HAL_SPI_EVENT_TX_RX_DONE, context->arg);
    }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    spi_ctx_t *context = find_context(hspi);
    if (context != NULL && context->callback != NULL) {
        context->callback(hspi, XY_HAL_SPI_EVENT_ERROR, context->arg);
    }
}

#endif
