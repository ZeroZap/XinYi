/**
 * @file xy_hal_error.h
 * @brief HAL Error Codes
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
    XY_HAL_ERROR_IO = -8,
} xy_hal_error_t;

#endif /* XY_HAL_ERROR_H */
