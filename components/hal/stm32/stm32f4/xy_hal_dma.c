/**
 * @file xy_hal_dma.c
 * @brief DMA HAL STM32F4 implementation
 *
 * Rewritten against the current public `xy_hal_dma.h` contract.  The previous
 * version was deferred from the STM32F4 build because it returned `int` from
 * entry points declared `xy_hal_error_t` and, worse, published `XY_HAL_DMA_*`
 * enum values where the vendor HAL expects its own `DMA_*` constants: the
 * direction/mode/priority/width helpers returned the OSAL enum unchanged, the
 * width helper returned undeclared `XY_HAL_DMA_PDATAALIGN_*` names, one
 * alignment helper served both the peripheral and memory sides, the peripheral
 * increment helper was reused for memory (`DMA_PINC_*` where `DMA_MINC_*` is
 * required), and `register_callback` reported success without registering
 * anything.
 */

#include "../../inc/xy_hal_dma.h"

#ifdef STM32_HAL_ENABLED

#include "stm32_hal.h"
#include <string.h>

#define DMA_EVENT_COUNT 3U
#define MAX_DMA_INSTANCES 16U

typedef struct {
    DMA_HandleTypeDef *hdma;
    xy_hal_dma_callback_t callbacks[DMA_EVENT_COUNT];
    void *args[DMA_EVENT_COUNT];
    uint8_t initialized;
} dma_ctx_t;

static dma_ctx_t dma_contexts[MAX_DMA_INSTANCES];

static dma_ctx_t *find_context(const void *dma)
{
    uint32_t index;

    if (dma == NULL) return NULL;

    for (index = 0U; index < MAX_DMA_INSTANCES; ++index) {
        if (dma_contexts[index].initialized != 0U && dma_contexts[index].hdma == dma) {
            return &dma_contexts[index];
        }
    }
    return NULL;
}

static dma_ctx_t *allocate_context(void)
{
    uint32_t index;

    for (index = 0U; index < MAX_DMA_INSTANCES; ++index) {
        if (dma_contexts[index].initialized == 0U) {
            return &dma_contexts[index];
        }
    }
    return NULL;
}

static void dispatch_callback(DMA_HandleTypeDef *hdma, uint32_t index,
                              xy_hal_dma_event_t event)
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

/* The XY enums are adapter-side numbering; every value below must be a vendor
 * DMA_* constant.  Returning the XY enum here silently programmed the wrong
 * transfer direction and priority on a real device. */
static uint32_t to_direction(xy_hal_dma_direction_t direction)
{
    switch (direction) {
    case XY_HAL_DMA_DIR_PERIPH_TO_MEM:
        return DMA_PERIPH_TO_MEMORY;
    case XY_HAL_DMA_DIR_MEM_TO_PERIPH:
        return DMA_MEMORY_TO_PERIPH;
    case XY_HAL_DMA_DIR_MEM_TO_MEM:
    default:
        return DMA_MEMORY_TO_MEMORY;
    }
}

