/******************************************************************************
 * @brief   短信管理
 *
 * Copyright (c) 2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2020-09-22     Morro        Initial version. 
 ******************************************************************************/
#ifndef _SMS_H_

#include "ril_core.h"
#include "ril_device.h"

/*pdu 编码方式 ---------------------------------------------------------------*/
#define PDU_ENCODE_7BIT               1
#define PDU_ENCODE_8BIT               2

int sms_init(struct ril_device *r);
int sms_send(struct ril_device *r, const char *phone, const char *msg);
int sms_recv(struct ril_device *r, char *phone, char *msg, int size);

#endif

