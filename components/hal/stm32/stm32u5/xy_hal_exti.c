/**
 * @file xy_hal_exti.c
 * @brief EXTI HAL — STM32U5 implementation against the function-based EXTI v2 API.
 *
 * STM32U5 dropped the F4-era `__HAL_EXTI_*_TRIGGER` register macros; configuration
 * now goes through HAL_EXTI_SetConfigLine() on a per-line EXTI_HandleTypeDef.
 */

#include "../../inc/xy_hal_exti.h"

#if defined(STM32U5) || defined(STM32U5xx)

#include "stm32u5xx_hal.h"
#include <string.h>

typedef struct {
    EXTI_HandleTypeDef     hexti;
    xy_hal_exti_callback_t callback;
    void                  *arg;
    xy_hal_exti_trigger_t  trigger;
    uint8_t                configured;
} exti_line_ctx_t;

typedef struct {
    exti_line_ctx_t lines[XY_HAL_EXTI_LINE_MAX];
    uint8_t         initialized;
} exti_ctx_t;

static exti_ctx_t g_exti_ctx;

/* Map XY line index (0..15) to U5 SDK EXTI_LINE_n encoding. */
static const uint32_t k_line_to_sdk[XY_HAL_EXTI_LINE_MAX] = {
    EXTI_LINE_0,  EXTI_LINE_1,  EXTI_LINE_2,  EXTI_LINE_3,
    EXTI_LINE_4,  EXTI_LINE_5,  EXTI_LINE_6,  EXTI_LINE_7,
    EXTI_LINE_8,  EXTI_LINE_9,  EXTI_LINE_10, EXTI_LINE_11,
    EXTI_LINE_12, EXTI_LINE_13, EXTI_LINE_14, EXTI_LINE_15,
};

static IRQn_Type line_to_irqn(xy_hal_exti_line_t line)
{
    /* STM32U5 EXTI 0..15 each map 1:1 to EXTIn_IRQn. */
    return (IRQn_Type)((int)EXTI0_IRQn + (int)line);
}

static uint32_t xy_to_sdk_trigger(xy_hal_exti_trigger_t trigger)
{
    switch (trigger) {
    case XY_HAL_EXTI_TRIGGER_RISING:  return EXTI_TRIGGER_RISING;
    case XY_HAL_EXTI_TRIGGER_FALLING: return EXTI_TRIGGER_FALLING;
    case XY_HAL_EXTI_TRIGGER_BOTH:    return EXTI_TRIGGER_RISING_FALLING;
    default:                          return EXTI_TRIGGER_NONE;
    }
}

