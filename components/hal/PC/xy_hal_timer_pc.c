/**
 * @file xy_hal_timer_pc.c
 * @brief PC/Linux simulation layer for timer HAL
 *
 * A host has no counter to run, so what is modelled here is the part that is a contract: the
 * counter value a caller set is the value it reads back, arming an interrupt does not start or stop
 * anything, and each event has its own callback and its own armed flag - arming CC1 must not deliver
 * the update event. Delivery happens in xy_hal_timer_pc_dispatch(), which
 * xy_hal_timer_irq_handler() calls, because on a host the "interrupt" is whatever software tick the
 * test asks for.
 *
 * There is no NVIC. The rule that enable_irq must reach the CPU exists so a backend cannot enable a
 * peripheral interrupt that can never be delivered; the host equivalent is that enable_irq arms a
 * flag the dispatch path actually reads, and a test can observe the callback arriving.
 */

#include "xy_hal_timer.h"
#include <stdint.h>
#include <string.h>

#define PC_TIMER_SLOTS 4U
#define PC_TIMER_EVENT_COUNT 5U

typedef struct {
    void *handle;
    uint8_t used;
    uint8_t initialized;
    uint8_t running;
    xy_hal_timer_config_t config;
    uint32_t counter;
    uint32_t period;
    uint8_t armed[PC_TIMER_EVENT_COUNT];
    xy_hal_timer_callback_t callbacks[PC_TIMER_EVENT_COUNT];
    void *args[PC_TIMER_EVENT_COUNT];
} pc_timer_ctx_t;

static pc_timer_ctx_t g_timer_ctx[PC_TIMER_SLOTS];

static pc_timer_ctx_t *pc_timer_find(const void *handle)
{
    unsigned int index;

    if (handle == NULL) {
        return NULL;
    }
    for (index = 0U; index < PC_TIMER_SLOTS; index++) {
        if (g_timer_ctx[index].used != 0U && g_timer_ctx[index].handle == handle) {
            return &g_timer_ctx[index];
        }
    }
    return NULL;
}

static pc_timer_ctx_t *pc_timer_slot(void)
{
    unsigned int index;

    for (index = 0U; index < PC_TIMER_SLOTS; index++) {
        if (g_timer_ctx[index].used == 0U) {
            g_timer_ctx[index].used = 1U;
            return &g_timer_ctx[index];
        }
    }
    return NULL;
}

static int pc_timer_config_valid(const xy_hal_timer_config_t *config)
{
    if (config->prescaler > 0xFFFFU) return 0;
    if (config->mode > XY_HAL_TIMER_COUNT_CENTER3) return 0;
    if (config->clock_div > XY_HAL_TIMER_CKDIV_2) return 0;
    if (config->auto_reload_preload > 1U) return 0;
    return 1;
}

xy_hal_error_t xy_hal_timer_init(void *timer, const xy_hal_timer_config_t *config)
{
    pc_timer_ctx_t *ctx;

    if (timer == NULL || config == NULL || !pc_timer_config_valid(config)) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_timer_find(timer);
    if (ctx != NULL && ctx->initialized != 0U) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }
    if (ctx == NULL) {
        ctx = pc_timer_slot();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
        ctx->handle = timer;
    }
    ctx->config = *config;
    ctx->period = config->period;
    ctx->counter = 0U;
    ctx->running = 0U;
    ctx->initialized = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_deinit(void *timer)
{
    pc_timer_ctx_t *ctx;

    if (timer == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_timer_find(timer);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    memset(ctx, 0, sizeof(*ctx));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_start(void *timer)
{
    pc_timer_ctx_t *ctx = pc_timer_find(timer);

    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    ctx->running = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_stop(void *timer)
{
    pc_timer_ctx_t *ctx = pc_timer_find(timer);

    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    ctx->running = 0U;
    return XY_HAL_OK;
}

int xy_hal_timer_get_counter(void *timer)
{
    pc_timer_ctx_t *ctx = pc_timer_find(timer);

    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return (int)ctx->counter;
}

xy_hal_error_t xy_hal_timer_set_counter(void *timer, uint32_t value)
{
    pc_timer_ctx_t *ctx = pc_timer_find(timer);

    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    ctx->counter = value;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_set_period(void *timer, uint32_t period)
{
    pc_timer_ctx_t *ctx = pc_timer_find(timer);

    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (period == 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx->period = period;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_register_callback(void *timer, xy_hal_timer_event_t event,
                                              xy_hal_timer_callback_t callback, void *arg)
{
    pc_timer_ctx_t *ctx;
    size_t index = (size_t)event;

    if (timer == NULL || index >= PC_TIMER_EVENT_COUNT) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_timer_find(timer);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    ctx->callbacks[index] = callback;
    ctx->args[index] = arg;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_enable_irq(void *timer, xy_hal_timer_event_t event)
{
    pc_timer_ctx_t *ctx = pc_timer_find(timer);
    size_t index = (size_t)event;

    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (index >= PC_TIMER_EVENT_COUNT) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    /* Arm this event and nothing else: not the counter, not the other events. The dispatch path
     * below reads this flag, which is the only sense in which a host has an interrupt line. */
    ctx->armed[index] = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_disable_irq(void *timer, xy_hal_timer_event_t event)
{
    pc_timer_ctx_t *ctx = pc_timer_find(timer);
    size_t index = (size_t)event;

    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (index >= PC_TIMER_EVENT_COUNT) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx->armed[index] = 0U;
    return XY_HAL_OK;
}

/**
 * @brief Deliver one timer interrupt on the host.
 *
 * Called by xy_hal_timer_irq_handler(). Every armed event with a registered callback is delivered,
 * and nothing else is - the same rule the vendor backends follow by enabling only the requested
 * interrupt source, and the reason a caller that armed CC1 does not receive an update event.
 */
void xy_hal_timer_pc_irq(void *timer)
{
    pc_timer_ctx_t *ctx = pc_timer_find(timer);
    unsigned int index;

    if (ctx == NULL || ctx->initialized == 0U) {
        return;
    }
    for (index = 0U; index < PC_TIMER_EVENT_COUNT; index++) {
        if (ctx->armed[index] != 0U && ctx->callbacks[index] != NULL) {
            ctx->callbacks[index](timer, (xy_hal_timer_event_t)index, ctx->args[index]);
        }
    }
}

uint8_t xy_hal_timer_pc_is_armed(void *timer, xy_hal_timer_event_t event)
{
    pc_timer_ctx_t *ctx = pc_timer_find(timer);
    size_t index = (size_t)event;

    if (ctx == NULL || ctx->initialized == 0U || index >= PC_TIMER_EVENT_COUNT) {
        return 0U;
    }
    return ctx->armed[index];
}
