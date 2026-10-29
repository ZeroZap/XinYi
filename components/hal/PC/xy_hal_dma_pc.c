/**
 * @file xy_hal_dma_pc.c
 * @brief PC/Linux simulation layer for DMA HAL
 *
 * A host has no transfer engine, so this backend models the part of DMA that is a contract: a
 * channel that was never initialised answers NOT_INIT, an item count of zero or one above what the
 * hardware counter can hold is refused rather than truncated, a zero timeout waits rather than
 * failing, and the completion callback registered for an event actually fires once. Everything else
 * - addresses, bus arbitration, half-transfer timing - has no meaning here and is not pretended.
 *
 * The item count is items of the configured width, not bytes, and 0xFFFF is the counter width the
 * real backends are held to. Circular mode is honoured by resetting the counter when it completes.
 */

#include "xy_hal_dma.h"
#include <stdint.h>
#include <string.h>

#define PC_DMA_SLOTS 4U
#define PC_DMA_EVENT_COUNT 3U

typedef struct {
    void *handle;
    uint8_t used;
    uint8_t initialized;
    uint8_t running;
    xy_hal_dma_config_t config;
    uint32_t remaining;
    xy_hal_dma_callback_t callbacks[PC_DMA_EVENT_COUNT];
    void *args[PC_DMA_EVENT_COUNT];
} pc_dma_ctx_t;

static pc_dma_ctx_t g_dma_ctx[PC_DMA_SLOTS];

static pc_dma_ctx_t *pc_dma_find(const void *handle)
{
    unsigned int index;

    if (handle == NULL) {
        return NULL;
    }
    for (index = 0U; index < PC_DMA_SLOTS; index++) {
        if (g_dma_ctx[index].used != 0U && g_dma_ctx[index].handle == handle) {
            return &g_dma_ctx[index];
        }
    }
    return NULL;
}

static pc_dma_ctx_t *pc_dma_slot(void)
{
    unsigned int index;

    for (index = 0U; index < PC_DMA_SLOTS; index++) {
        if (g_dma_ctx[index].used == 0U) {
            g_dma_ctx[index].used = 1U;
            return &g_dma_ctx[index];
        }
    }
    return NULL;
}

static int pc_dma_config_valid(const xy_hal_dma_config_t *config)
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
    pc_dma_ctx_t *ctx;

    if (dma == NULL || config == NULL || !pc_dma_config_valid(config)) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_dma_find(dma);
    if (ctx != NULL && ctx->initialized != 0U) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }
    if (ctx == NULL) {
        ctx = pc_dma_slot();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
        ctx->handle = dma;
    }
    ctx->config = *config;
    ctx->remaining = 0U;
    ctx->running = 0U;
    ctx->initialized = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dma_deinit(void *dma)
{
    pc_dma_ctx_t *ctx;

    if (dma == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_dma_find(dma);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    memset(ctx, 0, sizeof(*ctx));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dma_start(void *dma, uint32_t src_addr, uint32_t dst_addr, size_t data_len)
{
    pc_dma_ctx_t *ctx = pc_dma_find(dma);

    (void)src_addr;
    (void)dst_addr;
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    /* The same two refusals the real backends make: no transfer is not a transfer, and a count
     * beyond the hardware counter would be truncated into one. */
    if (data_len == 0U || data_len > UINT16_MAX) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (ctx->running != 0U) {
        return XY_HAL_ERROR_BUSY;
    }
    ctx->remaining = (uint32_t)data_len;
    ctx->running = 1U;
    /* Nothing moves here, so the transfer is complete as soon as it is asked for - and the
     * completion callback fires exactly once, which is the part host code can observe. */
    ctx->remaining = 0U;
    ctx->running = 0U;
    if (ctx->config.mode == XY_HAL_DMA_MODE_CIRCULAR) {
        ctx->remaining = (uint32_t)data_len;
    }
    if (ctx->callbacks[XY_HAL_DMA_EVENT_COMPLETE] != NULL) {
        ctx->callbacks[XY_HAL_DMA_EVENT_COMPLETE](dma, XY_HAL_DMA_EVENT_COMPLETE,
                                                  ctx->args[XY_HAL_DMA_EVENT_COMPLETE]);
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dma_stop(void *dma)
{
    pc_dma_ctx_t *ctx;

    if (dma == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_dma_find(dma);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    ctx->running = 0U;
    ctx->remaining = 0U;
    return XY_HAL_OK;
}

int xy_hal_dma_get_counter(void *dma)
{
    pc_dma_ctx_t *ctx = pc_dma_find(dma);

    /* A channel that was never set up has no counter to report; a number here would be
     * indistinguishable from a real remaining count. */
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return (int)ctx->remaining;
}

xy_hal_error_t xy_hal_dma_poll_complete(void *dma, uint32_t timeout)
{
    pc_dma_ctx_t *ctx = pc_dma_find(dma);

    (void)timeout; /* milliseconds; the simulated bus never blocks */
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    /* Completion is instantaneous here, so there is never anything to wait for. */
    return ctx->remaining == 0U ? XY_HAL_OK : XY_HAL_ERROR_TIMEOUT;
}

xy_hal_error_t xy_hal_dma_register_callback(void *dma, xy_hal_dma_event_t event,
                                            xy_hal_dma_callback_t callback, void *arg)
{
    pc_dma_ctx_t *ctx;
    size_t index = (size_t)event;

    if (dma == NULL || index >= PC_DMA_EVENT_COUNT) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_dma_find(dma);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    /* Registering does not arm anything on a host either: there is no interrupt line, and the
     * completion callback fires from start() because that is when the transfer finishes. */
    ctx->callbacks[index] = callback;
    ctx->args[index] = arg;
    return XY_HAL_OK;
}
