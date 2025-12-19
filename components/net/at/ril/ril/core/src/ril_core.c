/******************************************************************************
 * @brief   RIL core services
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
#include "ril_port.h"
#include "ril_device_impl.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Notification data ---------------------------------------------------------*/
typedef struct {
    ril_notify_type type;
    int   size;
    unsigned char data[0];
} notify_data_t;

void ril_socket_init(void);
void ril_socket_status_watch(void);
void ril_sock_dispose(void);

/**
  * @brief   Indicates whether the device is registered to the network.
  */
bool net_isreg(ril_obj_t *r)
{
    return r->status.netreg == NETREG_REG || r->status.netreg == NETREG_ROAMING;
}

/**
  * @brief   Indicates whether the device is connected to the network.
 */
bool net_isconn(ril_obj_t *r)
{
    return r->status.conn == NETCONN_ONLINE;
}

/**
 * @brief   Indicates whether the SIM card is ready.
 */
bool sim_isready(ril_obj_t *r)
{
    return r->status.sim == SIM_READY;
}

/**
 * @brief   Probe whether the module is run normally.
 */
static bool device_probe(ril_obj_t *r)
{
    return at_do_cmd(&r->at, NULL, "AT") == AT_RET_OK;
}

/**
 * @brief   关机处理
 */
static void __ril_off(ril_obj_t *r, ril_ctx_t *ctx)
{
    if (r->isopen) {        
        r->status.conn = NETCONN_OFFLINE;
        ril_dev_pdp_contrl(&r->dev, false);                  /* 去激活PDP*/        
        ril_sock_dispose();                                  /* 清理socket资源*/        
        ril_dev_shutdown(&r->dev);                           /* 关机*/
        r->isopen = false;        
        RIL_INFO("Shutdown\r\n");
    } 
    
    if (r->open_enable)
        ctx->next_state(RIL_STAT_STARTUP);
}

/**
 * @brief   Processing device exception.
 * @param[in]   none
 */
static void __ril_error_proc(ril_obj_t *r, ril_ctx_t *ctx)
{
    r->error = true;    
    __ril_off(r, ctx);  
    r->error  = false;
}

/**
 * @brief  设备开机处理
 */
static void __ril_on(ril_obj_t *r, ril_ctx_t *ctx)
{
    /* 开机异常等待时间表 -----------------------------------------------------*/
    const short wait_tbl[] = {0, 20, 50, 150, 400, 600};    
    if (r->open_enable) {
        if (ctx->istimeout(1000 * wait_tbl[ctx->retry % ARRAY_COUNT(wait_tbl)] )) {
            RIL_INFO("Startup\r\n");
            if (ril_dev_startup(&r->dev) != RIL_OK) {
                if (ctx->retry < ARRAY_COUNT(wait_tbl) - 1) {  /* 开机异常统计 */ 
                    ctx->retry++;
                }
                RIL_ERR("Startup failed, retry:%d\r\n", ctx->retry);
                ril_dev_shutdown(&r->dev);
                ctx->reset_timer();
            } else {
                r->isopen = true;                
                ril_send_singleline("ATE0");                   /* 强制关闭回显*/
                ctx->next_state(RIL_STAT_SIM);
                RIL_INFO("Startup OK\r\n");
                RIL_INFO("Check SIM Card...\r\n");
            }
        }
    }
}

/**
 * @brief  复位设备
 */
static void __ril_reset(ril_obj_t *r, ril_ctx_t *ctx)
{
    RIL_INFO("Device reset...\r\n");
    ril_dev_reset(&r->dev);
    at_delay(1000);
    if (ril_send_singleline("AT") == RIL_OK)
        ctx->next_state(RIL_STAT_INIT);
    else
        ctx->next_state(RIL_STAT_STARTUP);
}

/**
 * @brief  sim卡检测
 */
