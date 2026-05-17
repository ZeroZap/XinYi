/**
 * @file xy_hal_exti.c
 * @brief EXTI HAL STM32F4 Implementation
 */

#include "../../inc/xy_hal_exti.h"

#ifdef STM32_HAL_ENABLED

#include "stm32_hal.h"
#include <string.h>

typedef struct {
    EXTI_HandleTypeDef  hexti;
    xy_hal_exti_callback_t callback;
    void *arg;
    uint8_t configured;
} exti_ctx_t;

#define MAX_EXTI_LINES  16
static exti_ctx_t g_exti_ctx[MAX_EXTI_LINES];

static uint32_t xy_to_stm32_exti_line(xy_hal_exti_line_t line)
{
    static const uint32_t tbl[] = {
        EXTI_LINE_0,  EXTI_LINE_1,  EXTI_LINE_2,  EXTI_LINE_3,
        EXTI_LINE_4,  EXTI_LINE_5,  EXTI_LINE_6,  EXTI_LINE_7,
        EXTI_LINE_8,  EXTI_LINE_9,  EXTI_LINE_10, EXTI_LINE_11,
        EXTI_LINE_12, EXTI_LINE_13, EXTI_LINE_14, EXTI_LINE_15,
    };
    if ((uint32_t)line < MAX_EXTI_LINES) {
        return tbl[line];
    }
    return EXTI_LINE_0;
}

static uint32_t xy_to_stm32_trigger(xy_hal_exti_trigger_t trig)
{
    switch (trig) {
    case XY_HAL_EXTI_TRIGGER_RISING:  return EXTI_TRIGGER_RISING;
    case XY_HAL_EXTI_TRIGGER_FALLING: return EXTI_TRIGGER_FALLING;
    case XY_HAL_EXTI_TRIGGER_BOTH:    return EXTI_TRIGGER_RISING_FALLING;
    default:                           return EXTI_TRIGGER_RISING;
    }
}

xy_hal_error_t xy_hal_exti_init(void)
{
    memset(g_exti_ctx, 0, sizeof(g_exti_ctx));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_exti_deinit(void)
{
    memset(g_exti_ctx, 0, sizeof(g_exti_ctx));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_exti_configure(const xy_hal_exti_config_t *config)
{
    if (!config || (uint32_t)config->line >= MAX_EXTI_LINES) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    exti_ctx_t *ctx = &g_exti_ctx[config->line];

    EXTI_ConfigTypeDef exti_cfg = { 0 };
    exti_cfg.Line    = xy_to_stm32_exti_line(config->line);
    exti_cfg.Mode    = EXTI_MODE_INTERRUPT;
    exti_cfg.Trigger = xy_to_stm32_trigger(config->trigger);
    exti_cfg.GPIOSel = EXTI_GPIOA; /* default — updated via xy_hal_exti_map_gpio */

    if (HAL_EXTI_GetHandle(&ctx->hexti, exti_cfg.Line) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    if (HAL_EXTI_SetConfigLine(&ctx->hexti, &exti_cfg) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    ctx->configured = 1;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_exti_enable(xy_hal_exti_line_t line)
{
    if ((uint32_t)line >= MAX_EXTI_LINES) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    uint32_t stm_line = xy_to_stm32_exti_line(line);
    __HAL_GPIO_EXTI_CLEAR_IT(stm_line);
    NVIC_EnableIRQ((IRQn_Type)(EXTI0_IRQn + (uint32_t)line));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_exti_disable(xy_hal_exti_line_t line)
{
    if ((uint32_t)line >= MAX_EXTI_LINES) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    NVIC_DisableIRQ((IRQn_Type)(EXTI0_IRQn + (uint32_t)line));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_exti_register_callback(xy_hal_exti_line_t line,
                                              xy_hal_exti_callback_t callback,
                                              void *arg)
{
    if ((uint32_t)line >= MAX_EXTI_LINES) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    g_exti_ctx[line].callback = callback;
    g_exti_ctx[line].arg      = arg;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_exti_unregister_callback(xy_hal_exti_line_t line)
{
    if ((uint32_t)line >= MAX_EXTI_LINES) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    g_exti_ctx[line].callback = NULL;
    g_exti_ctx[line].arg      = NULL;
    return XY_HAL_OK;
}

int xy_hal_exti_get_pending(xy_hal_exti_line_t line)
{
    if ((uint32_t)line >= MAX_EXTI_LINES) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    uint32_t stm_line = xy_to_stm32_exti_line(line);
    return __HAL_GPIO_EXTI_GET_IT(stm_line) ? 1 : 0;
}

xy_hal_error_t xy_hal_exti_set_pending(xy_hal_exti_line_t line)
{
    if ((uint32_t)line >= MAX_EXTI_LINES) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    uint32_t stm_line = xy_to_stm32_exti_line(line);
    HAL_EXTI_GenerateSWI(&g_exti_ctx[line].hexti);
    XY_UNUSED(stm_line);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_exti_clear_pending(xy_hal_exti_line_t line)
{
    if ((uint32_t)line >= MAX_EXTI_LINES) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    uint32_t stm_line = xy_to_stm32_exti_line(line);
    __HAL_GPIO_EXTI_CLEAR_IT(stm_line);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_exti_generate_software_interrupt(xy_hal_exti_line_t line)
{
    if ((uint32_t)line >= MAX_EXTI_LINES) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    HAL_EXTI_GenerateSWI(&g_exti_ctx[line].hexti);
    return XY_HAL_OK;
}

void xy_hal_exti_irq_handler(xy_hal_exti_line_t line)
{
    if ((uint32_t)line >= MAX_EXTI_LINES) {
        return;
    }

    exti_ctx_t *ctx = &g_exti_ctx[line];
    uint32_t stm_line = xy_to_stm32_exti_line(line);
    __HAL_GPIO_EXTI_CLEAR_IT(stm_line);

    if (ctx->callback) {
        ctx->callback(line, ctx->arg);
    }
}

xy_hal_error_t xy_hal_exti_map_gpio(uint8_t port, uint8_t pin,
                                    xy_hal_exti_line_t line)
{
    XY_UNUSED(port);
    XY_UNUSED(pin);
    XY_UNUSED(line);
    /* GPIO-EXTI mapping is done via SYSCFG_EXTILineConfig, typically
     * called from HAL_GPIO_Init when GPIO_MODE_IT_* is used. */
    return XY_HAL_OK;
}

#endif /* STM32_HAL_ENABLED */
