/**
 * @file xy_hal_dma.c
 * @brief DMA HAL — STM32U5 GPDMA implementation.
 *
 * GPDMA replaces the F4 stream model: there are no fixed peripheral/memory
 * ports any more, just source and destination with independent inc/width.
 * Transfer direction is dictated by the addresses passed to HAL_DMA_Start
 * plus the Request line; the Init.Direction field really controls whether
 * a hardware request line drives the channel.
 *
 * Notes:
 *  - CIRCULAR mode in xy_hal_dma_config_t is silently downgraded to NORMAL:
 *    GPDMA only supports continuous transfer through linked-list mode, which
 *    needs a different API surface (HAL_DMAEx_List_*) not covered here.
 *  - The HAL Init.Request field defaults to DMA_REQUEST_SW (memory-to-memory).
 *    For peripheral-driven transfers, set hdma->Init.Request before
 *    xy_hal_dma_init to the relevant GPDMA1_REQUEST_* value (e.g.
 *    GPDMA1_REQUEST_USART1_TX) — non-zero values are respected.
 */

#include "../../inc/xy_hal_dma.h"

#if defined(STM32U5) || defined(STM32U5xx)

#include "stm32u5xx_hal.h"
#include <string.h>

typedef struct {
    DMA_HandleTypeDef     *hdma;
    xy_hal_dma_callback_t  callbacks[3];
    void                  *args[3];
    uint8_t                initialized;
} dma_ctx_t;

#define MAX_DMA_INSTANCES 16
static dma_ctx_t g_dma_ctx[MAX_DMA_INSTANCES];

static dma_ctx_t *find_dma_ctx(const void *dma)
{
    for (size_t i = 0; i < MAX_DMA_INSTANCES; i++) {
        if (g_dma_ctx[i].hdma == dma) {
            return &g_dma_ctx[i];
        }
    }
    return NULL;
}

static dma_ctx_t *alloc_dma_ctx(void)
{
    for (size_t i = 0; i < MAX_DMA_INSTANCES; i++) {
        if (g_dma_ctx[i].hdma == NULL) {
            return &g_dma_ctx[i];
        }
    }
    return NULL;
}

static uint32_t to_src_inc(xy_hal_dma_incr_t incr)
{
    return (incr == XY_HAL_DMA_INCR_ENABLE)
               ? DMA_SINC_INCREMENTED : DMA_SINC_FIXED;
}

static uint32_t to_dst_inc(xy_hal_dma_incr_t incr)
{
    return (incr == XY_HAL_DMA_INCR_ENABLE)
               ? DMA_DINC_INCREMENTED : DMA_DINC_FIXED;
}

static uint32_t to_src_width(xy_hal_dma_width_t w)
{
    switch (w) {
    case XY_HAL_DMA_WIDTH_HALFWORD: return DMA_SRC_DATAWIDTH_HALFWORD;
    case XY_HAL_DMA_WIDTH_WORD:     return DMA_SRC_DATAWIDTH_WORD;
    case XY_HAL_DMA_WIDTH_BYTE:
    default:                        return DMA_SRC_DATAWIDTH_BYTE;
    }
}

static uint32_t to_dst_width(xy_hal_dma_width_t w)
{
    switch (w) {
    case XY_HAL_DMA_WIDTH_HALFWORD: return DMA_DEST_DATAWIDTH_HALFWORD;
    case XY_HAL_DMA_WIDTH_WORD:     return DMA_DEST_DATAWIDTH_WORD;
    case XY_HAL_DMA_WIDTH_BYTE:
    default:                        return DMA_DEST_DATAWIDTH_BYTE;
    }
}

static uint32_t to_priority(xy_hal_dma_priority_t p)
{
    switch (p) {
    case XY_HAL_DMA_PRIORITY_MEDIUM:    return DMA_LOW_PRIORITY_MID_WEIGHT;
    case XY_HAL_DMA_PRIORITY_HIGH:      return DMA_LOW_PRIORITY_HIGH_WEIGHT;
    case XY_HAL_DMA_PRIORITY_VERY_HIGH: return DMA_HIGH_PRIORITY;
    case XY_HAL_DMA_PRIORITY_LOW:
    default:                            return DMA_LOW_PRIORITY_LOW_WEIGHT;
    }
}

/* HAL_DMA_RegisterCallback wrapper used as a generic shim — looks up the
 * user callback by event index from the ctx. */
static void dma_dispatch(DMA_HandleTypeDef *hdma, size_t idx,
                         xy_hal_dma_event_t evt)
{
    dma_ctx_t *ctx = find_dma_ctx(hdma);
    if (ctx && ctx->callbacks[idx]) {
        ctx->callbacks[idx](hdma, evt, ctx->args[idx]);
    }
}

static void dma_cb_complete(DMA_HandleTypeDef *hdma)
{   dma_dispatch(hdma, 0, XY_HAL_DMA_EVENT_COMPLETE); }
static void dma_cb_half(DMA_HandleTypeDef *hdma)
{   dma_dispatch(hdma, 1, XY_HAL_DMA_EVENT_HALF_COMPLETE); }
static void dma_cb_error(DMA_HandleTypeDef *hdma)
{   dma_dispatch(hdma, 2, XY_HAL_DMA_EVENT_ERROR); }

