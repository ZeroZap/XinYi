#ifndef _USB2P_PORT_H_
#define _USB2P_PORT_H_

struct usb2p_ops {
    int (*init)(void);
    int (*deinit)(void);
    int (*read)(void *data, uint16_t len, uint16_t timeout);
    int (*write)(void *data, uint16_t len, uint16_t timeout);
    int (*status)(void);
};
typedef struct usb2p_ops *usb2p_ops_t;

#endif
