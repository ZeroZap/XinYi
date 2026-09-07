#include "xy_hal_delay.h"

#include "stm32l4xx_hal.h"

void xy_hal_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

void xy_hal_delay_us(uint32_t us)
{
    uint32_t cycles = (SystemCoreClock / 1000000U) * us;

    while (cycles-- != 0U) {
        __NOP();
    }
}
