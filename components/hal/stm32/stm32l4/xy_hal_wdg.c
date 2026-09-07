/**
 * @file xy_hal_wdg.c
 * @brief Watchdog HAL STM32L4 implementation
 */

#include "xy_hal_wdg.h"

#include "stm32l4xx_hal.h"

#include <stddef.h>

static IWDG_HandleTypeDef *iwdg_handle;
static uint8_t iwdg_initialized;

static uint32_t iwdg_prescaler_divider(uint32_t prescaler)
{
    switch (prescaler) {
    case IWDG_PRESCALER_4:
        return 4U;
    case IWDG_PRESCALER_8:
        return 8U;
    case IWDG_PRESCALER_16:
        return 16U;
    case IWDG_PRESCALER_32:
        return 32U;
    case IWDG_PRESCALER_64:
        return 64U;
    case IWDG_PRESCALER_128:
        return 128U;
    case IWDG_PRESCALER_256:
        return 256U;
    default:
        return 0U;
    }
}

static xy_hal_error_t iwdg_validate_handle(void *wdg)
{
    if (wdg == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (iwdg_initialized == 0U || iwdg_handle != (IWDG_HandleTypeDef *)wdg) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_iwdg_init(void *wdg, const xy_hal_iwdg_config_t *config)
{
    IWDG_HandleTypeDef *handle;

    if (wdg == NULL || config == NULL || config->reload > IWDG_RLR_RL ||
        iwdg_prescaler_divider(config->prescaler) == 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (iwdg_initialized != 0U) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }

    handle = (IWDG_HandleTypeDef *)wdg;
    handle->Init.Prescaler = config->prescaler;
    handle->Init.Reload = config->reload;
    handle->Init.Window = IWDG_WINDOW_DISABLE;
    if (HAL_IWDG_Init(handle) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    iwdg_handle = handle;
    iwdg_initialized = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_iwdg_start(void *wdg)
{
    xy_hal_error_t result = iwdg_validate_handle(wdg);

    if (result != XY_HAL_OK) {
        return result;
    }
    return HAL_IWDG_Refresh(iwdg_handle) == HAL_OK ? XY_HAL_OK : XY_HAL_ERROR_FAIL;
}

xy_hal_error_t xy_hal_iwdg_feed(void *wdg)
{
    return xy_hal_iwdg_start(wdg);
}

int xy_hal_iwdg_get_remaining_time(void *wdg)
{
    uint32_t divider;
    xy_hal_error_t result = iwdg_validate_handle(wdg);

    if (result != XY_HAL_OK) {
        return result;
    }
    divider = iwdg_prescaler_divider(iwdg_handle->Init.Prescaler);
    return (int)(((uint64_t)(iwdg_handle->Init.Reload + 1U) * divider * 1000U) / 32000U);
}

xy_hal_error_t xy_hal_iwdg_set_timeout(void *wdg, uint32_t timeout_ms)
{
    uint32_t reload;
    uint32_t divider;
    static const uint32_t prescalers[] = {
        IWDG_PRESCALER_4,   IWDG_PRESCALER_8,   IWDG_PRESCALER_16, IWDG_PRESCALER_32,
        IWDG_PRESCALER_64,  IWDG_PRESCALER_128, IWDG_PRESCALER_256,
    };
    xy_hal_error_t result = iwdg_validate_handle(wdg);

    if (result != XY_HAL_OK || timeout_ms == 0U) {
        return result != XY_HAL_OK ? result : XY_HAL_ERROR_INVALID_PARAM;
    }

    for (size_t index = 0U; index < sizeof(prescalers) / sizeof(prescalers[0]); ++index) {
        divider = iwdg_prescaler_divider(prescalers[index]);
        reload = (uint32_t)(((uint64_t)timeout_ms * 32000U + divider * 1000U - 1U) /
                            (divider * 1000U));
        if (reload >= 1U && reload <= IWDG_RLR_RL + 1U) {
            iwdg_handle->Init.Prescaler = prescalers[index];
            iwdg_handle->Init.Reload = reload - 1U;
            return HAL_IWDG_Init(iwdg_handle) == HAL_OK ? XY_HAL_OK : XY_HAL_ERROR_FAIL;
        }
    }
    return XY_HAL_ERROR_INVALID_PARAM;
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

xy_hal_error_t xy_hal_wwdg_register_ewi_callback(void *wdg, xy_hal_wdg_callback_t callback,
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
    NVIC_SystemReset();
}
