/******************************************************************************
 * @brief    无线接口层(Radio Interface Layer)
 *
 * Copyright (c) 2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2020-05-27     Morro        Initial version. 
 ******************************************************************************/

#ifndef _RIL_CORE_H_
#define _RIL_CORE_H_

#include "ril.h"
#include "at.h"
#include "ril_device.h"
#include "async_work.h"
#include <stdbool.h>

void ril_log(int level, const char *fmt, ...);

/*RIL请求类型 ----------------------------------------------------------------*/
typedef enum {
    RIL_REQ_OPEN,                                      /* 开机 */
    RIL_REQ_SLEEP,                                     /* 休眠 */
    RIL_REQ_CONN                                       /* 网络连接 */
}ril_request_type;

/*ril 状态 -------------------------------------------------------------------*/
typedef enum {
    RIL_STAT_OFF = 0,                                  /* 已关机 */    
    RIL_STAT_RESET,                                    /* 复位中 */  
    RIL_STAT_STARTUP,                                  /* 正在启动 */
    RIL_STAT_SIM,                                      /* 检查sim卡 */  
    RIL_STAT_INIT,                                     /* 初始化 */          
    RIL_STAT_NETREG,                                   /* 注册状态 */
    RIL_STAT_NETCONF,                                  /* 网络配置 */
    RIL_STAT_CONN,                                     /* 建立网络连接 */
    RIL_STAT_ONLINE,                                   /* 已上线,允许建立socket*/
    RIL_STAT_CALL,                                     /* 通话中 */
    RIL_STAT_ERR,                                      /* 异常状态 */
    RIL_STAT_MAX
}ril_run_state;

/*运行上下文 ------------------------------------------------------------------*/
typedef struct {
    struct ril_obj *r;                                /* ril对象 */
    unsigned int  start_time;                         /* 记录当前状态起始时间 */
    unsigned int  timer;                              /* 通用定时器 */
    unsigned int  retry;                              /* 重试次数 */
    bool         (*istimeout)(unsigned int ms);       /* 通用定时器超时判断 */
    void         (*reset_timer)(void);                /* 重置通用定时器 */
    unsigned int (*runtime)(void);                    /* 当前状态下运行总时间 */
    void         (*next_state)(ril_run_state state);  /* 跳转到下一状态 */
}ril_ctx_t;

/*ril对象 --------------------------------------------------------------------*/
typedef struct ril_obj {
    async_work_t       workqueue;                     /* 异步作业*/
    work_node_t        work_node[8];                  /* 作业队列结点*/
    ril_adapter_t      adt;                           /* 适配器*/
    ril_device_t       dev;                           /* ril设备*/
    at_obj_t           at;                            /* AT通信控制器*/
    ril_ctx_t          ctx;
    ril_config_t       cfg;
    ril_status_t       status;
    ril_run_state      run_state;                     /* 运行状态*/       
    unsigned           error        : 1;              /* 异常*/
    unsigned           issleep      : 1;              /* 休眠状态*/       
    unsigned           isopen       : 1;              /* 开机状态*/
    unsigned           open_enable  : 1;              /* 开机使能*/
    unsigned           sleep_enable : 1;              /* 休眠使能*/
    unsigned           conn_enable  : 1;              /* 网络连接使能*/
}ril_obj_t;


ril_obj_t *get_ril_obj(void);

bool ril_isreg(void);

const ril_device_t *find_device(const char *name);

/**
 * @brief  异步任务类型定义
 */
typedef void (*ril_async_work_t)(void *nullptr, ril_obj_t *r, void *params); 

void ril_core_init(ril_obj_t *r);

void ril_core_process(ril_obj_t *r);

bool ril_do_async_work(void *params, ril_async_work_t work);

#endif
