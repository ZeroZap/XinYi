/**
 * @file xy_hal_pwm.c
 * @brief WCH CH32V30x PWM HAL implementation
 */
#include "xy_hal_pwm.h"
#ifdef MCU_CH32
#include "ch32v30x.h"
#include <string.h>
#define PWM_TIMERS 4U
#define PWM_CHANNELS 4U
typedef struct {
    xy_hal_pwm_config_t config;
    uint32_t prescaler;
    uint32_t autoreload;
    uint32_t compare;
    xy_hal_pwm_callback_t callback;
    void* arg;
    uint8_t initialized;
    uint8_t running;
    uint8_t output;
    uint8_t fault;
} pwm_channel_ctx_t;
static pwm_channel_ctx_t contexts[PWM_TIMERS][PWM_CHANNELS];
static int timer_index(const void* pwm) {
    if (pwm == TIM1)
        return 0;
    if (pwm == TIM2)
        return 1;
    if (pwm == TIM3)
        return 2;
    if (pwm == TIM4)
        return 3;
    return -1;
}
static int valid(void* pwm, xy_hal_pwm_channel_t ch) {
    return timer_index(pwm) >= 0 && ch < XY_HAL_PWM_CHANNEL_MAX;
}
static void clock_enable(int i) {
    if (i == 0)
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
    else {
        static const uint32_t c[] = {RCC_APB1Periph_TIM2, RCC_APB1Periph_TIM3, RCC_APB1Periph_TIM4};
        RCC_APB1PeriphClockCmd(c[i - 1], ENABLE);
    }
}
static uint16_t hw_channel(xy_hal_pwm_channel_t ch) {
    static const uint16_t c[] = {TIM_Channel_1, TIM_Channel_2, TIM_Channel_3, TIM_Channel_4};
    return c[ch];
}
static void set_compare_hw(TIM_TypeDef* tim, xy_hal_pwm_channel_t ch, uint16_t value) {
    if (ch == 0)
        TIM_SetCompare1(tim, value);
    else if (ch == 1)
        TIM_SetCompare2(tim, value);
    else if (ch == 2)
        TIM_SetCompare3(tim, value);
    else
        TIM_SetCompare4(tim, value);
}
static void configure_oc(TIM_TypeDef* tim, xy_hal_pwm_channel_t ch, const pwm_channel_ctx_t* ctx) {
    TIM_OCInitTypeDef oc = {0};
    oc.TIM_OCMode = TIM_OCMode_PWM1;
    oc.TIM_OutputState = TIM_OutputState_Enable;
    oc.TIM_OutputNState = TIM_OutputNState_Disable;
    oc.TIM_Pulse = (uint16_t)ctx->compare;
    oc.TIM_OCPolarity =
        ctx->config.polarity == XY_HAL_PWM_POLARITY_HIGH ? TIM_OCPolarity_High : TIM_OCPolarity_Low;
    oc.TIM_OCNPolarity = TIM_OCNPolarity_High;
    oc.TIM_OCIdleState = TIM_OCIdleState_Reset;
    oc.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
    if (ch == 0) {
        TIM_OC1Init(tim, &oc);
        TIM_OC1PreloadConfig(tim, TIM_OCPreload_Enable);
    } else if (ch == 1) {
        TIM_OC2Init(tim, &oc);
        TIM_OC2PreloadConfig(tim, TIM_OCPreload_Enable);
    } else if (ch == 2) {
        TIM_OC3Init(tim, &oc);
        TIM_OC3PreloadConfig(tim, TIM_OCPreload_Enable);
    } else {
        TIM_OC4Init(tim, &oc);
        TIM_OC4PreloadConfig(tim, TIM_OCPreload_Enable);
    }
}
static xy_hal_error_t apply_timebase(void* pwm, pwm_channel_ctx_t* ctx, uint32_t frequency) {
    TIM_TimeBaseInitTypeDef tb = {0};
    uint32_t ticks;
    if (frequency == 0U)
        return XY_HAL_ERROR_INVALID_PARAM;
    ticks = SystemCoreClock / frequency;
    if (ticks == 0U)
        return XY_HAL_ERROR_INVALID_PARAM;
    ctx->prescaler = (ticks - 1U) / 65536U;
    ctx->autoreload = ticks / (ctx->prescaler + 1U) - 1U;
    if (ctx->autoreload > 65535U)
        return XY_HAL_ERROR_INVALID_PARAM;
    tb.TIM_Period = (uint16_t)ctx->autoreload;
    tb.TIM_Prescaler = (uint16_t)ctx->prescaler;
    tb.TIM_ClockDivision = TIM_CKD_DIV1;
    tb.TIM_CounterMode = ctx->config.mode == XY_HAL_PWM_MODE_CENTER_ALIGN
                             ? TIM_CounterMode_CenterAligned1
                             : TIM_CounterMode_Up;
    tb.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(pwm, &tb);
    ctx->config.frequency = frequency;
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_pwm_init(void* pwm, xy_hal_pwm_channel_t ch, const xy_hal_pwm_config_t* cfg) {
    int i = timer_index(pwm);
    pwm_channel_ctx_t* c;
    if (!valid(pwm, ch) || !cfg || cfg->frequency == 0 || cfg->duty_cycle > 10000 ||
        cfg->polarity > XY_HAL_PWM_POLARITY_LOW || cfg->mode > XY_HAL_PWM_MODE_CENTER_ALIGN ||
        cfg->wave_shape != XY_HAL_PWM_WAVE_SQUARE || cfg->complementary_enable)
        return XY_HAL_ERROR_INVALID_PARAM;
    c = &contexts[i][ch];
    if (c->initialized)
        return XY_HAL_ERROR_ALREADY_INIT;
    memset(c, 0, sizeof(*c));
    c->config = *cfg;
    clock_enable(i);
    if (apply_timebase(pwm, c, cfg->frequency) != XY_HAL_OK)
        return XY_HAL_ERROR_INVALID_PARAM;
    c->compare = ((c->autoreload + 1U) * cfg->duty_cycle) / 10000U;
    configure_oc(pwm, ch, c);
    TIM_ARRPreloadConfig(pwm, ENABLE);
    c->initialized = 1;
    c->output = 1;
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_pwm_deinit(void* pwm, xy_hal_pwm_channel_t ch) {
    int i = timer_index(pwm);
    if (!valid(pwm, ch))
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[i][ch].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    TIM_CCxCmd(pwm, hw_channel(ch), TIM_CCx_Disable);
    memset(&contexts[i][ch], 0, sizeof(contexts[i][ch]));
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_pwm_start(void* pwm, xy_hal_pwm_channel_t ch) {
    int i = timer_index(pwm);
    if (!valid(pwm, ch))
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[i][ch].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    TIM_CCxCmd(pwm, hw_channel(ch), TIM_CCx_Enable);
    TIM_Cmd(pwm, ENABLE);
    if (pwm == TIM1)
        TIM_CtrlPWMOutputs(pwm, ENABLE);
    contexts[i][ch].running = 1;
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_pwm_stop(void* pwm, xy_hal_pwm_channel_t ch) {
    int i = timer_index(pwm);
    if (!valid(pwm, ch))
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[i][ch].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    TIM_CCxCmd(pwm, hw_channel(ch), TIM_CCx_Disable);
    contexts[i][ch].running = 0;
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_pwm_set_frequency(void* pwm, xy_hal_pwm_channel_t ch, uint32_t f) {
    int i = timer_index(pwm);
    if (!valid(pwm, ch))
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[i][ch].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    if (apply_timebase(pwm, &contexts[i][ch], f) != XY_HAL_OK)
        return XY_HAL_ERROR_INVALID_PARAM;
    return xy_hal_pwm_set_duty_cycle(pwm, ch, contexts[i][ch].config.duty_cycle);
}
int32_t xy_hal_pwm_get_frequency(void* pwm, xy_hal_pwm_channel_t ch) {
    int i = timer_index(pwm);
    if (!valid(pwm, ch))
        return XY_HAL_ERROR_INVALID_PARAM;
    return contexts[i][ch].initialized ? (int32_t)contexts[i][ch].config.frequency
                                       : XY_HAL_ERROR_NOT_INIT;
}
xy_hal_error_t xy_hal_pwm_set_duty_cycle(void* pwm, xy_hal_pwm_channel_t ch, uint32_t duty) {
    int i = timer_index(pwm);
    pwm_channel_ctx_t* c;
    if (!valid(pwm, ch) || duty > 10000U)
        return XY_HAL_ERROR_INVALID_PARAM;
    c = &contexts[i][ch];
    if (!c->initialized)
        return XY_HAL_ERROR_NOT_INIT;
    c->config.duty_cycle = duty;
    c->compare = ((c->autoreload + 1U) * duty) / 10000U;
    set_compare_hw(pwm, ch, (uint16_t)c->compare);
    return XY_HAL_OK;
}
int32_t xy_hal_pwm_get_duty_cycle(void* pwm, xy_hal_pwm_channel_t ch) {
    int i = timer_index(pwm);
    if (!valid(pwm, ch))
        return XY_HAL_ERROR_INVALID_PARAM;
    return contexts[i][ch].initialized ? (int32_t)contexts[i][ch].config.duty_cycle
                                       : XY_HAL_ERROR_NOT_INIT;
}
xy_hal_error_t xy_hal_pwm_set_polarity(void* pwm, xy_hal_pwm_channel_t ch,
                                       xy_hal_pwm_polarity_t p) {
    int i = timer_index(pwm);
    if (!valid(pwm, ch) || p > XY_HAL_PWM_POLARITY_LOW)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[i][ch].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    contexts[i][ch].config.polarity = p;
    configure_oc(pwm, ch, &contexts[i][ch]);
    return XY_HAL_OK;
}
int32_t xy_hal_pwm_get_polarity(void* pwm, xy_hal_pwm_channel_t ch) {
    int i = timer_index(pwm);
    if (!valid(pwm, ch))
        return XY_HAL_ERROR_INVALID_PARAM;
    return contexts[i][ch].initialized ? (int32_t)contexts[i][ch].config.polarity
                                       : XY_HAL_ERROR_NOT_INIT;
}
xy_hal_error_t xy_hal_pwm_set_mode(void* pwm, xy_hal_pwm_channel_t ch, xy_hal_pwm_mode_t m) {
    int i = timer_index(pwm);
    if (!valid(pwm, ch) || m > XY_HAL_PWM_MODE_CENTER_ALIGN)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[i][ch].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    contexts[i][ch].config.mode = m;
    return apply_timebase(pwm, &contexts[i][ch], contexts[i][ch].config.frequency);
}
int32_t xy_hal_pwm_get_mode(void* pwm, xy_hal_pwm_channel_t ch) {
    int i = timer_index(pwm);
    if (!valid(pwm, ch))
        return XY_HAL_ERROR_INVALID_PARAM;
    return contexts[i][ch].initialized ? (int32_t)contexts[i][ch].config.mode
                                       : XY_HAL_ERROR_NOT_INIT;
}
xy_hal_error_t xy_hal_pwm_set_compare(void* pwm, xy_hal_pwm_channel_t ch, uint32_t v) {
    int i = timer_index(pwm);
    if (!valid(pwm, ch))
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[i][ch].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    if (v > contexts[i][ch].autoreload + 1U)
        return XY_HAL_ERROR_INVALID_PARAM;
    contexts[i][ch].compare = v;
    set_compare_hw(pwm, ch, (uint16_t)v);
    return XY_HAL_OK;
}
int32_t xy_hal_pwm_get_compare(void* pwm, xy_hal_pwm_channel_t ch) {
    int i = timer_index(pwm);
    if (!valid(pwm, ch))
        return XY_HAL_ERROR_INVALID_PARAM;
    return contexts[i][ch].initialized ? (int32_t)contexts[i][ch].compare : XY_HAL_ERROR_NOT_INIT;
}
xy_hal_error_t xy_hal_pwm_set_autoreload(void* pwm, xy_hal_pwm_channel_t ch, uint32_t v) {
    int i = timer_index(pwm);
    if (!valid(pwm, ch) || v > 65535U)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[i][ch].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    contexts[i][ch].autoreload = v;
    TIM_SetAutoreload(pwm, (uint16_t)v);
    return XY_HAL_OK;
}
int32_t xy_hal_pwm_get_autoreload(void* pwm, xy_hal_pwm_channel_t ch) {
    int i = timer_index(pwm);
    if (!valid(pwm, ch))
        return XY_HAL_ERROR_INVALID_PARAM;
    return contexts[i][ch].initialized ? (int32_t)contexts[i][ch].autoreload
                                       : XY_HAL_ERROR_NOT_INIT;
}
xy_hal_error_t xy_hal_pwm_set_prescaler(void* pwm, xy_hal_pwm_channel_t ch, uint32_t v) {
    int i = timer_index(pwm);
    if (!valid(pwm, ch) || v > 65535U)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[i][ch].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    contexts[i][ch].prescaler = v;
    TIM_PrescalerConfig(pwm, (uint16_t)v, TIM_PSCReloadMode_Immediate);
    return XY_HAL_OK;
}
int32_t xy_hal_pwm_get_prescaler(void* pwm, xy_hal_pwm_channel_t ch) {
    int i = timer_index(pwm);
    if (!valid(pwm, ch))
        return XY_HAL_ERROR_INVALID_PARAM;
    return contexts[i][ch].initialized ? (int32_t)contexts[i][ch].prescaler : XY_HAL_ERROR_NOT_INIT;
}
xy_hal_error_t xy_hal_pwm_set_output_state(void* pwm, xy_hal_pwm_channel_t ch, uint8_t s) {
    int i = timer_index(pwm);
    if (!valid(pwm, ch) || s > 1U)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[i][ch].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    TIM_CCxCmd(pwm, hw_channel(ch), s ? TIM_CCx_Enable : TIM_CCx_Disable);
    contexts[i][ch].output = s;
    return XY_HAL_OK;
}
int32_t xy_hal_pwm_get_output_state(void* pwm, xy_hal_pwm_channel_t ch) {
    int i = timer_index(pwm);
    if (!valid(pwm, ch))
        return XY_HAL_ERROR_INVALID_PARAM;
    return contexts[i][ch].initialized ? contexts[i][ch].output : XY_HAL_ERROR_NOT_INIT;
}
xy_hal_error_t xy_hal_pwm_register_callback(void* pwm, xy_hal_pwm_channel_t ch,
                                            xy_hal_pwm_callback_t cb, void* arg) {
    int i = timer_index(pwm);
    if (!valid(pwm, ch))
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[i][ch].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    contexts[i][ch].callback = cb;
    contexts[i][ch].arg = arg;
    return XY_HAL_OK;
}
uint32_t xy_hal_pwm_duty_to_compare(void* pwm, xy_hal_pwm_channel_t ch, uint32_t duty) {
    int i = timer_index(pwm);
    return valid(pwm, ch) && contexts[i][ch].initialized && duty <= 10000U
               ? ((contexts[i][ch].autoreload + 1U) * duty) / 10000U
               : 0U;
}
uint32_t xy_hal_pwm_compare_to_duty(void* pwm, xy_hal_pwm_channel_t ch, uint32_t cmp) {
    int i = timer_index(pwm);
    return valid(pwm, ch) && contexts[i][ch].initialized
               ? (cmp * 10000U) / (contexts[i][ch].autoreload + 1U)
               : 0U;
}
uint32_t xy_hal_pwm_get_period_us(void* pwm, xy_hal_pwm_channel_t ch) {
    int32_t f = xy_hal_pwm_get_frequency(pwm, ch);
    return f > 0 ? 1000000U / (uint32_t)f : 0U;
}
uint64_t xy_hal_pwm_get_period_ns(void* pwm, xy_hal_pwm_channel_t ch) {
    int32_t f = xy_hal_pwm_get_frequency(pwm, ch);
    return f > 0 ? 1000000000ULL / (uint32_t)f : 0ULL;
}
xy_hal_error_t xy_hal_pwm_set_wave_shape(void* pwm, xy_hal_pwm_channel_t ch,
                                         xy_hal_pwm_wave_shape_t s) {
    int i = timer_index(pwm);
    if (!valid(pwm, ch))
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[i][ch].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    if (s != XY_HAL_PWM_WAVE_SQUARE)
        return XY_HAL_ERROR_NOT_SUPPORTED;
    contexts[i][ch].config.wave_shape = s;
    return XY_HAL_OK;
}
int32_t xy_hal_pwm_get_wave_shape(void* pwm, xy_hal_pwm_channel_t ch) {
    int i = timer_index(pwm);
    if (!valid(pwm, ch))
        return XY_HAL_ERROR_INVALID_PARAM;
    return contexts[i][ch].initialized ? (int32_t)contexts[i][ch].config.wave_shape
                                       : XY_HAL_ERROR_NOT_INIT;
}
int32_t xy_hal_pwm_get_state(void* pwm, xy_hal_pwm_channel_t ch) {
    int i = timer_index(pwm);
    if (!valid(pwm, ch))
        return XY_HAL_ERROR_INVALID_PARAM;
    return contexts[i][ch].initialized ? contexts[i][ch].running : XY_HAL_ERROR_NOT_INIT;
}
#define UNSUP3(name, t3)                                                                           \
    xy_hal_error_t name(void* p, xy_hal_pwm_channel_t c, t3 v) {                                   \
        XY_UNUSED(p);                                                                              \
        XY_UNUSED(c);                                                                              \
        XY_UNUSED(v);                                                                              \
        return XY_HAL_ERROR_NOT_SUPPORTED;                                                         \
    }
UNSUP3(xy_hal_pwm_set_deadtime, uint32_t)
int32_t xy_hal_pwm_get_deadtime(void* p, xy_hal_pwm_channel_t c) {
    XY_UNUSED(p);
    XY_UNUSED(c);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
UNSUP3(xy_hal_pwm_enable_complementary, const xy_hal_pwm_complementary_config_t*)
xy_hal_error_t xy_hal_pwm_disable_complementary(void* p, xy_hal_pwm_channel_t c) {
    XY_UNUSED(p);
    XY_UNUSED(c);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
UNSUP3(xy_hal_pwm_set_fault_state, uint8_t)
int32_t xy_hal_pwm_get_fault_state(void* p, xy_hal_pwm_channel_t c) {
    XY_UNUSED(p);
    XY_UNUSED(c);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
xy_hal_error_t xy_hal_pwm_control(void* p, xy_hal_pwm_channel_t c, uint32_t cmd, void* a) {
    XY_UNUSED(p);
    XY_UNUSED(c);
    XY_UNUSED(cmd);
    XY_UNUSED(a);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
UNSUP3(xy_hal_pwm_config_pulse_mode, uint32_t)
xy_hal_error_t xy_hal_pwm_enable_pulse_mode(void* p, xy_hal_pwm_channel_t c) {
    XY_UNUSED(p);
    XY_UNUSED(c);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
xy_hal_error_t xy_hal_pwm_disable_pulse_mode(void* p, xy_hal_pwm_channel_t c) {
    XY_UNUSED(p);
    XY_UNUSED(c);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
xy_hal_error_t xy_hal_pwm_sync_output(void* p, uint8_t m) {
    XY_UNUSED(p);
    XY_UNUSED(m);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
int32_t xy_hal_pwm_input_capture(void* p, xy_hal_pwm_channel_t c, uint32_t t) {
    XY_UNUSED(p);
    XY_UNUSED(c);
    XY_UNUSED(t);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
UNSUP3(xy_hal_pwm_set_capture_polarity, xy_hal_pwm_polarity_t)
int32_t xy_hal_pwm_get_capture_polarity(void* p, xy_hal_pwm_channel_t c) {
    XY_UNUSED(p);
    XY_UNUSED(c);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
UNSUP3(xy_hal_pwm_config_encoder_mode, uint8_t)
int32_t xy_hal_pwm_get_encoder_count(void* p) {
    XY_UNUSED(p);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
xy_hal_error_t xy_hal_pwm_set_encoder_count(void* p, int32_t c) {
    XY_UNUSED(p);
    XY_UNUSED(c);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
xy_hal_error_t xy_hal_pwm_reset_encoder_count(void* p) {
    XY_UNUSED(p);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
UNSUP3(xy_hal_pwm_config_fault_protection, const void*)
xy_hal_error_t xy_hal_pwm_clear_fault(void* p, xy_hal_pwm_channel_t c) {
    XY_UNUSED(p);
    XY_UNUSED(c);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
int32_t xy_hal_pwm_check_fault(void* p, xy_hal_pwm_channel_t c) {
    XY_UNUSED(p);
    XY_UNUSED(c);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
#else
#error "WCH PWM backend requires MCU_CH32"
#endif
