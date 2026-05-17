/**
 * @file xy_hal_wdg.c
 * @brief Watchdog HAL STM32F4 Implementation
 */

#include "../../inc/xy_hal_wdg.h"

#ifdef STM32_HAL_ENABLED

#include "stm32_hal.h"
#include <string.h>

typedef struct {
    IWDG_HandleTypeDef *hiwdg;
    xy_hal_wdg_callback_t callback;
    void *arg;
    uint8_t initialized;
} iwdg_ctx_t;

static iwdg_ctx_t g_iwdg_ctx = { 0 };

xy_hal_error_t xy_hal_iwdg_init(void *wdg, const xy_hal_iwdg_config_t *config)
{
    if (!wdg || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (g_iwdg_ctx.initialized) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }

    IWDG_HandleTypeDef *hiwdg = (IWDG_HandleTypeDef *)wdg;
    hiwdg->Init.Prescaler = config->prescaler;
    hiwdg->Init.Reload    = config->reload;

    if (HAL_IWDG_Init(hiwdg) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    g_iwdg_ctx.hiwdg      = hiwdg;
    g_iwdg_ctx.initialized = 1;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_iwdg_start(void *wdg)
{
    if (!wdg) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_iwdg_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_IWDG_Init(g_iwdg_ctx.hiwdg) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_iwdg_feed(void *wdg)
{
    if (!wdg) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_iwdg_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_IWDG_Refresh(g_iwdg_ctx.hiwdg) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

int xy_hal_iwdg_get_remaining_time(void *wdg)
{
    if (!wdg) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_iwdg_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    IWDG_HandleTypeDef *hiwdg = (IWDG_HandleTypeDef *)wdg;
    uint32_t counter    = hiwdg->Instance->RLR;
    uint32_t prescaler  = (1U << ((hiwdg->Instance->PR & IWDG_PR_PR) >> 3));
    uint32_t lsi_freq   = 32000U;
    return (int)((counter * prescaler * 1000U) / lsi_freq);
}

xy_hal_error_t xy_hal_iwdg_set_timeout(void *wdg, uint32_t timeout_ms)
{
    if (!wdg || timeout_ms == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_iwdg_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    uint32_t lsi_freq     = 32000U;
    uint32_t counter      = (timeout_ms * lsi_freq) / 1000U;
    uint32_t prescaler_idx = 0;
    while (counter > 0xFFF && prescaler_idx < 7) {
        counter >>= 1;
        prescaler_idx++;
    }

    if (counter > 0xFFF || counter < 1) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    IWDG_HandleTypeDef *hiwdg = (IWDG_HandleTypeDef *)wdg;
    hiwdg->Init.Prescaler = prescaler_idx << 3;
    hiwdg->Init.Reload    = counter & 0xFFF;

    g_iwdg_ctx.initialized = 0;
    xy_hal_iwdg_config_t cfg = { .prescaler  = hiwdg->Init.Prescaler,
                                  .reload     = hiwdg->Init.Reload,
                                  .timeout_ms = timeout_ms };
    return xy_hal_iwdg_init(wdg, &cfg);
}

xy_hal_error_t xy_hal_wwdg_init(void *wdg, const xy_hal_wwdg_config_t *config)
{
    XY_UNUSED(wdg);
    XY_UNUSED(config);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_wwdg_start(void *wdg, uint8_t enable_early_wakeup)
{
    XY_UNUSED(wdg);
    XY_UNUSED(enable_early_wakeup);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_wwdg_feed(void *wdg, uint32_t counter)
{
    XY_UNUSED(wdg);
    XY_UNUSED(counter);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_wwdg_get_remaining_time(void *wdg)
{
    XY_UNUSED(wdg);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_wwdg_set_window(void *wdg, uint32_t window)
{
    XY_UNUSED(wdg);
    XY_UNUSED(window);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_wwdg_register_ewi_callback(void *wdg,
                                                  xy_hal_wdg_callback_t callback,
                                                  void *arg)
{
    XY_UNUSED(wdg);
    XY_UNUSED(callback);
    XY_UNUSED(arg);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_wdg_enable_irq(void *wdg)
{
    XY_UNUSED(wdg);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_wdg_disable_irq(void *wdg)
{
    XY_UNUSED(wdg);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

void xy_hal_wdg_system_reset(void)
{
    HAL_NVIC_SystemReset();
}

#endif /* STM32_HAL_ENABLED */
