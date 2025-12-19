/******************************************************************************
 * @brief    SIMCOM模组(sim800c,sim900a)相关接口实现
 *
 * Copyright (c) 2017~2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2021-05-15     Morro        Initial version. 
 ******************************************************************************/
#include "ril_device.h"
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* socket 收发信息 -----------------------------------------------------------*/
struct socket_info {
    socket_base_t *s;
    unsigned char *buf;
    int          bufsize;    
    int          count;    
};

static int simcom_sock_disconnect(struct ril_device *dev, socket_base_t *s);
/**
 * @brief       启动模组
 */ 
static int simcom_startup(struct ril_device *dev)
{
    unsigned int timer;
    int i = 0;
    for (i = 0; i < 3; i++) {
        dev->adap->pin_ctrl(RIL_PIN_RESET, 0, 1);
        dev->adap->pin_ctrl(RIL_PIN_PWRKEY, 0, 0);
        dev->adap->pin_ctrl(RIL_PIN_POWER, 0, 1);
        at_delay(2000);
        dev->adap->pin_ctrl(RIL_PIN_PWRKEY, 0, 1);
        timer = at_get_ms();
        while (!at_istimeout(timer, 10 * 1000)) {
            at_delay(10);
        }
        if (ril_send_singleline("AT") == RIL_OK) {                       
            return RIL_OK;
        }
        dev->adap->pin_ctrl(RIL_PIN_RESET, 0, 0);
        dev->adap->pin_ctrl(RIL_PIN_PWRKEY, 0, 1);
        dev->adap->pin_ctrl(RIL_PIN_POWER, 0, 1);        
        at_delay(2000);
    }
    return RIL_ERROR;
}


/* 
 * @brief       模块初始化
 */
static int simcom_init(struct ril_device *dev)
{
    const char *cmds[] = {
    "AT+CSCLK=0",                                              
    "AT+CIPHEAD=1",                                             //+RECEIVE,<n>,<data length>:
    "AT+CIPQSEND=1",                                            //快发模式
    "AT+CIPMUX=1",                                              //多IP支持
    "AT+CIPSPRT=>",
    "AT+CIPSRIP=0",         
    NULL
    };
    return ril_send_multiline(cmds);
}

/* 
 * @brief       关闭模块
 */ 
static int simcom_shutdown(struct ril_device *dev)
{
    char recv[32];
    int ret;
    at_respond_t resp = {"NORMAL POWER DOWN", recv, sizeof(recv), 20 * 1000};  //等待时间较长
    ret = ril_exec_cmdx(&resp, "AT+CPOWD=1");
    at_delay(2000);
    dev->adap->pin_ctrl(RIL_PIN_POWER, 0, 0);
    dev->adap->pin_ctrl(RIL_PIN_PWRKEY, 0, 1);
    dev->adap->pin_ctrl(RIL_PIN_RESET, 0, 0);
    return ret;
}


/* 
 * @brief       获取网络状态
 * @params[out] - status, 网络连接状态
 */
static int simcom_netconn_status(struct ril_device *dev, ril_netconn_status *status)
{

    char recv[128];
    at_respond_t resp = {"OK", recv, sizeof(recv), 60 * 1000};
    if (ril_exec_cmdx(&resp, "AT+CGACT?") != RIL_OK)
        return RIL_ERROR;
    //+CGACT: <cid>,<state>
    *status = (strstr(recv,"+CGACT: 1,1") == NULL) ? NETCONN_OFFLINE : NETCONN_ONLINE;
   return RIL_OK;
}


/* 
 * @brief    建立网络环境
 */
static int simcom_pdp_setup(struct ril_device *dev)
{
    ril_config_t *c = dev->config;
    ril_exec_cmdx(NULL, "AT+CGCLASS=\"B\"");
    ril_exec_cmdx(NULL, "AT+CGATT=1");
    ril_exec_cmdx(NULL, "AT+CGDCONT=1,\"IP\",,,0,0");    
    /* 
     * @brief    设置APN
     */
    return ril_exec_cmdx(NULL, "AT+CSTT=\"%s\",\"%s\",\"%s\"",
                         c->apn.apn, c->apn.user, c->apn.passwd);  
}

