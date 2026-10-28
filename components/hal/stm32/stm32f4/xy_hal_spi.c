/**
 * @file xy_hal_spi.c
 * @brief SPI HAL STM32F4 implementation
 *
 * Rewritten against the current public `xy_hal_spi.h` contract.  The previous
 * version was deferred from the STM32F4 build for the same class of defects as
 * the UART and I2C wrappers: every entry point was declared `int` instead of
 * `xy_hal_error_t` (eight conflicting-type errors at the platform gate), and
 * `xy_hal_spi_stop` / `xy_hal_spi_error` / `xy_hal_spi_set_error_cb` were never
 * implemented at all.  Beyond the compile break three configurations were
 * silently rewritten:
 *
 *   - `XY_HAL_SPI_DIR_2LINES_RXONLY` collapsed into `SPI_DIRECTION_1LINE`
 *     (a receive-only master would start driving the MOSI line),
 *   - `XY_HAL_SPI_NSS_HARD_INPUT` collapsed into `SPI_NSS_HARD_OUTPUT`
 *     (the peripheral would drive the chip-select pin it is supposed to read),
 *   - any `datasize` other than `XY_HAL_SPI_DATASIZE_8BIT` became 16-bit, and
 *     any `baudrate_prescaler` value was written into `CR1.BR` unchecked, so an
 *     arbitrary value programmed reserved bits.
 *
 * The mode conversion was also a bit-mask coincidence: it returned
 * `SPI_POLARITY_* | SPI_PHASE_*` and then re-derived CPOL/CPHA by testing that
 * mask against a vendor constant, which only works while `SPI_CR1_CPOL` and
 * `SPI_CR1_CPHA` keep their current bit positions.
 *
 * The wrapper now validates the whole configuration before the first
 * `hspi->Init` write, maps every public enum explicitly, tracks an initialized
 * context per handle so callback registration is real and the four vendor
 * completion callbacks dispatch, and publishes the exact error taxonomy.
 *
 * Frame count: F4 programs 8- or 16-bit data frames, and `HAL_SPI_*` takes a
 * size in frames while the public API takes a byte length.  For 16-bit mode the
 * byte length is converted to frames (and an odd byte length is rejected)
 * instead of passing the byte count through, which would have moved twice the
 * caller's buffer.  L4/U5 still publish the raw byte count here; that residual
 * inconsistency is tracked separately and is not silently changed in this slice.
 */

#include "../../inc/xy_hal_spi.h"

#ifdef STM32_HAL_ENABLED

#include "stm32_hal.h"
#include <string.h>

#define XY_HAL_SPI_MAX_INSTANCES 4U

typedef struct {
    SPI_HandleTypeDef *handle;
    xy_hal_spi_callback_t callback;
    void *arg;
    uint8_t datasize;
    uint8_t initialized;
} xy_hal_spi_context_t;

static xy_hal_spi_context_t spi_contexts[XY_HAL_SPI_MAX_INSTANCES];

static xy_hal_spi_context_t *spi_context_find(const void *spi)
{
    uint32_t index;

    if (spi == NULL) return NULL;

    for (index = 0U; index < XY_HAL_SPI_MAX_INSTANCES; ++index) {
        if (spi_contexts[index].initialized != 0U && spi_contexts[index].handle == spi) {
            return &spi_contexts[index];
        }
    }
    return NULL;
}

static xy_hal_spi_context_t *spi_context_allocate(void)
{
    uint32_t index;

    for (index = 0U; index < XY_HAL_SPI_MAX_INSTANCES; ++index) {
        if (spi_contexts[index].initialized == 0U) {
            return &spi_contexts[index];
        }
    }
    return NULL;
}

static xy_hal_error_t spi_map_status(HAL_StatusTypeDef status)
{
    if (status == HAL_OK) return XY_HAL_OK;
    if (status == HAL_TIMEOUT) return XY_HAL_ERROR_TIMEOUT;
    if (status == HAL_BUSY) return XY_HAL_ERROR_BUSY;
    return XY_HAL_ERROR_IO;
}

