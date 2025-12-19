/******************************************************************************
 * @brief    MQTT 客户端管理
 *
 * Copyright (c) 2021 <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2021-07-16     Morro        Initial version
 * 2021-11-28     Morro        优化自动重连管理
 ******************************************************************************/
#include "ril.h"
#include "ril_socket.h"
#include "MQTTPacket.h"
#include "mqtt_client.h"
#include <stdbool.h>
#include <string.h>
#include "comdef.h"

#define MAX_PACKET_ID 0xFFFF

/**
 * @brief    MQTT 请求状态
 */
typedef enum {
    MQTT_REQ_IDLE,
    MQTT_REQ_BUSY,
    MQTT_REQ_DONE,
    MQTT_REQ_FAILED,
    MQTT_REQ_TIMEOUT
} mqtt_request_state;

/**
 * @brief    MQTT 客户端信息
 */
typedef struct mqtt_info {
    mqtt_client_t  client;
    mqtt_config_t  config;                             /* 配置参数*/
    ril_sem_t      mutex;                              /* 互斥锁*/
    ril_sem_t      sem_ready;                          /* 就绪信号*/
    ril_socket_t   sockfd; 
    mqtt_request_state conn_state;                     /* 连接请求状态 */
    /* 发送请求状态(发布,订阅,解除订阅)*/
    mqtt_request_state send_state;
    unsigned char  state;                              /* 接收解析器状态 */
    unsigned char  reconnect_retry;                    /* 重连重次*/
    unsigned short packet_id;                          /* 当前包ID*/    
    unsigned int   reconnect_timer;                    /* 重连定时器*/
    unsigned int   recv_timer;                         /* 接收定时器*/
    unsigned int   keep_alive_timer;                   /* 心跳定时器 */
    unsigned int   last_sent, last_recv;               /* 记录最后1次发送/接收时刻*/
    bool           connected;                          /* 连接状态*/
    unsigned       error_cnt        : 8;               /* 异常计数*/    
    unsigned       ping             : 1;               /* 心跳发送标志*/
    unsigned       reconnect : 1;                      /* 重连使能*/
    unsigned short total;                              /* 总字节数*/
    unsigned short recvcnt;                            /* 接收计数器*/    
    unsigned short bufsize;
    unsigned char  buf[0];                             /* 接收缓冲区*/
} mqtt_info_t;

/**
 * @brief      连接状态判断
 */
static bool is_connected(mqtt_info_t *mi)
{
    return mi->connected;
}
//下一包的包ID
static int getNextPacketId(mqtt_info_t *mi)
{
    return mi->packet_id = (mi->packet_id == MAX_PACKET_ID) ? 1 : mi->packet_id + 1;
}

/**
 * @brief      异常处理
 * @params[in] errno - 错误码
 */
static void mqtt_error_check(mqtt_info_t *mi, int errno)
{
    if (errno == RIL_OK)
        mi->error_cnt = 0;
    else 
        mi->error_cnt++;
    //待添加处理程序....
}

/**
 * @brief   启动mqtt 会话
 */
static int start_session(mqtt_info_t *mi)
{
    return ril_sock_connect(mi->sockfd, mi->config.host, mi->config.port, RIL_SOCK_TCP);
}


/**
 * @brief   停止mqtt 会话
 */
static int stop_session(mqtt_info_t *mi)
{
    return ril_sock_online(mi->sockfd) ? ril_sock_disconnect(mi->sockfd) : RIL_OK;
}

/**
 * @brief	   发送数据包
 */
static int send_parket(mqtt_info_t *mi, void *buf, unsigned int size)
{
    int ret;
    unsigned int timer = ril_get_ms();
    int retry;
    while (ril_sock_busy(mi->sockfd) && !ril_istimeout(timer, 10 * 1000)) {
        ril_delay(10);
        if (++retry == 100){
            retry = 0;
            MQTT_DBG("MQTT send busy....\r\n");
        }
    }
    ret = ril_sock_send(mi->sockfd, buf, size);
    if (ret == RIL_OK)
        mi->last_sent = ril_get_ms();
    return ret;
}

/**
 * @brief	   上锁
 * @retval     none
 */
static void mqtt_lock(mqtt_info_t *mi)
{
    ril_sem_wait(mi->mutex, 0);
}

/*
 * @brief	   解锁
 * @retval     none
 */
static void mqtt_unlock(mqtt_info_t *mi)
{
    ril_sem_post(mi->mutex);
}
/*
 * @brief	   事件处理
 */
