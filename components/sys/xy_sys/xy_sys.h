#ifndef _XY_SYS_H_
#define _XY_SYS_H_

/** Retained reset-request reason codes for xy_sys_reset().
 *
 * The value passed to xy_sys_reset() is retained across the reset so the next
 * boot can report why the board restarted. Board backends that retain the code
 * must be able to store values in 0..255. */
#define XY_SYS_RESET_REQUEST_UNSPECIFIED 0
#define XY_SYS_RESET_REQUEST_APP_RESTART 1
#define XY_SYS_RESET_REQUEST_FOTA_HANDOFF 2
#define XY_SYS_RESET_REQUEST_RECOVERY 3

void xy_sys_init(void);

/** Reset the system, retaining @p reset_by as the request reason.
 *
 * @return XY_OK when the request was recorded and the reset was issued;
 *         XY_ERROR_INVALID_PARAM for a negative or non-retainable reason;
 *         XY_ERROR when recording or the reset request failed.
 */
int xy_sys_reset(int reset_by);

int xy_sys_reboot_reason(void *data);

/** Report the reason retained by the previous xy_sys_reset() call.
 *
 * @param[out] data uint32_t reason; XY_SYS_RESET_REQUEST_UNSPECIFIED when the
 *                  previous boot was not a SYS-requested reset.
 * @return XY_OK, XY_ERROR_INVALID_PARAM, or XY_ERROR_NOT_SUPPORTED.
 */
int xy_sys_reset_request(void *data);

int xy_sys_get_chip_id(void *data);

int xy_sys_get_sw_ver(void *data);

int xy_sys_get_hw_ver(void *data);

int xy_sys_get_mac_addr(void *data);


#endif
