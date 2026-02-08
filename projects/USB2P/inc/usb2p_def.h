#ifndef _USB2P_COMMON_H_
#define _USB2P_COMMON_H_
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    usb2p_type_none = 0,
    usb2p_type_gpio,
    usb2p_type_uart,
    usb2p_type_i2c,
    usb2p_type_spi,
    usb2p_type_pwm,
    usb2p_type_adc,
    usb2p_type_sensor,
    usb2p_type_log = 0xfe,
    usb2p_type_cfg = 0xff, /**< usb2p_cfg: register id. */
} usb2p_type_t;

typedef struct {
    uint8_t port_type;
    uint8_t port_num;
    uint16_t len;
    uint8_t *pdata;
} usb2p_t;


typedef enum {
    usb2p_error_none = 0,
    usb2p_error_busy,
    usb2p_error_supported,
    usb2p_error_invalid_param,
    usb2p_error_over_length,
    usb2p_error_no_buffer_space,
    usb2p_error_init_failed,
} usb2p_error_t;


#ifdef __cplusplus
}
#endif
#endif