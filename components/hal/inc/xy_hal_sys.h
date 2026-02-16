#ifndef __XY_HAL_SYS_H__
#define __XY_HAL_SYS_H__

#ifdef __cplusplus
extern "C" {
#endif


xy_u32_t xy_tick_increase(void);
xy_u32_t xy_ticks_now(void);
xy_u32_t xy_ticks_set(xy_u32_t tick);
xy_u32_t xy_ticks_get(void);
xy_u32_t xy_ticks_get_from_isr(void);
xy_u32_t xy_ticks_since(xy_u32_t tick);


int xy_hal_sys_reset(int reset_by);

int xy_hal_sys_reboot_reason(void *data);

int xy_hal_sys_get_chip_id(void);

int xy_hal_sys_get_chip_ver(void);

int xy_hal_sys_get_chip_name(char *name, int len);

int xy_hal_sys_get_chip_mac(unsigned char *mac);

int xy_hal_sys_get_chip_mac_str(char *mac);

int xy_hal_sys_get_chip_mac_str_len(void);

int xy_hal_sys_get_chip_mac_str_hex(char *mac);

int xy_hal_sys_get_chip_mac_hex(unsigned char *mac);

int xy_hal_sys_get_chip_mac_hex_len(void);

int xy_hal_sys_get_chip_mac_hex_str(char *mac);

#ifdef __cplusplus
}
#endif

#endif