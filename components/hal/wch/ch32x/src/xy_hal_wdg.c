/**
 * @file xy_hal_wdg.c
 * @brief WCH CH32V30x watchdog HAL implementation
 */

#include "xy_hal_wdg.h"

#include "ch32v30x.h"

#include <stddef.h>

#define WCH_IWDG_CLOCK_HZ 40000U
#define WCH_IWDG_RELOAD_MAX 0x0FFFU
#define WCH_WWDG_COUNTER_MIN 0x40U
#define WCH_WWDG_COUNTER_MAX 0x7FU

static struct {
    uint8_t initialized;
    uint8_t started;
    uint8_t prescaler;
    uint16_t reload;
} iwdg_context;

static struct {
    uint8_t initialized;
    uint8_t started;
    uint8_t irq_enabled;
    uint8_t counter;
    uint8_t window;
    uint32_t prescaler;
    xy_hal_wdg_callback_t callback;
    void* callback_arg;
} wwdg_context;

static uint32_t iwdg_prescaler_divider(uint32_t prescaler) {
    switch (prescaler) {
        case IWDG_Prescaler_4:
            return 4U;
        case IWDG_Prescaler_8:
            return 8U;
        case IWDG_Prescaler_16:
            return 16U;
        case IWDG_Prescaler_32:
            return 32U;
        case IWDG_Prescaler_64:
            return 64U;
        case IWDG_Prescaler_128:
            return 128U;
        case IWDG_Prescaler_256:
            return 256U;
        default:
            return 0U;
    }
}

static uint32_t wwdg_prescaler_divider(uint32_t prescaler) {
    switch (prescaler) {
        case WWDG_Prescaler_1:
            return 1U;
        case WWDG_Prescaler_2:
            return 2U;
        case WWDG_Prescaler_4:
            return 4U;
        case WWDG_Prescaler_8:
            return 8U;
        default:
            return 0U;
    }
}

static xy_hal_error_t validate_iwdg(void* wdg) {
    if (wdg != IWDG) {
        return wdg == NULL ? XY_HAL_ERROR_INVALID_PARAM : XY_HAL_ERROR_NOT_FOUND;
    }
    return iwdg_context.initialized != 0U ? XY_HAL_OK : XY_HAL_ERROR_NOT_INIT;
}

static xy_hal_error_t validate_wwdg(void* wdg) {
    if (wdg != WWDG) {
        return wdg == NULL ? XY_HAL_ERROR_INVALID_PARAM : XY_HAL_ERROR_NOT_FOUND;
    }
    return wwdg_context.initialized != 0U ? XY_HAL_OK : XY_HAL_ERROR_NOT_INIT;
}

