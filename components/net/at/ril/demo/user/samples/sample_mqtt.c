/******************************************************************************
 * @brief    RIL mqtt演示程序
 *
 * Copyright (c) 2020  <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2021-02-05     Morro        Initial version
 ******************************************************************************/
#include "ril.h"
#include "mqtt_client.h"
#include "taskManager.h"
#include <stdio.h>
#include <string.h>

#define MQTT_SERVER    "123.456.789.62"           //服务器地址
#define MQTT_PORT      1883                      //服务器端口
#define MQTT_RECV_SIZE 256                       //接收缓冲区长度

#define MQTT_PUB_NAME  "ril-mqtt"                //默认发布的主题
#define MQTT_SUB_NAME  "mqtt-device"             //默认订阅的主题

static void mqtt_event_handler(mqtt_client_t *mc, mqtt_event_args_t *args);

static mqtt_client_t *mc;                        //MQTT 客户端

//mqtt 配置参数
static const mqtt_config_t mqtt_config = {
    .client_id          = "ril-mqtt-demo",
    .host               = MQTT_SERVER,
    .port               = MQTT_PORT,
    .recvbuf_size       = MQTT_RECV_SIZE,
    .event_handler      = mqtt_event_handler,
    //可选参数
    .reconnect_interval = 30,                    //断开后30s自动重连
    .heartbeat_interval = 300,                   //5min 1个心跳包
    .clean_session      = true
};

/**
 * @brief       MQTT事件处理程序
 * @return      none
 */ 
static void mqtt_event_handler(mqtt_client_t *mc, mqtt_event_args_t *args)
{   
    mqtt_qos qos;
    switch (args->type) {
    case MQTT_EVENT_ONLINE:
        printf("MQTT online....\r\n");           //上线通知
        //连接成功订阅主题
        mqtt_client_subscribe(mc, MQTT_SUB_NAME, QOS1, &qos);
        break;
    case MQTT_EVENT_OFFLINE:
        printf("MQTT offline....\r\n");          //掉线通知
        break;
    case MQTT_EVENT_RECONNECT:                   //重连通知
        break;        
    case MQTT_EVENT_DATA:                        //消息通知
        args->payload[args->payload_size] = 0;
        printf("MQTT DATA > Topic:%s, Message:%s\r\n", args->topic, args->payload);
        //匆中此事件中调用,发布/订阅这些接口,会造成线程锁死
        break;        
    }
}

/**
 * @brief       定时发送数据
 * @return      none
 */ 
static void send_data_regularly(mqtt_client_t *mc)
{
    static unsigned int timer;                   //发送定时器    
    const char *msg = "MQTT device message...";
    //大约1min发送1条数据
    if (ril_istimeout(timer, 60 * 1000)) {
        timer = ril_get_ms();    
        mqtt_client_publish(mc, MQTT_PUB_NAME, (void *)msg, strlen(msg), QOS1);
    }
}

/**
 * @brief       MQTT 任务
 * @return      none
 */ 
static void mqtt_task(void)
{    
    mc = mqtt_client_create(&mqtt_config);         //创建mqtt 客户端    
    if (mc == NULL) {
        printf("mqtt client create failed.\r\n");
        while(1) {}
    }
    while (1) {
        os_sleep(10);
        mqtt_client_process(mc);                   //MQTT任务处理
        if (mqtt_client_online(mc)) {
            send_data_regularly(mc);               //定时上报
        }
    }
}
task_define("mqtt-sample", mqtt_task, 512, 7);     //定义mqtt任务

/**
 * @brief       mqtt 数据处理任务
 * @return      none
 */ 
static void mqtt_recv_task(void)
{
    while (1) {
        os_sleep(1);
        mqtt_client_recv(mc);
    }
}
task_define("mqtt-recv", mqtt_recv_task, 512, 6);

////////////////////////////////////////////////////////////////////////////////
//下面是测试命令
#include "cli.h"
#include <stdlib.h>
/**
 * @brief   MQTT 发布命令
 *          命令格式:mq-pub,topic,message,qos[0-2]
 * @example mq-pub
 */    
static int do_cmd_publish(struct cli_obj *cli, int argc, char *argv[])
{
    if (argc < 4) {
        cli->print(cli,"Parameter error...\r\n");
        return -1;
    }
    if (!mqtt_client_online(mc)) {
        cli->print(cli,"Connection not ready...\r\n");
        return -1;        
    }
    mqtt_client_publish(mc, argv[1], argv[2], strlen(argv[2]), (mqtt_qos)atoi(argv[3]));
    cli->print(cli,"OK\r\n");  
    return 0;
}cmd_register("mq-pub", do_cmd_publish, "mqtt publish"); //注册发布命令

/**
 * @brief   MQTT 订阅命令
 *          命令格式:mq-sub,topic,qos[0-2]
 * @example mq-pub
 */    
static int do_cmd_subscribe(struct cli_obj *cli, int argc, char *argv[])
{
    mqtt_qos qos;
    if (argc < 3) {
        cli->print(cli,"Parameter error...\r\n");
        return -1;
    }
    if (!mqtt_client_online(mc)) {
        cli->print(cli,"Connection not ready...\r\n");
        return -1;        
    }
    mqtt_client_subscribe(mc, argv[1], (mqtt_qos)atoi(argv[2]), &qos);
    cli->print(cli,"Subscribe topic:%s, qos:%d, grantedQoS:%d\r\n", 
               argv[1], atoi(argv[2]), qos); 
    
    return 0;
}cmd_register("mq-sub", do_cmd_subscribe, "mqtt subscribe"); //注册订阅命令

/**
 * @brief   MQTT 取消订阅命令
 *          命令格式:mq-unsub,topic
 * @example mq-pub
 */    
static int do_cmd_unsubscribe(struct cli_obj *cli, int argc, char *argv[])
{
    if (argc < 2) {
        cli->print(cli,"Parameter error...\r\n");
        return -1;
    }
    if (!mqtt_client_online(mc)) {
        cli->print(cli,"Connection not ready...\r\n");
        return -1;        
    }
    mqtt_client_unsubscribe(mc, argv[1]);
    cli->print(cli,"Unsubscribe topic:%s\r\n", argv[1]); 
    
    return 0;
}cmd_register("mq-unsub", do_cmd_unsubscribe, "mqtt subscribe"); //注册取消订阅命令
