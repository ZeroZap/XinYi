/******************************************************************************
 * @brief        基于gsm0710协议虚拟串口管理
 *
 * Copyright (c) 2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2020-06-20     Morro        Initial version. 
 ******************************************************************************/
#include "vcom.h"
#include "mux.h"
#include <stdio.h>

#pragma section="_section_vcom"
#define vcom_start  __section_begin("_section_vcom")        
#define vcom_end    __section_end("_section_vcom") 

#define bit_set(mask, bit)    ((mask) |= 1 << (bit))
#define bit_clr(mask, bit)    ((mask) &= ~(1 << (bit)))
#define bit_check(mask, bit)  ((mask) & (1 << (bit)))

typedef struct {
    int  error;                                       /*错误计数*/
    bool start;
    unsigned int  request, open;                      /*请求打开通道,已打开通道*/
    mux_obj_t     mux;
    vcom_config_t vconfig;
}vcom_service_t;
static vcom_service_t vs;

static bool is_timeout(unsigned int start, int time)
{
    return vs.vconfig.get_ms() - start > time;
}

/*
 * @brief   检测通道是否需要打开
 */
static bool is_request(struct vcom *v)
{
    return bit_check(vs.request, v->channel);
}

/*
 * @brief   打开串口
 */
static void vcom_open(struct vcom *v)
{
    if (v->isopen(v))
        return;
    bit_set(vs.request, v->channel);
}

/*
 * @brief   关闭串口
 */
static void vcom_close(struct vcom *v)
{ 
    bit_clr(vs.request, v->channel);
    bit_clr(vs.open, v->channel);
    mux_close_channel(&vs.mux, v->channel);
    v->busy  = 0;
}

/*
 * @brief   发送数据到串口
 */
static bool vcom_send(struct vcom *v, const void *buf, int count)
{
    if (!v->isopen(v))
        return false;
    return mux_send_frame(&vs.mux, v->channel, MUX_UIH, buf, count); 
}

/*
 * @brief     测虚拟串口是否打开
 */
static bool vcom_isopen(struct vcom *v)
{
    return bit_check(vs.open, v->channel);
}

/*
 * @brief     搜索vcom
 */
struct vcom *search_vcom(int channel)
{
    vcom_t *v = vcom_start;
    while (v < (vcom_t *)vcom_end) {
        if (v->channel == channel)
            return v;
        else 
            v++;
    }

    return NULL;
}

/*
 * @brief   虚拟串口监视
 */
static void vcom_watch(vcom_t *v)
{        
    if (is_request(v) && !vcom_isopen(v)) {            //未打开成功
        if (!v->busy) {
            mux_open_channel(&vs.mux, v->channel);
            v->busy  = 1;
        } else if (is_timeout(v->timer, 2000)) {       //连接超时
            v->timer = vs.vconfig.get_ms();
            VCOM_DBG("Channel %d connect retry.\r\n", v->channel);
            if (vs.error++ > 3 && vs.vconfig.error) {
                 vs.vconfig.error();
                 vs.error = 0;
            } else 
                mux_open_channel(&vs.mux, v->channel);
        }
    }
}


/*
 * @brief     虚拟串口打开事件
 */
void onVcomOpen(int channel)
{
    vcom_t *v = search_vcom(channel);
    vs.error = 0;
    if ((v = search_vcom(channel))) {
        bit_set(vs.open, channel);
        v->busy = 0;
        VCOM_DBG("VCOM %d connected.\r\n", channel);
    }
}

/*
 * @brief     虚拟串口关闭事件
 */
void onVcomClose(int channel)
{
    vcom_t *v = search_vcom(channel);
    if ((v = search_vcom(channel))) {
        bit_clr(vs.open, channel);
        VCOM_DBG("VCOM %d disconnected.\r\n", channel);
    }
}
/*
 * @brief     数据接收处理
 */
void onDataRecv(int channel, const void *data, int len)
{
    vcom_t *v = search_vcom(channel);
    if ((v = search_vcom(channel)) && v->e.recv) {
        v->e.recv(data, len);
    }
}

/*
 * @brief     虚拟串口服务初始化
 * @param[in] config - 服务配置
 */
void onVcomTerminate(void)
{
    if (vs.start) {
        vs.open = 0;
        VCOM_DBG("VCOM terminate.\r\n");
    }
}

/*
 * @brief     mux接收事件
 */
void onMuxEvent(int channel, unsigned char type, const void *buf, int size)
{
    //VCOM_DBG("channel:%d, type:%d\r\n", channel, type);
    if (type == MUX_UIH || type == MUX_UI) {
        onDataRecv(channel, buf, size);
    } else if (type == MUX_UA){
        onVcomOpen(channel);
    } else if (type == MUX_DISC || type == MUX_DM) {
        onVcomClose(channel);
    }
}

/*
 * @brief     虚拟串口服务初始化
 * @param[in] config - 服务配置
 */
void vcom_service_init(const vcom_config_t *config)
{
    static mux_adater_t mux_adt;
    
    mux_adt.write = config->write;
    mux_adt.read  = config->read;
    mux_adt.get_tick = config->get_ms;
    mux_adt.recvEvent = onMuxEvent;
    
    vcom_t *v = vcom_start;
    mux_init(&vs.mux, &mux_adt);
    vs.vconfig    = *config;
    while (v < (vcom_t *)vcom_end) {
        v->open  = vcom_open;        
        v->close = vcom_close;
        v->send  = vcom_send;
        v->isopen= vcom_isopen; 
        v->busy  = 0;
        
        v++;
    }
}

/*
 * @brief   启动虚拟串品服务
 */
void vcom_service_start(void)
{
    vs.start = 1;
}

/*
 * @brief   停止虚拟串品服务
 */
void vcom_service_stop(void)
{
    vs.start = 0;
}

/*
 * @brief   虚拟串口服务线程
 */
void vcom_service_thread(void)
{
    static unsigned int time;
    vcom_t *v = vcom_start;
    if (!vs.start) {
        if (vs.open) {
            while (v < (vcom_t *)vcom_end)
                mux_close_channel(&vs.mux, v->channel);
            vs.open = 0;
        }
        return;
    }
    
    if (is_timeout(time, 100)) {
        time = vs.vconfig.get_ms();
        while (v < (vcom_t *)vcom_end)
            vcom_watch(v++);        
    }
    mux_process(&vs.mux);
}
