/**
 * @file xy_hal_timer_stm32.c
 * @brief Timer HAL STM32 Implementation
 * @version 1.0
 * @date 2025-10-26
 */

#include "../inc/xy_hal_timer.h"

#ifdef STM32_HAL_ENABLED

#include "stm32_hal.h"

static uint32_t xy_to_stm32_count_mode(xy_hal_timer_count_mode_t mode)
{
    switch (mode) {
    case XY_HAL_TIMER_COUNT_UP:
        return TIM_COUNTERMODE_UP;
    case XY_HAL_TIMER_COUNT_DOWN:
        return TIM_COUNTERMODE_DOWN;
    case XY_HAL_TIMER_COUNT_CENTER1:
        return TIM_COUNTERMODE_CENTERALIGNED1;
    case XY_HAL_TIMER_COUNT_CENTER2:
        return TIM_COUNTERMODE_CENTERALIGNED2;
    case XY_HAL_TIMER_COUNT_CENTER3:
        return TIM_COUNTERMODE_CENTERALIGNED3;
    default:
        return TIM_COUNTERMODE_UP;
    }

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

static uint32_t xy_to_stm32_ckdiv(xy_hal_timer_ckdiv_t ckdiv)
{
    switch (ckdiv) {
    case XY_HAL_TIMER_CKDIV_1:
        return TIM_CLOCKDIVISION_DIV1;
    case XY_HAL_TIMER_CKDIV_2:
        return TIM_CLOCKDIVISION_DIV2;
    case XY_HAL_TIMER_CKDIV_4:
        return TIM_CLOCKDIVISION_DIV4;
    default:
        return TIM_CLOCKDIVISION_DIV1;
    }

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_timer_init(void *timer, const xy_hal_timer_config_t *config)
{
    if (!timer || !config) {
        return XY_HAL_ERROR_FAIL;
    }

    TIM_HandleTypeDef *htim = (TIM_HandleTypeDef *)timer;

    htim->Init.Prescaler         = config->prescaler;
    htim->Init.CounterMode       = xy_to_stm32_count_mode(config->mode);
    htim->Init.Period            = config->period;
    htim->Init.ClockDivision     = xy_to_stm32_ckdiv(config->clock_div);
    htim->Init.AutoReloadPreload = config->auto_reload_preload
                                       ? TIM_AUTORELOAD_PRELOAD_ENABLE
                                       : TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(htim) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_deinit(void *timer)
{
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (HAL_TIM_Base_DeInit((TIM_HandleTypeDef *)timer) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_start(void *timer)
{
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (HAL_TIM_Base_Start((TIM_HandleTypeDef *)timer) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_stop(void *timer)
{
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (HAL_TIM_Base_Stop((TIM_HandleTypeDef *)timer) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

int xy_hal_timer_get_counter(void *timer)
{
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    return (int)__HAL_TIM_GET_COUNTER((TIM_HandleTypeDef *)timer);
}

xy_hal_error_t xy_hal_timer_set_counter(void *timer, uint32_t value)
{
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    __HAL_TIM_SET_COUNTER((TIM_HandleTypeDef *)timer, value);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_set_period(void *timer, uint32_t period)
{
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    __HAL_TIM_SET_AUTORELOAD((TIM_HandleTypeDef *)timer, period);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_register_callback(void *timer, xy_hal_timer_event_t event,
                                   xy_hal_timer_callback_t callback, void *arg)
{
    /* Store callback in context */
    return XY_HAL_OK;
}

/* Map a timer instance to the NVIC line that carries its interrupt.
 *
 * Only the general-purpose timers whose vector names are identical across the families this
 * wrapper serves are listed. TIM1 and TIM8 share their update vector with other peripherals and
 * the names differ per family (TIM1_UP_TIM16_IRQn on L4, TIM1_UP_TIM10_IRQn on F4, TIM1_UP_IRQn on
 * U5), so they are not claimed here: an instance this function does not recognise is refused
 * rather than armed into a line that would never fire. The IRQn constants are enum members, not
 * macros, so they cannot be guarded with #ifdef - the instance macro is what makes this portable.
 */
static int timer_irqn(const TIM_HandleTypeDef *handle, IRQn_Type *irqn)
{
#ifdef TIM2
    if (handle->Instance == (void *)TIM2) {
        *irqn = TIM2_IRQn;
        return 1;
    }
#endif
#ifdef TIM3
    if (handle->Instance == (void *)TIM3) {
        *irqn = TIM3_IRQn;
        return 1;
    }
#endif
#ifdef TIM4
    if (handle->Instance == (void *)TIM4) {
        *irqn = TIM4_IRQn;
        return 1;
    }
#endif
#ifdef TIM5
    if (handle->Instance == (void *)TIM5) {
        *irqn = TIM5_IRQn;
        return 1;
    }
#endif
#ifdef TIM6
    if (handle->Instance == (void *)TIM6) {
        /* F4 shares the TIM6 update vector with the DAC, so the name differs there. */
#if defined(STM32F4)
        *irqn = TIM6_DAC_IRQn;
#else
        *irqn = TIM6_IRQn;
#endif
        return 1;
    }
#endif
#ifdef TIM7
    if (handle->Instance == (void *)TIM7) {
        *irqn = TIM7_IRQn;
        return 1;
    }
#endif
    return 0;
}

/* Arming an event owns the whole first level: enabling the peripheral interrupt while leaving the
 * NVIC line alone is the shape that reports XY_HAL_OK and never fires. */
static int arm_nvic(const TIM_HandleTypeDef *handle)
{
    IRQn_Type irqn;

    if (!timer_irqn(handle, &irqn)) {
        return 0;
    }
    HAL_NVIC_SetPriority(irqn, 5U, 0U);
    HAL_NVIC_EnableIRQ(irqn);
    return 1;
}

xy_hal_error_t xy_hal_timer_enable_irq(void *timer, xy_hal_timer_event_t event)
{
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    /* The event selects which interrupt is armed; this used to be ignored in favour of starting
     * the counter with the update interrupt, so asking for a compare match got neither. */
    switch (event) {
    case XY_HAL_TIMER_EVENT_UPDATE:
        break;
    case XY_HAL_TIMER_EVENT_CC1:
    case XY_HAL_TIMER_EVENT_CC2:
    case XY_HAL_TIMER_EVENT_CC3:
    case XY_HAL_TIMER_EVENT_CC4:
        /* Compare channels are armed by the capture/compare HAL, which this wrapper does not
         * own yet; refusing beats arming the update interrupt and calling it done. */
        return XY_HAL_ERROR_NOT_SUPPORTED;
    default:
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!arm_nvic((TIM_HandleTypeDef *)timer)) {
        return XY_HAL_ERROR_NOT_SUPPORTED;
    }
    __HAL_TIM_ENABLE_IT((TIM_HandleTypeDef *)timer, TIM_IT_UPDATE);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_disable_irq(void *timer, xy_hal_timer_event_t event)
{
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    switch (event) {
    case XY_HAL_TIMER_EVENT_UPDATE:
        break;
    case XY_HAL_TIMER_EVENT_CC1:
    case XY_HAL_TIMER_EVENT_CC2:
    case XY_HAL_TIMER_EVENT_CC3:
    case XY_HAL_TIMER_EVENT_CC4:
        return XY_HAL_ERROR_NOT_SUPPORTED;
    default:
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    /* Mask the source only. Stopping the timer here would silently end timekeeping for a caller
     * that asked to disarm one interrupt. */
    __HAL_TIM_DISABLE_IT((TIM_HandleTypeDef *)timer, TIM_IT_UPDATE);
    return XY_HAL_OK;
}

void xy_hal_timer_irq_handler(void *timer)
{
    if (timer != NULL) {
        HAL_TIM_IRQHandler((TIM_HandleTypeDef *)timer);
    }
}

#endif /* STM32_HAL_ENABLED */
