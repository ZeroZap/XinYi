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
#include "ril_core.h"
#include "ril_device.h"
#include <string.h>

/** 
 * @brief       启动模组
 * @return      true - 成功启动, false - 启动失败
 */ 
int __ril_comm_startup(struct ril_device *r)
{
    unsigned int timer;
    int i = 0;
    for (i = 0; i < 3; i++) {
        r->adap->pin_ctrl(RIL_PIN_RESET, 0, 1);
        r->adap->pin_ctrl(RIL_PIN_PWRKEY, 0, 0);
        r->adap->pin_ctrl(RIL_PIN_POWER, 0, 1);
        at_delay(500);
        r->adap->pin_ctrl(RIL_PIN_PWRKEY, 0, 1);
        r->adap->pin_ctrl(RIL_PIN_DTR, 0, 0); 
        timer = at_get_ms();

        while (r->at->urc_cnt < 3 && !at_istimeout(timer, 10 * 1000)) {
            at_delay(1);
        }
        
        if (ril_send_singleline("AT") == RIL_OK) {
            return RIL_OK;
        }
        r->adap->pin_ctrl(RIL_PIN_RESET, 0, 0);
        r->adap->pin_ctrl(RIL_PIN_PWRKEY, 0, 0);
        r->adap->pin_ctrl(RIL_PIN_POWER, 0, 0);        
        at_delay(2000);        
    }
    return RIL_ERROR;
}

/** 
 * @brief       模组初始化
 */
int __ril_comm_init(struct ril_device *r)
{
    const char *cmds[] = {"ATE0",NULL};
    return ril_send_multiline(cmds);
}

/** 
 * @brief       关闭模组(强制关机)
 */ 
int __ril_comm_shutdown(struct ril_device *r)
{
    r->adap->pin_ctrl(RIL_PIN_POWER, 0, 0);
    r->adap->pin_ctrl(RIL_PIN_PWRKEY, 0, 0);
    r->adap->pin_ctrl(RIL_PIN_RESET, 0, 0);
    return RIL_OK;
}

/** 
 * @brief       复位模组
 */ 
int __ril_comm_reset(struct ril_device *r)
{
    r->adap->pin_ctrl(RIL_PIN_RESET, 0, 0);
    at_delay(100);
    r->adap->pin_ctrl(RIL_PIN_RESET, 0, 1);
    return RIL_OK;
}

/** 
 * @brief       唤醒设备
 */ 
int __ril_comm_wakeup(struct ril_device *r)
{
    r->adap->pin_ctrl(RIL_PIN_DTR, 0, 0);              //位低唤醒模组
    return RIL_OK;
}

/** 
 * @brief       设备休眠
 */ 
int __ril_comm_sleep(struct ril_device *r)
{
    r->adap->pin_ctrl(RIL_PIN_DTR, 0, 1);              //拉高允许休眠
    return RIL_OK;
}

/** 
 * @brief    建立网络环境
 */
int __ril_comm_pdp_setup(struct ril_device *r)
{
    return RIL_OK;
}

/**
 * @brief     pdp控制
 * @param[in] active - 激活/去激活PDP
 */ 
int __ril_comm_pdp_ctrl(struct ril_device *r, int active)
{
    return RIL_OK; 
}
/** 
 * @brief   获取sim卡状态
 */ 
int __ril_comm_sim_status(struct ril_device *r, ril_sim_status *status)
{
    char recv[64];
    at_respond_t resp = {"OK", recv, sizeof(recv), 5 * 1000};    
    ril_sim_status sstat = SIM_UNKNOW;
    if (ril_exec_cmdx(&resp, "AT+CPIN?") != RIL_OK)
        return RIL_ERROR;
    if (strstr(recv, "READY"))
        sstat = SIM_READY;
    else if (strstr(recv, "SIM PIN"))
        sstat = SIM_PIN;
    else if (strstr(recv, "SIM PUK"))
        sstat = SIM_PUK;    
    *status = sstat;
   return RIL_OK;
}

/** 
 * @brief    获取csq值
 */ 