static void __ril_check_sim(ril_obj_t *r, ril_ctx_t *ctx)
{
     /*sim卡检测间隔 ---------------------------------------------------------*/
    const char sim_check_tbl[] = {0, 2, 3, 5, 15, 30};   
    if (ctx->runtime() >  600 * 1000) {      /*600s检测不到sim卡,关机重新开始  */
        ctx->next_state(RIL_STAT_OFF);
        RIL_WARN("SIM check timeout\r\n");
    } else if (ctx->istimeout(1000 * sim_check_tbl[ctx->retry % 
                            ARRAY_COUNT(sim_check_tbl)] )) {
        ctx->reset_timer();
        
        if (ril_dev_request(&r->dev, RIL_REQ_GET_SIM_STATUS, &r->status.sim, 
                            sizeof(ril_sim_status)) != RIL_OK)
            r->status.sim = SIM_UNKNOW;
        
        if (r->status.sim == SIM_READY) {
            ctx->next_state(RIL_STAT_INIT);    /* 检测到卡,转到初始化状态*/
            RIL_INFO("SIM Ready\r\n");
        } else if (r->status.sim == SIM_PIN || r->status.sim == SIM_PUK) {
            RIL_WARN("SIM lock\r\n");          /* 卡被锁了*/
        }
        if (ctx->retry < ARRAY_COUNT(sim_check_tbl) - 1) 
            ctx->retry++;
    }
}

/**
 * @brief  设备初始化
 */
static void __ril_init(ril_obj_t *r, ril_ctx_t *ctx)
{
    const char *cmds[] = {     //通用初始化命令
        "AT+CLIP=1",
        "AT+CREG=0",
        "AT+CGREG=0",
        "AT+CTZU=1",           //Automatic Time Zone Update
        "AT+CTZR=1",
        "AT+CCLK?",
        NULL
    };
    RIL_INFO("Device initialize...\r\n"); 
    ril_send_multiline(cmds);
    ril_dev_init(&r->dev);     //设备相关初始化
    sms_init(&r->dev);         //短信初始化
    ctx->next_state(RIL_STAT_NETREG);
    RIL_INFO("Wait for network registration...\r\n");
}

/**
 * @brief  注册管理
 */
static void __ril_netreg(ril_obj_t *r, ril_ctx_t *ctx)
{
    /* 网络注册查询间隔 -------------------------------------------------------*/
    const char ret_chk_tbl[] = {0, 2, 4, 5, 5, 5, 5, 5, 10};
    unsigned int timeout;
    ril_csq_t csq;
    if (!sim_isready(r)) {
        ctx->next_state(RIL_STAT_SIM);                     //未插入卡
        return;
    }
    if (r->conn_enable) {
        timeout = 1000 * ret_chk_tbl[ctx->retry % ARRAY_COUNT(ret_chk_tbl)];
        //定时更新网络注册状态
        if (ctx->istimeout(timeout)) {
            ctx->reset_timer();
            ril_dev_request(&r->dev, RIL_REQ_GET_CSQ, &csq, sizeof(csq)); //更新csq值
            //更新注册状态
            ril_dev_request(&r->dev, RIL_REQ_GET_REG_STATUS, &r->status.netreg, 
                            sizeof(ril_netreg_status));   
            if (ctx->retry < ARRAY_COUNT(ret_chk_tbl) - 1) 
                ctx->retry++;
            
            RIL_INFO("Update registration status:%d\r\n", r->status.netreg);
        }
        if (net_isreg(r)) {
            RIL_INFO("Register successfully, rssi:%d\r\n", csq.rssi);
            ctx->next_state(RIL_STAT_NETCONF); 
        } 
        if (ctx->runtime() > 600 * 1000) {                //10 min 都没注册成功
            //进入手动注册模式
            //重启模组
            ctx->next_state(RIL_STAT_RESET);           
        }
    }
}
/**
 * @brief  配置网络
 */
static void __ril_netconfig(ril_obj_t *r, ril_ctx_t *ctx)
{
    RIL_INFO("PDP setup...\r\n"); 
    ril_dev_pdp_setup(&r->dev);      
    if (ril_dev_pdp_contrl(&r->dev, true) != RIL_OK) {  //激活PDP
        RIL_WARN("PDP activation failed...\r\n"); 
    }                 
    r->status.conn = NETCONN_PENDING;     
    ctx->next_state(RIL_STAT_CONN);
}
/**
 * @brief  网络连接管理
 */
