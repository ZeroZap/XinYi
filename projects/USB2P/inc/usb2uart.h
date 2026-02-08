#ifndef _USB2UART_H_
#define _USBUART_H_
#ifdef __cplusplus
extern "C" {
#endif

int usb2uart_init(uint8_t channel, void *param, uint16_t len, uint16_t timeout);
int usb2uart_deinit(uint8_t channel);
int usb2uart_read(uint8_t channel, uint8_t *pdata, uint16_t len);
int usb2uart_write(uint8_t channel, uint8_t *pdata, uint16_t len);
int usb2uart_status(uint8_t channel, uint8_t *pdata, uint16_t len)

#ifdef __cplusplus
}
#endif
#endif