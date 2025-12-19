/******************************************************************************
 * @brief    Sierra模组(HL8518)相关接口实现
 *
 * Copyright (c) 2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2021-1-23      Morro        Initial version.
 * 2021-2-28      Morro        完成socket收发接口调试.
 ******************************************************************************/
#include "ril_device.h"
#include <string_tools.h>
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

static int hl8518_disconnect(struct ril_device *dev, socket_base_t *s);

/* 
 * @brief       模组初始化
 */
static int hl8518_init(struct ril_device *dev)
{
    const char *cmds[] = {
        "AT&D1",
        "AT&V0",
        "AT+KSLEEP=2",
        NULL
    };
    return ril_send_multiline(cmds);
}

static int hl8518_power_up = false;

/*
 * @brief 开机事件
 */
static void power_on_handler(at_urc_ctx_t *ctx)
{
    hl8518_power_up = true;
}ril_urc_register("+KSUP:", power_on_handler);

/**
 * @brief       启动模组
 */ 
static int hl8518_startup(struct ril_device *dev)
{
    unsigned int timer;
    int i = 0;
    hl8518_power_up = false;
    for (i = 0; i < 3; i++) {
        dev->adap->pin_ctrl(RIL_PIN_RESET, 0, 1);
        dev->adap->pin_ctrl(RIL_PIN_PWRKEY, 0, 0);
        dev->adap->pin_ctrl(RIL_PIN_POWER, 0, 1);
        at_delay(500);
        dev->adap->pin_ctrl(RIL_PIN_PWRKEY, 0, 1);
        timer = at_get_ms();
        while (!hl8518_power_up &&!at_istimeout(timer, 20 * 1000)) {
            at_delay(10);
        }
        if (ril_send_singleline("AT") == RIL_OK) {                       
            return RIL_OK;
        }
        dev->adap->pin_ctrl(RIL_PIN_RESET, 0, 0);
        dev->adap->pin_ctrl(RIL_PIN_PWRKEY, 0, 0);
        dev->adap->pin_ctrl(RIL_PIN_POWER, 0, 0);        
        at_delay(2000);
    }
    return RIL_ERROR;
}

/* 
 * @brief       关闭模组
 */ 
static int hl8518_shutdown(struct ril_device *dev)
{
    char recv[32];
    int ret;
    at_respond_t resp = {"OK", recv, sizeof(recv), 30 * 1000};  //等待时间较长
    ret = ril_exec_cmdx(&resp, "AT+CPOF");
    at_delay(2000);
    dev->adap->pin_ctrl(RIL_PIN_POWER, 0, 0);
    dev->adap->pin_ctrl(RIL_PIN_PWRKEY, 0, 0);
    dev->adap->pin_ctrl(RIL_PIN_RESET, 0, 0);
    return ret;
}

/* 
 * @brief       获取网络状态
 * @params[out] - status, 网络连接状态
 */
static int hl8518_netconn_status(struct ril_device *dev, ril_netconn_status *status)
{
    char *p, *argv[13];
    int argc;
    int net_state = 0;  
    char recv[128];
    at_respond_t resp = {"OK", recv, sizeof(recv), 30 * 1000};
    if (ril_exec_cmdx(&resp, "AT+KCNXCFG?") != RIL_OK)
        return RIL_ERROR;
    /*
     * =>+KCNXCFG: 1,"GPRS","cmnet","","","IPV4","10.110.212.100","101.106.100.6","112.95.235.4",2
     */    
    if ((p = strstr(recv, "+KCNXCFG: ")) != NULL) {
        p += 10;
        strtrim(p, "\"");                                   /* 去掉所有 '"' */
        argc = strsplit(p, ",", argv, 14);                  /* 分解出各个字段 */
        if (argc != 10 && argc != 13){                      /* 格式错误 */        
            RIL_DBG("KCNXCFG Fomat Error\r\n");
            return RIL_ERROR;
        }
        /*获取网络状态 --------------------------------------------------------*/
        if (argc == 10) {
            net_state = atoi(argv[9]);
        }
        else if (argc == 13) {
            net_state = atoi(argv[12]);
        }
    }
    *status = (net_state== 2) ? NETCONN_ONLINE : NETCONN_OFFLINE;
        
   return RIL_OK;
}

/* 
 * @brief    建立PDP
 */
