/******************************************************************************
 * @brief    Socket related interface definition.
 *
 * Copyright (c) 2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2020-09-14     Morro        Initial version. 
 ******************************************************************************/
#ifndef _RIL_SOCK_INTERNAL_H_
#define _RIL_SOCK_INTERNAL_H_

#include "ril_socket.h"

/**
 * @brief     Socket 通知类型定义.
 */
typedef enum {
    /**
     * @brief       Online (successfully connected to server.)
     */    
    SOCK_NOTFI_ONLINE = 0,
    /**
     * @brief       由于外部原因造成的离线(如服务器主动断开)
     */     
    SOCK_NOTFI_OFFLINE,
    /**
     * @brief       数据发送失败
     */     
    SOCK_NOTFI_SEND_FAILED,
    /**
     * @brief       数据发送成功
     */     
    SOCK_NOTFI_SEND_SUCCESS,
    /**
     * @brief       模组收到数据,等待读取(适用于主动读取数据的模组)
     * @param[in]   data -> [int unread_data],剩余待读取数据长度(如果未知,填0)
     */     
    SOCK_NOTFI_DATA_INCOMMING,
    /**
     * @brief       模组数据上报(适用于主动上报数据的模组)
     * @param[in]   data -> [unsigned char *buf],数据缓冲区
     * @param[in]   size -> 数据长度
     */   
    SOCK_NOTFI_DATA_REPORT
}sock_notify_type;

/* socket 基本信息 -----------------------------------------------------------*/
typedef struct {
    const char       *host;                            /* 远程服务器主机*/
    unsigned short   port;                             /* 远程服务器端口*/
    ril_socket_type  type;                             /* socket类型*/
    unsigned char    id;                               /* id索引,创建时自动分配*/
    void             *tag;                             /* 附属数据 */
} socket_base_t;

/**
 * @brief  socket 通知
 */
void ril_socket_notify(socket_base_t *s, sock_notify_type type, void *data, int size);

/**
 * @brief       为socket设置附属数据
 * @param[in]   s       - socket
 * @param[in]   tag     - 附属数据
 */
void set_socket_tag(socket_base_t *s, void *tag);

/**
 * @brief  通过id查询socket
 */
socket_base_t *find_socket_by_id(int id);

/**
 * @brief  通过tag查询socket
 */
socket_base_t *find_socket_by_tag(void *tag);

#endif
