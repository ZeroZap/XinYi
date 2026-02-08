#include <stdio.h>
#include <string.h>
#include "usb2p.h"
#include "usb2p_cfg.h"
#include "usb2p_def.h"

typedef struct {
    uint16_t id; /**< must be(usb2p_type_t<<8)& port_num. */
    uint16_t len;
    char *pdata;
} usb2p_pkg_t;


typedef struct usb2p {
    struct usb2p *next;
    uint16_t id;       /**< must be(usb2p_type_t<<8)& port_num. */
    uint16_t max_size; /**< max pkg size, not include header. */
    data_process pfn;
    usb2p_pkg_t pkg; // 修改为结构体而非指针
} usb2p_t;

typedef struct {
    usb2p_t *head;
    uint16_t active_num;
} usb2p_manager_t;

static usb2p_manager_t usb2p_mgr = { 0 };

int32_t usb2p_register(uint16_t id, uint16_t max_size, data_process pfn)
{
    printf("insert id = %d\r\n", id);
    usb2p_t *node = usb2p_mgr.head;
    while (node) {
        if (node->id == id) {
            return usb2p_error_busy;
        }
        node = node->next;
    }
    node = (usb2p_t *)malloc(sizeof(usb2p_t));
    if (!node)
        return usb2p_error_init_failed;
    node->id       = id;
    node->max_size = max_size;
    node->pfn      = pfn;
    node->next     = usb2p_mgr.head;
    usb2p_mgr.head = node;
    usb2p_mgr.active_num++;
    return usb2p_error_none;
}

int32_t usb2p_unregister(uint16_t id)
{
    usb2p_t *prev = NULL, *node = usb2p_mgr.head;
    while (node) {
        if (node->id == id) {
            if (prev)
                prev->next = node->next;
            else
                usb2p_mgr.head = node->next;
            if (node->pkg.pdata)
                free(node->pkg.pdata);
            free(node);
            usb2p_mgr.active_num--;
            return usb2p_error_none;
        }
        prev = node;
        node = node->next;
    }
    return usb2p_error_none;
}

int32_t usb2p_send(uint16_t id, char *pdata, uint16_t len)
{
    // 加入到消息队列
    return usb2p_error_none;
}

int32_t usb2p_recv(uint16_t id, char *pdata, uint16_t len)
{
    usb2p_t *node = usb2p_mgr.head;
    printf("total id: %d\r\n", usb2p_mgr.active_num);
    printf("active id: %d\r\n", node->id);
    printf("active id max size: %d\r\n", node->max_size);
    while (node) {
        if (node->id == id) {
            if (node->pfn) {

                return node->pfn(pdata, len);
            }
        }
        node = node->next;
    }
    return usb2p_error_invalid_param;
}

void usb2p_send_process(void *arg)
{
    // 执行 USB 发送
    // send_data(pdata, len);
}

void usb2p_receive_process(void *arg)
{
    // get from USB data 接收完整的 TLV 或者 分割 TLV 数据
    // usb2p_recv(uint16_t id, char *pdata, uint16_t len)
}