static int hl8518_pdp_setup(struct ril_device *dev)
{
    ril_config_t *c = dev->config;
    return ril_exec_cmdx(NULL, 
                         "AT+KCNXCFG=1,\"GPRS\",\"%s\",\"%s\",\"%s\"\r\n",
                         c->apn.apn, c->apn.user, c->apn.passwd);
}

/* 
 * @brief   网络启用/禁用控制(对应PDP激活)
 */
static int hl8518_pdp_ctrl(struct ril_device *dev, bool enable)
{
    char recv[32];
    at_respond_t resp = {"OK", recv, sizeof(recv), 30 * 1000};  //等待时间较长
    return ril_exec_cmdx(&resp, "AT+%s=1", enable ? "KCNXUP" : "KCNXDOWN");  
}

/** 
 * @brief   连接服务器
 */
static int hl8518_sock_connect(struct ril_device *dev, socket_base_t *s)
{
    int ret;
    char *p;
    char recv[64];
    at_respond_t resp = {"OK", recv, sizeof(recv), 30 * 1000};

    if (s->type == RIL_SOCK_TCP) {      /*TCP*/
        ret = ril_exec_cmdx(&resp, "AT+KTCPCFG=1,0,\"%s\",%d,,1,1,0",
                            s->host, s->port);        
    } else {                              /*UDP*/
        ret =  ril_exec_cmdx(&resp, "AT+KUDPCFG=1,0,0,1,\"%s\",%d,0",
                             s->host, s->port);
    }
    RIL_DBG("Create session successfully\r\n");
    /* 
     * @brief 创建成功之后,模组自动分配并返回一个session 号(1-32)
     * Response
     * +KTCPCFG: <session_id>
     * OK
     */
    if (ret != RIL_OK)
        return ret;
    
    p = (s->type == RIL_SOCK_TCP) ? strstr(recv,"+KTCPCFG: ") : strstr(recv,"+KUDPCFG: ");    
    if (!p) {
        return RIL_ERROR;
    }
    set_socket_tag(s, (void *)(atoi(p + 10))); //将session号当附属数据挂到当前socket
    RIL_DBG("New session number:%d\r\n", s->tag);
    
    if (s->type == RIL_SOCK_TCP) 
        return ril_exec_cmdx(&resp, "AT+KTCPCNX=%d", s->tag);
    else
        return RIL_OK;
}

/*
 * @brief       数据发送处理
 * @retval      none
 */
static int socket_send_handler(at_work_ctx_t *e)
{
    const char *eof = "--EOF--Pattern--";
    struct socket_info *i = (struct socket_info *)e->params;
    socket_base_t *s = i->s;
    
    if (s->type == RIL_SOCK_TCP)
        e->printf(e, "AT+KTCPSND=%d,%d", s->tag, i->bufsize);
    else
        e->printf(e, "AT+KUDPSND=%d,%s,%d,%d",s->tag, s->host, s->port,
                  i->bufsize);        

    if (e->wait_resp(e, "CONNECT", 5000) != AT_RET_OK) {       //等待提示符
       goto Error;
    }         
    e->write(e, i->buf, i->bufsize);                             //发送数据
    
    e->write(e, eof, strlen(eof));                               //发送结束符

    if (e->wait_resp(e, "OK", 5000) == AT_RET_OK)
        return RIL_OK;
    else {   
Error:
        e->write(e, eof, strlen(eof));
        return RIL_ERROR;
    }
}

/**
 * @brief       socket 数据发送
 * @param[in]   s   - socket
 * @param[in]   buf - 数据缓冲区
 * @param[in]   len - 缓冲区长度
 */
static int hl8518_sock_send(struct ril_device *dev, socket_base_t *s, 
                             const void *buf, unsigned int len)
{
    struct socket_info info = {s, (unsigned char *)buf, len, 0};
    if (len == 0)
        return RIL_REJECT;       
    return at_do_work(dev->at, (at_work)socket_send_handler, &info);
}


/*******************************************************************************
 * @brief       数据通知
 *              +KTCP_DATA: <session_id>,<ndata available>[,<data>]<CR><LF>
 *              +KTCP_DATA: <session_id>,<ndata available>[,<data>]
 * @param[in]   ctx  
 * @retval      none 
 ******************************************************************************/
