/**
 * @file xy_error.h
 * @brief XinYi canonical error code definitions
 *
 * Single source of truth for error codes. Numerically aligned with
 * xy_hal_error.h (-1..-13) and xy_device_error.h (which already aliases
 * these values via #defines).
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
    XY_ERROR_NOT_SUPPORTED   = -3,
    XY_ERROR_TIMEOUT         = -4,
    XY_ERROR_BUSY            = -5,
    XY_ERROR_NO_MEMORY       = -6,
    XY_ERROR_IO              = -7,
    XY_ERROR_NOT_INIT        = -8,
    XY_ERROR_ALREADY_INIT    = -9,
    XY_ERROR_NO_RESOURCE     = -10,
    XY_ERROR_FAIL            = -11,
    XY_ERROR_NO_DATA         = -12,
    XY_ERROR_OVERFLOW        = -13,
    XY_ERROR_UNDERFLOW       = -14,
    XY_ERROR_CRC             = -15,
    XY_ERROR_CHECKSUM        = -16,
    XY_ERROR_AUTH            = -17,
    XY_ERROR_ACCESS_DENIED   = -18,
    XY_ERROR_NOT_FOUND       = -19,
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
