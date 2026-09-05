/**
 * @file xy_hal_dma.c
 * @brief DMA HAL STM32L4 implementation
 */

#include "../../inc/xy_hal_dma.h"

#if defined(STM32L4) || defined(STM32L4xx)

#include "stm32l4xx_hal.h"
#include <string.h>

typedef struct {
    DMA_HandleTypeDef *hdma;
    xy_hal_dma_callback_t callbacks[3];
    void *args[3];
    uint8_t initialized;
} dma_ctx_t;

#define MAX_DMA_INSTANCES 16U
static dma_ctx_t dma_contexts[MAX_DMA_INSTANCES];

static dma_ctx_t *find_context(const void *dma)
{
    for (size_t index = 0U; index < MAX_DMA_INSTANCES; ++index) {
        if (dma_contexts[index].hdma == dma) {
            return &dma_contexts[index];
        }
    }
    return NULL;
}

static dma_ctx_t *allocate_context(void)
{
    for (size_t index = 0U; index < MAX_DMA_INSTANCES; ++index) {
        if (dma_contexts[index].hdma == NULL) {
            return &dma_contexts[index];
        }
    }
    return NULL;
}

static void dispatch_callback(DMA_HandleTypeDef *hdma, size_t index, xy_hal_dma_event_t event)
{
    dma_ctx_t *context = find_context(hdma);

    if (context != NULL && context->callbacks[index] != NULL) {
        context->callbacks[index](hdma, event, context->args[index]);
    }
}

static void transfer_complete_callback(DMA_HandleTypeDef *hdma)
{
    dispatch_callback(hdma, 0U, XY_HAL_DMA_EVENT_COMPLETE);
}

static void transfer_half_callback(DMA_HandleTypeDef *hdma)
{
    dispatch_callback(hdma, 1U, XY_HAL_DMA_EVENT_HALF_COMPLETE);
}

static void transfer_error_callback(DMA_HandleTypeDef *hdma)
{
    dispatch_callback(hdma, 2U, XY_HAL_DMA_EVENT_ERROR);
}

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
    dma_ctx_t *context;

    if (dma == NULL || config == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    hdma = (DMA_HandleTypeDef *)dma;
    context = find_context(dma);
    if (context != NULL && context->initialized != 0U) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }
    if (context == NULL) {
        context = allocate_context();
        if (context == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
    }
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
    if (HAL_DMA_Init(hdma) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
    context->hdma = hdma;
    memset(context->callbacks, 0, sizeof(context->callbacks));
    memset(context->args, 0, sizeof(context->args));
    context->initialized = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dma_deinit(void *dma)
{
    dma_ctx_t *context;

    if (dma == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    context = find_context(dma);
    if (context == NULL || context->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (HAL_DMA_DeInit((DMA_HandleTypeDef *)dma) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
    memset(context, 0, sizeof(*context));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dma_start(void *dma, uint32_t src_addr, uint32_t dst_addr, size_t data_len)
{
    DMA_HandleTypeDef *hdma = (DMA_HandleTypeDef *)dma;
    dma_ctx_t *context;
    HAL_StatusTypeDef status;

    if (hdma == NULL || data_len == 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    context = find_context(dma);
    if (context == NULL || context->initialized == 0U || hdma->State == HAL_DMA_STATE_RESET) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (context->callbacks[0] != NULL || context->callbacks[1] != NULL ||
        context->callbacks[2] != NULL) {
        status = HAL_DMA_Start_IT(hdma, src_addr, dst_addr, (uint32_t)data_len);
    } else {
        status = HAL_DMA_Start(hdma, src_addr, dst_addr, (uint32_t)data_len);
    }
    return status == HAL_OK ? XY_HAL_OK : XY_HAL_ERROR_BUSY;
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
    static const HAL_DMA_CallbackIDTypeDef callback_ids[] = {
        HAL_DMA_XFER_CPLT_CB_ID,
        HAL_DMA_XFER_HALFCPLT_CB_ID,
        HAL_DMA_XFER_ERROR_CB_ID,
    };
    static void (*const hal_callbacks[])(DMA_HandleTypeDef *) = {
        transfer_complete_callback,
        transfer_half_callback,
        transfer_error_callback,
    };
    dma_ctx_t *context;
    size_t index = (size_t)event;

    if (dma == NULL || index >= 3U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    context = find_context(dma);
    if (context == NULL || context->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (HAL_DMA_RegisterCallback((DMA_HandleTypeDef *)dma, callback_ids[index],
                                 hal_callbacks[index]) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
    context->callbacks[index] = callback;
    context->args[index] = arg;
    return XY_HAL_OK;
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
