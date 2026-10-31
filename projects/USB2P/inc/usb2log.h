#ifndef _USB2LOG_H_
#define _USB2LOG_H_
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

int usb2log_init(uint8_t channel, void *param, uint16_t len, uint16_t timeout);
int usb2log_deinit(uint8_t channel);
int usb2log_read(uint8_t channel, uint8_t *pdata, uint16_t len);
int usb2log_write(uint8_t channel, uint8_t *pdata, uint16_t len);
#ifdef __cplusplus
}
#endif
#endif