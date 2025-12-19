/******************************************************************************
 * @brief    移远模组(EC21,BC35,BG96)相关接口实现
 *
 * Copyright (c) 2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2020-10-20     Morro        Initial version.
 * 2021-07-20     Morro        解决读数据时残留的数据导致其它命令状态异常的问题
 ******************************************************************************/
#include "ril_device.h"
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* socket 收发信息 -----------------------------------------------------------*/
struct socket_info {
    const socket_base_t *s;
    unsigned char *buf;
    int          bufsize;    
    int          count;    
};

static int ec21_disconnect(struct ril_device *dev, socket_base_t *s);

/**  
 * @brief       模组初始化
 */
static int ec21_init(struct ril_device *dev)
{
    const char *cmds[] = {
    "AT&D1",
    "ATI",
    "AT+QINDCFG=\"all\",1",                   
    "AT+QCFG=\"risignaltype\",\"respective\"",
    "AT+QCFG=\"urc/delay\",1",   
    "AT+QURCCFG=\"urcport\",\"uart1\"",       
    "AT+QICFG=\"transpktsize\",1460",
    "AT+QICFG=\"dataformat\",0,0",  
    "AT+GCAP",
    NULL
    };
    return ril_send_multiline( cmds);
}

/** 
 * @brief       关闭模组
 */ 
static int ec21_shutdown(struct ril_device *dev)
{
    char recv[32];
    int ret;
    at_respond_t resp = {"POWERED DOWN", recv, sizeof(recv), 30 * 1000};  //等待时间较长
    ret = ril_exec_cmdx(&resp, "AT+QPOWD");
    at_delay(2000);
    dev->adap->pin_ctrl(RIL_PIN_POWER, 0, 0);
    dev->adap->pin_ctrl(RIL_PIN_PWRKEY, 0, 0);
    dev->adap->pin_ctrl(RIL_PIN_RESET, 0, 0);
    return ret;
}

/** 
 * @brief       获取网络状态
 * @params[out] - status, 网络连接状态
 */
static int ec21_netconn_status(struct ril_device *dev, ril_netconn_status *status)
{
    int state;
    char recv[128];
    at_respond_t resp = {"OK", recv, sizeof(recv), 120 * 1000};
    if (ril_exec_cmdx(&resp, "AT+QIACT?") != RIL_OK)
        return RIL_ERROR;
    //+CGATT: <state>
    if (sscanf(recv, "%*[^+]+QIACT: %d", &state) != 1) 
        return RIL_ERROR;
    *status = state ? NETCONN_ONLINE : NETCONN_OFFLINE;
   return RIL_OK;
}


#if 0
/** 
 * @brief       配置网络搜索顺序
 * @params[in]  rat - 网络制式表优先级从低到高(即制式[0]优先搜索)
 */
static int set_search_order(struct ril_device *dev, ril_rat_type rat[], int count)
{
    char seq[20];                                    //搜索顺序表
    int i;        
    strcpy(seq, count ? "": "00");
    for (i = 0; i < count && i < 8; i++) {
        switch (rat[i]) {
        case RAT_TYPE_AUTO:
            strcat(seq, "00");
            break;
        case RAT_TYPE_2G:
            strcat(seq, "01");
            break;
        case RAT_TYPE_4GM1:
            strcat(seq, "02");
            break;
        case RAT_TYPE_4GNB:
            strcat(seq, "03");
            break;
        default:
            strcat(seq, "00");
            break;
        }
    }
    return ril_exec_cmdx(NULL, "AT+QCFG=\"nwscanseq\",%s,1", seq);    
}
#endif

/** 
 * @brief    建立PDP
 */
static int ec21_pdp_setup(struct ril_device *dev)
{
    ril_config_t *c = dev->config;
    /* 
     * @brief    设置APN
     */
    if (c->apn.apn == NULL || strlen(c->apn.apn) == 0)
        return ril_exec_cmdx(NULL, "AT+CGDCONT=2,\"IPV4V6\",\"VZWADMIN\"");
    else
        return ril_exec_cmdx(NULL, "AT+QICSGP=1,1,\"%s\",\"%s\",\"%s\",0",
                             c->apn.apn, c->apn.user, c->apn.passwd);
}

/**  
 * @brief    网络启用/禁用控制(对应PDP激活)
 */
static int ec21_pdp_ctrl(struct ril_device *dev, bool enable)
{
    char recv[32];
    at_respond_t resp = {"OK", recv, sizeof(recv), 120 * 1000};  //等待时间较长
    return ril_exec_cmdx(&resp, "AT+%s=1", enable ? "QIACT" : "QIDEACT");  
}


