/******************************************************************************
 * @brief        基于gsm0710协议虚拟串口管理
 *
 * Copyright (c) 2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2020-06-20     Morro        Initial version. 
 ******************************************************************************/
#ifndef _VCOM_H_
#define _VCOM_H_

#include <comdef.h>
#include <stdbool.h>


#define VCOM_DBG(...)                printf(__VA_ARGS__)

/*虚拟串口服务配置 -----------------------------------------------------------*/
typedef struct {
    unsigned int (*read)(void *buf, unsigned int size);
    unsigned int (*write)(const void *buf, unsigned int size);
    unsigned int (*get_ms)(void);
    void         (*error)(void);
}vcom_config_t;

typedef struct vcom {
    const char *name;    
    unsigned char channel;
    struct {
        void (*recv)(const void *buf, int count);   //数据接收事件
    }e;    
    unsigned int  timer;                            //定时器 
    unsigned char busy;
    /*Public  --------------------------------------------------------------*/
	void (*open)(struct vcom *this);    
    void (*close)(struct vcom *this);
    void (*reopen)(struct vcom *this);
	bool (*send)(struct vcom *v, const void *buf, int count);
    
	/*Property  ------------------------------------------------------------*/
	bool (*isopen)(struct vcom *this);            //指示是否已打开    
}vcom_t;

#define __vcom_declare(name, channel, recv_event) \
RESERVE vcom_t __vcom_##channel\
        SECTION("_section_vcom") UNUSED  =\
        {#name, channel, .e.recv = recv_event};\
static vcom_t *name = &__vcom_##channel

/*
 * @brief	 声明虚拟串口
 * @param    name       - 串口名称
 * @param    recv_event - 数据接收事件
 *               @arg (void (*recv)(unsigned char *buf, size_t count))
 */
#define vcom_declare(name, channel, recv_event)\
__vcom_declare(name, channel, recv_event)


void vcom_service_init(const vcom_config_t *config);

void vcom_service_start(void);

void vcom_service_stop(void);

void vcom_service_thread(void);

#endif