static void event_invoke(mqtt_info_t *mi,  mqtt_event_args_t *args)
{
    if (mi->config.event_handler) {
        mi->config.event_handler(&mi->client, args);          //递交到上层处理
    }    
}

/**
 * @brief	   PUBLISH包处理
 * @retval     true | false
 */
static bool publish_packet_process(mqtt_info_t *mi)
{
    mqtt_event_args_t *args;
    int         qos, len;
    unsigned char dup, retain;
    unsigned short id;
    MQTTString  topic_name;
    bool        result = false;
    
    args = ril_malloc(sizeof(mqtt_event_args_t) + mi->recvcnt);
    
    if (args == NULL) {
        MQTT_DBG("Publish packet malloc failed\r\n");
        return false;
    }
    if (!MQTTDeserialize_publish(&dup, &qos, &retain, &id, &topic_name,
                                 (unsigned char **)&args->payload, 
                                 &args->payload_size, 
                                 mi->buf, mi->recvcnt)){
        MQTT_DBG("Packet[%d] parse failed\r\n", mi->buf[0]);
        goto exit;
    }
    args->type   = MQTT_EVENT_DATA; 
    args->qos    = (mqtt_qos)qos;  
    args->dup    = dup;
    args->retain = retain;
    if (topic_name.cstring) {
        args->topic      = topic_name.cstring;
        args->topic_size = strlen(topic_name.cstring);
    } else {
        args->topic  = topic_name.lenstring.data;
        args->topic_size = topic_name.lenstring.len;
    }
    event_invoke(mi, args);                              //递交到上层处理
    
    result = true;
    //应答主机
    if (qos != QOS0) {
        len = MQTTSerialize_ack(mi->buf, mi->bufsize, qos == QOS1 ? PUBACK : PUBREC, 0, id);
        if (len <= 0)
            goto exit;
        else {
            result = send_parket(mi, mi->buf, len) == RIL_OK;
        }
    }
exit:    
    ril_free(args);
    return result;
}
/**
 * @brief	   发布释放包处理
 * @retval     true | false
 */
static bool pubrel_packet_process(mqtt_info_t *mi)
{
    int len;
    unsigned short id;
    unsigned char  dup, type;
    MQTTHeader header;
    header.byte = mi->buf[0];
    if (!MQTTDeserialize_ack(&type, &dup, &id, mi->buf, mi->bufsize))
        return false;        
        
    if ((len = MQTTSerialize_ack(mi->buf, mi->bufsize, 
              (header.byte == PUBREC) ? PUBREL : PUBCOMP, 0, id)) <= 0)
        return false;                                      
    
    if (send_parket(mi, mi->buf, len) != RIL_OK)
        return false;

    //这里就不等PUBCOMP最后一包了,直接认为发成功
    if (mi->send_state == MQTT_REQ_BUSY)
        ril_sem_post(mi->sem_ready); 
    
    return true;
}

/**
 * @brief	   mqtt 包解析
 * @retval     none
 */
static void mqtt_packet_parse(mqtt_info_t *mi)
{
    MQTTHeader header;

    header.byte = mi->buf[0];

    switch (header.bits.type) {        
    case CONNACK:                          //连接包回复   
        if (mi->conn_state == MQTT_REQ_BUSY) {
            ril_sem_post(mi->sem_ready);
        }  
        break;
    case PUBACK:                           //发布包对QoS1的确认
    case SUBACK:                           //订阅确认
    case UNSUBACK:                         //解除订阅确认
    case PUBCOMP:                          //QoS2第4包(server->client)
        MQTT_DBG("server ack :%d \r\n", header.bits.type);
        if (mi->send_state == MQTT_REQ_BUSY)
            ril_sem_post(mi->sem_ready);
        break;
        
    case PUBLISH: {                        //来自服务器发布的消息
        if (!publish_packet_process(mi)) { 
            MQTT_DBG("PUBLISH error\r\n"); //异常处理
        }
        break;
    }
    case PUBREC:                           //QoS2第2个包(server->client)
    case PUBREL: {                         //QoS2第3个包(client->server)
        if (!pubrel_packet_process(mi)) {  
            MQTT_DBG("PUBREL error\r\n");  //异常处理
        }        
        break;
    }
    case PINGRESP:
        MQTT_DBG("Ping response.\r\n");
        mi->ping = 0;         //ping 响应
        break;
    }
}
/**
 * @brief	   解析包长度
 * @retval     RIL_OK - 成功获取, RIL_ERROR - 异常, RIL_ONGOING - 进行中
 */
