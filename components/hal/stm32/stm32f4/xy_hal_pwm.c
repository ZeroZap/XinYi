/**
 * @file xy_hal_pwm_stm32.c
 * @brief PWM HAL STM32 (F4/L4 shared wrapper) Implementation
 * @version 2.0
 * @date 2026-10-28
 *
 * This wrapper is shared by the STM32F4 and STM32L4 platform branches.  It
 * follows the same public contract and init-state tracking as the STM32U5
 * wrapper so the three STM32 families behave identically through xy_hal_pwm.h.
 */

#include "../inc/xy_hal_pwm.h"

#ifdef STM32_HAL_ENABLED

#include "stm32_hal.h"

/* PWM context structure */
typedef struct {
    TIM_HandleTypeDef* htimer;
    uint32_t frequency;
    uint8_t initialized;
} pwm_ctx_t;

/* Maximum PWM instances */
#define MAX_PWM_INSTANCES 8
static pwm_ctx_t g_pwm_ctx[MAX_PWM_INSTANCES];

static pwm_ctx_t* find_pwm_ctx(void* timer) {
    for (size_t i = 0; i < MAX_PWM_INSTANCES; i++) {
        if (g_pwm_ctx[i].htimer == timer) {
            return &g_pwm_ctx[i];
        }
    }
    return NULL;
}

static pwm_ctx_t* alloc_pwm_ctx(void) {
    for (size_t i = 0; i < MAX_PWM_INSTANCES; i++) {
        if (g_pwm_ctx[i].htimer == NULL) {
            return &g_pwm_ctx[i];
        }
    }
    return NULL;
}

static uint32_t xy_to_stm32_channel(xy_hal_pwm_channel_t channel) {
    switch (channel) {
        case XY_HAL_PWM_CHANNEL_1:
            return TIM_CHANNEL_1;
        case XY_HAL_PWM_CHANNEL_2:
            return TIM_CHANNEL_2;
        case XY_HAL_PWM_CHANNEL_3:
            return TIM_CHANNEL_3;
        case XY_HAL_PWM_CHANNEL_4:
            return TIM_CHANNEL_4;
        default:
            return TIM_CHANNEL_1;
    }
}

static uint32_t xy_to_stm32_polarity(xy_hal_pwm_polarity_t polarity) {
    return (polarity == XY_HAL_PWM_POLARITY_HIGH) ? TIM_OCPOLARITY_HIGH : TIM_OCPOLARITY_LOW;
}

static uint32_t get_timer_clock(TIM_HandleTypeDef* htimer) {
    uint32_t clock = HAL_RCC_GetPCLK1Freq();

    /* Timers on APB2 run from PCLK2. */
#if defined(TIM1)
    if (htimer->Instance == TIM1) {
        clock = HAL_RCC_GetPCLK2Freq();
    }
#endif
#if defined(TIM8)
    if (htimer->Instance == TIM8) {
        clock = HAL_RCC_GetPCLK2Freq();
    }
#endif
#if defined(TIM15)
    if (htimer->Instance == TIM15) {
        clock = HAL_RCC_GetPCLK2Freq();
    }
#endif
#if defined(TIM16)
    if (htimer->Instance == TIM16) {
        clock = HAL_RCC_GetPCLK2Freq();
    }
#endif
#if defined(TIM17)
    if (htimer->Instance == TIM17) {
        clock = HAL_RCC_GetPCLK2Freq();
    }
#endif

    return clock;
}

