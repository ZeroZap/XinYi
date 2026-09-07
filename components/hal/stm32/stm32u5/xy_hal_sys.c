#include "xy_hal_sys.h"

#include "stm32u5xx_hal.h"

void xy_hal_sys_tick_irq_handler(void)
{
    HAL_IncTick();
}