static int parse_packet_size(mqtt_info_t *mi)
{
    int i, multiplier;
    int byte;
    unsigned int total = 0;
    multiplier = 1;
    for (i = 1; i < mi->recvcnt; i++) {
        byte = mi->buf[i];
        total +=  (byte & 127) * multiplier;
        multiplier *= 128;
        if ((byte & 128) == 0) {
            mi->total = total;
            return RIL_OK;
        }
        if (i >= 4)
            return RIL_ERROR;
    }
    return RIL_ONGOING;
}


/**
 * @brief    心跳包
 */
static void keepalive(mqtt_info_t *mi)
{
    int len;
    if (mi->config.heartbeat_interval == 0)
        return; 
    mqtt_lock(mi);
    if (ril_istimeout(mi->last_sent, mi->config.heartbeat_interval * 1000) && 
        ril_istimeout(mi->last_recv, mi->config.heartbeat_interval * 1000)) {            
            if (mi->ping) {
                mi->ping = 0;                
                goto exit;
            } else {
            len = MQTTSerialize_pingreq(mi->buf, mi->bufsize);
            
            if (len > 0 && send_parket(mi, mi->buf, len) == RIL_OK) {
                MQTT_DBG("Mqtt ping...\r\n");
                mi->keep_alive_timer = ril_get_ms();
                mi->ping = 1;
            }
        }
    }
exit:
    mqtt_unlock(mi);
}

/**
 * @brief    重连处理
 */
static void reconnect_process(mqtt_info_t *mi)
{
    //重连间隔表(S)
    static const unsigned short interval_tbl[] = {1, 3, 10, 20, 60, 120, 300};
    unsigned int interval;
    mqtt_event_args_t args;
    if (!mi->config.reconnect_enable || is_connected(mi)) 
        return;
    interval = interval_tbl[mi->reconnect_retry % ARRAY_COUNT(interval_tbl)];     
    
    if (ril_istimeout(mi->reconnect_timer, interval * 1000)) {
        mi->reconnect_timer = ril_get_ms();
        if (mqtt_client_connect(&mi->client) == RIL_OK) {
            mi->reconnect_retry = 0;
            MQTT_DBG("Reconnection successful.\r\n");            //重连成功   
            args.payload_size = 0;            
            args.type      = MQTT_EVENT_RECONNECT;
            event_invoke(mi, &args);            
        } else {
            /* 重连异常统计 */ 
            if (mi->reconnect_retry < ARRAY_COUNT(interval_tbl) - 1) { 
                mi->reconnect_retry++;
            }            
        }
    }
}

/**
 * @brief	   mqtt 数据解析
 * @retval     none
 */
static void mqtt_data_parse(mqtt_info_t *mi)
{
    
    int len, ret;
    len = ril_sock_recv(mi->sockfd, &mi->buf[mi->recvcnt], 
                        mi->bufsize - mi->recvcnt);
    mi->recvcnt += len;    
    if (len == 0) {
        if(mi->state && ril_istimeout(mi->recv_timer, 3000)) {  //接收超时处理
            mi->state   = 0;
            mi->recvcnt = 0;
            MQTT_DBG("Recv timeout.\r\n");          
        }
        return;
    }
    //Packet type
    if (mi->state == 0 && mi->recvcnt > 1) {      
        mi->state++;
        mi->recv_timer = ril_get_ms();
    } 
    //Remaining Length  
    if (mi->state == 1) {                         
        ret = parse_packet_size(mi);                           //解析包大小
        if (ret == RIL_OK)
            mi->state++;
        else if (ret == RIL_ERROR) {
            MQTT_DBG("Remaining Length error.\r\n"); 
            mi->state = 0; 
        }
    }
    
    if (mi->state == 2 && mi->recvcnt >= mi->total) {
        mqtt_packet_parse(mi);        
        mi->last_recv = ril_get_ms(); //记录最后一包接收时刻
        mi->state   = 0;              //解析完一帧,回到初始状态
        mi->recvcnt = 0;        
    }
}

/**
 * @brief      创建mqtt客户端
 * @param[in]  e    - 事件处理接口
 * @param[in]  host - 主机地址(www.xxx.com)
 * @param[in]  port - 端口(一般填1883)
 * @return     NULL - 创建失败, 其它值 - mqtt客户端
 */