xy_hal_error_t xy_hal_pwm_init(void* timer, xy_hal_pwm_channel_t channel,
                               const xy_hal_pwm_config_t* config) {
    if (!timer || !config || config->frequency == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    TIM_HandleTypeDef* htimer = (TIM_HandleTypeDef*)timer;

    pwm_ctx_t* ctx = find_pwm_ctx(timer);
    if (ctx != NULL && ctx->initialized) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }

    if (ctx == NULL) {
        ctx = alloc_pwm_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
    }

    uint32_t timer_clock = get_timer_clock(htimer);

    /* Target a 1 MHz timer clock so duty-cycle math stays integer-exact. */
    uint32_t prescaler = (timer_clock / 1000000U) - 1U;
    uint32_t period = (1000000U / config->frequency) - 1U;

    htimer->Init.Prescaler = prescaler;
    htimer->Init.CounterMode = TIM_COUNTERMODE_UP;
    htimer->Init.Period = period;
    htimer->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htimer->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    if (HAL_TIM_PWM_Init(htimer) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = xy_to_stm32_polarity(config->polarity);
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;

    uint32_t stm_channel = xy_to_stm32_channel(channel);
    if (HAL_TIM_PWM_ConfigChannel(htimer, &sConfigOC, stm_channel) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    ctx->htimer = htimer;
    ctx->frequency = config->frequency;
    ctx->initialized = 1;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_pwm_deinit(void* timer, xy_hal_pwm_channel_t channel) {
    XY_UNUSED(channel);
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    pwm_ctx_t* ctx = find_pwm_ctx(timer);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_TIM_PWM_DeInit((TIM_HandleTypeDef*)timer) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    ctx->initialized = 0;
    ctx->htimer = NULL;
    ctx->frequency = 0;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_pwm_start(void* timer, xy_hal_pwm_channel_t channel) {
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    pwm_ctx_t* ctx = find_pwm_ctx(timer);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_TIM_PWM_Start((TIM_HandleTypeDef*)timer, xy_to_stm32_channel(channel)) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_pwm_stop(void* timer, xy_hal_pwm_channel_t channel) {
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    pwm_ctx_t* ctx = find_pwm_ctx(timer);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_TIM_PWM_Stop((TIM_HandleTypeDef*)timer, xy_to_stm32_channel(channel)) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_pwm_set_duty_cycle(void* timer, xy_hal_pwm_channel_t channel,
                                         uint32_t duty_cycle) {
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (duty_cycle > 10000U) {
        duty_cycle = 10000U;
    }

    TIM_HandleTypeDef* htimer = (TIM_HandleTypeDef*)timer;
    uint32_t period = htimer->Init.Period;
    uint32_t compare = (duty_cycle * period) / 10000U;

    __HAL_TIM_SET_COMPARE(htimer, xy_to_stm32_channel(channel), compare);

    return XY_HAL_OK;
}

int32_t xy_hal_pwm_get_duty_cycle(void* timer, xy_hal_pwm_channel_t channel) {
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    TIM_HandleTypeDef* htimer = (TIM_HandleTypeDef*)timer;
    uint32_t period = htimer->Init.Period;
    if (period == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    uint32_t compare = __HAL_TIM_GET_COMPARE(htimer, xy_to_stm32_channel(channel));

    return (int32_t)((compare * 10000U) / period);
}

xy_hal_error_t xy_hal_pwm_set_frequency(void* timer, xy_hal_pwm_channel_t channel,
                                        uint32_t frequency) {
    XY_UNUSED(channel);
    if (!timer || frequency == 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    pwm_ctx_t* ctx = find_pwm_ctx(timer);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    TIM_HandleTypeDef* htimer = (TIM_HandleTypeDef*)timer;
    uint32_t timer_clock = get_timer_clock(htimer);
    uint32_t prescaler = htimer->Init.Prescaler;
    uint32_t period = (timer_clock / ((prescaler + 1U) * frequency)) - 1U;

    if (period > 65535U) {
        prescaler = timer_clock / (65536U * frequency);
        period = (timer_clock / ((prescaler + 1U) * frequency)) - 1U;
        htimer->Init.Prescaler = prescaler;
    }

    htimer->Init.Period = period;
    __HAL_TIM_SET_AUTORELOAD(htimer, period);

    ctx->frequency = frequency;

    return XY_HAL_OK;
}

int32_t xy_hal_pwm_get_frequency(void* timer, xy_hal_pwm_channel_t channel) {
    XY_UNUSED(channel);
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    pwm_ctx_t* ctx = find_pwm_ctx(timer);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    return (int32_t)ctx->frequency;
}

#else /* !STM32_HAL_ENABLED */

/*
 * This wrapper is only built by the STM32F4/STM32L4 platform branches, which
 * must define STM32_HAL_ENABLED.  Without it the translation unit would
 * silently compile to nothing and the platform gate would pass while exposing
 * no HAL symbols at all, so fail the build instead.
 */
#if defined(STM32F4) || defined(STM32F4xx) || defined(STM32L4) || defined(STM32L4xx) ||            \
    defined(STM32L475xx)
#error "STM32 platform selected without STM32_HAL_ENABLED: HAL wrapper would compile empty"
#endif

#endif /* STM32_HAL_ENABLED */
