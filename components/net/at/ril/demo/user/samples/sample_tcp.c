/******************************************************************************
 * @brief    RIL TCP演示程序
 *
 * Copyright (c) 2020  <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2021-02-05     Morro        Initial version
 ******************************************************************************/
#include "ril.h"
#include "ril_socket.h"
#include "taskManager.h"
#include <stdio.h>
#include <string.h>

#define TCP_SERVER    "123.45.67.88"      //服务器地址
#define TCP_PORT      1234                //端口

/**
 * @brief       socket 事件处理
 * @return      none
 */ 
static void socket_event(ril_socket_t s, socket_event_type type)
{   
    switch (type) {
    case SOCK_EVENT_CONN:
        if (!ril_sock_online(s))
            printf("TCP connection lost...\r\n");
        break;
    default:
        break;
    }
}

/**
 * @brief       定时发送数据
 * @return      none
 */ 
static void send_data_regularly(ril_socket_t sockfd)
{
    char buf[128];
    int  sendcnt;    
    static unsigned int timer;                   //发送定时器
    //大约1min发送1条数据
    if (ril_istimeout(timer, 60 * 1000)) {
        timer = ril_get_ms();
        sendcnt = snprintf(buf,sizeof(buf), "TCP socket send tests");
        if (ril_sock_send(sockfd, buf, sendcnt) == RIL_OK)
            printf("TCP send successfully.\r\n");
        else 
            printf("TCP send failed.\r\n");             
    }
}

/** 
 * @brief       数据接收处理
 * @return      none
 */ 
static void recv_data_process(ril_socket_t sockfd)
{
    char buf[128];                                
    int  recvcnt;    
    //数据接收处理
    do {
        recvcnt = ril_sock_recv(sockfd, buf, sizeof(buf) - 1);
        if (recvcnt) {
            printf("Receive %d bytes from %s.\r\n", recvcnt, TCP_SERVER); 
            buf[sizeof(buf) - 1] = '\0';
            printf("%10s...\r\n", buf);
        }
    } while (recvcnt);    
}

/**
 * @brief       tcp 任务
 * @return      none
 */ 
static void tcp_task(void)
{
    int  retry = 0;
    bool result;
    ril_socket_t sockfd;                          //socket
    sockfd = ril_sock_create(socket_event, 512);  //创建socket，并设置512 Bytes的接收缓冲区
    printf("TCP socket create %s.\r\n", sockfd != RIL_INVALID_SOCKET ? "OK" : "ERR");
    if (sockfd == 0) {
        return;
    }
    while (1) {
        os_sleep(10);
        if (!ril_isonline())                       //等待网络连接
            continue;
        if (!ril_sock_online(sockfd)) {             
            result = ril_sock_connect(sockfd, TCP_SERVER, TCP_PORT, RIL_SOCK_TCP);
            if (result != RIL_OK) {
                retry %= 10;
                os_sleep(10000 * retry++);        //连续失败等待一段时间再尝试
            } 
            printf("TCP Socket connect %s.\r\n", result == RIL_OK ? "OK" : "ERR");
        } else {
           recv_data_process(sockfd);             //数据接收处理
           send_data_regularly(sockfd);           //数据发送处理
        }
    }
}
task_define("tcp-sample", tcp_task, 256, 6);     //定义TCP任务
