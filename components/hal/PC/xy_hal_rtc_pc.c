#include "xy_hal_rtc.h"

static uint32_t backup_registers[XY_HAL_RTC_BACKUP_REGISTER_COUNT];

xy_hal_error_t xy_hal_rtc_backup_read(uint32_t index, uint32_t *value)
{
    if (value == NULL || index >= XY_HAL_RTC_BACKUP_REGISTER_COUNT) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    *value = backup_registers[index];
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_backup_write(uint32_t index, uint32_t value)
{
    if (index >= XY_HAL_RTC_BACKUP_REGISTER_COUNT) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    backup_registers[index] = value;
    return XY_HAL_OK;
}