static void tcp_udp_recv_notify(at_urc_ctx_t *ctx)
{
    socket_base_t *s;
    int session, ndata;                      //session 号,接收数据长度
    
    unsigned int timer;
    int          readcnt, max_read_len;      //读计数, 最大读取长度
    char        *start;
    int          remain;                     //首包剩余数据
    if (sscanf(ctx->buf, "+KTCP_DATA:%d,", &session) != 1 &&
        sscanf(ctx->buf, "+KUDP_DATA:%d,", &session) != 1)
        return;
    
    s = find_socket_by_tag((void *)session);
    
    if (s == NULL) {
        RIL_WARN("Invalid sesssion :%d\r\n", session);
        return;
    }
    
    /* 解析数据长度 -----------------------------------------------------------*/
    readcnt = 0;
    timer = ril_get_ms();
    do {
        readcnt += ctx->read(&ctx->buf[readcnt], ctx->bufsize - readcnt);
        ctx->buf[readcnt] = '\0';
        start = strchr(ctx->buf, ',');
    }while (start == NULL && !ril_istimeout(timer, 200));
    
    if (start == NULL) {
        RIL_ERR("KTCP_DATA recv failed\r\n");
        return;
    }
    ndata = atoi(ctx->buf);
    start++;
    
    RIL_DBG("Data input %d size.\r\n", ndata);
    remain = &ctx->buf[readcnt] - start;
    
    /* 递交首包剩余数据 */
    if (ndata > remain) {
        ril_socket_notify(s, SOCK_NOTFI_DATA_REPORT, start, remain);
        ndata -= remain;
    } else {
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
        
        if (readcnt) {
            ndata -= readcnt;       
            ril_socket_notify(s, SOCK_NOTFI_DATA_REPORT, ctx->buf, readcnt);            
        } else {
            ril_delay(1);
        }

    } 
    if (ndata)
        RIL_WARN("The remaining %d bytes of data cannot be read\r\n", ndata);
}
ril_urc_ex_register("+KTCP_DATA:", ",", tcp_udp_recv_notify);
ril_urc_ex_register("+KUDP_DATA:", ",", tcp_udp_recv_notify);


/**
 * @brief 接收数据
 * @param[in]   s   - socket
 * @param[in]   buf - 接收缓冲区
 * @param[in]   len - 缓冲区长度
 * @retval      实际接收到数据长度
 */
static unsigned int hl8518_recv(struct ril_device *dev, socket_base_t *s, 
                                void *buf, unsigned int max_recv_size)
{
    /*HL8518 的socket数据是通过URC直接上报的，故这里直接返回0*/
    return 0;
}

/**  
 * @brief    断开连接
 */
static int hl8518_disconnect(struct ril_device *dev, socket_base_t *s)
{
    if (s->type == RIL_SOCK_TCP) {       //对于TCP，先关闭再删除
        ril_exec_cmdx(NULL, "AT+KTCPCLOSE=%d", s->tag);
        return ril_exec_cmdx(NULL, "AT+KTCPDEL=%d", s->tag);
    } else {
        return ril_exec_cmdx(NULL, "AT+KUDPCLOSE=%d", s->tag);
    }
}


/**  
 * @brief    RIL请求
 */
int hl8518_request(struct ril_device *dev, ril_request_code num, void *data, 
                   int size)
{
    switch (num) {
    case RIL_REQ_GET_CONN_STATUS:
        return hl8518_netconn_status(dev, (ril_netconn_status *)data);
        
    default:
        return ril_request_default_proc(dev, num, data, size);
    }
}

/*
 * @brief  获取连接状态
 *         命令格式:
 *                  =>AT+KTCPSTAT=<session_id>
 *              
 *                  <=
 *                  +KTCPSTAT: <status>,<tcp_notif>,<rem_data>,<rcv_data>
 *                  OK
 */
static sock_request_status hl8518_conn_status(struct ril_device *dev, socket_base_t *s)
{    
    char recv[64];
    
    int status;
    
    if (s->type == RIL_SOCK_UDP)
        return SOCK_STAT_DONE;
    
    if (ril_exec_cmd(recv, sizeof(recv), "AT+KTCPSTAT=%d" ,s->tag) != RIL_OK)
        return SOCK_STAT_UNKNOW;
    
    //+KTCPSTAT: <status>,<tcp_notif>,<rem_data>,<rcv_data>
    if (sscanf(recv, "%*[^+]+KTCPSTAT:%d", &status) != 1)
        return SOCK_STAT_FAILED;
    
    switch (status) {
    case 2:
    case 4:    
        return SOCK_STAT_BUSY;
    case 3:
        return SOCK_STAT_DONE;
    case 0:
    case 1:
    case 5:             
    default:
        return SOCK_STAT_FAILED;        
    }
}


