/**
 * @file xy_hal.h
 * @brief XinYi Hardware Abstraction Layer main header
 * @version 2.0
 * @date 2025-10-26
 */

#ifndef XY_HAL_H
#define XY_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief HAL Error Codes
 *
 * All HAL functions return these standardized error codes.
 * Success is represented by XY_HAL_OK (0), errors are negative values.
 */
typedef enum {
    XY_HAL_OK                  = 0,   /**< Success */
    XY_HAL_ERROR               = -1,  /**< Generic error */
    XY_HAL_ERROR_INVALID_PARAM = -2,  /**< Invalid parameter */
    XY_HAL_ERROR_NOT_SUPPORT   = -3,  /**< Feature not supported */
    XY_HAL_ERROR_TIMEOUT       = -4,  /**< Operation timeout */
    XY_HAL_ERROR_BUSY          = -5,  /**< Resource busy */
    XY_HAL_ERROR_NO_MEM        = -6,  /**< Out of memory */
    XY_HAL_ERROR_IO            = -7,  /**< I/O error */
    XY_HAL_ERROR_NOT_INIT      = -8,  /**< Not initialized */
    XY_HAL_ERROR_ALREADY_INIT  = -9,  /**< Already initialized */
    XY_HAL_ERROR_NO_RESOURCE   = -10, /**< Resource not available */
    XY_HAL_ERROR_FAIL          = -11, /**< Operation failed */
} xy_hal_error_t;

typedef enum {
    XY_HAL_STATUS_OK          = 0,
    XY_HAL_STATUS_ERROR       = -1,
    XY_HAL_STATUS_TIMEOUT     = -2,
    XY_HAL_STATUS_BUSY        = -3,
    XY_HAL_STATUS_NOT_SUPPORT = -4,
    XY_HAL_STATUS_NO_MEM      = -5,
    XY_HAL_STATUS_NOT_INIT    = -6,
} xy_hal_status_t;

typedef struct {
	//strcut xy_obj parent;
	
}xy_hal_t


#include "xy_hal_sys.h"
#include "xy_hal_pin.h"
#include "xy_hal_uart.h"
#include "xy_hal_spi.h"
#include "xy_hal_i2c.h"
#include "xy_hal_timer.h"
#include "xy_hal_pwm.h"
#include "xy_hal_rtc.h"
#include "xy_hal_dma.h"
#include "xy_hal_lp_timer.h"

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_H */