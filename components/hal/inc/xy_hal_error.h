/**
 * @file xy_hal_error.h
 * @brief HAL return type (xy_hal_error_t).
 *
 * Values are aligned with xy_error.h / xy_device_error.h: callers can
 * freely cross-compare with XY_OK / XY_ERROR_* numerically. Use this
 * header for HAL implementations; device/driver code should prefer
 * xy_device_error.h which already includes this file.
 */

#ifndef XY_HAL_ERROR_H
#define XY_HAL_ERROR_H

#include <stdint.h>

typedef enum {
    XY_HAL_OK = 0,
    XY_HAL_ERROR = -1,
    XY_HAL_ERROR_INVALID_PARAM = -2,
    XY_HAL_ERROR_TIMEOUT = -3,
    XY_HAL_ERROR_BUSY = -4,
    XY_HAL_ERROR_NOT_SUPPORTED = -5,
    XY_HAL_ERROR_NOT_FOUND = -6,
    XY_HAL_ERROR_NO_MEMORY = -7,
    XY_HAL_ERROR_NO_RESOURCE = -8,
    XY_HAL_ERROR_IO = -9,
    XY_HAL_ERROR_FAIL = -10,
    XY_HAL_ERROR_CRC = -11,
    XY_HAL_ERROR_OVERFLOW = -12,
    XY_HAL_ERROR_NOT_INIT = -13,
} xy_hal_error_t;

#endif /* XY_HAL_ERROR_H */