/**  
 * @brief    连接服务器
 */
static int ec21_sock_connect(struct ril_device *dev, socket_base_t *s)
{
    bool ret;
    char recv[64];
    at_respond_t resp = {"OK", recv, sizeof(recv), 120 * 1000};
    ret = ril_exec_cmdx(&resp, "AT+QIOPEN=1,%d,%s,\"%s\",%d,%d,0",
                        s->id, s->type == RIL_SOCK_TCP ? "\"TCP\"":"\"UDP\"", 
                        s->host, s->port, 0);
    if (ret != RIL_OK)
        ec21_disconnect(dev, s);
    return ret;
}

/**  
 * @brief    数据发送处理
 */
static int socket_send_handler(at_work_ctx_t *e)
{
    struct socket_info *i = (struct socket_info *)e->params;
    const socket_base_t *s    = i->s;
 
    e->printf(e, "AT+QISEND=%d,%d",s->id, i->bufsize);
    
    if (e->wait_resp(e, ">", 5000) != AT_RET_OK)     //等待提示符
        return RIL_ERROR;

    e->write(e, i->buf, i->bufsize);
    return (e->wait_resp(e, "SEND OK", 5000) == AT_RET_OK) ? RIL_OK : RIL_ERROR;
    
}

/**  
 * @brief    数据接收处理
 */
static at_return socket_recv_handler(at_work_ctx_t *e)
{   
    int  datalen = 0;
    int  recvcnt = 0;
    char *start, *end;
    bool sync_header = false;                                    /* 接收头标志 */
    unsigned int timer;                                          /* 超时定时器 */    
    struct socket_info *i = (struct socket_info *)e->params;
    const socket_base_t  *s = i->s;
    /**
     * 命令格式
     *
     *    =>AT+QIRD=<connectID>[,<read_length>]
     *
     *    <= +QIRD: <read_actual_length><CR><LF><data>
     *    <= 
     *    <= OK              //结束符
     *   
     */
    //发送接收指令
    e->printf(e, "AT+QIRD=%d,%d",s->id, i->bufsize);
    timer = ril_get_ms();
    while (!ril_istimeout(timer, 3000 + i->bufsize)) {
        recvcnt += e->read(e, i->buf + recvcnt, i->bufsize - recvcnt);
        if (sync_header && recvcnt >= datalen) {
            e->wait_resp(e, "OK", 100);                         /* 等待结束符 */
            i->count = datalen;
            return AT_RET_OK;
        } else if (!sync_header && recvcnt >= 5) {
            datalen = 0;
            i->buf[recvcnt] = '\0';
            start = strstr((char *)i->buf, "+QIRD:");
            if (!start)
                continue;
            end = strstr(start, "\r\n");
            if (!end)
                continue;
            datalen = atoi(start + 6);                          /* 有效数据长度*/
            if (datalen) {
                RIL_DBG("%s\r\n", start);
                RIL_INFO("Recv %d data size.\r\n", datalen);                
            }
            /*将实际收到的数据移到数组前面 ------------------------------------*/
            recvcnt -= end + 2 - (char *)i->buf;              
            memmove(i->buf, end + 2, recvcnt); 
            sync_header = true;
        }
    }
    return AT_RET_TIMEOUT;
}


/**  
 * @brief    发送数据
 */
static int ec21_send(struct ril_device *dev, socket_base_t *s, 
                      const void *buf, unsigned int len)
{
    struct socket_info info = {s, (unsigned char *)buf, len, 0};
    if (len == 0)
        return RIL_REJECT;       
    return at_do_work(dev->at, (at_work)socket_send_handler, &info);
}

/**  
 * @brief    接收数据
 */
static unsigned int ec21_recv(struct ril_device *dev, socket_base_t *s,
                              void *buf, unsigned int max_recv_size)
{
    struct socket_info info = {s, (unsigned char *)buf, max_recv_size, 0};
    at_do_work(dev->at, (at_work)socket_recv_handler, &info);
    return info.count;
}

/**  
 * @brief    断开连接
 */
static int ec21_disconnect(struct ril_device *dev, socket_base_t *s)
{
    return ril_exec_cmdx(NULL, "AT+QICLOSE=%d", s->id);
}

/**  
 * @brief    RIL请求
 */