/*
 * @brief 获取发送状态
 */
static sock_request_status hl8518_send_status(struct ril_device *dev, socket_base_t *s)
{
    char recv[100];    
    int status, tcp_notif, rem_data, rcv_data;
    
    if (s->type == RIL_SOCK_UDP)
        return SOCK_STAT_DONE;
    
    if (ril_exec_cmd(recv, sizeof(recv), "AT+KTCPSTAT=%d",s->tag) != RIL_OK)
        return SOCK_STAT_FAILED;
    
    if (sscanf(recv, "%*[^+]+KTCPSTAT:%d,%d,%d,%d", &status, &tcp_notif, 
               &rem_data, &rcv_data) != 4)
        return SOCK_STAT_FAILED;
    if (tcp_notif != -1)
        return SOCK_STAT_FAILED;
    
    return rem_data == 0 ? SOCK_STAT_DONE : SOCK_STAT_BUSY;
}

/*******************************************************************************
 * @brief       TCP数据发送状态更新 [+KTCP_ACK: <session_id>,<result> <CR><LF>]
 *              <session_id> Index of the TCP session
 *              <result> 0 Data sent failure
 *                       1 Data sent success
 * @param[in]   buf  
 * @retval      none 
 ******************************************************************************/
static void tcp_sendack_urc(at_urc_ctx_t *ctx)
{
    int session, status;
    socket_base_t *s;
    if (sscanf(ctx->buf, "+KTCP_ACK: %d,%d", &session, &status) == 2){
        
        s = find_socket_by_tag((void *)session);  
        if (s == NULL) {
            //异常session,应强制关闭
            //....
            return;
        }
        if (status)
            ril_socket_notify(s, SOCK_NOTFI_SEND_SUCCESS, 0, 0);
        else
            ril_socket_notify(s, SOCK_NOTFI_SEND_FAILED, 0, 0);
    }
}
ril_urc_register("+KTCP_ACK:", tcp_sendack_urc);

/*
 * @brief   连接状态更新URC
 */
static void connection_updated_handler(at_urc_ctx_t *ctx)
{
    int status;
    ril_netconn_status netconn;
    if (sscanf(ctx->buf, "+KCNX_IND: %d",&status) == 1){
        netconn = status ? NETCONN_ONLINE : NETCONN_OFFLINE;        
        ril_notify(RIL_NOTIF_NETCONN, &netconn, sizeof(int *));    
    }
}ril_urc_register("+KCNX_IND:", connection_updated_handler);

/*******************************************************************************
 * @brief       Session 状态更新[+KTCP_NOTIF: <session_id>, <tcp_notif> ]
 *              
 * @retval      none 
 ******************************************************************************/
static void tcp_notif_urc(at_urc_ctx_t *ctx)
{
    int session, status;
    socket_base_t *s;
    if (sscanf(ctx->buf, "+KTCP_NOTIF: %d,%d", &session, &status) == 2) {        
        s = find_socket_by_tag((void *)session); 
        
        if (status == 8) {                 
            /*数据发送异常,强制终止 */    
            ril_send_singleline("--EOF--Pattern--");
        } else if (s)
            ril_socket_notify(s, SOCK_NOTFI_OFFLINE, NULL, 0);    //连接断开
    }
}
ril_urc_register("+KTCP_NOTIF:", tcp_notif_urc);

/*
 * @brief sim卡丢失事件
 */
static void sim_urc_handler(at_urc_ctx_t *ctx)
{
    ril_sim_status sim = SIM_ABSENT;
    ril_notify(RIL_NOTIF_SIM, &sim, sizeof(int *));              //发送SIM卡丢失事件
}ril_urc_register("+CPIN: 0", sim_urc_handler);


/*HL8518设备函数操作集 --------------------------------------------------------*/
static const ril_device_ops_t hl8518 = {
    .init            = hl8518_init,
    .startup         = hl8518_startup,
    .shutdown        = hl8518_shutdown,
    .pdp_setup       = hl8518_pdp_setup,
    .pdp_contrl      = hl8518_pdp_ctrl,
    .request         = hl8518_request,
    .sock = {
        .connect     = hl8518_sock_connect,
        .disconnect  = hl8518_disconnect,
        .send        = hl8518_sock_send,
        .recv        = hl8518_recv,
        .conn_status = hl8518_conn_status,
        .send_status = hl8518_send_status
    }
};

/*安装hl8518 模组*/
ril_device_install("hl8518", hl8518);

