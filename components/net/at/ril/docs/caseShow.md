# 案例演示

本小节将演示几个基于RIL开发的案例，包括:

- TCP通信
- UDP通信
- HTTP文件下载
- 短信收发
- MQTT 通信

## TCP 通信

该示例主要演示了TCP通信基本流程，包含服务器连接，数据收发操作，完整的代码可以参考/demo/user/samples/sample_tcp.c。

建立一个TCP通信程序需要完成以下几个步骤：

1. 通过ril_socket_t定义socket
2. 调用ril_sock_create函数创建socket，并指定事件处理程序及接收缓冲区大小
3. 通过ril_isonline函数确定模组是否注册上线
4. 通过ril_sock_connect函数启动服务器连接（指定主机、端口、连接类型设置为RIL_SOCK_TCP）
5. 完成以上步骤就可以进行数据收发了

```c
//...
/**
 * @brief       tcp 任务
 * @return      none
 */ 
static void tcp_task(void)
{
    int  retry = 0;
    bool result;
    ril_socket_t sockfd;                          //socket
    sockfd = ril_sock_create(socket_event, 256);  //创建socket，并设置256 Bytes的接收缓冲区
    printf("TCP socket create %s.\r\n", sockfd != RIL_INVALID_SOCKET ? "OK" : "ERR");
    if (sockfd == 0) {
        return;
    }
    while (1) {
        os_sleep(10);
        if (!ril_isonline())                       //等待网络连接
            continue;
        if (!ril_sock_online(sockfd)) {             
            result = ril_sock_connect(sockfd, TCP_SERVER, TCP_PORT, RIL_SOCK_TCP);
            if (result != RIL_OK) {
                retry %= 10;
                os_sleep(10000 * retry++);        //连续失败等待一段时间再尝试
            } 
            printf("TCP Socket connect %s.\r\n", result == RIL_OK ? "OK" : "ERR");
        } else {
           recv_data_process(sockfd);             //数据接收处理
           send_data_regularly(sockfd);           //数据发送处理
        }
    }
}
task_define("tcp-sample", tcp_task, 256, 6);     //定义TCP任务
```



## UDP 通信

该示例主要演示了UDP通信基本流程，包含服务器连接，数据收发操作，完整的代码可以参考/demo/user/samples/sample_udp.c。

建立一个UDP通信程序步骤与[TCP通信]()基本一样，只是连接类型需要设置为RIL_SOCK_UDP。

```c
//......
/** 
 * @brief       udp 任务
 * @return      none
 */ 
static void udp_task(void)
{
    int  retry = 0;
    bool result;
    ril_socket_t sockfd;                          //socket
    sockfd = ril_sock_create(socket_event, 256);  //创建socket，并设置256 Bytes的接收缓冲区
    printf("UDP socket create %s.\r\n", sockfd != RIL_INVALID_SOCKET ? "OK" : "ERR");
    if (sockfd == 0) {
        return;
    }
    while (1) {
        os_sleep(10);
        if (!ril_isonline())                       //等待网络连接
            continue;
        if (!ril_sock_online(sockfd)) {             
            result = ril_sock_connect(sockfd, UDP_SERVER, UDP_PORT, RIL_SOCK_UDP);
            if (result != RIL_OK) {
                retry %= 10;
                os_sleep(10000 * retry++);        //连续失败等待一段时间再尝试
            } 
            printf("UDP Socket connect %s.\r\n", result == RIL_OK ? "OK" : "ERR");
        } else {
           recv_data_process(sockfd);             //数据接收处理
           send_data_regularly(sockfd);           //数据发送处理
        }
    }
}
task_define("udp-sample", udp_task, 256, 6);     //定义UDP任务
```

## 短信收发

!> 注:在使用短信功能前需要先确保您的SIM卡已经开通短信业务。

下面示例主要演示了短信发送/接收功能，完整的例子可以参考/demo/user/samples/sample_sms.c。

### 发送短信

