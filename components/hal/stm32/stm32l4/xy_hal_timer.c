/**
 * @file xy_hal_timer.c
 * @brief Timer HAL STM32L4 implementation
 */

#include "../../inc/xy_hal_timer.h"

#if defined(STM32L4) || defined(STM32L4xx)

#include "stm32l4xx_hal.h"
#include <string.h>

#define MAX_TIMER_INSTANCES 4U
#define MAX_TIMER_CALLBACKS 5U

typedef struct {
    TIM_HandleTypeDef *htim;
    xy_hal_timer_callback_t callbacks[MAX_TIMER_CALLBACKS];
    void *args[MAX_TIMER_CALLBACKS];
    uint8_t initialized;
} timer_ctx_t;

static timer_ctx_t timer_contexts[MAX_TIMER_INSTANCES];

static timer_ctx_t *find_context(const void *timer)
{
    for (size_t index = 0U; index < MAX_TIMER_INSTANCES; ++index) {
        if (timer_contexts[index].htim == timer) {
            return &timer_contexts[index];
        }
    }
    return NULL;
}

static timer_ctx_t *allocate_context(void)
{
    for (size_t index = 0U; index < MAX_TIMER_INSTANCES; ++index) {
        if (timer_contexts[index].htim == NULL) {
            return &timer_contexts[index];
        }
    }
    return NULL;
}

static uint32_t to_count_mode(xy_hal_timer_count_mode_t mode)
{
    switch (mode) {
    case XY_HAL_TIMER_COUNT_DOWN:
        return TIM_COUNTERMODE_DOWN;
    case XY_HAL_TIMER_COUNT_CENTER1:
        return TIM_COUNTERMODE_CENTERALIGNED1;
    case XY_HAL_TIMER_COUNT_CENTER2:
        return TIM_COUNTERMODE_CENTERALIGNED2;
    case XY_HAL_TIMER_COUNT_CENTER3:
        return TIM_COUNTERMODE_CENTERALIGNED3;
    case XY_HAL_TIMER_COUNT_UP:
    default:
        return TIM_COUNTERMODE_UP;
    }
}

static uint32_t to_clock_division(xy_hal_timer_ckdiv_t division)
{
    switch (division) {
    case XY_HAL_TIMER_CKDIV_2:
        return TIM_CLOCKDIVISION_DIV2;
    case XY_HAL_TIMER_CKDIV_4:
        return TIM_CLOCKDIVISION_DIV4;
    case XY_HAL_TIMER_CKDIV_1:
    default:
        return TIM_CLOCKDIVISION_DIV1;
    }
}

static int event_valid(xy_hal_timer_event_t event)
{
    return event <= XY_HAL_TIMER_EVENT_CC4;
}

xy_hal_error_t xy_hal_timer_init(void *timer, const xy_hal_timer_config_t *config)
{
    TIM_HandleTypeDef *htim;
    timer_ctx_t *context;

    if (timer == NULL || config == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    context = find_context(timer);
    if (context != NULL && context->initialized != 0U) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }
    if (context == NULL) {
        context = allocate_context();
        if (context == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
    }

    htim = (TIM_HandleTypeDef *)timer;
    htim->Init.Prescaler = config->prescaler;
    htim->Init.CounterMode = to_count_mode(config->mode);
    htim->Init.Period = config->period;
    htim->Init.ClockDivision = to_clock_division(config->clock_div);
    htim->Init.AutoReloadPreload = config->auto_reload_preload != 0U
                                        ? TIM_AUTORELOAD_PRELOAD_ENABLE
                                        : TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(htim) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    memset(context, 0, sizeof(*context));
    context->htim = htim;
    context->initialized = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_deinit(void *timer)
{
    timer_ctx_t *context;

    if (timer == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    context = find_context(timer);
    if (context == NULL || context->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (HAL_TIM_Base_DeInit((TIM_HandleTypeDef *)timer) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
    memset(context, 0, sizeof(*context));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_start(void *timer)
{
    timer_ctx_t *context;

    if (timer == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    context = find_context(timer);
    if (context == NULL || context->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return HAL_TIM_Base_Start((TIM_HandleTypeDef *)timer) == HAL_OK ? XY_HAL_OK
                                                                    : XY_HAL_ERROR_FAIL;
}

xy_hal_error_t xy_hal_timer_stop(void *timer)
{
    timer_ctx_t *context;

    if (timer == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    context = find_context(timer);
    if (context == NULL || context->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return HAL_TIM_Base_Stop((TIM_HandleTypeDef *)timer) == HAL_OK ? XY_HAL_OK
                                                                   : XY_HAL_ERROR_FAIL;
}

int xy_hal_timer_get_counter(void *timer)
{
    return timer == NULL ? XY_HAL_ERROR_INVALID_PARAM
                         : (int)__HAL_TIM_GET_COUNTER((TIM_HandleTypeDef *)timer);
}

xy_hal_error_t xy_hal_timer_set_counter(void *timer, uint32_t value)
{
    if (timer == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    __HAL_TIM_SET_COUNTER((TIM_HandleTypeDef *)timer, value);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_set_period(void *timer, uint32_t period)
{
    if (timer == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    __HAL_TIM_SET_AUTORELOAD((TIM_HandleTypeDef *)timer, period);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_register_callback(void *timer, xy_hal_timer_event_t event,
                                              xy_hal_timer_callback_t callback, void *arg)
{
    timer_ctx_t *context;

    if (timer == NULL || callback == NULL || !event_valid(event)) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    context = find_context(timer);
    if (context == NULL || context->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    context->callbacks[event] = callback;
    context->args[event] = arg;
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
    timer_ctx_t *context;

    if (timer == NULL || !event_valid(event)) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    context = find_context(timer);
    if (context == NULL || context->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (event != XY_HAL_TIMER_EVENT_UPDATE) {
        return XY_HAL_ERROR_NOT_SUPPORTED;
    }
    /* Arming an interrupt is not starting the timer: the previous implementation called
     * HAL_TIM_Base_Start_IT here, so a caller that only wanted the interrupt also started the
     * counter - and stopping the interrupt stopped timekeeping. */
    if (!arm_nvic((TIM_HandleTypeDef *)timer)) {
        return XY_HAL_ERROR_NOT_SUPPORTED;
    }
    __HAL_TIM_ENABLE_IT((TIM_HandleTypeDef *)timer, TIM_IT_UPDATE);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_disable_irq(void *timer, xy_hal_timer_event_t event)
{
    timer_ctx_t *context;

    if (timer == NULL || !event_valid(event)) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    context = find_context(timer);
    if (context == NULL || context->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (event != XY_HAL_TIMER_EVENT_UPDATE) {
        return XY_HAL_ERROR_NOT_SUPPORTED;
    }
    /* Mask the source; the counter keeps running and the NVIC line stays enabled, so a later
     * enable_irq() needs no second arming and does not restart anything. */
    __HAL_TIM_DISABLE_IT((TIM_HandleTypeDef *)timer, TIM_IT_UPDATE);
    return XY_HAL_OK;
}

void xy_hal_timer_irq_handler(void *timer)
{
    if (timer != NULL) {
        HAL_TIM_IRQHandler((TIM_HandleTypeDef *)timer);
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    timer_ctx_t *context = find_context(htim);

    if (context != NULL && context->callbacks[XY_HAL_TIMER_EVENT_UPDATE] != NULL) {
        context->callbacks[XY_HAL_TIMER_EVENT_UPDATE](
            htim, XY_HAL_TIMER_EVENT_UPDATE, context->args[XY_HAL_TIMER_EVENT_UPDATE]);
    }
}

#endif