xy_hal_error_t xy_hal_exti_init(void)
{
    if (g_exti_ctx.initialized) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }
    memset(&g_exti_ctx, 0, sizeof(g_exti_ctx));
    g_exti_ctx.initialized = 1;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_exti_deinit(void)
{
    if (!g_exti_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    for (int i = 0; i < XY_HAL_EXTI_LINE_MAX; i++) {
        if (g_exti_ctx.lines[i].configured) {
            HAL_EXTI_ClearConfigLine(&g_exti_ctx.lines[i].hexti);
        }
    }
    memset(&g_exti_ctx, 0, sizeof(g_exti_ctx));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_exti_configure(const xy_hal_exti_config_t *config)
{
    if (!config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (!g_exti_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (config->line >= XY_HAL_EXTI_LINE_MAX) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    uint32_t trig = xy_to_sdk_trigger(config->trigger);
    if (trig == EXTI_TRIGGER_NONE) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    __HAL_RCC_SYSCFG_CLK_ENABLE();

    exti_line_ctx_t *ctx = &g_exti_ctx.lines[config->line];
    ctx->trigger = config->trigger;
    ctx->hexti.Line = k_line_to_sdk[config->line];

    EXTI_ConfigTypeDef cfg = {
        .Line    = k_line_to_sdk[config->line],
        .Mode    = EXTI_MODE_INTERRUPT,
        .Trigger = trig,
        .GPIOSel = EXTI_GPIOA,  /* default; caller wires actual port via HAL_GPIO_Init */
    };
    if (HAL_EXTI_SetConfigLine(&ctx->hexti, &cfg) != HAL_OK) {
        return XY_HAL_ERROR_IO;
    }
    ctx->configured = 1;

    if (config->enable) {
        HAL_NVIC_EnableIRQ(line_to_irqn(config->line));
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_exti_enable(xy_hal_exti_line_t line)
{
    if (!g_exti_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (line >= XY_HAL_EXTI_LINE_MAX) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    HAL_NVIC_EnableIRQ(line_to_irqn(line));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_exti_disable(xy_hal_exti_line_t line)
{
    if (!g_exti_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (line >= XY_HAL_EXTI_LINE_MAX) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    HAL_NVIC_DisableIRQ(line_to_irqn(line));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_exti_register_callback(xy_hal_exti_line_t line,
                                             xy_hal_exti_callback_t callback,
                                             void *arg)
{
    if (!callback) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (!g_exti_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (line >= XY_HAL_EXTI_LINE_MAX) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    g_exti_ctx.lines[line].callback = callback;
    g_exti_ctx.lines[line].arg      = arg;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_exti_unregister_callback(xy_hal_exti_line_t line)
{
    if (!g_exti_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (line >= XY_HAL_EXTI_LINE_MAX) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    g_exti_ctx.lines[line].callback = NULL;
    g_exti_ctx.lines[line].arg      = NULL;
    return XY_HAL_OK;
}

int xy_hal_exti_get_pending(xy_hal_exti_line_t line)
{
    if (line >= XY_HAL_EXTI_LINE_MAX || !g_exti_ctx.lines[line].configured) {
        return 0;
    }
    EXTI_HandleTypeDef *h = &g_exti_ctx.lines[line].hexti;
    return (HAL_EXTI_GetPending(h, EXTI_TRIGGER_RISING) ||
            HAL_EXTI_GetPending(h, EXTI_TRIGGER_FALLING)) ? 1 : 0;
}

xy_hal_error_t xy_hal_exti_set_pending(xy_hal_exti_line_t line)
{
    if (line >= XY_HAL_EXTI_LINE_MAX || !g_exti_ctx.lines[line].configured) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    HAL_EXTI_GenerateSWI(&g_exti_ctx.lines[line].hexti);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_exti_clear_pending(xy_hal_exti_line_t line)
{
    if (line >= XY_HAL_EXTI_LINE_MAX || !g_exti_ctx.lines[line].configured) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    HAL_EXTI_ClearPending(&g_exti_ctx.lines[line].hexti,
                          EXTI_TRIGGER_RISING_FALLING);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_exti_generate_software_interrupt(xy_hal_exti_line_t line)
{
    return xy_hal_exti_set_pending(line);
}

void xy_hal_exti_irq_handler(xy_hal_exti_line_t line)
{
    if (line >= XY_HAL_EXTI_LINE_MAX) {
        return;
    }
    exti_line_ctx_t *ctx = &g_exti_ctx.lines[line];
    if (ctx->callback) {
        ctx->callback(line, ctx->arg);
    }
}

xy_hal_error_t xy_hal_exti_map_gpio(uint8_t port, uint8_t pin,
                                    xy_hal_exti_line_t line)
{
    /* On STM32U5 the EXTI source GPIO multiplexer is configured by
     * HAL_GPIO_Init() when the pin Mode includes GPIO_MODE_IT_*, so we
     * have nothing to do here.  The arguments are accepted for API
     * portability with platforms that need an explicit mapping step. */
    XY_UNUSED(port);
    XY_UNUSED(pin);
    XY_UNUSED(line);
    return XY_HAL_OK;
}

/* HAL forwards GPIO EXTI events through these weak callbacks. */
void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
{
    for (int i = 0; i < XY_HAL_EXTI_LINE_MAX; i++) {
        if (GPIO_Pin & (1U << i)) {
            xy_hal_exti_irq_handler((xy_hal_exti_line_t)i);
        }
    }
}

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
    for (int i = 0; i < XY_HAL_EXTI_LINE_MAX; i++) {
        if (GPIO_Pin & (1U << i)) {
            xy_hal_exti_irq_handler((xy_hal_exti_line_t)i);
        }
    }
}

#endif /* STM32U5 || STM32U5xx */