/** 
 * @brief    网络启动/禁用控制(对应PDP激活)
 */
static int simcom_pdp_ctrl(struct ril_device *dev, bool enable)
{
    int ret = RIL_OK;
    char recv[128];
    at_respond_t resp = {"OK", recv, sizeof(recv), 65 * 1000};
    if (enable) {
        ril_exec_cmdx(&resp, "AT+CGACT=1,1");                 //激活网络
        ret =  ril_exec_cmdx(&resp, "AT+CIICR");              //激活移动场景
        resp.matcher = "\n";                                  //不需要响应
        ril_exec_cmdx(&resp, "AT+CIFSR");
        return ret;
    } else {
        return ril_exec_cmdx(&resp, "AT+CIPSHUT"); 
    }
}

/**  
 * @brief    获取本地IP
 */
static int simcom_get_ipaddr(struct ril_device *dev, char *buf, int bufsize)
{
    char recv[64];
    at_respond_t resp = {"\n", recv, sizeof(recv), 120 * 1000};
    if (ril_exec_cmdx(&resp, "AT+CIFSR") != RIL_OK)
        return false;
    return sscanf(recv, "%[^ ]", buf) == 1;     
}

/**  
 * @brief    RIL请求
 */
static int simcom_request(struct ril_device *dev, ril_request_code num, void *data, int size)
{
    switch (num) {
    case RIL_REQ_GET_CONN_STATUS:
        return simcom_netconn_status(dev, (ril_netconn_status *)data);
    case RIL_REQ_GET_IPADDR:
        return simcom_get_ipaddr(dev, (char *)data, size);
    default:/*ril 默认请求处理*/
        return ril_request_default_proc(dev, num, data, size);
    }
}

/** 
 * @brief   连接服务器
 */
static int simcom_sock_connect(struct ril_device *dev, socket_base_t *s)
{
    static int ret;
    char recv[64];
    at_respond_t resp = {"OK", recv, sizeof(recv), 120 * 1000};
    ret = ril_exec_cmdx(&resp, "AT+CIPSTART=%d,\"%s\",\"%s\",%d",
                        s->id, s->type == RIL_SOCK_TCP ? "TCP":"UDP", 
                        s->host, s->port);
    if (ret != RIL_OK)
        simcom_sock_disconnect(dev, s);
    return ret;
}

/** 
 * @brief       SIM800C数据发送处理
 * @retval      none
 */
static int socket_send_handler(at_work_ctx_t *e)
{
    struct socket_info *i = (struct socket_info *)e->params;
    socket_base_t *s    = i->s;
    unsigned char ctrla   = 0x1A;
    
    e->printf(e, "AT+CIPSEND=%d,%d",s->id, i->bufsize);
    
    if (e->wait_resp(e, ">", 5000) != AT_RET_OK)       //等待提示符
        return RIL_ERROR;
    
    e->write(e, i->buf, i->bufsize);
    e->write(e, &ctrla, 1);                                //启动发送
    return (e->wait_resp(e, "ACCEPT:", 3000) == AT_RET_OK) ? RIL_OK : RIL_ERROR;
}

/*
 * @brief       SIM800C数据发送
 * @retval      none
 */
static int simcom_sock_send(struct ril_device *dev, socket_base_t *s,
                            const void *buf, unsigned int len)
{
    struct socket_info info = {s, (unsigned char *)buf, len, 0};
    if (len == 0)
        return RIL_ERROR;      
    return at_do_work(dev->at, (at_work)socket_send_handler, &info);
}

/*******************************************************************************
 * @brief       数据接收处理
 *              格式 - +RECEIVE,<n>,<data length>:\r\n<data>
 * @param[in]   ctx  - urc上下文
 * @retval      none 
 ******************************************************************************/