static int ec21_request(struct ril_device *dev, ril_request_code num, void *data, 
                        int size)
{
    switch (num) {
    case RIL_REQ_GET_CONN_STATUS:
        return ec21_netconn_status(dev, (ril_netconn_status *)data);        
    default:
        return ril_request_default_proc(dev, num, data, size);
    }
}

/**
 * @brief  获取连接状态
 */
static sock_request_status ec21_conn_status(struct ril_device *dev, 
                                            socket_base_t *s)
{
    int line_num;
    char recv[100];    
    char *lines[8];
    int id, status;
    
    if (ril_exec_cmd( recv, sizeof(recv), "AT+QISTATE=1,%d",s->id) != RIL_OK)
        return SOCK_STAT_UNKNOW;
    line_num = at_split_respond_lines(recv, lines, 8, ',');
    if (line_num < 6)
        return SOCK_STAT_FAILED;
    if (sscanf(recv, "%*[^+]+QISTATE: %d", &id) != 1 || id != s->id)
        return SOCK_STAT_FAILED;
    status = atoi(lines[5]);
    
    if (status == 2)
        return SOCK_STAT_DONE;
    else if (status == 4)
        return SOCK_STAT_FAILED;
    else 
        return SOCK_STAT_BUSY;
}

/**
 * @brief 获取发送状态
 */
static sock_request_status ec21_send_status(struct ril_device *dev, socket_base_t *s)
{
    char recv[100];    
    int total, acked, unacked;
    if (ril_exec_cmd( recv, sizeof(recv), "AT+QISEND=%d,0",s->id) != RIL_OK)
        return SOCK_STAT_FAILED;
    if (sscanf(recv, "%*[^+]+QISEND: %d,%d,%d", &total, &acked, &unacked) != 3)
        return SOCK_STAT_FAILED;  //#warning SOCK_STAT_UNKNOW
    return unacked == 0 ? SOCK_STAT_DONE : SOCK_STAT_BUSY;
}

/**
 * @brief   urc事件
 *          1. +QIURC: "closed",<connectID>   socket关闭
 *          2. +QIURC: "recv",<connectID>     收到数据
 *          3. +QIURC: "incoming full"        数据缓冲区满
 *          4. +QIURC: "pdpdeact",<contextID>  PDP关闭
 */
static void tcp_urc_handler(at_urc_ctx_t *ctx)
{
    int line_num;
    char *lines[2];
    int id;
    socket_base_t *s = NULL;
    ril_netconn_status netconn = NETCONN_OFFLINE;
    line_num = at_split_respond_lines(ctx->buf, lines, 2, ',');
    if (line_num > 1) {
        id = atoi(lines[1]);
        s = find_socket_by_id(id);
    }
    if (strstr(ctx->buf, "closed") && s != NULL)
        ril_socket_notify(s, SOCK_NOTFI_OFFLINE, NULL, 0);
    else if (strstr(ctx->buf, "recv") && s != NULL)
        ril_socket_notify(s, SOCK_NOTFI_DATA_INCOMMING, NULL, 0);
    else if (strstr(ctx->buf, "pdpdeact"))
        ril_notify(RIL_NOTIF_NETCONN, &netconn, sizeof(int *));
    
}ril_urc_register("+QIURC: ", tcp_urc_handler);


/**
 * @brief sim卡丢失事件
 */
static void sim_urc_handler(at_urc_ctx_t *ctx)
{
    ril_sim_status sim = SIM_ABSENT;
    ril_notify(RIL_NOTIF_SIM, &sim, sizeof(int *));
}ril_urc_register("+CPIN: NOT READY", sim_urc_handler);


/*EC21设备函数操作集 ----------------------------------------------------------*/
static const ril_device_ops_t ec21 = {
    .init            = ec21_init,
    .shutdown        = ec21_shutdown,
    .pdp_setup       = ec21_pdp_setup,
    .pdp_contrl      = ec21_pdp_ctrl,
    .request         = ec21_request,
    .sock = {
        .connect     = ec21_sock_connect,
        .disconnect  = ec21_disconnect,
        .send        = ec21_send,
        .recv        = ec21_recv,
        .conn_status = ec21_conn_status,
        .send_status = ec21_send_status
    }
};

/* 安装ec21模组*/
ril_device_install("ec21", ec21);

/* 安装EC25模组(兼容EC21指令)*/
ril_device_install("ec25", ec21);

/* 安装bg96模组(兼容EC21指令)*/
ril_device_install("bg96", ec21);

/* 安装BC35模组(兼容EC21指令)*/
ril_device_install("bc35", ec21);