/* CR1.BR holds the prescaler as a three-bit divider field: only the eight
 * vendor constants are representable, anything else sets reserved bits. */
static uint8_t spi_prescaler_valid(uint32_t prescaler)
{
    switch (prescaler) {
    case SPI_BAUDRATEPRESCALER_2:
    case SPI_BAUDRATEPRESCALER_4:
    case SPI_BAUDRATEPRESCALER_8:
    case SPI_BAUDRATEPRESCALER_16:
    case SPI_BAUDRATEPRESCALER_32:
    case SPI_BAUDRATEPRESCALER_64:
    case SPI_BAUDRATEPRESCALER_128:
    case SPI_BAUDRATEPRESCALER_256:
        return 1U;
    default:
        return 0U;
    }
}

static uint32_t spi_direction(xy_hal_spi_direction_t direction)
{
    switch (direction) {
    case XY_HAL_SPI_DIR_2LINES_RXONLY:
        return SPI_DIRECTION_2LINES_RXONLY;
    case XY_HAL_SPI_DIR_1LINE:
        return SPI_DIRECTION_1LINE;
    case XY_HAL_SPI_DIR_2LINES:
        return SPI_DIRECTION_2LINES;
    default:
        return SPI_DIRECTION_2LINES;
    }
}

static uint32_t spi_nss(xy_hal_spi_nss_t nss)
{
    switch (nss) {
    case XY_HAL_SPI_NSS_HARD_INPUT:
        return SPI_NSS_HARD_INPUT;
    case XY_HAL_SPI_NSS_HARD_OUTPUT:
        return SPI_NSS_HARD_OUTPUT;
    case XY_HAL_SPI_NSS_SOFT:
        return SPI_NSS_SOFT;
    default:
        return SPI_NSS_SOFT;
    }
}

/* CPOL/CPHA per mode, enumerated instead of re-derived from a bit mask. */
static uint32_t spi_polarity(xy_hal_spi_mode_t mode)
{
    switch (mode) {
    case XY_HAL_SPI_MODE_2:
        return SPI_POLARITY_HIGH;
    case XY_HAL_SPI_MODE_3:
        return SPI_POLARITY_HIGH;
    case XY_HAL_SPI_MODE_0:
        return SPI_POLARITY_LOW;
    case XY_HAL_SPI_MODE_1:
        return SPI_POLARITY_LOW;
    default:
        return SPI_POLARITY_LOW;
    }
}

static uint32_t spi_phase(xy_hal_spi_mode_t mode)
{
    switch (mode) {
    case XY_HAL_SPI_MODE_1:
        return SPI_PHASE_2EDGE;
    case XY_HAL_SPI_MODE_3:
        return SPI_PHASE_2EDGE;
    case XY_HAL_SPI_MODE_0:
        return SPI_PHASE_1EDGE;
    case XY_HAL_SPI_MODE_2:
        return SPI_PHASE_1EDGE;
    default:
        return SPI_PHASE_1EDGE;
    }
}

static uint32_t spi_datasize(xy_hal_spi_datasize_t datasize)
{
    switch (datasize) {
    case XY_HAL_SPI_DATASIZE_16BIT:
        return SPI_DATASIZE_16BIT;
    case XY_HAL_SPI_DATASIZE_8BIT:
        return SPI_DATASIZE_8BIT;
    default:
        return SPI_DATASIZE_8BIT;
    }
}

static uint32_t spi_firstbit(xy_hal_spi_firstbit_t firstbit)
{
    switch (firstbit) {
    case XY_HAL_SPI_FIRSTBIT_LSB:
        return SPI_FIRSTBIT_LSB;
    case XY_HAL_SPI_FIRSTBIT_MSB:
        return SPI_FIRSTBIT_MSB;
    default:
        return SPI_FIRSTBIT_MSB;
    }
}

