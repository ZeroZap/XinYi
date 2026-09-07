#include "xy_hal_sys.h"

#include "stm32l4xx_hal.h"

uint32_t xy_hal_sys_get_tick_count(void)
{
    return HAL_GetTick();
}

uint32_t xy_hal_sys_get_tick_freq(void)
{
    return 1000U;
}

xy_hal_error_t xy_hal_sys_software_reset(void)
{
    __DSB();
    NVIC_SystemReset();
    return XY_HAL_ERROR;
}

void xy_hal_sys_reset(void)
{
    (void)xy_hal_sys_software_reset();
    for (;;) {
    }
}

uint32_t xy_hal_sys_get_reset_reason(void)
{
    uint32_t csr = RCC->CSR;
    uint32_t reason = XY_HAL_SYS_RESET_REASON_NONE;

    if ((csr & RCC_CSR_PINRSTF) != 0U) {
        reason |= XY_HAL_SYS_RESET_REASON_EXTERNAL_PIN;
    }
    if ((csr & RCC_CSR_BORRSTF) != 0U) {
        reason |= XY_HAL_SYS_RESET_REASON_BROWNOUT;
    }
    if ((csr & RCC_CSR_SFTRSTF) != 0U) {
        reason |= XY_HAL_SYS_RESET_REASON_SOFTWARE;
    }
    if ((csr & (RCC_CSR_IWDGRSTF | RCC_CSR_WWDGRSTF)) != 0U) {
        reason |= XY_HAL_SYS_RESET_REASON_WATCHDOG;
    }
    if ((csr & RCC_CSR_LPWRRSTF) != 0U) {
        reason |= XY_HAL_SYS_RESET_REASON_LOW_POWER;
    }
    return reason;
}

xy_hal_error_t xy_hal_sys_clear_reset_reason(void)
{
    __HAL_RCC_CLEAR_RESET_FLAGS();
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_sys_get_unique_id(uint32_t id[3])
{
    if (id == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    id[0] = HAL_GetUIDw0();
    id[1] = HAL_GetUIDw1();
    id[2] = HAL_GetUIDw2();
    return XY_HAL_OK;
}