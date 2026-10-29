/**
 * @file xy_error.h
 * @brief XinYi canonical error code definitions
 *
 * Single source of truth for error codes, and it says this because it is checked:
 * tests/unit/hal_component/check_error_code_alignment.py compares every name shared with
 * xy_hal_error.h and xy_device_error.h and fails on any difference.
 *
 * The rule: a name that also exists in xy_hal_error.h carries the HAL number, because the
 * HAL is the layer underneath and its header is included by the others. Names that only
 * exist here are numbered after the HAL range, which is why they start at -15.
 *
 * This header used to claim alignment while thirteen of fifteen shared names disagreed -
 * XY_ERROR_NOT_SUPPORTED was -3, the same number as XY_HAL_ERROR_TIMEOUT. Anything that
 * compared a HAL status against a clib code for that kind of failure got a wrong answer.
 *
 * Layering:
 *   - xy_hal_error.h     -> HAL return type (xy_hal_error_t enum)
 *   - xy_device_error.h  -> int alias xy_error_t for device/driver code
 *   - this file          -> canonical enum, used by xy_clib internally
 */

#ifndef XY_ERROR_H
#define XY_ERROR_H

#include "xy_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Standard error codes. Values must stay compatible with
 *        xy_hal_error.h and xy_device_error.h.
 */
typedef enum {
    XY_OK                    =  0,
    XY_ERROR                 = -1,
    XY_ERROR_INVALID_PARAM   = -2,
    XY_ERROR_NOT_SUPPORTED   = -5,
    XY_ERROR_TIMEOUT         = -3,
    XY_ERROR_BUSY            = -4,
    XY_ERROR_NO_MEMORY       = -7,
    XY_ERROR_IO              = -9,
    XY_ERROR_NOT_INIT        = -13,
    XY_ERROR_ALREADY_INIT    = -14,
    XY_ERROR_NO_RESOURCE     = -8,
    XY_ERROR_FAIL            = -10,
    XY_ERROR_NO_DATA         = -15,
    XY_ERROR_OVERFLOW        = -12,
    XY_ERROR_UNDERFLOW       = -16,
    XY_ERROR_CRC             = -11,
    XY_ERROR_CHECKSUM        = -17,
    XY_ERROR_AUTH            = -18,
    XY_ERROR_ACCESS_DENIED   = -19,
    XY_ERROR_NOT_FOUND       = -6,
    XY_ERROR_INVALID_STATE   = -20,
    XY_ERROR_INVALID_SIZE    = -21,
    XY_ERROR_INVALID_ADDR    = -22,
    XY_ERROR_NOT_READY       = -23,
    XY_ERROR_OUT_OF_RANGE    = -24,
    XY_ERROR_ALREADY_EXISTS  = -25,
    XY_ERROR_NOT_AVAILABLE   = -26,
    XY_ERROR_NOT_IMPLEMENTED = -27,
    XY_ERROR_ABORTED         = -28,
    XY_ERROR_INTERRUPTED     = -29,
} xy_error_t;

/* ==================== Convenience Macros ==================== */

#define XY_IS_SUCCESS(err) ((err) >= 0)
#define XY_IS_ERROR(err)   ((err) < 0)
#define XY_FAILED(err)     XY_IS_ERROR(err)
#define XY_SUCCEEDED(err)  XY_IS_SUCCESS(err)

#define XY_RETURN_ON_ERROR(expr) \
    do { \
        xy_error_t _err = (expr); \
        if (XY_FAILED(_err)) { return _err; } \
    } while (0)

#define XY_RETURN_VAL_ON_ERROR(expr, ret_val) \
    do { \
        xy_error_t _err = (expr); \
        if (XY_FAILED(_err)) { return (ret_val); } \
    } while (0)

#define XY_GOTO_ON_ERROR(expr, label) \
    do { \
        xy_error_t _err = (expr); \
        if (XY_FAILED(_err)) { goto label; } \
    } while (0)

/* ==================== Error String Conversion ==================== */

static inline const char *xy_error_to_string(xy_error_t err)
{
    switch (err) {
    case XY_OK:                    return "XY_OK";
    case XY_ERROR:                 return "XY_ERROR";
    case XY_ERROR_INVALID_PARAM:   return "XY_ERROR_INVALID_PARAM";
    case XY_ERROR_NOT_SUPPORTED:   return "XY_ERROR_NOT_SUPPORTED";
    case XY_ERROR_TIMEOUT:         return "XY_ERROR_TIMEOUT";
    case XY_ERROR_BUSY:            return "XY_ERROR_BUSY";
    case XY_ERROR_NO_MEMORY:       return "XY_ERROR_NO_MEMORY";
    case XY_ERROR_IO:              return "XY_ERROR_IO";
    case XY_ERROR_NOT_INIT:        return "XY_ERROR_NOT_INIT";
    case XY_ERROR_ALREADY_INIT:    return "XY_ERROR_ALREADY_INIT";
    case XY_ERROR_NO_RESOURCE:     return "XY_ERROR_NO_RESOURCE";
    case XY_ERROR_FAIL:            return "XY_ERROR_FAIL";
    case XY_ERROR_NO_DATA:         return "XY_ERROR_NO_DATA";
    case XY_ERROR_OVERFLOW:        return "XY_ERROR_OVERFLOW";
    case XY_ERROR_UNDERFLOW:       return "XY_ERROR_UNDERFLOW";
    case XY_ERROR_CRC:             return "XY_ERROR_CRC";
    case XY_ERROR_CHECKSUM:        return "XY_ERROR_CHECKSUM";
    case XY_ERROR_AUTH:            return "XY_ERROR_AUTH";
    case XY_ERROR_ACCESS_DENIED:   return "XY_ERROR_ACCESS_DENIED";
    case XY_ERROR_NOT_FOUND:       return "XY_ERROR_NOT_FOUND";
    case XY_ERROR_INVALID_STATE:   return "XY_ERROR_INVALID_STATE";
    case XY_ERROR_INVALID_SIZE:    return "XY_ERROR_INVALID_SIZE";
    case XY_ERROR_INVALID_ADDR:    return "XY_ERROR_INVALID_ADDR";
    case XY_ERROR_NOT_READY:       return "XY_ERROR_NOT_READY";
    case XY_ERROR_OUT_OF_RANGE:    return "XY_ERROR_OUT_OF_RANGE";
    case XY_ERROR_ALREADY_EXISTS:  return "XY_ERROR_ALREADY_EXISTS";
    case XY_ERROR_NOT_AVAILABLE:   return "XY_ERROR_NOT_AVAILABLE";
    case XY_ERROR_NOT_IMPLEMENTED: return "XY_ERROR_NOT_IMPLEMENTED";
    case XY_ERROR_ABORTED:         return "XY_ERROR_ABORTED";
    case XY_ERROR_INTERRUPTED:     return "XY_ERROR_INTERRUPTED";
    default:                       return "XY_ERROR_UNKNOWN";
    }
}

#ifdef __cplusplus
}
#endif

#endif /* XY_ERROR_H */