对于短信发送功能，使用<ril_sms_send>函数实现，其的定义如下：

```c
/**
 * @brief     发送短信
 * @param[in] phone  - 目标手机号
 * @param[in] msg    - 短信消息( < 164 bytes)
 * @return    RIL_OK - 发送成功, 其它值 - 异常
 */
int ril_sms_send(const char *phone, const char *msg);
```

**使用范例：**

下面是一个使用命令行发送短信的例子，用户只需要在串口终端中输入相应的命令即可完成短信发送功能。

命令格式如下：

sms+目标手机号+短信内容

```c
//...
/*
 * @brief   sms短信发送命令
 *          命令格式:sms,phone,message text
 * @example sms,18512344321,sms test
 */    
static int do_cmd_sms(struct cli_obj *cli, int argc, char *argv[])
{
    bool res;
    if (!ril_isreg()) {
        cli->print(cli, "unreg to network.");      //未注册到网络,无法发送短信
        return -1;
    }    
    if (argc != 3) {
        cli->print(cli, "Command format error!!!\r\n"
                 "format:sms,phone,message text.\r\n"
                 "Example:sms,18512344321,sms test.\r\n");
        return -1;
    }
    res = ril_sms_send(argv[1], argv[2]);
    cli->print(cli, "sms send %s\r\n", res ? "OK" : "ERROR");
    return 0;
} 
cmd_register("sms", do_cmd_sms, "send sms");     //注册短信发送命令

```

### 接收短信

对于短信接收功能，则是通过订阅RIL通知来实现。

```c
/*
 * @brief   短信接收处理
 */
static void sms_recv_handler(sms_info_t *sms, int data_size)
{
    printf("Receive sms=> \r\nphone:%s\r\nText:%s\r\n",sms->phone, sms->msg);
}
ril_on_notify(RIL_NOTIF_SMS, sms_recv_handler);         //注册短信接收事件
```



## HTTP文件下载

HTTP是RIL基于TCP协议内置的组件，应用于远程文件下载及FOTA功能。

### 基本特性

- 支持下载超时控制，在规定时间段时如果网络断开会自动重连。
- 支持断点续传功能，即使由于网络原因中断，下一次还可以接着下载。

**HTTP 事件**

启动文件下载请求之后，HTTP组件将以事件回调的形式将下载相关状态、数据递交到上层应用程序。事件的原型定义如下：

```c
typedef void (*http_event_t)(http_event_args_t *args); /* http 事件回调*/
```

**HTTP事件参数**

事件参数(http_event_args_t)包含当前下载状态、文件总大小、已接收数据长度等信息。

```c
/*HTTP 事件参数 ---------------------------------------------------------------*/
typedef struct {
    struct http_client *client;                             
    unsigned char state;                                /* 当前状态   */
    unsigned int  filesize;                             /* 文件大小   */
    unsigned int  spand_time;                           /* 已使用时间 */    
    unsigned int  offset;                               /* 写指针偏移 */
    unsigned char *data;                                /* 数据指针   */
    unsigned int  datalen;                              /* 数据长度   */
}http_event_args_t;
```

**HTTP状态**

HTTP 状态反映了当前的下载阶段，其定义如下：

```c
/* http 状态 -----------------------------------------------------------------*/
#define HTTP_STAT_START          0                      /* 开始下载 */
#define HTTP_STAT_DATA           1                      /* 接收数据*/
#define HTTP_STAT_DONE           2                      /* 下载完成*/
#define HTTP_STAT_FAILED         3                      /* 下载失败*/
```

1. HTTP_STAT_START阶段用于通知应用程序文件准备开始下载，应用程序以在此阶段创建文件或者将擦除FLASH文件存储区，为下面写数据做准备。
2. HTTP_STAT_DATA是真正的数据下载阶段，通过将数据源源不断递交上来，直接文件下载完成。
3. 最后是HTTP_STAT_DONE和HTTP_STAT_FAILED，表明文件最终下载是否成功。

### 使用步骤

