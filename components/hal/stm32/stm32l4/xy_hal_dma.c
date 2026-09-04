/**
 * @file xy_hal_dma.c
 * @brief DMA HAL STM32L4 implementation
 */

#include "../../inc/xy_hal_dma.h"

#if defined(STM32L4) || defined(STM32L4xx)

#include "stm32l4xx_hal.h"

static uint32_t to_direction(xy_hal_dma_direction_t direction)
{
    switch (direction) {
    case XY_HAL_DMA_DIR_PERIPH_TO_MEM:
        return DMA_PERIPH_TO_MEMORY;
    case XY_HAL_DMA_DIR_MEM_TO_PERIPH:
        return DMA_MEMORY_TO_PERIPH;
    case XY_HAL_DMA_DIR_MEM_TO_MEM:
        return DMA_MEMORY_TO_MEMORY;
    default:
        return DMA_PERIPH_TO_MEMORY;
    }
}

static uint32_t to_mode(xy_hal_dma_mode_t mode)
{
    return mode == XY_HAL_DMA_MODE_CIRCULAR ? DMA_CIRCULAR : DMA_NORMAL;
}

static uint32_t to_priority(xy_hal_dma_priority_t priority)
{
    switch (priority) {
    case XY_HAL_DMA_PRIORITY_MEDIUM:
        return DMA_PRIORITY_MEDIUM;
    case XY_HAL_DMA_PRIORITY_HIGH:
        return DMA_PRIORITY_HIGH;
    case XY_HAL_DMA_PRIORITY_VERY_HIGH:
        return DMA_PRIORITY_VERY_HIGH;
    case XY_HAL_DMA_PRIORITY_LOW:
    default:
        return DMA_PRIORITY_LOW;
    }
}

static uint32_t to_periph_alignment(xy_hal_dma_width_t width)
{
    switch (width) {
    case XY_HAL_DMA_WIDTH_HALFWORD:
        return DMA_PDATAALIGN_HALFWORD;
    case XY_HAL_DMA_WIDTH_WORD:
        return DMA_PDATAALIGN_WORD;
    case XY_HAL_DMA_WIDTH_BYTE:
    default:
        return DMA_PDATAALIGN_BYTE;
    }
}

static uint32_t to_mem_alignment(xy_hal_dma_width_t width)
{
    switch (width) {
    case XY_HAL_DMA_WIDTH_HALFWORD:
        return DMA_MDATAALIGN_HALFWORD;
    case XY_HAL_DMA_WIDTH_WORD:
        return DMA_MDATAALIGN_WORD;
    case XY_HAL_DMA_WIDTH_BYTE:
    default:
        return DMA_MDATAALIGN_BYTE;
    }
}

xy_hal_error_t xy_hal_dma_init(void *dma, const xy_hal_dma_config_t *config)
{
    DMA_HandleTypeDef *hdma;

    if (dma == NULL || config == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    hdma = (DMA_HandleTypeDef *)dma;
    hdma->Init.Direction = to_direction(config->direction);
    hdma->Init.PeriphInc = config->periph_incr == XY_HAL_DMA_INCR_ENABLE ? DMA_PINC_ENABLE
                                                                         : DMA_PINC_DISABLE;
    hdma->Init.MemInc = config->mem_incr == XY_HAL_DMA_INCR_ENABLE ? DMA_MINC_ENABLE
                                                                   : DMA_MINC_DISABLE;
    hdma->Init.PeriphDataAlignment = to_periph_alignment(config->periph_width);
    hdma->Init.MemDataAlignment = to_mem_alignment(config->mem_width);
    hdma->Init.Mode = to_mode(config->mode);
    hdma->Init.Priority = to_priority(config->priority);
#if defined(DMAMUX1)
    if (config->direction == XY_HAL_DMA_DIR_MEM_TO_MEM) {
        hdma->Init.Request = DMA_REQUEST_MEM2MEM;
    }
#endif
    return HAL_DMA_Init(hdma) == HAL_OK ? XY_HAL_OK : XY_HAL_ERROR_FAIL;
}

xy_hal_error_t xy_hal_dma_deinit(void *dma)
{
    if (dma == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    return HAL_DMA_DeInit((DMA_HandleTypeDef *)dma) == HAL_OK ? XY_HAL_OK : XY_HAL_ERROR_FAIL;
}

xy_hal_error_t xy_hal_dma_start(void *dma, uint32_t src_addr, uint32_t dst_addr, size_t data_len)
{
    DMA_HandleTypeDef *hdma = (DMA_HandleTypeDef *)dma;

    if (hdma == NULL || data_len == 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (hdma->State == HAL_DMA_STATE_RESET) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return HAL_DMA_Start(hdma, src_addr, dst_addr, (uint32_t)data_len) == HAL_OK
               ? XY_HAL_OK
               : XY_HAL_ERROR_BUSY;
}

xy_hal_error_t xy_hal_dma_stop(void *dma)
{
    if (dma == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    return HAL_DMA_Abort((DMA_HandleTypeDef *)dma) == HAL_OK ? XY_HAL_OK : XY_HAL_ERROR_FAIL;
}

xy_hal_error_t xy_hal_dma_register_callback(void *dma, xy_hal_dma_event_t event,
                                            xy_hal_dma_callback_t callback, void *arg)
{
    XY_UNUSED(dma);
    XY_UNUSED(event);
    XY_UNUSED(callback);
    XY_UNUSED(arg);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_dma_get_counter(void *dma)
{
    if (dma == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    return (int)__HAL_DMA_GET_COUNTER((DMA_HandleTypeDef *)dma);
}

xy_hal_error_t xy_hal_dma_poll_complete(void *dma, uint32_t timeout)
{
    HAL_StatusTypeDef status;

    if (dma == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (((DMA_HandleTypeDef *)dma)->State == HAL_DMA_STATE_RESET) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    status = HAL_DMA_PollForTransfer((DMA_HandleTypeDef *)dma, HAL_DMA_FULL_TRANSFER, timeout);
    if (status == HAL_OK) {
        return XY_HAL_OK;
    }
    return status == HAL_TIMEOUT ? XY_HAL_ERROR_TIMEOUT : XY_HAL_ERROR_FAIL;
}

#endif
