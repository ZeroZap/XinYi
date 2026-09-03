#include "xy_sys.h"

#include "stm32l4xx_hal.h"
#include "xy_error.h"

static uint32_t pandora_reset_reason;

void xy_sys_init(void)
{
    pandora_reset_reason = RCC->CSR;
    __HAL_RCC_CLEAR_RESET_FLAGS();
}

int xy_sys_reset(int reset_by)
{
    if (reset_by < 0) {
        return XY_ERROR_INVALID_PARAM;
    }

    __DSB();
    NVIC_SystemReset();
    for (;;) {
    }
}

int xy_sys_reboot_reason(void *data)
{
    if (data == NULL) {
        return XY_ERROR_INVALID_PARAM;
    }

    *(uint32_t *)data = pandora_reset_reason;
    return XY_OK;
}

int xy_sys_get_chip_id(void *data)
{
    uint32_t *chip_id = data;

    if (chip_id == NULL) {
        return XY_ERROR_INVALID_PARAM;
    }

    chip_id[0] = HAL_GetUIDw0();
    chip_id[1] = HAL_GetUIDw1();
    chip_id[2] = HAL_GetUIDw2();
    return XY_OK;
}