static void __ril_netconn(ril_obj_t *r, ril_ctx_t *ctx)
{    
    /* 网络连续查询间隔 -------------------------------------------------------*/
    const short conn_chk_tbl[] = {0, 2, 4, 5, 5, 5, 5, 5, 10};    
    unsigned int timeout;
    if (!net_isreg(r)) {
        ctx->next_state(RIL_STAT_NETREG);                //未注册网络
        return;
    }
    if (!r->conn_enable) {        
        ril_dev_pdp_contrl(&r->dev, false);              //关闭网络 
        r->status.conn = NETCONN_OFFLINE;
        ctx->next_state(RIL_STAT_NETREG);
    } else if (net_isconn(r)){
        ctx->next_state(RIL_STAT_ONLINE);
        RIL_INFO("Online\r\n");
    } else if (ctx->runtime() < 300 * 1000) {
        timeout = 1000 * conn_chk_tbl[ctx->retry % ARRAY_COUNT(conn_chk_tbl)];
        if (ctx->istimeout(timeout))  {                //每隔一段时间更新网络状态
            ctx->reset_timer();
            ril_dev_request(&r->dev, RIL_REQ_GET_CONN_STATUS, &r->status.conn,
                            sizeof(ril_netconn_status));
            if (ctx->retry < ARRAY_COUNT(conn_chk_tbl) - 1) 
                ctx->retry++;
            else {
                RIL_WARN("Try activating the PDP again...\r\n"); 
                ril_dev_pdp_contrl(&r->dev, false);   //重新激活
                ril_dev_pdp_contrl(&r->dev, true);
            }
        }
    } else {                                      
        RIL_ERR("Unable to connect to the network for a long time, restart the device.\r\n");
        ctx->next_state(RIL_STAT_RESET);                  //300s连不上网,重启模组
    }
}

/**
 * @brief  online
 */
static void __ril_online(ril_obj_t *r, ril_ctx_t *ctx)
{
    if (!net_isconn(r) || !r->conn_enable) {
        if (net_isconn(r))
            ril_dev_pdp_contrl(&r->dev, false);       //去激活PDP    
        
        ril_sock_dispose();                           //清理socket资源
        ctx->next_state(RIL_STAT_NETREG);             //掉线处理        
    }
}

/* ril 状态机*/
static void (*const ril_fsm[RIL_STAT_MAX])(ril_obj_t *,ril_ctx_t *) = 
{
    [RIL_STAT_OFF]   = __ril_off,
    [RIL_STAT_RESET] = __ril_reset,
    [RIL_STAT_STARTUP]  = __ril_on,
    [RIL_STAT_INIT]  = __ril_init,
    [RIL_STAT_SIM]   = __ril_check_sim,    
    [RIL_STAT_NETCONF]  = __ril_netconfig,
    [RIL_STAT_NETREG]= __ril_netreg,
    [RIL_STAT_CONN]  = __ril_netconn,
    [RIL_STAT_ONLINE]= __ril_online,
    //[RIL_STAT_CALL]  = NULL,
    [RIL_STAT_ERR]   = __ril_error_proc
};


/** 
 * @brief       sim卡状态更新
 */
static void on_sim_status_chagned(ril_obj_t *r, ril_sim_status new)
{
    if (r->status.sim != new) {
        r->status.sim = new;
        if (new != SIM_READY) {
            r->status.netreg = NETREG_UNREG;
            r->status.conn   = NETCONN_OFFLINE;
            r->ctx.next_state(RIL_STAT_SIM);
        }   
    }
}

/** 
 * @brief       网络注册状态更新
 */
static void on_netreg_status_chagned(ril_obj_t *r, ril_netreg_status new)
{
    if (r->status.netreg != new) {
        r->status.netreg = new;
        if (!net_isreg(r)) {
            r->status.conn   = NETCONN_OFFLINE;
            r->ctx.next_state(RIL_STAT_NETCONF);  //网络未注册                
        }  
    }
}

/** 
 * @brief       网络连接状态更新
 */
static void on_conn_status_chagned(ril_obj_t *r, ril_netconn_status new)
{
    if (r->status.conn != new) {
        r->status.conn = new;
        if (!net_isconn(r))
            r->ctx.next_state(RIL_STAT_NETREG);   //掉线处理
    }    
}

/**
 * @brief       ril 通知处理
 * @return      none
 */