完成一个HTTP文件功能功能需要执行以下几个步骤：
1. 通过http_client_create创建http_client，并指定事件回调接口、服务器地址、服务器端口号。
2. 通过http_start_download启动下载，并指定文件名称，超时时间。
4. 下载结束后，通过http_client_destroy销毁http_client。

### 示例代码

!> 在使用这个功能前，需要先搭建一个HTTP文件服务器，笔者使用的是[Http File Server]，大家自行到网上下载。

下面示例演示了通过命令行控制文件下载的功能，用户可以在终端串口上按命令格式[http,服务器地址,端口号,文件名, 下载超时时间(s)]执行下载，如果执行成功会在串口终端上打印下载的进度信息，完整的代码可以参考/demo/user/samples/sample_http.c。

```c
//.....
/**
 * @brief       http 事件处理
 */
static void http_event(http_event_args_t *e)
{
    unsigned int recvsize = e->offset + e->datalen;
    if (e->state == HTTP_STAT_DATA) {
        printf("%d/%d bytes %.1f%% completed.\r\n", recvsize , e->filesize, 
                 100.0 * recvsize/ e->filesize);
        //write(file, e->data, e->datalen);   保存文件数据
    }    
    if (e->state == HTTP_STAT_DONE)
        printf("\r\nDownload complete, spand time:%d\r\n", e->spand_time);
}

/**
 * @brief   http文件下载命令
 *          命令格式:http,host,port,filename, timeout(s)
 * @example http,123.146.152.12,1234,/ril-demo.hex,120
 */    
static int do_cmd_http(struct cli_obj *cli, int argc, char *argv[])
{
    http_client_t *http;
    const char *host, *file;
    int port, timeout;
    if (argc < 5) {
        cli->print(cli, "Command format error!!!\r\n"
                 "Format:http,host,port,filename, timeout(s)\r\n"
                 "Example:http,123.146.152.12,1234,/ril-demo.hex,120\r\n");
        return -1;
    }
    host = argv[1];                                       //服务器地址
    port = atoi(argv[2]);                                 //端口号
    file = argv[3];                                       //文件名称
    timeout = atoi(argv[4]);                              //超时时间 
    cli->print(cli, "Download file [%s] from [%s].\r\n", file, host);
    http = http_client_create(http_event, host, port);   //创建HTTP客户端
    if (http == NULL) {
        cli->print(cli, "Input error, http client create failed.\r\n");
        return -1;
    }
    http_start_download(http, file, timeout);             //启动HTTP下载
    http_client_destroy(http);                            //销毁客户端
    return 0;
                      
}cmd_register("http", do_cmd_http, "http file download"); //注册http命令
```

## MQTT 通信

与HTTP一样，MQTT 也是在TCP协议层基础上开发的组件，它也是在Paho MQTT 源码包的基础上封装出来的一套 MQTT 客户端程序。跟mqtt通信相关的文件在ril/case目录下，包含MQTTPacket及mqtt_client两部分。

```shell
ril
|
|───case 
    |---- mqtt_client.c
    |---- MQTTPacket
```

### 基本特性

- 自动重连管理，当由于外部因素异常网络断开时，会自动重连。
- 心跳管理，根据应用程序指定的heartbeat_interval间隔定时上报心跳包。
- 支持Qos0、Qos1、Qos2服务质量。

**MQTT 配置 **

在MQTT功能之前，你需要对它进行配置，mqtt_config_t 定义了MQT组件运行相关的参数，包含MQTT事件处理程序、客户端id、连接服务器的地址等信息。虽然参数很多，但并不是所有都要填写，一般使用时只需要event_handler，client_id，host，recvbuf_size，port这几个即可，其它都是选填的。

