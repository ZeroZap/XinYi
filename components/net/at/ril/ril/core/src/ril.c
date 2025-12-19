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
#include "sms.h"
#include "ril_core.h"
#include "ril_device.h"
#include "ril_device_impl.h"
#include "ril_port.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

static ril_obj_t robj;

/**
  * @brief ril对象
  */
ril_obj_t *get_ril_obj(void)
{
    return &robj;
}

/**
 * @brief 超时判断
 */
static bool env_istimeout(unsigned int ms)  
{
    return ril_istimeout(robj.ctx.timer, ms);
}
/**
 * @brief 重置定时器
 */
static void env_reset_timer(void)  
{
    robj.ctx.timer  = ril_get_ms();
}

/**
 * @brief 运行时间
 */
unsigned int env_runtime(void)
{
    return ril_get_ms() - robj.ctx.start_time;
}

/**
 * @brief       跳转到下一状态
 */
void env_next_state(ril_run_state state)
{
    robj.ctx.start_time  = ril_get_ms();
    robj.ctx.timer = ril_get_ms();
    robj.ctx.retry = 0;
    RIL_DBG("Run state switch from %d to %d.\r\n", robj.run_state, state);
    robj.run_state = state;
}

/**
 * @brief       Debug信息
 * @param[in]   fmt - 格式串
 */
static void at_debug(const char *fmt, ...)
{
#define  MAX_DBG_LINE    512
    char     *line;                           
    va_list args;	
    line = ril_malloc(MAX_DBG_LINE);
    if (line == NULL)
        return;
   
    va_start(args, fmt);
    vsnprintf(line, MAX_DBG_LINE, fmt, args);
    RIL_DBG("%s", line);
    va_end(args);

    ril_free(line);
}
/**
 * @brief       ril设备初始化
 */
void ril_device_init(ril_obj_t *r, struct ril_device *d)
{
    d->adap  = &r->adt;
    d->at    = &r->at;
    d->config= &r->cfg;
}

/**
 * @brief       ril初始化
 * @param[in]   adt - 接口适配器
 * @param[in]   cfg - 配置参数
 */
void ril_init(ril_adapter_t *adt, ril_config_t *cfg)
{
    static const urc_item_t urc_tbl_start SECTION("_section.ril.urc.0") = {"",0};
    static const urc_item_t urc_tbl_end   SECTION("_section.ril.urc.2") = {"",0};    
    static char urc_buf[128];
    const ril_device_t *device;
    at_adapter_t atcfg = {                                    /**AT配置参数*/
        .urc_buf     = urc_buf,
        .urc_bufsize = sizeof(urc_buf),
        .utc_tbl     = (urc_item_t *)(&urc_tbl_start + 1),
        .urc_tbl_count = &urc_tbl_end - &urc_tbl_start - 1,
    };
    
    robj.adt    = *adt;
    atcfg.write = adt->write;
    atcfg.read  = adt->read;
    atcfg.debug = at_debug;
    at_obj_init(&robj.at, &atcfg);                          /**AT控制器初始化*/   
    
    //初始化env
    robj.ctx.r = &robj;                                            
    robj.ctx.next_state = env_next_state;
    robj.ctx.istimeout  = env_istimeout;
    robj.ctx.reset_timer= env_reset_timer;
    robj.ctx.runtime    = env_runtime;
    
    device = find_device("def");                            /**使用默认设备*/
    robj.cfg = *cfg;
    ril_core_init(&robj);
    if (device) {
        robj.dev.name = device->name;
        robj.dev.ops  = device->ops;
        robj.dev.adap = &robj.adt;
        robj.dev.at   = &robj.at;
        robj.dev.config= &robj.cfg;
    }
}

/**
 * @brief       选择使用的设备    
 * @param[in]   name - 设备名称
 * @return      RIL_OK - 选择成功, RIL_ERROR - 设备不存在
 */
int ril_use_device(const char *name)
{
    const struct ril_device *dev= find_device(name);
    
    if (dev == NULL)
        return RIL_ERROR;
        
    robj.dev.name = dev->name;
    robj.dev.ops  = dev->ops;  
    return RIL_OK;
}

/**
 * @brief       开机请求
 */
void ril_open(void)
{
    robj.open_enable = true;
}

/**
 * @brief       关机请求
 */
void ril_close(void)
{    
    robj.open_enable = false;
}

/**
 * @brief   Enables or disables the lowerpowe.
 * @param[in] enable - 启用/禁用低功耗
 */
void ril_lowpower(bool enable)
{
    robj.sleep_enable = enable != false;
}

/**
 * @brief       网络连接控制
 * @param[in]   enable - 启用/禁用网络连接
 */
void ril_netconn(bool enable)
{
    robj.conn_enable = enable != false;
}

/**
 * @brief   模组开机判断
 */
bool ril_isopen(void)
{
    return robj.run_state != RIL_STAT_OFF;
}

/**
 * @brief   ril系统状态
 */
ril_status_t *ril_get_status(void)
{
    return &robj.status;
}

/**
 * @brief   指示是否注册
 */
bool ril_isreg(void)
{
    ril_netreg_status netreg = ril_get_status()->netreg;
    return netreg == NETREG_REG || netreg == NETREG_ROAMING;
}
/**
 * @brief   指示设备是否已经开机就绪
 */
bool ril_device_ready(void)
{
    return robj.run_state >= RIL_STAT_INIT && robj.run_state <= RIL_STAT_CALL;
}

/**
 * @brief   指示网络是否可用
 */
bool ril_isonline(void)
{
    return robj.run_state == RIL_STAT_ONLINE;
}

/**
 * @brief     发送短信
 * @param[in] phone  - 目标手机号
 * @param[in] msg    - 短信消息( < 164 bytes)
 * @return    RIL_OK - 发送成功, 其它值 - 异常
 */
int ril_sms_send(const char *phone, const char *msg)
{
    return sms_send(&robj.dev, phone, msg);
}

/**
 * @brief       设置APN
 * @param[in]   none
 */
void ril_set_apn(const char *apn, const char *username, const char *password)
{
    ril_obj_t *r = get_ril_obj();
    
    if (strcmp(r->cfg.apn.apn, apn) || strcmp(r->cfg.apn.user, username) ||
        strcmp(r->cfg.apn.passwd, password)) {        
    }
    r->cfg.apn.apn = apn;
    r->cfg.apn.user = username;
    r->cfg.apn.passwd = password;
    
    //应执行重新注册流程...,待实现
}

/**
 * @brief        ril请求处理
 * @param[inout] data   - 参考ril_request_code描述
 * @param[in]    size   - data大小
 * @return       RIL_OK - 请求成功, 其它值 - 异常
 */
int ril_request(ril_request_code num, void *data, int size)
{
    if (!ril_isopen())                                   /**未打开模组*/
        return RIL_REJECT;    
    return ril_dev_request(&get_ril_obj()->dev, num, data, size);
}

/**
 * @brief       获取CSQ值
 * @param[out]  csq
 * @return      RIL_OK - 获取成功, 其它值 - 异常
 */
int ril_get_csq(ril_csq_t *csq)
{
    return ril_request(RIL_REQ_GET_CSQ, csq, sizeof(csq));
}

/**
 * @brief       ril主任务
 * @param[in]   none
 */
void ril_main_task(void)
{
    while (1) {
        ril_core_process(&robj);
        ril_delay(10);
    }
}

/**
 * @brief       ril AT命令处理任务
 * @param[in]   none
 */
void ril_atcmd_task(void)
{
    while (1) {
        at_process(&robj.at);
        ril_delay(1);        
    }
}