static uint32_t to_mode(xy_hal_dma_mode_t mode)
{
    return (mode == XY_HAL_DMA_MODE_CIRCULAR) ? DMA_CIRCULAR : DMA_NORMAL;
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

/* The peripheral and memory alignments are distinct register fields
 * (PSIZE/MSIZE); one shared helper cannot configure both. */
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

/* DMA_PINC_* and DMA_MINC_* are different register bits; using the peripheral
 * constant for the memory side left memory increment permanently disabled. */
static uint32_t to_periph_incr(xy_hal_dma_incr_t incr)
{
    return (incr == XY_HAL_DMA_INCR_ENABLE) ? DMA_PINC_ENABLE : DMA_PINC_DISABLE;
}

static uint32_t to_mem_incr(xy_hal_dma_incr_t incr)
{
    return (incr == XY_HAL_DMA_INCR_ENABLE) ? DMA_MINC_ENABLE : DMA_MINC_DISABLE;
}

/* An out-of-range enum cannot be masked into a legal register field: reject it
 * before the first vendor access instead of silently defaulting. */
static int dma_config_valid(const xy_hal_dma_config_t *config)
{
    if (config->direction > XY_HAL_DMA_DIR_MEM_TO_MEM) return 0;
    if (config->mode > XY_HAL_DMA_MODE_CIRCULAR) return 0;
    if (config->priority > XY_HAL_DMA_PRIORITY_VERY_HIGH) return 0;
    if (config->periph_width > XY_HAL_DMA_WIDTH_WORD) return 0;
    if (config->mem_width > XY_HAL_DMA_WIDTH_WORD) return 0;
    if (config->periph_incr > XY_HAL_DMA_INCR_ENABLE) return 0;
    if (config->mem_incr > XY_HAL_DMA_INCR_ENABLE) return 0;
    return 1;
}

xy_hal_error_t xy_hal_dma_init(void *dma, const xy_hal_dma_config_t *config)
{
    DMA_HandleTypeDef *hdma;
    dma_ctx_t *context;

    if (dma == NULL || config == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (!dma_config_valid(config)) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    hdma = (DMA_HandleTypeDef *)dma;
    context = find_context(dma);
    if (context != NULL) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }
    context = allocate_context();
    if (context == NULL) {
        return XY_HAL_ERROR_NO_RESOURCE;
    }

    /* Init.Channel is owned by the caller (F4 streams are not self-routing). */
    hdma->Init.Direction = to_direction(config->direction);
    hdma->Init.PeriphInc = to_periph_incr(config->periph_incr);
    hdma->Init.MemInc = to_mem_incr(config->mem_incr);
    hdma->Init.PeriphDataAlignment = to_periph_alignment(config->periph_width);
    hdma->Init.MemDataAlignment = to_mem_alignment(config->mem_width);
    hdma->Init.Mode = to_mode(config->mode);
    hdma->Init.Priority = to_priority(config->priority);

    if (HAL_DMA_Init(hdma) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    memset(context, 0, sizeof(*context));
    context->hdma = hdma;
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
    if (context == NULL) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (HAL_DMA_DeInit((DMA_HandleTypeDef *)dma) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
    memset(context, 0, sizeof(*context));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dma_start(void *dma, uint32_t src_addr, uint32_t dst_addr,
                                size_t data_len)
{
    DMA_HandleTypeDef *hdma;
    dma_ctx_t *context;
    HAL_StatusTypeDef status;
    uint8_t interrupt_driven;

    /* DMA_SxNDTR holds 16 bits: a larger item count is truncated by the hardware and the
     * transfer stops early with no error anywhere. The WCH backend has always refused it. */
    if (dma == NULL || data_len == 0U || data_len > 0xFFFFU) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    context = find_context(dma);
    if (context == NULL) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    hdma = (DMA_HandleTypeDef *)dma;

    interrupt_driven = (context->callbacks[0] != NULL ||
                        context->callbacks[1] != NULL ||
                        context->callbacks[2] != NULL)
                           ? 1U
                           : 0U;
    status = interrupt_driven != 0U
                 ? HAL_DMA_Start_IT(hdma, src_addr, dst_addr, (uint32_t)data_len)
                 : HAL_DMA_Start(hdma, src_addr, dst_addr, (uint32_t)data_len);

    return (status == HAL_OK) ? XY_HAL_OK : XY_HAL_ERROR_BUSY;
}

xy_hal_error_t xy_hal_dma_stop(void *dma)
{
    if (dma == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (find_context(dma) == NULL) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return (HAL_DMA_Abort((DMA_HandleTypeDef *)dma) == HAL_OK) ? XY_HAL_OK
                                                              : XY_HAL_ERROR_FAIL;
}

xy_hal_error_t xy_hal_dma_register_callback(void *dma, xy_hal_dma_event_t event,
                                            xy_hal_dma_callback_t callback,
                                            void *arg)
{
    static const HAL_DMA_CallbackIDTypeDef callback_ids[DMA_EVENT_COUNT] = {
        HAL_DMA_XFER_CPLT_CB_ID,
        HAL_DMA_XFER_HALFCPLT_CB_ID,
        HAL_DMA_XFER_ERROR_CB_ID,
    };
    static void (*const hal_callbacks[DMA_EVENT_COUNT])(DMA_HandleTypeDef *) = {
        transfer_complete_callback,
        transfer_half_callback,
        transfer_error_callback,
    };
    dma_ctx_t *context;
    uint32_t index;

    if (dma == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    index = (uint32_t)event;
    if (index >= DMA_EVENT_COUNT) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    context = find_context(dma);
    if (context == NULL) {
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
    if (find_context(dma) == NULL) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return (int)__HAL_DMA_GET_COUNTER((DMA_HandleTypeDef *)dma);
}

xy_hal_error_t xy_hal_dma_poll_complete(void *dma, uint32_t timeout)
{
    HAL_StatusTypeDef status;

    if (dma == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (find_context(dma) == NULL) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    status = HAL_DMA_PollForTransfer((DMA_HandleTypeDef *)dma, HAL_DMA_FULL_TRANSFER,
                                     timeout);
    if (status == HAL_OK) {
        return XY_HAL_OK;
    }
    return (status == HAL_TIMEOUT) ? XY_HAL_ERROR_TIMEOUT : XY_HAL_ERROR_FAIL;
}

void xy_hal_dma_irq_handler(void *dma)
{
    if (dma != NULL) {
        HAL_DMA_IRQHandler((DMA_HandleTypeDef *)dma);
    }
}

#endif /* STM32_HAL_ENABLED */
