/**
 * @file xy_device_error.h
 * @brief Canonical error code header for device and driver code.
 *
 * Defines xy_error_t (int alias) plus the XY_OK / XY_ERROR_* macro family.
 * Numerically aligned with xy_hal_error.h (which it re-includes) and
 * xy_clib/xy_error.h. Driver authors should #include this header.
 */

#ifndef XY_DEVICE_ERROR_H
#define XY_DEVICE_ERROR_H

#include "xy_hal_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Canonical error type for the device layer (int for full range) */
typedef int xy_error_t;

/* Success / generic failure */
#define XY_OK                    0
#define XY_ERROR                 (-1)

/* Standard error codes (values match xy_error.h in clib) */
#define XY_ERROR_INVALID_PARAM   (-2)
#define XY_ERROR_NOT_SUPPORTED   (-3)
#define XY_ERROR_TIMEOUT         (-4)
#define XY_ERROR_BUSY            (-5)
#define XY_ERROR_NO_MEMORY       (-6)
#define XY_ERROR_IO              (-7)
#define XY_ERROR_NOT_INIT        (-8)
#define XY_ERROR_ALREADY_INIT    (-9)
#define XY_ERROR_NO_RESOURCE     (-10)
#define XY_ERROR_FAIL            (-11)
#define XY_ERROR_NO_DATA         (-12)
#define XY_ERROR_OVERFLOW        (-13)
#define XY_ERROR_ACCESS_DENIED   (-18)
#define XY_ERROR_NOT_FOUND       (-19)
#define XY_ERROR_NOT_READY       (-23)
#define XY_ERROR_ALREADY_EXISTS  (-25)
#define XY_ERROR_NOT_AVAILABLE   (-26)

/* Device-specific error codes (map to HAL values) */
#define XY_DEVICE_OK              XY_HAL_OK
#define XY_DEVICE_ERROR           XY_HAL_ERROR
#define XY_DEVICE_INVALID_PARAM   XY_HAL_ERROR_INVALID_PARAM
#define XY_DEVICE_NOT_SUPPORT     XY_HAL_ERROR_NOT_SUPPORTED
#define XY_DEVICE_TIMEOUT         XY_HAL_ERROR_TIMEOUT
#define XY_DEVICE_BUSY            XY_HAL_ERROR_BUSY
#define XY_DEVICE_NO_MEM          XY_HAL_ERROR_NO_MEMORY
#define XY_DEVICE_IO_ERROR        XY_HAL_ERROR_IO
#define XY_DEVICE_NOT_INIT        XY_HAL_ERROR_NOT_INIT
#define XY_DEVICE_ALREADY_INIT    XY_HAL_ERROR_ALREADY_INIT
#define XY_DEVICE_NO_RESOURCE     XY_ERROR_NO_RESOURCE
#define XY_DEVICE_FAIL            XY_HAL_ERROR_FAIL
#define XY_DEVICE_NOT_FOUND       XY_ERROR_NOT_FOUND

/* Legacy XY_ERR_* aliases used by some drivers */
#define XY_ERR_OK              XY_OK
#define XY_ERR_INVALID_PARAM   XY_ERROR_INVALID_PARAM
#define XY_ERR_NO_MEM          XY_ERROR_NO_MEMORY
#define XY_ERR_NOT_SUPPORT     XY_ERROR_NOT_SUPPORTED
#define XY_ERR_TIMEOUT         XY_ERROR_TIMEOUT
#define XY_ERR_BUSY            XY_ERROR_BUSY
#define XY_ERR_IO              XY_ERROR_IO
#define XY_ERR_NOT_FOUND       XY_ERROR_NOT_FOUND
#define XY_ERR_ALREADY_EXISTS  XY_ERROR_ALREADY_EXISTS

/* Utility macro */
#ifndef XY_UNUSED
#define XY_UNUSED(x) ((void)(x))
#endif

#ifdef __cplusplus
}
#endif

#endif /* XY_DEVICE_ERROR_H */