xy_hal_error_t xy_hal_dma_init(void *dma, const xy_hal_dma_config_t *config)
{
    if (!dma || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    DMA_HandleTypeDef *hdma = (DMA_HandleTypeDef *)dma;
    dma_ctx_t *ctx = find_dma_ctx(dma);
    if (ctx != NULL && ctx->initialized) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }
    if (ctx == NULL) {
        ctx = alloc_dma_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
    }

    /* GPDMA direction: PERIPH_TO_MEMORY for any peripheral-driven channel,
     * MEMORY_TO_MEMORY only for pure mem-to-mem SW-triggered transfers. */
    if (config->direction == XY_HAL_DMA_DIR_MEM_TO_MEM) {
        hdma->Init.Direction = DMA_MEMORY_TO_MEMORY;
        if (hdma->Init.Request == 0U) {
            hdma->Init.Request = DMA_REQUEST_SW;
        }
    } else {
        hdma->Init.Direction = DMA_PERIPH_TO_MEMORY;
        /* hdma->Init.Request left untouched — caller sets peripheral request. */
    }

    /* SrcInc/DestInc + widths follow the actual data flow direction. */
    switch (config->direction) {
    case XY_HAL_DMA_DIR_PERIPH_TO_MEM:
        hdma->Init.SrcInc       = to_src_inc(config->periph_incr);
        hdma->Init.DestInc      = to_dst_inc(config->mem_incr);
        hdma->Init.SrcDataWidth = to_src_width(config->periph_width);
        hdma->Init.DestDataWidth= to_dst_width(config->mem_width);
        break;
    case XY_HAL_DMA_DIR_MEM_TO_PERIPH:
        hdma->Init.SrcInc       = to_src_inc(config->mem_incr);
        hdma->Init.DestInc      = to_dst_inc(config->periph_incr);
        hdma->Init.SrcDataWidth = to_src_width(config->mem_width);
        hdma->Init.DestDataWidth= to_dst_width(config->periph_width);
        break;
    case XY_HAL_DMA_DIR_MEM_TO_MEM:
    default:
        hdma->Init.SrcInc       = to_src_inc(config->mem_incr);
        hdma->Init.DestInc      = to_dst_inc(config->mem_incr);
        hdma->Init.SrcDataWidth = to_src_width(config->mem_width);
        hdma->Init.DestDataWidth= to_dst_width(config->mem_width);
        break;
    }

    hdma->Init.Priority             = to_priority(config->priority);
    hdma->Init.Mode                 = DMA_NORMAL;  /* CIRCULAR → linked-list, not handled */
    hdma->Init.BlkHWRequest         = DMA_BREQ_SINGLE_BURST;
    hdma->Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 |
                                        DMA_DEST_ALLOCATED_PORT0;
    hdma->Init.TransferEventMode    = DMA_TCEM_BLOCK_TRANSFER;

    if (HAL_DMA_Init(hdma) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    /* Wire shims so HAL_DMA_IRQHandler dispatches to per-channel callbacks. */
    HAL_DMA_RegisterCallback(hdma, HAL_DMA_XFER_CPLT_CB_ID,     dma_cb_complete);
    HAL_DMA_RegisterCallback(hdma, HAL_DMA_XFER_HALFCPLT_CB_ID, dma_cb_half);
    HAL_DMA_RegisterCallback(hdma, HAL_DMA_XFER_ERROR_CB_ID,    dma_cb_error);

    ctx->hdma        = hdma;
    memset(ctx->callbacks, 0, sizeof(ctx->callbacks));
    memset(ctx->args, 0, sizeof(ctx->args));
    ctx->initialized = 1;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dma_deinit(void *dma)
{
    if (!dma) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    dma_ctx_t *ctx = find_dma_ctx(dma);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (HAL_DMA_DeInit((DMA_HandleTypeDef *)dma) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
    memset(ctx, 0, sizeof(*ctx));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dma_start(void *dma, uint32_t src_addr, uint32_t dst_addr,
                                size_t data_len)
{
    if (!dma || data_len == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    dma_ctx_t *ctx = find_dma_ctx(dma);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    HAL_StatusTypeDef st = HAL_DMA_Start((DMA_HandleTypeDef *)dma,
                                         src_addr, dst_addr,
                                         (uint32_t)data_len);
    return (st == HAL_OK) ? XY_HAL_OK : XY_HAL_ERROR_BUSY;
}

xy_hal_error_t xy_hal_dma_stop(void *dma)
{
    if (!dma) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    dma_ctx_t *ctx = find_dma_ctx(dma);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return (HAL_DMA_Abort((DMA_HandleTypeDef *)dma) == HAL_OK)
               ? XY_HAL_OK : XY_HAL_ERROR_FAIL;
}

xy_hal_error_t xy_hal_dma_register_callback(void *dma, xy_hal_dma_event_t event,
                                            xy_hal_dma_callback_t callback,
                                            void *arg)
{
    if (!dma) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    size_t idx = (size_t)event;
    if (idx >= 3) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    dma_ctx_t *ctx = find_dma_ctx(dma);
    if (ctx == NULL) {
        ctx = alloc_dma_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
        ctx->hdma = (DMA_HandleTypeDef *)dma;
    }
    ctx->callbacks[idx] = callback;
    ctx->args[idx]      = arg;
    return XY_HAL_OK;
}

int xy_hal_dma_get_counter(void *dma)
{
    if (!dma) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    return (int)__HAL_DMA_GET_COUNTER((DMA_HandleTypeDef *)dma);
}

xy_hal_error_t xy_hal_dma_poll_complete(void *dma, uint32_t timeout)
{
    if (!dma) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    dma_ctx_t *ctx = find_dma_ctx(dma);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    HAL_StatusTypeDef st = HAL_DMA_PollForTransfer((DMA_HandleTypeDef *)dma,
                                                   HAL_DMA_FULL_TRANSFER,
                                                   timeout);
    if (st == HAL_OK) {
        return XY_HAL_OK;
    }
    return (st == HAL_TIMEOUT) ? XY_HAL_ERROR_TIMEOUT : XY_HAL_ERROR_FAIL;
}

#endif /* STM32U5 || STM32U5xx */