static void data_recv_notify(at_urc_ctx_t *ctx)
{
    socket_base_t *s;
    int id, ndata;                            //socket id 号,接收数据长度
    
    unsigned int timer;
    int          readcnt, max_read_len;      //读计数, 最大读取长度
    char        *start;
    int          remain;                     //首包剩余数据
    if (sscanf(ctx->buf, "+RECEIVE,%d,%d", &id, &ndata) != 2)
        return;
    
    s = find_socket_by_id(id);
    
    if (s == NULL) {
        RIL_WARN("Invalid id :%d\r\n", id);
        return;
    }
    readcnt = 0;
    start = strchr(ctx->buf, '\n');          //转到数据区
    if (start == NULL) {
        RIL_ERR("RECEIVE recv error[%s]\r\n", ctx->buf);
        return;
    }
    start++;
    RIL_DBG("Data input %d size.\r\n", ndata);
    remain = &ctx->buf[ctx->recvlen] - start;
    
    /* 递交首包剩余数据 */
    if (ndata > remain) {
        if (remain)
            ril_socket_notify(s, SOCK_NOTFI_DATA_REPORT, start, remain);
        ndata -= remain;
    } else{
        ril_socket_notify(s, SOCK_NOTFI_DATA_REPORT, start, ndata);
        return;
    }
    readcnt = 0;
    timer = ril_get_ms();
    while (ndata && !ril_istimeout(timer, 1000 + ndata)) {
        //为防止接收队列溢出，每次读取1半
        max_read_len = DEF_SOCK_RECV_BUFSIZE / 2;
        max_read_len = (max_read_len > ctx->bufsize) ? ctx->bufsize : max_read_len;

        if (ndata > max_read_len)
            readcnt = ctx->read(ctx->buf, max_read_len);
        else
            readcnt = ctx->read(ctx->buf, ndata);
        
        ndata -= readcnt;
        
        ril_socket_notify(s, SOCK_NOTFI_DATA_REPORT, ctx->buf, readcnt);
    } 
    if (ndata)
        RIL_WARN("The remaining %d bytes of data cannot be read\r\n", ndata);
}
ril_urc_register("+RECEIVE", data_recv_notify);

/**
 * @brief 接收数据
 * @param[in]   s   - socket
 * @param[in]   buf - 接收缓冲区
 * @param[in]   len - 缓冲区长度
 * @retval      实际接收到数据长度
 */
unsigned int simcom_sock_recv(struct ril_device *dev, socket_base_t *s, 
                              void *buf, unsigned int max_recv_size)
{
    /*SIM800C 的socket数据是通过URC直接上报的，故这里直接返回0*/
    return 0;
}

/**  
 * @brief    断开连接
 */
static int simcom_sock_disconnect(struct ril_device *dev, socket_base_t *s)
{
    return ril_exec_cmdx(NULL, "AT+CIPCLOSE=%d,1", s->id);
}

/**
 * @brief  获取连接状态
 */
sock_request_status simcom_sock_conn_status(struct ril_device *dev, socket_base_t *s)
{
    char recv[128];
    char *argv[6];
    int argc;
    if (s->type == RIL_SOCK_UDP)
        return SOCK_STAT_DONE;
    
    if (ril_exec_cmd(recv, sizeof(recv), "AT+CIPSTATUS=%d" ,s->id) != RIL_OK)
        return SOCK_STAT_UNKNOW;
    
    // +CIPSTATUS: <n>,<bearer>, <TCP/UDP>, <IP address>, <port>, <client state>
    //socket状态存放在第5个字段,这里先进行字符串分割
    argc = at_split_respond_lines(recv, argv, 6, ',');
    if (argc != 6) {
        RIL_WARN("Failed to obtain the status of socket %d.\r\n");
        return SOCK_STAT_UNKNOW;
    }
    if (strstr(argv[5], "CONNECTING") != NULL)
        return SOCK_STAT_BUSY;
    else if (strstr(argv[5], "CONNECTED") != NULL)
        return SOCK_STAT_DONE;
    else
        return SOCK_STAT_FAILED;
}

