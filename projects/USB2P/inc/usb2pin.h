#ifndef _USB2PIN_H_
#define _USB2PIN_H_
#define USB2PIN_MAX 16
#ifdef __cplusplus
extern "C" {
#endif
int usb2pin_init(uint8_t channel, void *param, uint16_t len, uint16_t timeout);
int usb2pin_deinit(uint8_t channel);
int usb2pin_read(uint8_t channel, uint8_t *pdata, uint16_t len,
                 uint16_t timeout);
int usb2pin_write(uint8_t channel, uint8_t *pdata, uint16_t len,
                  uint16_t timeout);
#ifdef __cplusplus
}
#endif
#endif
