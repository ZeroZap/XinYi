/**
 * @file xy_hal_exti.c
 * @brief WCH CH32V30x EXTI HAL implementation
 */
#include "xy_hal_exti.h"
#if defined(MCU_CH32) || defined(CH32V30x)
#include "ch32v30x_exti.h"
#include "ch32v30x_gpio.h"
#include "ch32v30x_misc.h"
#include "ch32v30x_rcc.h"
#include <string.h>

static xy_hal_exti_handle_t handles[XY_HAL_EXTI_LINE_MAX];
static uint8_t initialized;

static int valid_line(xy_hal_exti_line_t line) {
    return line >= XY_HAL_EXTI_LINE_0 && line < XY_HAL_EXTI_LINE_MAX;
}
static uint32_t line_mask(xy_hal_exti_line_t line) {
    return 1UL << (uint32_t)line;
}
static uint8_t irq_channel(xy_hal_exti_line_t line) {
    static const uint8_t channels[] = {
        EXTI0_IRQn,     EXTI1_IRQn,     EXTI2_IRQn,     EXTI3_IRQn,
        EXTI4_IRQn,     EXTI9_5_IRQn,   EXTI9_5_IRQn,   EXTI9_5_IRQn,
        EXTI9_5_IRQn,   EXTI9_5_IRQn,   EXTI15_10_IRQn, EXTI15_10_IRQn,
        EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn};
    return channels[line];
}
static void set_nvic(xy_hal_exti_line_t line, FunctionalState state) {
    NVIC_InitTypeDef nvic = {0};
    nvic.NVIC_IRQChannel = irq_channel(line);
    nvic.NVIC_IRQChannelPreemptionPriority = 1U;
    nvic.NVIC_IRQChannelSubPriority = 0U;
    nvic.NVIC_IRQChannelCmd = state;
    NVIC_Init(&nvic);
}
static EXTITrigger_TypeDef trigger_value(xy_hal_exti_trigger_t trigger) {
    static const EXTITrigger_TypeDef values[] = {EXTI_Trigger_Rising, EXTI_Trigger_Falling,
                                                 EXTI_Trigger_Rising_Falling};
    return values[trigger];
}
xy_hal_error_t xy_hal_exti_init(void) {
    if (initialized)
        return XY_HAL_ERROR_ALREADY_INIT;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    memset(handles, 0, sizeof(handles));
    for (uint32_t i = 0; i < XY_HAL_EXTI_LINE_MAX; ++i)
        handles[i].line = (xy_hal_exti_line_t)i;
    initialized = 1U;
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_exti_deinit(void) {
    if (!initialized)
        return XY_HAL_ERROR_NOT_INIT;
    EXTI_DeInit();
    memset(handles, 0, sizeof(handles));
    initialized = 0U;
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_exti_configure(const xy_hal_exti_config_t* config) {
    EXTI_InitTypeDef exti = {0};
    if (config == NULL || !valid_line(config->line) || config->trigger > XY_HAL_EXTI_TRIGGER_BOTH ||
        config->enable > 1U)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!initialized)
        return XY_HAL_ERROR_NOT_INIT;
    exti.EXTI_Line = line_mask(config->line);
    exti.EXTI_Mode = EXTI_Mode_Interrupt;
    exti.EXTI_Trigger = trigger_value(config->trigger);
    exti.EXTI_LineCmd = config->enable ? ENABLE : DISABLE;
    EXTI_Init(&exti);
    EXTI_ClearITPendingBit(exti.EXTI_Line);
    set_nvic(config->line, config->enable ? ENABLE : DISABLE);
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_exti_enable(xy_hal_exti_line_t line) {
    if (!valid_line(line))
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!initialized)
        return XY_HAL_ERROR_NOT_INIT;
    EXTI->INTENR |= line_mask(line);
    set_nvic(line, ENABLE);
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_exti_disable(xy_hal_exti_line_t line) {
    if (!valid_line(line))
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!initialized)
        return XY_HAL_ERROR_NOT_INIT;
    EXTI->INTENR &= ~line_mask(line);
    EXTI_ClearITPendingBit(line_mask(line));
    set_nvic(line, DISABLE);
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_exti_register_callback(xy_hal_exti_line_t line,
                                             xy_hal_exti_callback_t callback, void* arg) {
    if (!valid_line(line) || callback == NULL)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!initialized)
        return XY_HAL_ERROR_NOT_INIT;
    handles[line].callback = callback;
    handles[line].arg = arg;
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_exti_unregister_callback(xy_hal_exti_line_t line) {
    if (!valid_line(line))
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!initialized)
        return XY_HAL_ERROR_NOT_INIT;
    handles[line].callback = NULL;
    handles[line].arg = NULL;
    return XY_HAL_OK;
}
int xy_hal_exti_get_pending(xy_hal_exti_line_t line) {
    if (!valid_line(line) || !initialized)
        return 0;
    return EXTI_GetFlagStatus(line_mask(line)) == SET;
}
xy_hal_error_t xy_hal_exti_set_pending(xy_hal_exti_line_t line) {
    return xy_hal_exti_generate_software_interrupt(line);
}
xy_hal_error_t xy_hal_exti_clear_pending(xy_hal_exti_line_t line) {
    if (!valid_line(line))
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!initialized)
        return XY_HAL_ERROR_NOT_INIT;
    EXTI_ClearITPendingBit(line_mask(line));
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_exti_generate_software_interrupt(xy_hal_exti_line_t line) {
    if (!valid_line(line))
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!initialized)
        return XY_HAL_ERROR_NOT_INIT;
    EXTI_GenerateSWInterrupt(line_mask(line));
    NVIC_SetPendingIRQ((IRQn_Type)irq_channel(line));
    return XY_HAL_OK;
}
void xy_hal_exti_irq_handler(xy_hal_exti_line_t line) {
    if (!valid_line(line) || EXTI_GetITStatus(line_mask(line)) == RESET)
        return;
    EXTI_ClearITPendingBit(line_mask(line));
    if (handles[line].callback != NULL)
        handles[line].callback(line, handles[line].arg);
}
xy_hal_error_t xy_hal_exti_map_gpio(uint8_t port, uint8_t pin, xy_hal_exti_line_t line) {
    if (!initialized || port > GPIO_PortSourceGPIOE || pin > 15U || !valid_line(line) ||
        pin != (uint8_t)line)
        return XY_HAL_ERROR_INVALID_PARAM;
    GPIO_EXTILineConfig(port, pin);
    return XY_HAL_OK;
}
#define WCH_FAST_IRQ __attribute__((interrupt("WCH-Interrupt-fast")))
void WCH_FAST_IRQ EXTI0_IRQHandler(void) {
    xy_hal_exti_irq_handler(XY_HAL_EXTI_LINE_0);
}
void WCH_FAST_IRQ EXTI1_IRQHandler(void) {
    xy_hal_exti_irq_handler(XY_HAL_EXTI_LINE_1);
}
void WCH_FAST_IRQ EXTI2_IRQHandler(void) {
    xy_hal_exti_irq_handler(XY_HAL_EXTI_LINE_2);
}
void WCH_FAST_IRQ EXTI3_IRQHandler(void) {
    xy_hal_exti_irq_handler(XY_HAL_EXTI_LINE_3);
}
void WCH_FAST_IRQ EXTI4_IRQHandler(void) {
    xy_hal_exti_irq_handler(XY_HAL_EXTI_LINE_4);
}
void WCH_FAST_IRQ EXTI9_5_IRQHandler(void) {
    for (int i = 5; i <= 9; ++i)
        xy_hal_exti_irq_handler((xy_hal_exti_line_t)i);
}
void WCH_FAST_IRQ EXTI15_10_IRQHandler(void) {
    for (int i = 10; i <= 15; ++i)
        xy_hal_exti_irq_handler((xy_hal_exti_line_t)i);
}
#endif