int __ril_comm_csq(struct ril_device *r, ril_csq_t *csq)
{
    char recv[64];
    at_respond_t resp = {"OK", recv, sizeof(recv), 15 * 1000};
    if (ril_exec_cmdx(&resp, "AT+CSQ") != RIL_OK)
        return RIL_ERROR;
    if (sscanf(recv, "%*[^:]: %d,%d", &csq->rssi, &csq->error_rate) == 2)
        return RIL_OK;
    else
        return RIL_ERROR;
}

/** 
 * @brief    获取网络注册状态
 */
int __ril_comm_netreg_status(struct ril_device *r, ril_netreg_status *stat)
{
    char recv[64];
    int n, netreg;
    at_respond_t resp = {"OK", recv, sizeof(recv), 15 * 1000};
    if (ril_exec_cmdx(&resp, "AT+CREG?") != RIL_OK)
        return RIL_ERROR;    
    /*+CREG: <n>,<stat>[,<lac>,<ci>[,<AcT>]]*/
    if (sscanf(recv, "%*[^:]: %d,%d", &n, &netreg) != 2)
        return RIL_ERROR;
    *stat = (ril_netreg_status)netreg;
    return RIL_OK;
}

/** 
 * @brief    读取指定信息
 */
static int __ril_comm_get_info(const char *cmd, char *buf, int size)
{
    char regex[15];
    char recv[64];
    snprintf(regex, sizeof(regex), "%%s%d", size);
    if (ril_exec_cmd(recv, sizeof(recv), cmd) != RIL_OK)
        return RIL_ERROR;
    return sscanf(recv, regex, buf) == 1 ? RIL_OK : RIL_ERROR;    
}

/**
 * @brief        ril请求
 * @param[in]    code - 请求码
 * @param[inout] data - 数据内容,参考ril_request_code描述
 * @param[in]    size - data大小
 */
int __ril_comm_request(struct ril_device *r, ril_request_code num, void *data, 
                        int size)
{
    switch (num) {
    case RIL_REQ_GET_SIM_STATUS:
        return __ril_comm_sim_status(r, (ril_sim_status *)data);        
    case RIL_REQ_GET_REG_STATUS:
        return __ril_comm_netreg_status(r, (ril_netreg_status *)data);
    case RIL_REQ_GET_CONN_STATUS:        
        break;       
    case RIL_REQ_GET_CSQ:
        return __ril_comm_csq(r, (ril_csq_t *)data);
    case RIL_REQ_GET_IMEI:
        return __ril_comm_get_info("AT+CGSN", (char *)data, size);
    case RIL_REQ_GET_IMSI:
         return __ril_comm_get_info("AT+CIMI", (char *)data, size);
    case RIL_REQ_GET_HWVER:
        return __ril_comm_get_info("AT+CGMR", (char *)data, size); 
    case RIL_REQ_GET_SWVER:
        break;
    case RIL_REQ_GET_MODEL:
        return __ril_comm_get_info("AT+CGMM", (char *)data, size);       
    case RIL_REQ_DIAL:        
        return ril_exec_cmdx(NULL,"ATD%s", data);    /* 拨打电话*/
    case RIL_REQ_HANGUP:
        return ril_send_singleline("ATH");           /* 挂断电话*/
    case RIL_REQ_ANSWER:
        return ril_send_singleline("ATA");           /* 接通电话*/
    default:
        break;
    }
    RIL_WARN("The request[%d] interface is not implemented\r\n", num);
    return RIL_NOIMPL;
}

/**
 * @brief        ril默认请求处理
 * @param[in]    code - 请求码
 * @param[inout] data - 数据内容,参考ril_request_code描述
 * @param[in]    size - data大小
 */
int ril_request_default_proc(struct ril_device *dev, ril_request_code code, 
                              void *data, int size)
{
    return __ril_comm_request(dev, code, data, size);
}

/*安装默认设备 ---------------------------------------------------------------*/
static const ril_device_ops_t defdev = {NULL};

ril_device_install("def", defdev);
