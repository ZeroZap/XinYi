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
#include "ril_device.h"
#include "ril_device_impl.h"
#include "ril_core.h"
#include <string.h>
#include <stdarg.h>

/**
 * @brief       查找设备
 * @param[in]   name - 设备名称
 */
const ril_device_t *find_device(const char *name)
{
    static const ril_device_t dev_tbl_start SECTION("_section.ril.dev.0") = {0};
    static const ril_device_t dev_tbl_end   SECTION("_section.ril.dev.2") = {0};
    const ril_device_t *dev;
    for (dev = &dev_tbl_start + 1; dev < &dev_tbl_end; dev++) {
        if (strcasecmp(dev->name, name) == 0) 
            return dev;
    }
    return NULL;
}

static int __do_at_cmd(at_respond_t *respond, const char *fmt, va_list args)
{
#define  MAX_SEND_LINE    512
    char     *line;                                    //发送行
    int       i = 0;
    at_return ret;
    ril_obj_t *r = get_ril_obj();
    if (r->error) {
        RIL_WARN("Device error.\r\n");
        return RIL_REJECT;                              //设备异常
    }
    line = (char *)ril_malloc(MAX_SEND_LINE);
    if (line == NULL) {
        RIL_ERR("Mem malloc failed when exec cmd.\r\n");
        return RIL_NOMEM;
    }  
    vsnprintf(line, MAX_SEND_LINE, fmt, args);
    do {
        ret = at_do_cmd(&r->at, respond, line);
    }while (++i < 2 && ret == AT_RET_TIMEOUT);    //超时响应,重发
    
    ril_free(line);
    
    //AT超时处理
    if (ret == AT_RET_TIMEOUT) {
        ril_notify(RIL_NOTIF_TIMEOUT, NULL, 0);
        return RIL_TIMEOUT;
    } else 
        return ret == AT_RET_OK ? RIL_OK : RIL_ERROR; 
}

/**
 * @brief      执行AT命令(默认等待OK,超时3S)
 * @param[in]  recvbuf - 接收缓冲区(如果不需要响应数据则填写NULL)
 * @param[in]  bufsize - 缓冲区大小(如果不需要响应数据则填写0)
 * @param[in]  fmt     - 格式化打印串
 *
 * @return     RIL_OK - 执行成功, Others - 执行超时或者失败
 *
 */
int ril_exec_cmd(char *recvbuf, int bufsize, const char *fmt, ...)
{
    va_list   args;
    int ret;
    at_respond_t respond = {"OK", recvbuf, bufsize, 3000};    
    va_start(args, fmt);       
    ret = __do_at_cmd(&respond, fmt, args);    
    va_end(args);
    return ret;
}

/**
 * @brief      执行AT命令(默认等待OK返回)
 * @param[in]  recvbuf - 接收缓冲区(如果不需要响应数据则填写NULL)
 * @param[in]  bufsize - 缓冲区大小(如果不需要响应数据则填写0)
 * @param[in]  timeout - 命令超时时间(ms)
 * @param[in]  fmt     - 格式化打印串
 *
 * @return     RIL_OK - 执行成功, Others - 执行超时或者失败
 *
 */
int ril_exec_cmdx(at_respond_t *resp, const char *fmt, ...)
{
    va_list   args;
    int ret;
    
    va_start(args, fmt);  
    
    ret = __do_at_cmd(resp, fmt, args);
    
    va_end(args);
    
    return ret;
}

/**
 * @brief      发送单行(默认等待OK返回, 3000 ms超时)
 * @param[in]  singleline - 单行命令
 * @return     RIL_OK - 执行成功, Others - 执行超时或者失败
 */
int ril_send_singleline(const char *singleline)
{
    return ril_exec_cmdx(NULL, singleline);
}

/**
 * @brief     发送多行
 * @param[in] multiline - 多行命令, 以NULL做为结尾
 * @return    true - 执行成功, false - 执行超时或者失败
 * @attention 如果有一些命令执行失败，它并不会立即退出，而是接着往下执行。
 */
int ril_send_multiline(const char **multiline)
{
    int ret = false;
    while (*multiline) {
        ret = ril_send_singleline(*multiline++);
        if (ret != RIL_OK)
            at_delay(200);
    }
    return ret;
}
