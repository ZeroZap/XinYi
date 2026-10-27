/**
 * @file xy_hal_lp_timer_stm32.c
 * @brief Low Power Timer HAL STM32 (F4/L4 shared wrapper) Implementation
 * @version 2.0
 * @date 2026-10-28
 *
 * The wrapper is shared by the STM32F4 and STM32L4 platform branches.  STM32F4
 * has no LPTIM peripheral, so that family resolves to explicit
 * XY_HAL_ERROR_NOT_SUPPORTED stubs instead of silently succeeding; STM32L4
 * drives LPTIM1 through the L4 HAL counter API with a bounded instance context
 * so the registered callback actually fires.
 */

#include "../inc/xy_hal_lp_timer.h"

#ifdef STM32_HAL_ENABLED

#include "stm32_hal.h"

/*
 * This wrapper is only built by the STM32F4/STM32L4 platform branches, which
 * must define STM32_HAL_ENABLED.  Keep the negative side explicit so a
 * mis-selected platform cannot pass a build while exposing no HAL symbols.
 */
#if !defined(STM32F4) && !defined(STM32F4xx) && !defined(STM32L4) && !defined(STM32L4xx) &&        \
    !defined(STM32L475xx)
#error "xy_hal_lp_timer.c built for an unsupported STM32 family"
#endif

#if defined(LPTIM1) && (defined(STM32L4) || defined(STM32L4xx) || defined(STM32L475xx))

/* LP Timer context structure */
typedef struct {
    LPTIM_HandleTypeDef* hlptim;
    xy_hal_lptimer_callback_t callback;
    void* arg;
    uint32_t period;
    uint8_t initialized;
} lptimer_ctx_t;

#define MAX_LPTIMER_INSTANCES 2
static lptimer_ctx_t g_lptimer_ctx[MAX_LPTIMER_INSTANCES];

static lptimer_ctx_t* find_lptimer_ctx(void* lptimer) {
    for (size_t i = 0; i < MAX_LPTIMER_INSTANCES; i++) {
        if (g_lptimer_ctx[i].hlptim == lptimer) {
            return &g_lptimer_ctx[i];
        }
    }
    return NULL;
}

static lptimer_ctx_t* alloc_lptimer_ctx(void) {
    for (size_t i = 0; i < MAX_LPTIMER_INSTANCES; i++) {
        if (g_lptimer_ctx[i].hlptim == NULL) {
            return &g_lptimer_ctx[i];
        }
    }
    return NULL;
}

static uint32_t xy_to_stm32_clk_src(xy_hal_lptimer_clk_src_t clk_src) {
    return (clk_src == XY_HAL_LPTIMER_CLK_EXTERNAL) ? LPTIM_CLOCKSOURCE_ULPTIM
                                                    : LPTIM_CLOCKSOURCE_APBCLOCK_LPOSC;
}

static uint32_t xy_to_stm32_prescaler(xy_hal_lptimer_prescaler_t prescaler) {
    switch (prescaler) {
        case XY_HAL_LPTIMER_PRESCALER_1:
            return LPTIM_PRESCALER_DIV1;
        case XY_HAL_LPTIMER_PRESCALER_2:
            return LPTIM_PRESCALER_DIV2;
        case XY_HAL_LPTIMER_PRESCALER_4:
            return LPTIM_PRESCALER_DIV4;
        case XY_HAL_LPTIMER_PRESCALER_8:
            return LPTIM_PRESCALER_DIV8;
        case XY_HAL_LPTIMER_PRESCALER_16:
            return LPTIM_PRESCALER_DIV16;
        case XY_HAL_LPTIMER_PRESCALER_32:
            return LPTIM_PRESCALER_DIV32;
        case XY_HAL_LPTIMER_PRESCALER_64:
            return LPTIM_PRESCALER_DIV64;
        case XY_HAL_LPTIMER_PRESCALER_128:
            return LPTIM_PRESCALER_DIV128;
        default:
            return LPTIM_PRESCALER_DIV1;
    }
}