mqtt_client_t *mqtt_client_create(const mqtt_config_t *config)
{
    mqtt_info_t *info;
    info = (mqtt_info_t *)ril_malloc(sizeof(mqtt_info_t) + config->recvbuf_size);
    if (info == NULL)
        return NULL;
    
    memset(info, 0, sizeof(mqtt_info_t));
    info->config    = *config;                                  //配置参数
    info->mutex     = ril_sem_new(1);                           //互斥量            
    info->sem_ready = ril_sem_new(0);                           //完成量   
           
    info->sockfd    =  ril_sock_create(NULL, 512);              //创建socket            
    if (info->sockfd == RIL_INVALID_SOCKET){
        ril_free(info);        
        return NULL;
    }
    
    info->recvcnt = 0;
    info->bufsize = config->recvbuf_size;
    return &info->client;    
}

/**
 * @brief    销毁mqtt客户端
 */
void mqtt_client_destroy(mqtt_client_t *mc)
{
    mqtt_info_t *mi = container_of(mc, mqtt_info_t, client);  
    ril_sock_destroy(mi->sockfd);
    ril_sem_free(mi->mutex);
    ril_sem_free(mi->sem_ready);    
    ril_free(mi);
}

/**
 * @brief	   连接服务器
 * @params[in] mc      - mqtt_client
 * @return     RIL_OK  -  连接成功, 其它值 - 异常
 */
int mqtt_client_connect(mqtt_client_t *mc)
{    
    unsigned char sendbuf[128];
    int len, ret;        
    unsigned char rc, sessionPresent;
    mqtt_info_t *mi = container_of(mc, mqtt_info_t, client);
    mqtt_config_t *mconfig;
    mqtt_event_args_t args;
    MQTTPacket_connectData options = MQTTPacket_connectData_initializer;
    
    if (!ril_isonline())
        return RIL_REJECT;                       //网络不可用
    
    if (is_connected(mi))                        //已上线
        return RIL_OK;
    
    //构造连接参数
    mconfig                  = &mi->config;
    options.cleansession     = mconfig->clean_session;
    options.keepAliveInterval= mconfig->heartbeat_interval;
    options.username.cstring = (char *)mconfig->username;
    options.password.cstring = (char *)mconfig->userpwd;
    options.clientID.cstring = (char *)mconfig->client_id;
    options.willFlag         = mconfig->will_options.will_flag;
    options.will.qos         = mconfig->will_options.qos;
    options.will.retain      = mconfig->will_options.retain;
    options.will.topicName.cstring = (char *)mconfig->will_options.topic;
    options.will.message.cstring   = (char *)mconfig->will_options.msg;
    mqtt_lock(mi);
    mi->conn_state          = MQTT_REQ_BUSY;    
    ret = start_session(mi);
    if (ret != RIL_OK) {
        MQTT_DBG("Server connection failed.\r\n");
        goto exit;
    }
    len = MQTTSerialize_connect(sendbuf, sizeof(sendbuf), &options);  
    if (len <= 0) {
        ret =  RIL_INVALID;
        goto exit;
    }
    /* 发送连接包*/
    if ((ret = send_parket(mi, sendbuf, len)) != RIL_OK)
        goto exit;
    /* 等待连接结束*/
    if (ril_sem_wait(mi->sem_ready, MQTT_CONN_TIMEOUT * 1000)) {        
        if (MQTTDeserialize_connack(&sessionPresent, &rc, mi->buf, 
                                    mi->bufsize) == 1 && rc == 0) {
            mi->conn_state = MQTT_REQ_DONE;
            mi->connected = true;
        } else {
            mi->conn_state = MQTT_REQ_FAILED;
            mi->connected = false;    
            ret = RIL_FAILED;
        }
    } else {
        mi->conn_state = MQTT_REQ_TIMEOUT;
        ret = RIL_TIMEOUT;        
    }
exit:
    MQTT_DBG("%s to connect to server.\r\n", ret == RIL_OK ? "Successfully":"Failed");
    /* 连接失败处理*/
    if (ret != RIL_OK)
        stop_session(mi);
    
    mqtt_unlock(mi);
    /* 生成上线事件*/
    if (ret == RIL_OK) {
        args.payload_size = 0;
        args.type      = MQTT_EVENT_ONLINE;  
        event_invoke(mi, &args);
    }
    return ret;
}
/**
 * @brief	   上线状态
 * @params[in] mc - mqtt_client
 * @return     true -  已上线,可以收发数据, false - 离线
 */
bool mqtt_client_online(mqtt_client_t *mc)
{
    return is_connected(container_of(mc, mqtt_info_t, client));
}