```c
/**
 * @brief    mqtt client 配置
 */
typedef struct {
    /* 事件处理程序 */
    void (*event_handler)(mqtt_client_t *, mqtt_event_args_t *args);
    const char    *client_id;                     /* 客户端id */
    const char    *host;                          /* 远程服务器主机名 */
    const char    *username;                      /* 用户名称*/
    const char    *userpwd;                       /* 用户密码*/
    unsigned short recvbuf_size;                  /* 接收缓冲区大小(取决于playload)*/
    unsigned short port;                          /* 服务器端口号 */
    unsigned short heartbeat_interval;            /* 心跳间隔 (unit:s)*/
    unsigned short reconnect_interval;            /* 重连间隔 (unit:s) */
    unsigned  char clean_session;                 /* 离线包处理方式*/
    /** 
     * @brief 遗属信息
     */
    struct {                                      
        unsigned char will_flag;                  
        unsigned char retain;
        mqtt_qos      qos;                         
        const char   *topic;
        const char   *msg;
    } will_options;
} mqtt_config_t;
```

**MQTT事件**

MQTT组件支持异步事件上报，当发生事件时(收到订阅消息、重连等)，它通过通过event_handler接口中的事件参数（mqtt_event_args_t）呈递给应用程序，它包含了事件类型、主题及主题内容等信息。

```c


/**
 * @brief    mqtt事件参数
 */
typedef struct {
    mqtt_event_type type;                         /* 事件类型*/
    /*下面是服务器推送的MQTT_EVENT_DATA信息 */                          
    mqtt_qos       qos;
    unsigned char  retain;
    unsigned char  dup;    
    const char    *topic;                         /* 主题*/
    int            topic_size;                    /* 主题长度*/
    unsigned char *payload;                       /* 载荷 */
    int            payload_size;                  /* 载荷长度 */
}mqtt_event_args_t;
```

事件类型定义如下：

```c
/**
 * @brief MQTT 事件类型
 */
typedef enum {
    MQTT_EVENT_ERROR = 0,          //未知错误
    MQTT_EVENT_OFFLINE,            //已断开连接
    MQTT_EVENT_RECONNECT,          //重连成功
    MQTT_EVENT_ONLINE,             //连接成功
    MQTT_EVENT_DATA,               //来自服务器的数据包
}mqtt_event_type; 
```

### 使用步骤

1. 使用mqtt_config_t定义MQTT配置参数.
2. 使用mqtt_client_create 创建MQTT客户端实例，并指定配置参数，如果成功则返回非NULL值
3. 启动一个任务, 并间歇调用 mqtt_client_process 函数
4. 启动一个任务, 并持续调用 mqtt_client_recv 函数
5. 调用mqtt_client_connect连接服务器(如何使能了自动重连则可以省略此步骤)
6. 完成以上步骤就可以跟服务器进行数据交互了(订阅主题/发布消息)

### 示例代码

下面示例演示了一个基本的MQTT 通信程序，完整的代码可以参考/demo/user/samples/sample_mqtt.c。

**定义MQTT配置参数**

```c
//mqtt 配置参数
static const mqtt_config_t mqtt_config = {
    .client_id          = "ril-mqtt-demo",       //客户端樯
    .host               = "123.456.789.1",       //服务器主机
    .port               = 1883,                  //端口号
    .recvbuf_size       = 256,                   //接收缓冲区大小,依据你订阅的主题消息长度来定
    .event_handler      = mqtt_event_handler,    //事件处理程序
    //可选参数
    .reconnect_interval = 30,                    //断开后30s自动重连
    .heartbeat_interval = 300,                   //5min 1个心跳包
    .clean_session      = true
};
```

**MQTT事件处理程序**

```c
/**
 * @brief       MQTT事件处理程序
 * @return      none
 */ 
static void mqtt_event_handler(mqtt_client_t *mc, mqtt_event_handler_args_t *args)
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
```

**创建MQTT客户端**

这里直接在mqtt_task任务开始时创建MQTT客户端, 并在主循环中间歇调用 mqtt_client_process 函数

```c
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
```

**创建接收处理任务**

MQTT组件需要运行在两个任务上，除了主任务外，还需要一个任务处理接收到的数据。

```c
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
```