/**
 * @file xy_hal_timer.c
 * @brief WCH CH32V30x timer HAL implementation
 */

#include "xy_hal_timer.h"

#ifdef MCU_CH32

#include "ch32v30x.h"
#include <string.h>

#define WCH_TIMER_COUNT 4U
#define WCH_TIMER_EVENT_COUNT 5U

typedef struct {
    TIM_TypeDef* timer;
    xy_hal_timer_config_t config;
    xy_hal_timer_callback_t callbacks[WCH_TIMER_EVENT_COUNT];
    void* args[WCH_TIMER_EVENT_COUNT];
    uint8_t initialized;
    uint8_t running;
} wch_timer_context_t;

static wch_timer_context_t contexts[WCH_TIMER_COUNT];

static int timer_index(const void* timer) {
    if (timer == TIM1)
        return 0;
    if (timer == TIM2)
        return 1;
    if (timer == TIM3)
        return 2;
    if (timer == TIM4)
        return 3;
    return -1;
}

static void enable_clock(int index) {
    if (index == 0) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
    } else {
        static const uint32_t clocks[] = {RCC_APB1Periph_TIM2, RCC_APB1Periph_TIM3,
                                          RCC_APB1Periph_TIM4};
        RCC_APB1PeriphClockCmd(clocks[index - 1], ENABLE);
    }
}

static uint16_t counter_mode(xy_hal_timer_count_mode_t mode) {
    static const uint16_t modes[] = {TIM_CounterMode_Up, TIM_CounterMode_Down,
                                     TIM_CounterMode_CenterAligned1, TIM_CounterMode_CenterAligned2,
                                     TIM_CounterMode_CenterAligned3};
    return modes[mode];
}

static uint16_t clock_division(xy_hal_timer_ckdiv_t division) {
    static const uint16_t divisions[] = {TIM_CKD_DIV1, TIM_CKD_DIV2, TIM_CKD_DIV4};
    return divisions[division];
}

static uint16_t interrupt_source(xy_hal_timer_event_t event) {
    static const uint16_t sources[] = {TIM_IT_Update, TIM_IT_CC1, TIM_IT_CC2, TIM_IT_CC3,
                                       TIM_IT_CC4};
    return sources[event];
}

xy_hal_error_t xy_hal_timer_init(void* timer, const xy_hal_timer_config_t* config) {
    TIM_TimeBaseInitTypeDef init = {0};
    int index = timer_index(timer);

    if (index < 0 || config == NULL || config->prescaler > 0xFFFFU || config->period > 0xFFFFU ||
        config->mode > XY_HAL_TIMER_COUNT_CENTER3 || config->clock_div > XY_HAL_TIMER_CKDIV_4 ||
        config->auto_reload_preload > 1U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (contexts[index].initialized != 0U)
        return XY_HAL_ERROR_ALREADY_INIT;

    enable_clock(index);
    TIM_DeInit(timer);
    init.TIM_Prescaler = (uint16_t)config->prescaler;
    init.TIM_Period = (uint16_t)config->period;
    init.TIM_CounterMode = counter_mode(config->mode);
    init.TIM_ClockDivision = clock_division(config->clock_div);
    init.TIM_RepetitionCounter = 0U;
    TIM_TimeBaseInit(timer, &init);
    TIM_ARRPreloadConfig(timer, config->auto_reload_preload ? ENABLE : DISABLE);

    memset(&contexts[index], 0, sizeof(contexts[index]));
    contexts[index].timer = timer;
    contexts[index].config = *config;
    contexts[index].initialized = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_deinit(void* timer) {
    int index = timer_index(timer);
    if (index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (contexts[index].initialized == 0U)
        return XY_HAL_ERROR_NOT_INIT;
    TIM_Cmd(timer, DISABLE);
    TIM_DeInit(timer);
    memset(&contexts[index], 0, sizeof(contexts[index]));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_start(void* timer) {
    int index = timer_index(timer);
    if (index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (contexts[index].initialized == 0U)
        return XY_HAL_ERROR_NOT_INIT;
    TIM_Cmd(timer, ENABLE);
    contexts[index].running = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_stop(void* timer) {
    int index = timer_index(timer);
    if (index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (contexts[index].initialized == 0U)
        return XY_HAL_ERROR_NOT_INIT;
    TIM_Cmd(timer, DISABLE);
    contexts[index].running = 0U;
    return XY_HAL_OK;
}

int xy_hal_timer_get_counter(void* timer) {
    int index = timer_index(timer);
    if (index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (contexts[index].initialized == 0U)
        return XY_HAL_ERROR_NOT_INIT;
    return (int)TIM_GetCounter(timer);
}

xy_hal_error_t xy_hal_timer_set_counter(void* timer, uint32_t value) {
    int index = timer_index(timer);
    if (index < 0 || value > 0xFFFFU)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (contexts[index].initialized == 0U)
        return XY_HAL_ERROR_NOT_INIT;
    TIM_SetCounter(timer, (uint16_t)value);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_set_period(void* timer, uint32_t period) {
    int index = timer_index(timer);
    if (index < 0 || period > 0xFFFFU)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (contexts[index].initialized == 0U)
        return XY_HAL_ERROR_NOT_INIT;
    TIM_SetAutoreload(timer, (uint16_t)period);
    contexts[index].config.period = period;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_register_callback(void* timer, xy_hal_timer_event_t event,
                                              xy_hal_timer_callback_t callback, void* arg) {
    int index = timer_index(timer);
    if (index < 0 || event > XY_HAL_TIMER_EVENT_CC4)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (contexts[index].initialized == 0U)
        return XY_HAL_ERROR_NOT_INIT;
    contexts[index].callbacks[event] = callback;
    contexts[index].args[event] = arg;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_enable_irq(void* timer, xy_hal_timer_event_t event) {
    int index = timer_index(timer);
    if (index < 0 || event > XY_HAL_TIMER_EVENT_CC4)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (contexts[index].initialized == 0U)
        return XY_HAL_ERROR_NOT_INIT;
    TIM_ITConfig(timer, interrupt_source(event), ENABLE);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_disable_irq(void* timer, xy_hal_timer_event_t event) {
    int index = timer_index(timer);
    if (index < 0 || event > XY_HAL_TIMER_EVENT_CC4)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (contexts[index].initialized == 0U)
        return XY_HAL_ERROR_NOT_INIT;
    TIM_ITConfig(timer, interrupt_source(event), DISABLE);
    return XY_HAL_OK;
}

void xy_hal_timer_irq_handler(void* timer) {
    int index = timer_index(timer);
    size_t event;
    if (index < 0 || contexts[index].initialized == 0U)
        return;
    for (event = 0U; event < WCH_TIMER_EVENT_COUNT; ++event) {
        uint16_t source = interrupt_source((xy_hal_timer_event_t)event);
        if (TIM_GetITStatus(timer, source) != RESET) {
            TIM_ClearITPendingBit(timer, source);
            if (contexts[index].callbacks[event] != NULL) {
                contexts[index].callbacks[event](timer, (xy_hal_timer_event_t)event,
                                                 contexts[index].args[event]);
            }
        }
    }
}

#else
#error "WCH timer backend requires MCU_CH32"
#endif