/* The public API takes a byte length; HAL_SPI_* takes a frame count. */
static xy_hal_error_t spi_frame_count(const xy_hal_spi_context_t *context, size_t len,
                                      uint16_t *frames)
{
    if (len == 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (context->datasize == (uint8_t)XY_HAL_SPI_DATASIZE_16BIT) {
        if ((len & 1U) != 0U) {
            return XY_HAL_ERROR_INVALID_PARAM;
        }
        len >>= 1;
    }
    if (len > UINT16_MAX) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    *frames = (uint16_t)len;
    return XY_HAL_OK;
}

/* Resolve the handle to its live context and the transfer length to frames.
 * Every transfer entry point shares this so the three failure classes
 * (argument, state, length) are reported identically. */
static xy_hal_error_t spi_validate_transfer(void *spi, const void *data, size_t len,
                                            xy_hal_spi_context_t **context, uint16_t *frames)
{
    xy_hal_spi_context_t *live;

    if (spi == NULL || data == NULL || len == 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    live = spi_context_find(spi);
    if (live == NULL) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    *context = live;
    return spi_frame_count(live, len, frames);
}

/* Reject an out-of-domain configuration before the first vendor field write so
 * a bogus value never reaches the peripheral. */
static xy_hal_error_t spi_config_check(const xy_hal_spi_config_t *config)
{
    if (config->is_master > 1U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (config->mode > XY_HAL_SPI_MODE_3) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (config->direction > XY_HAL_SPI_DIR_1LINE) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (config->datasize > XY_HAL_SPI_DATASIZE_16BIT) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (config->firstbit > XY_HAL_SPI_FIRSTBIT_LSB) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (config->nss > XY_HAL_SPI_NSS_HARD_OUTPUT) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (spi_prescaler_valid(config->baudrate_prescaler) == 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_init(void *spi, const xy_hal_spi_config_t *config)
{
    SPI_HandleTypeDef *handle = (SPI_HandleTypeDef *)spi;
    xy_hal_spi_context_t *context;
    xy_hal_error_t status;

    if (handle == NULL || config == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    status = spi_config_check(config);
    if (status != XY_HAL_OK) {
        return status;
    }
    if (spi_context_find(spi) != NULL) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }
    context = spi_context_allocate();
    if (context == NULL) {
        return XY_HAL_ERROR_NO_RESOURCE;
    }

    handle->Init.Mode = (config->is_master != 0U) ? SPI_MODE_MASTER : SPI_MODE_SLAVE;
    handle->Init.Direction = spi_direction(config->direction);
    handle->Init.DataSize = spi_datasize(config->datasize);
    handle->Init.CLKPolarity = spi_polarity(config->mode);
    handle->Init.CLKPhase = spi_phase(config->mode);
    handle->Init.NSS = spi_nss(config->nss);
    handle->Init.BaudRatePrescaler = config->baudrate_prescaler;
    handle->Init.FirstBit = spi_firstbit(config->firstbit);
    handle->Init.TIMode = SPI_TIMODE_DISABLE;
    handle->Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    handle->Init.CRCPolynomial = 7U;

    if (HAL_SPI_Init(handle) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    context->handle = handle;
    context->callback = NULL;
    context->arg = NULL;
    context->datasize = (uint8_t)config->datasize;
    context->initialized = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_deinit(void *spi)
{
    xy_hal_spi_context_t *context;

    if (spi == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    context = spi_context_find(spi);
    if (context == NULL) {
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
    xy_hal_spi_context_t *context = NULL;
    uint16_t frames = 0U;
    xy_hal_error_t status;

    status = spi_validate_transfer(spi, data, len, &context, &frames);
    if (status != XY_HAL_OK) {
        return status;
    }
    return spi_map_status(
        HAL_SPI_Transmit((SPI_HandleTypeDef *)spi, (uint8_t *)data, frames, timeout));
}

xy_hal_error_t xy_hal_spi_receive(void *spi, uint8_t *data, size_t len, uint32_t timeout)
{
    xy_hal_spi_context_t *context = NULL;
    uint16_t frames = 0U;
    xy_hal_error_t status;

    status = spi_validate_transfer(spi, data, len, &context, &frames);
    if (status != XY_HAL_OK) {
        return status;
    }
    return spi_map_status(HAL_SPI_Receive((SPI_HandleTypeDef *)spi, data, frames, timeout));
}

xy_hal_error_t xy_hal_spi_transmit_receive(void *spi, const uint8_t *tx_data, uint8_t *rx_data,
                                           size_t len, uint32_t timeout)
{
    xy_hal_spi_context_t *context = NULL;
    uint16_t frames = 0U;
    xy_hal_error_t status;

    if (rx_data == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    status = spi_validate_transfer(spi, tx_data, len, &context, &frames);
    if (status != XY_HAL_OK) {
        return status;
    }
    return spi_map_status(HAL_SPI_TransmitReceive((SPI_HandleTypeDef *)spi, (uint8_t *)tx_data,
                                                  rx_data, frames, timeout));
}

xy_hal_error_t xy_hal_spi_transmit_dma(void *spi, const uint8_t *data, size_t len)
{
    xy_hal_spi_context_t *context = NULL;
    uint16_t frames = 0U;
    xy_hal_error_t status;

    status = spi_validate_transfer(spi, data, len, &context, &frames);
    if (status != XY_HAL_OK) {
        return status;
    }
    return spi_map_status(HAL_SPI_Transmit_DMA((SPI_HandleTypeDef *)spi, (uint8_t *)data, frames));
}

xy_hal_error_t xy_hal_spi_receive_dma(void *spi, uint8_t *data, size_t len)
{
    xy_hal_spi_context_t *context = NULL;
    uint16_t frames = 0U;
    xy_hal_error_t status;

    status = spi_validate_transfer(spi, data, len, &context, &frames);
    if (status != XY_HAL_OK) {
        return status;
    }
    return spi_map_status(HAL_SPI_Receive_DMA((SPI_HandleTypeDef *)spi, data, frames));
}

xy_hal_error_t xy_hal_spi_transmit_receive_dma(void *spi, const uint8_t *tx_data, uint8_t *rx_data,
                                               size_t len)
{
    xy_hal_spi_context_t *context = NULL;
    uint16_t frames = 0U;
    xy_hal_error_t status;

    if (rx_data == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    status = spi_validate_transfer(spi, tx_data, len, &context, &frames);
    if (status != XY_HAL_OK) {
        return status;
    }
    return spi_map_status(
        HAL_SPI_TransmitReceive_DMA((SPI_HandleTypeDef *)spi, (uint8_t *)tx_data, rx_data, frames));
}

xy_hal_error_t xy_hal_spi_stop(void *spi)
{
    if (spi == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (spi_context_find(spi) == NULL) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return spi_map_status(HAL_SPI_DMAStop((SPI_HandleTypeDef *)spi));
}

xy_hal_error_t xy_hal_spi_register_callback(void *spi, xy_hal_spi_callback_t callback, void *arg)
{
    xy_hal_spi_context_t *context;

    if (spi == NULL || callback == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    context = spi_context_find(spi);
    if (context == NULL) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    context->callback = callback;
    context->arg = arg;
    return XY_HAL_OK;
}

/* Software chip select is owned by the driver above this wrapper (the NSS pin
 * is a plain GPIO), so there is nothing for the HAL to drive. */
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

static void spi_dispatch_event(SPI_HandleTypeDef *handle, xy_hal_spi_event_t event)
{
    xy_hal_spi_context_t *context = spi_context_find(handle);

    if (context != NULL && context->callback != NULL) {
        context->callback(handle, event, context->arg);
    }
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *handle)
{
    spi_dispatch_event(handle, XY_HAL_SPI_EVENT_TX_DONE);
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *handle)
{
    spi_dispatch_event(handle, XY_HAL_SPI_EVENT_RX_DONE);
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *handle)
{
    spi_dispatch_event(handle, XY_HAL_SPI_EVENT_TX_RX_DONE);
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *handle)
{
    spi_dispatch_event(handle, XY_HAL_SPI_EVENT_ERROR);
}

#endif /* STM32_HAL_ENABLED */
