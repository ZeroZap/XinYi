/******************************************************************************
 * @brief    ril socket
 *
 * Copyright (c) 2020~2021, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2020-10-20     Morro        Initial version. 
 * 2021-04-23     Morro        Fix the issue of not exiting the critical region 
 *                             correctly when allocating Socket id.
 * 2021-05-04     Morro        Fix the issue of repeatedly closing the socket 
 *                             when the remote host is disconnected.
 * 2021-12-08     Morro        Fix the problem of receiving abnormal data under 
 *                             multitasking system
 ******************************************************************************/

#ifndef _RIL_SOCK_H_
#define _RIL_SOCK_H_


/*默认socket 接收缓冲区大小 ---------------------------------------------------*/
#define    DEF_SOCK_RECV_BUFSIZE           128           /* 默认接收缓冲区大小*/
#define    MAX_SOCK_CONN_TIME              120           /* 最大连接时间(s)*/
#define    MAX_SOCK_SEND_TIME              120           /* 最大发送时间(s)*/

#define    RIL_INVALID_SOCKET              0

/**
 * @brief socket 描述符
 */
typedef long ril_socket_t;

/*socket 类型 ----------------------------------------------------------------*/
typedef enum {
    RIL_SOCK_TCP = 0,                                    /* TCP */
    RIL_SOCK_UDP                                         /* UDP */
}ril_socket_type;

/**
 * @brief socket (发送/连接)状态
 */
typedef enum {
    SOCK_STAT_UNKNOW,                                     /* 未知 */
    SOCK_STAT_BUSY,                                       /* 进行中 */
    SOCK_STAT_DONE,                                       /* 请求完成 */
    SOCK_STAT_FAILED,                                     /* 请求失败 */
    SOCK_STAT_TIMEOUT,                                    /* 请求超时 */
    SOCK_STAT_MAX
} sock_request_status;

/**
 * @brief socket 事件类型定义
 */
typedef enum {
    /**
     * @brief       连接状态更新(可以通过ril_sock_connstat 获取连接状态)
     */    
    SOCK_EVENT_CONN = 0,                                                   
    /**
     * @brief       数据发送状态更新(可以通过ril_sock_sendstat 获取发送状态)
     */     
    SOCK_EVENT_SEND,
    /**
     * @brief       数据接收事件(可以通过ril_sock_recv 接口读取数据)
     */   
    SOCK_EVENT_RECV
        
} socket_event_type;

/**
 * @brief socket 事件
 */
typedef void (*socket_event_t)(ril_socket_t s, socket_event_type type);

ril_socket_t ril_sock_create(socket_event_t e, unsigned int bufsize);

void ril_sock_destroy(ril_socket_t s);

int ril_sock_connect(ril_socket_t s, const char *host, unsigned short port, 
                     ril_socket_type type);
int ril_sock_send(ril_socket_t s, const void *buf, unsigned int len);

/** 非阻塞接口 ---------------------------------------------------------------*/
unsigned int ril_sock_recv(ril_socket_t s, void *buf, unsigned int len);

int ril_sock_connect_async(ril_socket_t s, const char *host, 
                           unsigned short port, ril_socket_type type);
int ril_sock_send_async(ril_socket_t s, const void *buf, unsigned int len);

int ril_sock_disconnect(ril_socket_t s);

/** socket 状态相关 ----------------------------------------------------------*/
bool ril_sock_online(ril_socket_t s);

bool ril_sock_busy(ril_socket_t s);

sock_request_status ril_sock_connstat(ril_socket_t s);

sock_request_status ril_sock_sendstat(ril_socket_t s);

#endif