xy_hal_error_t xy_hal_iwdg_init(void* wdg, const xy_hal_iwdg_config_t* config) {
    if (wdg != IWDG || config == NULL || config->reload > WCH_IWDG_RELOAD_MAX ||
        iwdg_prescaler_divider(config->prescaler) == 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (iwdg_context.initialized != 0U) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }

    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
    IWDG_SetPrescaler((uint8_t)config->prescaler);
    IWDG_SetReload((uint16_t)config->reload);
    IWDG_ReloadCounter();

    iwdg_context.initialized = 1U;
    iwdg_context.started = 0U;
    iwdg_context.prescaler = (uint8_t)config->prescaler;
    iwdg_context.reload = (uint16_t)config->reload;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_iwdg_start(void* wdg) {
    xy_hal_error_t result = validate_iwdg(wdg);

    if (result != XY_HAL_OK) {
        return result;
    }
    if (iwdg_context.started == 0U) {
        IWDG_Enable();
        iwdg_context.started = 1U;
    }
    IWDG_ReloadCounter();
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_iwdg_feed(void* wdg) {
    xy_hal_error_t result = validate_iwdg(wdg);

    if (result != XY_HAL_OK) {
        return result;
    }
    if (iwdg_context.started == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    IWDG_ReloadCounter();
    return XY_HAL_OK;
}

int xy_hal_iwdg_get_remaining_time(void* wdg) {
    uint32_t divider;
    xy_hal_error_t result = validate_iwdg(wdg);

    if (result != XY_HAL_OK) {
        return result;
    }
    divider = iwdg_prescaler_divider(iwdg_context.prescaler);
    return (int)(((uint64_t)(iwdg_context.reload + 1U) * divider * 1000U) / WCH_IWDG_CLOCK_HZ);
}

xy_hal_error_t xy_hal_iwdg_set_timeout(void* wdg, uint32_t timeout_ms) {
    static const uint8_t prescalers[] = {IWDG_Prescaler_4,  IWDG_Prescaler_8,  IWDG_Prescaler_16,
                                         IWDG_Prescaler_32, IWDG_Prescaler_64, IWDG_Prescaler_128,
                                         IWDG_Prescaler_256};
    xy_hal_error_t result = validate_iwdg(wdg);

    if (result != XY_HAL_OK) {
        return result;
    }
    if (timeout_ms == 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    for (size_t index = 0U; index < sizeof(prescalers) / sizeof(prescalers[0]); ++index) {
        uint32_t divider = iwdg_prescaler_divider(prescalers[index]);
        uint64_t ticks =
            ((uint64_t)timeout_ms * WCH_IWDG_CLOCK_HZ + (uint64_t)divider * 1000U - 1U) /
            ((uint64_t)divider * 1000U);

        if (ticks >= 1U && ticks <= WCH_IWDG_RELOAD_MAX + 1U) {
            IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
            IWDG_SetPrescaler(prescalers[index]);
            IWDG_SetReload((uint16_t)(ticks - 1U));
            IWDG_ReloadCounter();
            iwdg_context.prescaler = prescalers[index];
            iwdg_context.reload = (uint16_t)(ticks - 1U);
            return XY_HAL_OK;
        }
    }
    return XY_HAL_ERROR_INVALID_PARAM;
}

xy_hal_error_t xy_hal_wwdg_init(void* wdg, const xy_hal_wwdg_config_t* config) {
    if (wdg != WWDG || config == NULL || wwdg_prescaler_divider(config->prescaler) == 0U ||
        config->window < WCH_WWDG_COUNTER_MIN || config->window > WCH_WWDG_COUNTER_MAX ||
        config->counter < WCH_WWDG_COUNTER_MIN || config->counter > WCH_WWDG_COUNTER_MAX) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (wwdg_context.initialized != 0U) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_WWDG, ENABLE);
    WWDG_DeInit();
    WWDG_SetPrescaler(config->prescaler);
    WWDG_SetWindowValue((uint8_t)config->window);
    WWDG_SetCounter((uint8_t)config->counter);
    WWDG_ClearFlag();

    wwdg_context.initialized = 1U;
    wwdg_context.started = 0U;
    wwdg_context.irq_enabled = 0U;
    wwdg_context.counter = (uint8_t)config->counter;
    wwdg_context.window = (uint8_t)config->window;
    wwdg_context.prescaler = config->prescaler;
    wwdg_context.callback = NULL;
    wwdg_context.callback_arg = NULL;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_wwdg_start(void* wdg, uint8_t enable_early_wakeup) {
    xy_hal_error_t result = validate_wwdg(wdg);

    if (result != XY_HAL_OK) {
        return result;
    }
    if (enable_early_wakeup > 1U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (enable_early_wakeup != 0U) {
        WWDG_EnableIT();
        wwdg_context.irq_enabled = 1U;
    }
    WWDG_Enable(wwdg_context.counter);
    wwdg_context.started = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_wwdg_feed(void* wdg, uint32_t counter) {
    xy_hal_error_t result = validate_wwdg(wdg);

    if (result != XY_HAL_OK) {
        return result;
    }
    if (wwdg_context.started == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (counter < WCH_WWDG_COUNTER_MIN || counter > WCH_WWDG_COUNTER_MAX) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    WWDG_SetCounter((uint8_t)counter);
    wwdg_context.counter = (uint8_t)counter;
    return XY_HAL_OK;
}

int xy_hal_wwdg_get_remaining_time(void* wdg) {
    RCC_ClocksTypeDef clocks;
    uint32_t divider;
    uint32_t counter;
    xy_hal_error_t result = validate_wwdg(wdg);

    if (result != XY_HAL_OK) {
        return result;
    }
    RCC_GetClocksFreq(&clocks);
    divider = wwdg_prescaler_divider(wwdg_context.prescaler);
    counter = WWDG->CTLR & WCH_WWDG_COUNTER_MAX;
    if (clocks.PCLK1_Frequency == 0U || counter <= 0x3FU) {
        return 0;
    }
    return (int)(((uint64_t)(counter - 0x3FU) * 4096U * divider * 1000U) / clocks.PCLK1_Frequency);
}

xy_hal_error_t xy_hal_wwdg_set_window(void* wdg, uint32_t window) {
    xy_hal_error_t result = validate_wwdg(wdg);

    if (result != XY_HAL_OK) {
        return result;
    }
    if (window < WCH_WWDG_COUNTER_MIN || window > WCH_WWDG_COUNTER_MAX) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    WWDG_SetWindowValue((uint8_t)window);
    wwdg_context.window = (uint8_t)window;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_wwdg_register_ewi_callback(void* wdg, xy_hal_wdg_callback_t callback,
                                                 void* arg) {
    xy_hal_error_t result = validate_wwdg(wdg);

    if (result != XY_HAL_OK) {
        return result;
    }
    wwdg_context.callback = callback;
    wwdg_context.callback_arg = arg;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_wdg_enable_irq(void* wdg) {
    xy_hal_error_t result = validate_wwdg(wdg);

    if (result != XY_HAL_OK) {
        return result;
    }
    WWDG_EnableIT();
    wwdg_context.irq_enabled = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_wdg_disable_irq(void* wdg) {
    xy_hal_error_t result = validate_wwdg(wdg);

    if (result != XY_HAL_OK) {
        return result;
    }
    if (wwdg_context.irq_enabled == 0U) {
        return XY_HAL_OK;
    }
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

void xy_hal_wdg_system_reset(void) {
    NVIC_SystemReset();
}

void WWDG_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void WWDG_IRQHandler(void) {
    WWDG_ClearFlag();
    if (wwdg_context.callback != NULL) {
        wwdg_context.callback(WWDG, wwdg_context.callback_arg);
    }
}
