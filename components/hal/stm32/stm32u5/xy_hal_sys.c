#include "xy_hal_sys.h"

#include "stm32u5xx_hal.h"

xy_hal_error_t xy_hal_sys_enter_pwr_mode(xy_hal_sys_pwr_mode_t mode)
{
    if (mode != XY_HAL_SYS_PWR_SLEEP) {
        return XY_HAL_ERROR_NOT_SUPPORTED;
    }
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_sys_exit_pwr_mode(void)
{
    return XY_HAL_OK;
}

void xy_hal_sys_tick_irq_handler(void)
{
    HAL_IncTick();
}

xy_hal_error_t xy_hal_sys_get_reset_raw_flags(uint32_t *raw)
{
    /* The U5 SYS backend does not own reset-cause reporting yet; report absence
     * instead of inventing a value from a register this file never reads. */
    (void)raw;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}