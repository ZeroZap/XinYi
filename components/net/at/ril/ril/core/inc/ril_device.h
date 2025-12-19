/******************************************************************************
 * @brief    ril 设备管理
 *
 * Copyright (c) 2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2020-10-20     Morro        Initial version. 
 ******************************************************************************/
#ifndef _RIL_DEV_H_
#define _RIL_DEV_H_

#include "ril.h"
#include "at.h"
#include "ril_socket_internal.h"
#include "comdef.h"
#include <stddef.h>

#define __ril_urc_register(prefix, end_mark, handler) \
USED ANONY_TYPE(const urc_item_t,__urc_##handler)\
        SECTION("_section.ril.urc.1") UNUSED  =\
        {prefix, end_mark, handler};\

            
#define __ril_device_install(name, dev) \
USED ANONY_TYPE(const ril_device_t ,__ril_dev_##dev)\
        SECTION("_section.ril.dev.1") UNUSED  = { name, &dev, NULL, NULL};

/* Exported types ------------------------------------------------------------*/
struct ril_device;        

/*设备操作接口(operations) ----------------------------------------------------*/
typedef struct ril_device_ops {
    /*基本操作接口 ------------------------------------------------------------*/
    /**
     * @brief   设备开机
     */        
    int (*startup)(struct ril_device *);
    /**
     * @brief   设备关机
     */     
    int (*shutdown)(struct ril_device *);
    /**
     * @brief   设备初始化
     */      
    int (*init)(struct ril_device *);
    /**
     * @brief   复位设备
     */        
    int (*reset)(struct ril_device *);
    /*网络相关操作 ------------------------------------------------------------*/
    /**
     * @brief   创建PDP
     */    
    int (*pdp_setup)(struct ril_device *);
    
    /**
     * @brief     PDP控制
     * @param[in] active - 激活/去激活PDP
     */        
    int (*pdp_contrl)(struct ril_device *, bool active);
    /**
     * @brief         设备请求
     * @param[in/out] data - 参考ril_request_code描述
     * @param[in]     size - data大小
     * @return        RIL_OK - 执行成功, 其它值 - 异常 
     */
    int (*request)(struct ril_device *, ril_request_code code, void *data, int size);                         
    /* socket 相关接口 -------------------------------------------------------*/
    struct {        
        /**
         * @brief     连接服务器         
         * @attention 为了避免在这里停留时间太长，影响其它任务运行，在这里应只发送
         * 连接请求给模组，不需要在这里等待连接结果,因为RIL会定时去查询连接状态,另
         * 外，还可以通过捕获URC事件将连接结果通过ril_socket_nofity上报给RIL
         */ 
        int (*connect) (struct ril_device *, socket_base_t *s);
        /**
         * @brief     断开服务器连接
         */         
        int (*disconnect)(struct ril_device *, socket_base_t *s);   
        
        /**
         * @brief     发送数据
         * @attention 为了避免在这里停留时间太长，影响其它任务运行，在这里应只发送
         * 数据请求给模组，不需要在这里等待发送结果,因为RIL会定时去查询发送状态,另
         * 外，还可以通过捕获URC事件将发送结果通过ril_socket_nofity上报给RIL
         */            
        int (*send)(struct ril_device *r, socket_base_t *s ,const void *buf,
                     unsigned int len); 
        /**
         * @brief     接收数据
         * @attention 一些模组不支持主动读取socket数据,这里可以直接返回0.当收到URC
         *            主动上报来的数据之后,可以通过ril_socket_nofity递交给RIL.
         */         
        unsigned int  (*recv) (struct ril_device *, socket_base_t *s, void *buf,
                               unsigned int len); 
        /**
         * @brief     获取连接状态
         */         
        sock_request_status (*conn_status)(struct ril_device *, socket_base_t *);
        /**
         * @brief     获取发送状态
         */         
        sock_request_status (*send_status)(struct ril_device *, socket_base_t *);
    } sock;
} ril_device_ops_t;

/*ril设备 --------------------------------------------------------------------*/        
typedef struct ril_device {
    char                   *name;                   /* 设备名称 */
    const ril_device_ops_t *ops;                    /* 设备操作接口 */
    ril_adapter_t          *adap;                   /* 接口适配器 */   
    at_obj_t               *at;                     /* AT通信控制器 */    
    ril_config_t           *config;                 /* 设备配置参数*/
}ril_device_t;

/**
 * @brief   注册URC接收处理程序
 * @param   prefix   - urc 串前缀(如"+CSQ:"),默认"\n"作为结束符
 * @param   handler  - urc接收处理程序
 *              void (*handler)(at_urc_ctx_t *ctx);
 */
#define ril_urc_register(prefix, handler) \
__ril_urc_register(prefix, "\n", handler)

/**
 * @brief   注册URC接收处理程序(支持指定结束标记)
 * @param   prefix   - urc 串前缀(如"+CSQ:")
 * @param   end_mark - 结束标记(支持[+,\r\n : "]，如果不指定则默认为"\r\n")
 * @param   handler  - urc接收处理程序
 *              void (*handler)(at_urc_ctx_t *ctx);
 * @attention   URC码必须是唯一的,当前版本还不能处理不同设备同一个URC的情况
 */
#define ril_urc_ex_register(prefix, end_mark, handler) \
__ril_urc_register(prefix, end_mark, handler)

/**
 * @brief   安装RIL设备
 * @param   name       - 设备名称(与ril_use_device指定名称匹配)
 * @param   operations - 设备操作方法集(类型:ril_device_ops_t)
 */
#define ril_device_install(name, operations) \
__ril_device_install(name, operations)

/**
 * @brief        ril默认请求处理
 * @param[in]    code - 请求码
 * @param[inout] data - 数据内容,参考ril_request_code描述
 * @param[in]    size - data大小
 */
int ril_request_default_proc(struct ril_device *dev, ril_request_code code, 
                              void *data, int size);

/**
 * @brief       ril通知(一般用于URC事件)
 * @param[in]   type - 通知类型
 *                  @arg  RIL_NOTIF_SIM           SIM 卡状态更新
 *                  @arg  RIL_NOTIF_NETSTAT       注册状态更新 
 *                  @arg  RIL_NOTIF_NETCONN       连接状态更新
 *                  @arg  RIL_NOTIF_SMS           收到短信
 *                  ...
 *
 * @param[in]   data       - 事件数据,参考ril_notify_type描述
 * @param[in]   data_size  - 事件数据长度
 * 
 */
int ril_notify(ril_notify_type type, void *data, int data_size);

int ril_exec_cmd(char *recvbuf, int bufsize, const char *fmt, ...);

int ril_exec_cmdx(at_respond_t *resp, const char *fmt, ...);

int ril_send_singleline(const char *singleline);

int ril_send_multiline(const char **multiline);

#endif
