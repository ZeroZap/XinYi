/******************************************************************************
 * @brief    ril 设备默认接口实现
 *
 * Copyright (c) 2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2020-10-20     Morro        Initial version. 
 ******************************************************************************/

#ifndef _RIL_DEV_COMMON_H_
#define _RIL_DEV_COMMON_H_

#include "ril_device.h"

/** 
 * @brief       启动模组
 */ 

bool __ril_comm_startup(struct ril_device *);

/** 
 * @brief       模组初始化
 */ 
bool __ril_comm_init(struct ril_device *);

/** 
 * @brief       关闭模组
 */ 
bool __ril_comm_shutdown(struct ril_device *);

/** 
 * @brief       复位模组
 */ 
bool __ril_comm_reset(struct ril_device *);

/** 
 * @brief       唤醒模组
 */ 
bool __ril_comm_wakeup(struct ril_device *);

/** 
 * @brief       模组休眠
 */
bool __ril_comm_sleep(struct ril_device *);

/** 
 * @brief       建立网络环境
 */
bool __ril_comm_pdp_setup(struct ril_device *);

/**
 * @brief     pdp控制
 * @param[in] active - 激活/去激活PDP
 */ 
bool __ril_comm_pdp_ctrl(struct ril_device *r, bool active);

/** 
 * @brief       ril请求
 */
bool __ril_comm_request(struct ril_device *, ril_request_code num, void *data, int size);

#endif
