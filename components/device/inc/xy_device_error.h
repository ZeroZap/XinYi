/**
 * @file xy_device_error.h
 * @brief Device Framework Error Codes
 * @version 1.0.0
 * @date 2026-03-14
 */

#ifndef XY_DEVICE_ERROR_H
#define XY_DEVICE_ERROR_H

#include "../hal/inc/xy_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Legacy error codes for compatibility */
#define XY_ERROR_OK              XY_HAL_OK
#define XY_ERROR_INVALID_PARAM   XY_HAL_ERROR_INVALID_PARAM
#define XY_ERROR_NOT_SUPPORT     XY_HAL_ERROR_NOT_SUPPORT
#define XY_ERROR_TIMEOUT         XY_HAL_ERROR_TIMEOUT
#define XY_ERROR_BUSY            XY_HAL_ERROR_BUSY
#define XY_ERROR_NO_MEM          XY_HAL_ERROR_NO_MEM
#define XY_ERROR_IO              XY_HAL_ERROR_IO
#define XY_ERROR_NOT_INIT        XY_HAL_ERROR_NOT_INIT
#define XY_ERROR_ALREADY_INIT    XY_HAL_ERROR_ALREADY_INIT
#define XY_ERROR_NO_RESOURCE     XY_HAL_ERROR_NO_RESOURCE
#define XY_ERROR_FAIL            XY_HAL_ERROR_FAIL

typedef xy_hal_error_t xy_error_t;

#ifdef __cplusplus
}
#endif

#endif /* XY_DEVICE_ERROR_H */