static void ril_notify_proc(async_work_t *w, void *object, void *params)
{  
    static const ril_notify_item_t notify_start SECTION("_section.ril.notify.0") = {
        RIL_NOTIF_MAX, NULL
    };
    static const ril_notify_item_t notify_end   SECTION("_section.ril.notify.2") = {
        RIL_NOTIF_MAX, NULL
    };
    
    ril_obj_t *r          = (ril_obj_t *)object;
    notify_data_t *ndata  = params;
    ril_notify_type type  = ndata->type;
   const ril_notify_item_t  *it;
    RIL_INFO("Notification:%d, data size:%d\r\n", (int)type, ndata->size);
    switch (type) {
    case RIL_NOTIF_SIM:                                 //sim卡状态更新
        on_sim_status_chagned(r, *((ril_sim_status *)ndata->data));
        break;
    case RIL_NOTIF_NETREG:                              //网络状态更新
        on_netreg_status_chagned(r,  *((ril_netreg_status *)ndata->data));
        break;
    case RIL_NOTIF_NETCONN:                            //连接状态更新       
        on_conn_status_chagned(r, *((ril_netconn_status *)ndata->data));
        break;
    case RIL_NOTIF_ERROR:                              //未知错误处理
        r->ctx.next_state(RIL_STAT_ERR);
        break;
    case RIL_NOTIF_TIMEOUT:                            //通信超时
        if (!device_probe(r))
            ril_notify(RIL_NOTIF_ERROR, NULL, 0);
        else
            r->ctx.next_state(RIL_STAT_NETREG);
        break;
    default:
        break;
    }
    /**
     * 通知订阅者
     */
    for (it =  &notify_start + 1; it < &notify_end; it++) {
        if (it->type == type && it->handler)
            it->handler(ndata->data, ndata->size);
    }

    ril_free(ndata);
}


/**
 * @brief       ril通知(一般用于URC事件)
 * @param[in]   type - 通知类型
 *                  @arg  RIL_NOTIF_SIM           SIM 卡状态更新
 *                  @arg  RIL_NOTIF_NETSTAT       注册状态更新 
 *                  @arg  RIL_NOTIF_NETCONN       连接状态更新
 *                  @arg  RIL_NOTIF_SMS           收到短信
 *                  ...
 *
 * @param[in]   data       - 事件数据,参考ril_notify_type描述
 * @param[in]   data_size  - 事件数据长度
 * 
 */
int ril_notify(ril_notify_type type, void *data, int data_size)
{
    notify_data_t *n;
    bool result;
    ril_obj_t *r = get_ril_obj();
    n = ril_malloc(sizeof(notify_data_t) + data_size);
    if (n == NULL) {
        RIL_ERR("Unable to complete notification:%d \r\n", (int)type);
        return RIL_NOMEM;
    }    

    ril_enter_critical();
    
    n->type = type;
    n->size = data_size;
    if (data_size)
        memcpy(n->data, data, data_size);
    
    result = async_work_add(&r->workqueue, r, n, ril_notify_proc);
    ril_exit_critical();
    
    if (!result) {
        RIL_ERR("notify:%d failed\r\n", (int)type);
        ril_free(n);
        return RIL_ERROR;
    }
    return RIL_OK;
}

/**
 * @brief       执行ril异步作业
 * @param[in]   params  - 作业参数
 * @param[in]   work    - 作业入口
 * @return      true - 执行成功, false - 执行失败
 */
bool ril_do_async_work(void *params, ril_async_work_t work)
{
    int ret;
    ril_obj_t *r = get_ril_obj();
    ril_enter_critical();    
    ret = async_work_add(&r->workqueue, r, params, (async_work_func_t)work);
    ril_exit_critical();
    return ret;
}

/**
 * @brief       ril 电源监视
 * @return      none
 */
static void ril_power_watch(ril_obj_t *r)
{
    ril_ctx_t  *ctx = &r->ctx;
    if (!r->open_enable && r->run_state != RIL_STAT_OFF) {
        ctx->next_state(RIL_STAT_OFF);
    }
}

/**
 * @brief       ril 内核初始化
 * @return      none
 */ 
void ril_core_init(ril_obj_t *r)
{
    ril_socket_init();
    //初始化异步作业
    async_work_init(&r->workqueue, r->work_node, 
                    sizeof(r->work_node) / sizeof(r->work_node)[0]);    
}

/** 
 * @brief       ril 内核处理
 * @return      none
 */
void ril_core_process(ril_obj_t *r)
{
    //ril 状态机
    ril_fsm[r->run_state](r, &r->ctx);

    //socket监控
    ril_socket_status_watch();

    //异步作业处理
    async_work_process(&r->workqueue);

    //电源管理
    ril_power_watch(r);        
}
