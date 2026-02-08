#include "usb2uart.h"

typedef struct {
    uint16_t len;
    uint8_t type;
    uint8_t flag;
    uint8_t *payload;
} usbp2p_payload;

typedef enum {
    usb2uart_init = 0,
    usb2uart_deinit,
    usb2uart_cfg,
    usb2uart_data,
} usb2uart_type_t;