xy_hal_error_t xy_hal_lptimer_init(void* lptimer, const xy_hal_lptimer_config_t* config) {
    if (!lptimer || !config || config->period == 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    lptimer_ctx_t* ctx = find_lptimer_ctx(lptimer);
    if (ctx != NULL && ctx->initialized) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }

    if (ctx == NULL) {
        ctx = alloc_lptimer_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
    }

    LPTIM_HandleTypeDef* hlptim = (LPTIM_HandleTypeDef*)lptimer;

    hlptim->Init.Clock.Source = xy_to_stm32_clk_src(config->clk_src);
    hlptim->Init.Clock.Prescaler = xy_to_stm32_prescaler(config->prescaler);
    hlptim->Init.Trigger.Source = LPTIM_TRIGSOURCE_SOFTWARE;
    hlptim->Init.OutputPolarity = LPTIM_OUTPUTPOLARITY_HIGH;
    hlptim->Init.UpdateMode = LPTIM_UPDATE_IMMEDIATE;
    hlptim->Init.CounterSource = LPTIM_COUNTERSOURCE_INTERNAL;

    if (HAL_LPTIM_Init(hlptim) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    ctx->hlptim = hlptim;
    ctx->callback = NULL;
    ctx->arg = NULL;
    ctx->period = config->period;
    ctx->initialized = 1;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_lptimer_deinit(void* lptimer) {
    if (!lptimer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    lptimer_ctx_t* ctx = find_lptimer_ctx(lptimer);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_LPTIM_DeInit((LPTIM_HandleTypeDef*)lptimer) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    ctx->initialized = 0;
    ctx->hlptim = NULL;
    ctx->callback = NULL;
    ctx->arg = NULL;
    ctx->period = 0;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_lptimer_start(void* lptimer) {
    if (!lptimer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    lptimer_ctx_t* ctx = find_lptimer_ctx(lptimer);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    /* Counter mode with interrupt so the auto-reload match callback fires once
     * per period, matching the U5 wrapper's contract. */
    if (HAL_LPTIM_Counter_Start_IT((LPTIM_HandleTypeDef*)lptimer, ctx->period) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_lptimer_stop(void* lptimer) {
    if (!lptimer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    lptimer_ctx_t* ctx = find_lptimer_ctx(lptimer);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_LPTIM_Counter_Stop_IT((LPTIM_HandleTypeDef*)lptimer) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

int xy_hal_lptimer_get_counter(void* lptimer) {
    if (!lptimer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    lptimer_ctx_t* ctx = find_lptimer_ctx(lptimer);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    return (int)HAL_LPTIM_ReadCounter((LPTIM_HandleTypeDef*)lptimer);
}

xy_hal_error_t xy_hal_lptimer_register_callback(void* lptimer, xy_hal_lptimer_callback_t callback,
                                                void* arg) {
    if (!lptimer || callback == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    lptimer_ctx_t* ctx = find_lptimer_ctx(lptimer);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    ctx->callback = callback;
    ctx->arg = arg;

    return XY_HAL_OK;
}

/* ==================== HAL Callbacks ==================== */

void HAL_LPTIM_AutoReloadMatchCallback(LPTIM_HandleTypeDef* hlptim) {
    lptimer_ctx_t* ctx = find_lptimer_ctx(hlptim);
    if (ctx != NULL && ctx->initialized && ctx->callback != NULL) {
        ctx->callback(hlptim, ctx->arg);
    }
}

void HAL_LPTIM_CompareMatchCallback(LPTIM_HandleTypeDef* hlptim) {
    lptimer_ctx_t* ctx = find_lptimer_ctx(hlptim);
    if (ctx != NULL && ctx->initialized && ctx->callback != NULL) {
        ctx->callback(hlptim, ctx->arg);
    }
}

#else /* no LPTIM peripheral on this family */

xy_hal_error_t xy_hal_lptimer_init(void* lptimer, const xy_hal_lptimer_config_t* config) {
    XY_UNUSED(lptimer);
    XY_UNUSED(config);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_lptimer_deinit(void* lptimer) {
    XY_UNUSED(lptimer);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_lptimer_start(void* lptimer) {
    XY_UNUSED(lptimer);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_lptimer_stop(void* lptimer) {
    XY_UNUSED(lptimer);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_lptimer_get_counter(void* lptimer) {
    XY_UNUSED(lptimer);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_lptimer_register_callback(void* lptimer, xy_hal_lptimer_callback_t callback,
                                                void* arg) {
    XY_UNUSED(lptimer);
    XY_UNUSED(callback);
    XY_UNUSED(arg);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

#endif /* LPTIM1 && STM32L4 */

#endif /* STM32_HAL_ENABLED */