/**
 * @brief	   断开连接
 * @params[in] mc - mqtt_client
 * @return     RIL_OK  -  断开成功, 其它值 - 异常
 */
int mqtt_client_disconnect(mqtt_client_t *mc)
{
    int len, ret = RIL_OK;
    mqtt_info_t *mi = container_of(mc, mqtt_info_t, client);
    mqtt_lock(mi);
    if (is_connected(mi)) {
        len = MQTTSerialize_disconnect(mi->buf, mi->bufsize);
        if (len > 0)
            send_parket(mi, mi->buf, len);     //发送断开连接包
    }
    if (ril_sock_online(mi->sockfd))
        ret = ril_sock_disconnect(mi->sockfd);           //关闭 socket 
    mqtt_unlock(mi);
    MQTT_DBG("Disconnected.\r\n");
    return ret;
}

/**
 * @brief	   发布消息
 * @params[in] mc    - mqtt_client
 * @params[in] topic - 主题
 * @params[in] payload   - 数据
 * @params[in] payload_size   - 数据长度
 * @params[in] qos   - 消息质量
 * @retval     RIL_OK  -  发布成功, 其它值 - 异常
 */
int mqtt_client_publish(mqtt_client_t *mc, const char *topic, void *payload, 
                        int payload_size, mqtt_qos qos)
{
    int len, id, ret = RIL_ERROR;
    
    unsigned char *sendbuf;                         //发送缓冲区
    int send_bufsize = payload_size + 128;
    
    MQTTString topic_name = MQTTString_initializer;
    
    mqtt_info_t *mi = container_of(mc, mqtt_info_t, client);
    
    topic_name.cstring = (char *)topic;

    if (!is_connected(mi))                  //未连接,直接退出
        return RIL_REJECT;

    sendbuf = ril_malloc(send_bufsize);
    
    if (sendbuf == NULL)
        return RIL_NOMEM;
    
    mqtt_lock(mi);
    MQTT_DBG("Publish > topic \"%s\", qos:%d\r\n", topic, (int)qos);
    id = getNextPacketId(mi);                       //获取新包号
    len = MQTTSerialize_publish(sendbuf, send_bufsize, 0,  qos, 0,id, 
                                topic_name, (unsigned char *)payload, payload_size);      
    if (len <= 0)
        goto exit; 
    //更新发送状态
    mi->send_state = MQTT_REQ_BUSY;
    //
    ret = send_parket(mi, sendbuf, len);
    if (ret != RIL_OK || qos == QOS0)
        goto exit; 
  
    //QOS1, QOS2    
    /* 等待发送确认*/
    if (!ril_sem_wait(mi->sem_ready, MQTT_SEND_TIMEOUT * 1000))
        ret = RIL_TIMEOUT;
    else {
        unsigned short pid;
        unsigned char dup, type;
        if (MQTTDeserialize_ack(&type, &dup, &pid, mi->buf, mi->recvcnt) != 1)
            goto exit;        
        ret = RIL_OK;
    }
exit:
    MQTT_DBG("Topic \"%s\" publish %s\r\n", topic,
             ret== RIL_OK ? "successfully" : "failed");
    mqtt_error_check(mi, ret);
    mi->send_state = MQTT_REQ_IDLE;
    ril_free(sendbuf);
    mqtt_unlock(mi);
    return ret;    
}

/**
 * @brief	   订阅主题
 * @params[in] mc    - mqtt_client
 * @params[in] topic - 主题
 * @params[in] qos   - 服务质量
 * @params[out] grantedQoS   -  服务器保证质量
 * @retval     RIL_OK  -  订阅成功, 其它值 - 异常
 */
