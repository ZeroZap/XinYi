/*******************************************************************************
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

#ifndef _RIL_H_
#define _RIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ril_port.h"
#include "ril_types.h"
#include <stdbool.h>
#include <stdio.h>

/*debug ----------------------------------------------------------------------*/
#define RIL_DBG(...)      ril_log(RIL_LOG_DBG, __VA_ARGS__)
#define RIL_INFO(...)     ril_log(RIL_LOG_INFO, __VA_ARGS__)
#define RIL_WARN(...)     ril_log(RIL_LOG_WARN, __VA_ARGS__)
#define RIL_ERR(...)      ril_log(RIL_LOG_ERR, __VA_ARGS__)

/**
 * @brief    订阅 ril 通知
 * @param    type     - 通知类型(参考ril_notify_type定义)    
 * @param    handler  - 通知处理函数,类型: void (*)(void *data, int data_size))
 *                      其中data与data_size域的作用参考ril_notify_type对应说明
 */     
#define ril_on_notify(type, handler)\
__ril_on_notify(type, handler)

void ril_init(ril_adapter_t *adt, ril_config_t *cfg);      

int ril_use_device(const char *name);                             /* 选择设备 */

void ril_open(void);

void ril_close(void);

void ril_netconn(bool enable);

//void ril_set_config(ril_config_t *cfg);

//void ril_get_config(ril_config_t *cfg);

ril_status_t *ril_get_status(void); 

bool ril_device_ready(void);

bool ril_isonline(void);

int ril_request(ril_request_code code, void *data, int size);

int ril_sms_send(const char *phone, const char *msg);

void ril_main_task(void);

void ril_atcmd_task(void);

#ifdef __cplusplus
}
#endif

#endif