/**
 * @brief 获取发送状态
 */
sock_request_status simcom_sock_send_status(struct ril_device *dev, socket_base_t *s)
{
    char recv[100];    
    int txlen, acklen, nacklen;
    if (s->type == RIL_SOCK_UDP)                                 //UDP直接返回成功
        return SOCK_STAT_DONE;
    
    if (ril_exec_cmd( recv, sizeof(recv), "AT+CIPACK=%d",s->id) != RIL_OK)
        return SOCK_STAT_FAILED;
    if (sscanf(recv, "%*[^+]+CIPACK: %d,%d,%d", &txlen, &acklen, &nacklen) != 3)
        return SOCK_STAT_FAILED;  
    
    return nacklen == 0 ? SOCK_STAT_DONE : SOCK_STAT_BUSY;
}

/**
 * @brief sim卡丢失事件
 */
static void sim_urc_handler(at_urc_ctx_t *ctx)
{
    ril_sim_status sim = SIM_ABSENT;
    ril_notify(RIL_NOTIF_SIM, &sim, sizeof(int *));              //发送SIM卡丢失事件
}ril_urc_register("+CPIN: NOT INSERTED", sim_urc_handler);


/**
 * @brief   GPRS断开处理
 */
static void pdp_deative_handler(at_urc_ctx_t *ctx)
{
    ril_netconn_status netconn = NETCONN_OFFLINE;
    ril_notify(RIL_NOTIF_NETCONN, &netconn, sizeof(int *));   
} ril_urc_register("+PDP: DEACT", pdp_deative_handler);

/**
 * @brief       socket连接失败处理
 *              [<n>,]CONNECT FAIL
 * @retval      none 
 */
static void socket_connect_failed_handler(at_urc_ctx_t *ctx)
{
    int id;
    socket_base_t *s;
    if (sscanf(ctx->buf, "%d, CONNECT FAIL", &id) == 1) {
        RIL_WARN("Socket %d connect failed.\r\n", id);
        s = find_socket_by_id(id); 
        if (s != NULL)
            ril_socket_notify(s, SOCK_NOTFI_OFFLINE, NULL, 0);
    }
}
ril_urc_register(" CONNECT FAIL", socket_connect_failed_handler);

/**
 * @brief       socket掉线处理
 *              [<n>,] CLOSED
 * @retval      none 
 */
static void socket_closed_handler(at_urc_ctx_t *ctx)
{
    int id;
    socket_base_t *s;
    if (sscanf(ctx->buf, "%d, CLOSED", &id) == 1) {
        RIL_WARN("Socket %d closed\r\n", id);
        s = find_socket_by_id(id); 
        if (s != NULL)
            ril_socket_notify(s, SOCK_NOTFI_OFFLINE, NULL, 0);    //连接断开
    }
}
ril_urc_register(" CLOSED", socket_closed_handler);


/* 模块设备定义 --------------------------------------------------------------*/
static const ril_device_ops_t simcom = {
    .startup         = simcom_startup,
    .init            = simcom_init,
    .shutdown        = simcom_shutdown,
    .pdp_setup       = simcom_pdp_setup,
    .pdp_contrl      = simcom_pdp_ctrl,
    .request         = simcom_request,
    .sock = {
        .connect     = simcom_sock_connect,
        .disconnect  = simcom_sock_disconnect,
        .send        = simcom_sock_send,
        .recv        = simcom_sock_recv,
        .conn_status = simcom_sock_conn_status,
        .send_status = simcom_sock_send_status
    }
};

/*安装sim800c模组(已调通)*/
ril_device_install("sim800c", simcom);
/*安装sim900a模组(未调过)*/
ril_device_install("sim900a", simcom);