int mqtt_client_subscribe(mqtt_client_t *mc, const char *topic, mqtt_qos qos, 
                   mqtt_qos *grantedQoS)
{
    int len, ret = RIL_REJECT;
    unsigned char sendbuf[128];
    MQTTString topic_name = MQTTString_initializer;
    
    mqtt_info_t *mi = container_of(mc, mqtt_info_t, client);
    
    topic_name.cstring = (char *)topic;
    
    if (!is_connected(mi))
        return RIL_REJECT;
            
    mqtt_lock(mi);
    MQTT_DBG("Subscribe > topic:\"%s\", qos:%d\r\n", topic, (int)qos);
    len = MQTTSerialize_subscribe(sendbuf, sizeof(sendbuf), 0,
                                getNextPacketId(mi), 
                                1, &topic_name, (int *)&qos);
    if (len <= 0)
        goto exit; 
    //更新发送状态
    mi->send_state = MQTT_REQ_BUSY;
    //
    ret = send_parket(mi, sendbuf, len);
    if (ret != RIL_OK)
        goto exit; 

    if (!ril_sem_wait(mi->sem_ready, MQTT_SEND_TIMEOUT * 1000))
        ret = RIL_TIMEOUT;
    else {//SUBACK
        int count = 0;
        unsigned short id;
        *grantedQoS = QOS0;     
        if (MQTTDeserialize_suback(&id, 1, &count, (int *)grantedQoS, mi->buf, mi->recvcnt) != 1)
            goto exit;
        if (*grantedQoS != SUBFAIL) {
            ret = RIL_OK;
        }        
    }
exit:
    MQTT_DBG("Topic \"%s\" subscribe %s\r\n", topic, 
             ret== RIL_OK ? "successfully" : "failed");
    mqtt_error_check(mi, ret);
    mi->send_state = MQTT_REQ_IDLE;
    mqtt_unlock(mi);
    return ret;      
}
/**
 * @brief	   解除主题订阅
 * @params[in] mc     - mqtt_client
 * @params[in] topic  - 主题
 * @retval     RIL_OK -  操作成功, 其它值 - 异常
 */
int mqtt_client_unsubscribe(mqtt_client_t *mc, const char *topic)
{
    int len, ret = RIL_REJECT;
    unsigned char sendbuf[128];
    MQTTString topic_name = MQTTString_initializer;
    
    mqtt_info_t *mi = container_of(mc, mqtt_info_t, client);
    
    topic_name.cstring = (char *)topic;
    
    if (!is_connected(mi))
        return RIL_REJECT;
            
    mqtt_lock(mi);
    MQTT_DBG("Unsubscribe > topic:\"%s\"\r\n", topic);
    len = MQTTSerialize_unsubscribe(sendbuf, sizeof(sendbuf), 0,
                                  getNextPacketId(mi), 1, &topic_name);
    if (len <= 0)
        goto exit; 
    //更新发送状态
    mi->send_state = MQTT_REQ_BUSY;
    //
    ret = send_parket(mi, sendbuf, len);
    if (ret != RIL_OK)
        goto exit; 
  
    if (!ril_sem_wait(mi->sem_ready, MQTT_SEND_TIMEOUT * 1000))
        ret = RIL_TIMEOUT;
    else {//UNSUBACK
        unsigned short id;
        if (MQTTDeserialize_unsuback(&id, mi->buf, mi->recvcnt) != 1)
            goto exit;
        ret = RIL_OK;
    }
exit:
    MQTT_DBG("Topic \"%s\" unsubscribe %s\r\n", topic,
             ret== RIL_OK ? "successfully" : "failed");
    mqtt_error_check(mi, ret);
    mi->send_state = MQTT_REQ_IDLE;
    mqtt_unlock(mi);
    return ret;      
}

/**
 * @brief	   MQTT任务处理程序, 管理心跳发送及重连
 * @params[in] mc - mqtt_client
 * @note       该函数不允许与mqtt_client_recv放到同一个任务或者/线程中进行轮询
 * @return     none
 */
void mqtt_client_process(mqtt_client_t *mc)
{    
    mqtt_event_args_t args;
    mqtt_info_t *mi;
    if (mc == NULL)
        return;
    
    mi = container_of(mc, mqtt_info_t, client);

    keepalive(mi);

    reconnect_process(mi);
    
    if (is_connected(mi) && !ril_sock_online(mi->sockfd)) {
        mi->connected = false;
        MQTT_DBG("offline\r\n");
        args.payload_size = 0;
        args.type      = MQTT_EVENT_OFFLINE;
        event_invoke(mi, &args);
    }
}

/**
 * @brief	   MQTT数据接收处理程序
 * @params[in] mc - mqtt_client
 * @note       该函数应单独建立一个任务进行轮询调用
 * @return     none
 */
void mqtt_client_recv(mqtt_client_t *mc)
{
    mqtt_info_t *mi;
    if (mc == NULL)
        return;
    
    mi = container_of(mc, mqtt_info_t, client);
    mqtt_data_parse(mi);  
    
    /**
     * @breif 网络中断且有数据请求
     */
    if (is_connected(mi) && !ril_sock_online(mi->sockfd)) {
        if (mi->conn_state == MQTT_REQ_BUSY || mi->send_state == MQTT_REQ_BUSY) {
            ril_sem_post(mi->sem_ready);
        }          
    }
}
