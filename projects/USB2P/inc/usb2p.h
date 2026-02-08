#ifndef _USB2P_H_
#define _USB2P_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define usb2p_gen_id(port_type, port_num) (((port_type) << 8) | (port_num))
#define usb2p_id_type(id)                 ((id) >> 8)
#define usb2p_id_num(id)                  ((id) & 0xff)

// 接收数据处理
typedef int32_t (*data_process)(char *pdata, uint16_t len);
int32_t usb2p_register(uint16_t id, uint16_t max_size, data_process pfn);

int32_t usb2p_unregister(uint16_t id);
int32_t usb2p_status(void);
void usb2p_transmit_process(void *arg);
void usb2p_receive_process(void *arg);

/**
 * @brief send message to usb2p processor
 *
 * @param id
 * @param pdata
 * @param len
 * @return int32_t
 */
int32_t usb2p_send(uint16_t id, char *pdata, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* _USB2P_H_ */