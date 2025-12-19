## 前言

无线蜂窝通信模组是在电路板上集成基带芯片、存储器、射频功放，并提供标准的接口功能模块，并能使各种终端都可以借助无线模块实现通信功能。随着物联网的快速发展，基于无线蜂窝网络的物联网得到的广泛应用，并出现了针对物联网的NB-IoT和LTE-CatM1等新型蜂窝技术，这些技术将提供低成本，低带宽，低功耗等特性，适应了广阔的应用场景需求，进一步拓展了物联网的应用空间，为传统产业数字化转型升级，重塑、进一步推动其它行业与物联网融合提供了技术基础。

当下无线模组正处于更新换代阶段，虽然目前2G通信模组在市场上仍占据一席之地，4G通信模组已处于快速增长阶段，NB-Iot通信模组已经同步全球进入商用阶段。无线模组作为实现物联网设备“万物互联”的最核心组件之一，在物联网应用过程中，几乎每增加一个物联网终端，将增加1-2个无线模组。不同的应用场景下对模组使用的频段、制式要求也各有不同，即使同款产品由于使用的区域、认证要求，也需要使用不同的模组。另外是由于技术的限制、成本的考量以及差异化竞争的需要，在产品开发时也需要根据特定应用场景使用不同型号的模组，这就给无线模组应用软件开发者带来了新的挑战。

由于模组与控制器间的使用AT命令进行通信，不同模组厂商开发AT指令集并不完全一样，虽然大多数都支持3gpp 标准指令集(AT Command set for User Equipment(UE))，在应用流程上也基本类似；但是在频段/制式配置、网络连接、数据传输部分的指令集可以说是千差万别。故对于模组管理的这部分软件如果没有进行合理的规划、兼容性设计，在后续更换模组硬件时势必会增加软件开发及维护成本。

目前现有的比较著名的模组软件管理框架有Android RIL、Wince RIL，它们支持的功能可以说复杂而齐全，设计之初就是为了满足大形系统而构建的，所以无法通过简单的修改移植到资源受限的终端设备(MCU系统)上。为此，一些RTOS开发商（如Lite OS、Alios things、RT-thread、TencentOS tiny等）为了能够让这类终端也具备无线上网功能，他们也开始基于自己RTOS开发出了一些无线通信模组驱动程序，提供了相应的AT命令通信框架及SAL 层(Socket abstraction layer)接口。有了标准的SAL层接口，对应用层使用都来说并没有太大差异。但由于底层实现方式及系统API的差异，各个RTOS中的模组驱动程序并不能直接复用，而且它们只是作为网络通信一种补充，省略了不少模组相关的特性，如果注网管理、短信、通话等功能。为此，我开发了这款无线模组管理软件，同样也使用了RIL(Radio Interface Layer)的叫法，除了基本的网络通信之后，还支持网络注册管理、异常检测及恢复、短信收发等功能。

RIL对模组设备进行了抽象，将模组的管理逻辑与硬件驱动进行分离，并设计了相应的标准接口，即使使用了不同厂商、型号的模组都能以标准的接口进行操作。另外，组件化的设计思想可以让模组相关程序更容易维护和扩展，从而降低了开发成本，使得系统集成与产品推出的速度极大提升。

## 准备工作

下面将概述您的目标平台设置和配置RIL所需的基本步骤，从实例的角度，一步步教你如何控制模组注册并连接上网络，同时开发出一个简单TCP/UDP应用程序，让你能快速的入门RIL的使用。

 - 环境搭建
 - 初始RIL并建立网络通信环境
 - 事件通知订阅
 - RIL请求
 - Socket通信

### 环境准备

在开始之前，您需要先搭建好实验环境，为了使RIL能够正常运行，目标系统必须满足以下要求:

- ROM 资源：至少 13.5K Bytes(取决于挂载的模组个数及使用的组件)
- RAM 资源：至少 1.2K Bytes(取决于socket创建数量)
- RTOS及堆栈: RIL需要运行在两个任务中，每个任务的堆栈至少 256 Bytes
- 编译器：由于RIL使用了一些C99的特性(柔性数组、内联)，所以编译器需要开启对C99的支持。对于IAR，它默认是打开的，而Keil MDK需要手动增加编译选项(--c99 --gnu) 。

下面demo工程使用的平台及软硬件环境：

- MCU     ：STM32F401RD

- IDE     ：IAR7.4、Keil MDK 4.72a

- Module  :  EC21

- RTOS    :  Free-RTOS V202012.00


### 工程配置

下面视图展示了所有RIL子目录的内容，其中ril子文件夹包含ril服务相关功能，模组驱动，使用案例。demo用于ril演示工程。

```c
ril
│
|───docs          使用文档
|───demo          演示工程
│   │───driver    驱动文件
│   │───lib       MCU外设库文件
│   │───misc      常用功能库
|   |───os        操作系统相关
|   |───project   工程文件
|   └───user      
│       └─── samples  演示案例(TCP/UDP/HTTP/SMS)
│       └─── tasks    系统任务
└───ril
    │───build    工程文件，用于输出RIL库文件
    │───case     内置组件(HTTP/TFTP/MQTT)
    │───modules  模组驱动文件(ec21,bg96,hl8518 ...)
    │───common   通用库文件
    |───core     ril内核服务相关
    └───port     RTOS相关移植接口
```

#### 项目用到.h头文件

在使用时您应确保工程头文件路径含有以下目录：

- ril/core/inc
- ril/common/inc
- ril/port
- ril/case/(可选目录，如果你不需要ril内置的组件则不需要包含此项)

#### 项目用到.c源文件

一般而言，需要包含RIL的内核 c 文件、通用库文件、要使用的模组驱动文件以及连同可选的内置组件。

- 文件夹 ril/core/src中的所有 c 文件
- 文件夹 ril/common/src中的所有 c 文件
- 用到的模组驱动，根据硬件需要在文件夹ril/modules中选择对应的文件
- RTOS系统相关，根据OS平台选择文件夹ril/port中选择对应的文件

#### 生成库文件
为了加快你所在的项目编译速度，可以预先将RIL源码包编译成库形式，ril/build目录提供了IAR,MDK库及makefile编译工程，进入工程之前需要先选择对应的目标平台（如Cotext-M4），然后点击build或者make按钮，这样就在ril/output目录下生成了对应的库文件。

#### 补充说明
- 如果你在ril/port文件夹中没有找到与你当前所使用的RTOS对应的例程，可以参考[RTOS移植指南](http://moluo-tech.gitee.io/ril/#/OSPorting.md)。
- 如果你在ril/modules文件夹中没有找到与你硬件板上对应的模组驱动，可以参考[模组适配指南](http://moluo-tech.gitee.io/ril/#/modulePorting.md)。
- 在使用无线模组上网前确保你的硬件板上已经插入好SIM卡并开通流量服务。
- 如果你使用的是GCC来构建项目，需要在链接脚本添加以下两个保留段，防止编译器优化掉cli模块的命令表和ril的设备注册表。
```shell
  .ril :
  {
    KEEP (*(SORT(cli.cmd.*)))  
    KEEP (*(SORT(_section.ril.dev.*))) 
  }  
```


## 启动RIL

一个无线模组连接服务器前需要先经过开机、检测SIM卡、频段及制式配置(如果有需要)、网络注册、建立PDP并激活等一系列复杂步骤， 大致流程图如下所示：
![开机流程图](images\StartupFlowChart.png)

上面的流程读者只需要做一个大概的了解，这些过程已经由RIL内部的状态机自动完成，用户只需要按下面的步骤进行操作即可。

- 创建RIL任务，包括主任务及AT命令解析任务

- 使用接口适配器及配置参数初始化RIL

- 选择硬件对应的模组型号

- 打开模组设备

- 启用网络连接

### 创建RIL任务

第一步我们需要先给RIL创建任务，RIL内部使用两个任务进行管理，一个主任务和一个AT命令处理任务。主任务用于RIL内核服务管理，包含RIL状态机、电源管理、Socket管理、异步作业等。AT命令处理任务用于AT命令通信和URC解析上报。任务原型定义如下:

```c
void ril_main_task(void);          //主任务入口
void ril_atcmd_task(void);         //AT命令解析任务入口
```
使用范例:

在demo工程中，直接使用"task_define"宏定义任务，在OS启动之后，应用程序会自动完成任务的创建。这里需要注意一点，为了能够实时处理从模组收到的数据，AT解析任务的优先级应该高于主任务，同时提供的任务堆栈不小于256 Bytes。

```c
task_define("ril main", ril_main_task, 256 ,4);          //定义主任务
task_define("ril at",   ril_atcmd_task, 256, 3);         //AT接收处理任务
```

### 初始化RIL

在启动或者使用RIL相关服务之前，用户必须要先初始化RIL，初始化时需要向RIL提供适配器(ril_adapter_t)及系统运行所需的配置参数(ril_config_t)。

完整的ril初始化接口定义如下：

```c
/**
 * @brief       ril初始化
 * @param[in]   adt - 适配器
 * @param[in]   cfg - 配置参数
 */
void ril_init(ril_adapter_t *adt, ril_config_t *cfg);
```

#### 适配器(ril_adapter_t)

适配器定义了模组硬件相关的引脚控制接口，串口通信接口以及RIL事件通知接口。

``` c
/*ril接口适配器 --------------------------------------------------------------*/
typedef struct {
    /**
     * @brief       引脚控制
     * @param[in]   pin    - 引脚类型
     * @param[in]   isread - 指示是否进行读操作
     * @param[in]   level  - 0/1写入的电平值(非读操作有效)
     * @return      当前引脚电平
     */
    int (*pin_ctrl)(ril_pin_type pin, int isread, int level);
    /**
     * @brief       数据写操作
     * @param[in]   buf    - 数据缓冲区
     * @param[in]   len    - 数据长度
     * @return      实际成功写入的数据长度
     */
    unsigned int (*write)(const void *buf, unsigned int len);
    /**
     * @brief       数据读操作
     * @param[out]  buf    - 数据缓冲区
     * @param[in]   len    - 期待读取的数据长度
     * @return      实际读取的数据长度    
     */    
    unsigned int (*read) (void *buf, unsigned int len);
}ril_adapter_t;   

```

 - 引脚控制(pin_ctrl)

引脚控制接口主要用于RIL模组开关机时序控制，休眠唤醒控制等，考虑到不同模组的开关机时序并不一样，所以这个接口并不是RIL内核直接控制的，而是传递到模组驱动的ril_deivice中,关于这部分的详细使用可以参考[模组适配指南](http://moluo-tech.gitee.io/ril/#/modulePorting.md)。

 - 数据读写(write/read)

这部分主要是RIL与模组通信接口层，用于AT指令交互，通信硬件一般使用串口，默认波特率使用115200。

#### 配置参数(ril_config_t)

RIL配置参数包含无线模组运行时所需信息，当前版本仅支持APN参数，它在创建PDP上网时会用到，需要根据运营商来填写。事实上，根据实测结果，如果使用的是国内的运营商GSM网络的话，APN任意填写都是没有影响的。

```c
/*ril配置参数 ----------------------------------------------------------------*/
typedef struct {
    struct {
        const char    *apn;                               /* 接入点名称*/
        const char    *user;                              /* 用户名*/
        const char    *passwd;                            /* 密码 */
    } apn;
}ril_config_t; 
```

### 选择模组

!> 在您选择模组型号前，必须确保ril/modules有对应的模组驱动添加到了工程中，如果目录中没有找到对应的文件，你可以参考RIL自带的驱动程序或者[模组驱动指南](/modulePorting.md)自己实现一个。

下一步需要选择与你硬件板上型号相匹配的模组，所有的模组都是通过ril_device_install(name, operations)宏挂载安装到RIL中进行统一管理的。理论上只要你使用的平台资源足够，并没有个数限制。选择模组可以通过执行下面的函数完成，模组名称必须跟模组安装到RIL名称一致。

```c
/**
 * @brief       选择使用的设备    
 * @param[in]   name - 设备名称
 * @return      RIL_OK - 选择成功, RIL_ERROR - 设备不存在
 */
int ril_use_device(const char *name);
```

使用范例:

```c
if (ril_use_device("EC21") == RIL_OK) {
    printf("Device selected successfully\r\n");
} else {
    printf("Device not found\r\n");
}
```

### 启用网络连接

RIL打开之后它并不会主动去连接网络，我们还需要手动启用网络连接，接着RIL会自动完成PDP的附着与激活流程。

```c
/**
 * @brief       网络连接控制
 * @param[in]   enable - 启用/禁用网络连接
 */
void ril_netconn(bool enable);
```

**完整的例子演示：**

下面是一个完整的RIL初始化流程代码 ，详细可以参考源码:\ril\demo\user\tasks\ril_task.c

```c
....
/*
 * @brief   ril任务初始化
 */
static void ril_work_init(void)
{
    bool result;
    ril_adapter_t adt = {                 //适配器
        .write    = module_uart_write,
        .read     = module_uart_read,
        .pin_ctrl = io_ctrl,             
    };

    ril_config_t cfg = {                  //配置参数
        .apn = {
            .apn    = "cmnet",
            .user   = "",
            .passwd = "",
        },
    };
    port_init();
    module_uart_init(115200);           //模组通信串口初始化
    ril_init(&adt, &cfg);               //初始化RIL
    result = ril_use_device("EC21");    //选择模组型号
    printf("Ril select device %s\r\n", result == RIL_OK ? "OK": "ERROR");
    ril_open();                         //打开设备
    ril_netconn(true);                  //启动网络连接
} system_init("ril", ril_work_init);

task_define("ril main", ril_main_task, 256 ,4);          //定义主任务
task_define("ril at",   ril_atcmd_task, 256, 3);         //AT接收处理任务
```

到目前为止，我们已经将准备工作就绪，如果模组工作正常，您将在串口终端下看到如下打印信息：

```c
...
[RIL]:Startup
[RIL]:Startup OK
[RIL]:Check SIM Card...
[RIL]:SIM Ready
[RIL]:Device initialize...
[RIL]:Wait for network registration...
[RIL]:Update registration status:1
[RIL]:Register successfully, rssi:27
[RIL]:PDP setup...
[RIL]:Online
```

打印"Online"，表明已经建立好网络通信环境，接下来可以进行TCP/UDP通信了，关于这部分接口的使用，可以参考[Socket通信](#Socket通信)或者[使用案例](/caseShow.md)。

## 订阅通知(notify)

RIL内核运行时会实时检测无线模组的各种状态（例如网络状态更新、来电、收到短信等），一旦状态发生变化，它将以回调的形式通知上层应用，并传递相关的数据。针对不同的通知类型，RIL传递的数据内容并不一样，用户应根据通知类型提取相应的数据内容，当前RIL支持的通知类型定义如下：

| 通知类型(ril_notify_type) | 数据内容(data)               | 功能描述                                 |
| ------------------------- | ----------------------------| --------------------------------------- |
| RIL_NOTIF_SIM             | 类型为 ril_sim_status *      | sim卡状态更新，如果检测到卡插入或者移除   |
| RIL_NOTIF_NETREG          | 类型为  ril_netreg_status *  | 网络注册状态更新                        |
| RIL_NOTIF_NETCONN         | 类型为  ril_netconn_status * | 网络连接状态更新                        |
| RIL_NOTIF_SMS             | 类型为 sms_info_t *          | 收到短信(包含源号码及内容)              |
| RIL_NOTIF_RING            | data 指向 char *phone        | 来电                                    |
| RIL_NOTIF_TIMEOUT         | N/A                          | 命令执行超时                            |
| RIL_NOTIF_ERROR           | N/A                          | 未知错误                                |
| RIL_NOTIF_CUSTOM | 用户自定义 | 用户自定义通知，一般用于特殊的URC码 |

应用程序想要接收来自RIL中的通知时，先要进行订阅，可以通过ril_on_notify宏来实现，其定义如下：

```c
/**
 * @brief    订阅 ril 通知
 * @param    type     - 通知类型(参考ril_notify_type定义)    
 * @param    handler  - 通知处理函数,类型: void (*)(void *data, int data_size))
 *                      其中data与data_size域的作用参考ril_notify_type对应说明
 */     
#define ril_on_notify(type, handler)\
__ril_on_notify(type, handler)
```

这里包含两个参数，一个是通知类型，一个是通知处理函数。对于通知处理函数的类型，为了方便使用，宏内部已经做了强制类型转换，所以通知处理函数的类型可以根据实际通知数据类型(ril_notify_type有详细的描述)来实现，下面我们以订阅短信通知为例：

```c
/*
 * @brief   短信接收处理
 */
static void sms_recv_handler(sms_info_t *sms, int data_size)
{
    printf("Receive sms=> %s:%s\r\n",sms->phone, sms->msg);
}
ril_on_notify(RIL_NOTIF_SIM, sms_recv_handler);           //订阅短信通知
```

##  RIL请求(Request)

由于无线模组支持的AT命令种类众多，不可能为所有命令都单独封装并提供一个接口出来，因为这会给使用和扩展实现都带来不便，而且一些特殊命令类型并不是所有模组都支持。为此RIL提供<ril_request>接口处理这类命令，称之为RIL请求，它可以用来设置/获取模组特定的信息，基本的接口定义如下：

```c
/**
 * @brief        ril请求处理
 * @param[inout] data   - 参考ril_request_code描述
 * @param[in]    size   - data大小
 * @return       RIL_OK - 请求成功, 其它值 - 异常
 */
int ril_request(ril_request_code num, void *data, int size);
```

下表是当前RIL支持的请求码类型：

| 请求码类型              | 数据内容                                           | 功能描述         |
| ----------------------- | -------------------------------------------------- | ---------------- |
| RIL_REQ_GET_SIM_STATUS  | data -> ril_sim_status *                           | 获取sim卡状态    |
| RIL_REQ_GET_REG_STATUS  | data -> ril_netreg_status *                        | 获取网络注册状态 |
| RIL_REQ_GET_CONN_STATUS | data -> ril_netconn_status *                       | 获取网络连接状态 |
| RIL_REQ_GET_CSQ         | data -> ril_csq_t *                                | 获取CSQ值        |
| RIL_REQ_GET_IMEI        | data ->  char *imei                                | 获取IMEI号       |
| RIL_REQ_GET_IMSI        | data ->  char *imsi                                | 获取IMSI号       |
| RIL_REQ_GET_HWVER       | data ->  char *hwver                               | 获取硬件版本号   |
| RIL_REQ_GET_SWVER       | data ->  char *swver                               | 获取软件版本号   |
| RIL_REQ_GET_MODEL       | data -> const char *phone                          | 获取模组型号     |
| RIL_REQ_DIAL            | data -> const char *phone                          | 拨打电话         |
| RIL_REQ_HANGUP          | N/A                                                | 挂断电话         |
| RIL_REQ_ANSWER          | N/A                                                | 接听电话         |
| RIL_REQ_GET_IPADDR      | data -> char * ip                                  | 获取IP地址       |
| RIL_REQ_SET_DNS         | data[0] -> (char *)主DNS, data[1] -> (char *)备DNS | 设置DNS          |

## Socket通信

本节将介绍RIL Socket通信相关接口的使用，使用之前，读者需要先了解了TCP/IP通信协议基础知识，清楚TCP、UDP的基本概念及应用场景。

Socket通信相关数据结构及函数接口，定义在头文件</core/inc/ril_socket.h>中。

### 基本概念

Socket通常也称作"套接字，它是一个抽象的概念 ，其起源于 20 世纪 80 年代，最早应用于BSD UNIX 中，所以也称之为“BSD Socket ”。它是对网络中不同主机上的应用程序之间进行双向通信端点的抽象表示，也是TCP/IP 协议的网络通信应用的基本操作单元。在软件层面上，可以将Socket理解为针对应用程序封装出来的能够进行TCP/IP协议通信的一系列标准函数接口。

不同的系统有自己的一套 Socket API接口。 Windows 系统中使用的是 WinSock，UNIX/Linux 系统中使用的是 BSD Socket，HTML5中使用的是WebSocket。它们虽然接口风格不同， 但功能毫无二致。 RIL Socket 是根据无线模组内置TCP/IP协议栈抽象出来的API接口，与BSD Socket及WinSock类似，它支持TCP/UDP通信，并提供访问Internet所需求的基本常用操作，如服务器连接/断开，数据收发等，使用之前你需要了解它的几个基本概念。

- socket描述符
- socket类型
- socket状态
- socket事件

#### socket 描述符

socket 描述符是RIL内部管理socket 的一个唯一标识，在创建socket时生成并返回。在后续操作中包括服务器连接、数据发送、数据接收都要用到它。

```c
/**
 * @brief socket 描述符
 */
typedef int ril_socket_t;
```

#### socket 类型

在连接服务器前，可以指定连接类型，目前支持的连接类型定义如下：

```c
/*socket 类型 ----------------------------------------------------------------*/
typedef enum {
    RIL_SOCK_TCP = 0,                                    /* TCP */
    RIL_SOCK_UDP                                         /* UDP */
} socket_type;
```

#### socket 状态

应用程序在使用socket建立连接或者发送数据过程中，RIL内部会监视整个过程，并定时更新连接/发送状态。应用程序需要根据socket状态来进行相关操作。

```c
/**
 * @brief socket (发送/连接)状态
 */
typedef enum {
    SOCK_STAT_UNKNOW,                                     /* 未知 */
    SOCK_STAT_BUSY,                                       /* 进行中 */
    SOCK_STAT_DONE,                                       /* 请求完成 */
    SOCK_STAT_FAILED,                                     /* 请求失败 */
    SOCK_STAT_TIMEOUT,                                    /* 请求超时 */
    SOCK_STAT_MAX
} sock_request_status;
```
#### socket 事件

当RIL检测到socket连接断开、数据到来时就会生成socket事件，它以回调的形式通知上报给应用程序，事件类型定义如下:

```c
/**
 * @brief socket 事件类型定义
 */
typedef enum {
    /**
     * @brief       连接状态更新(可以通过ril_sock_connstat 获取连接状态)
     */    
    SOCK_EVENT_CONN = 0,
    /**
     * @brief       数据发送状态更新(可以通过ril_sock_sendstat 获取发送状态)
     */     
    SOCK_EVENT_SEND,
    /**
     * @brief       数据接收事件(可以通过ril_sock_recv 接口读取数据)
     */   
    SOCK_EVENT_RECV
} socket_event_type;

/**
 * @brief socket 事件
 */
typedef void (*socket_event_t)(ril_socket_t s, socket_event_type type);
```

### 接口说明

#### 创建socket

创建一个socket的函数是ril_sock_create，原型如下：

```c
ril_socket_t ril_sock_create(socket_event_t e, unsigned int bufsize);
```

**返回值:**

如果创建成功，则返回一个非0的有效socket标识，创建失败则返回RIL_INVALID_SOCKET。

**参数说明:**

- e : 表示socket事件回调函数，如果不需要,可以填NULL
- bufsize : 表示socket接收缓冲区大小，如果填0，系统会为它分配128 Bytes.

**使用范例:**

```c
ril_socket_t sockfd;

sockfd = ril_sock_create(NULL, 256);               //创建socket并设置接收缓冲区为256bytes

if (sockfd != RIL_INVALID_SOCKET) {
    printf("Socket created successfully.\r\n");
} else {
    printf("Socket created failed.\r\n");
}
```

#### 销毁socket

socket在创建时，RIL会为分配运行时所需的资源，如果你不再需要使用，请记得务必销毁它，否则会造成内存泄露，销毁socket的原型如下:

```c
void ril_sock_destroy(ril_socket_t s);
```
#### 连接服务器

一般来说控制无线模组连接服务器需要进行以下步骤：

1. 执行创建socket命令
2. 执行socket连接命令
3. 查询socket状态并等待连接结果

第1、2步一般都是比较快的，最后一步等待连接结果，由于不同网络环境的信号质量、强弱都会影响网络连接，所以等待的时间并不一样，从几秒到几百秒不等。针对这个特性RIL提供了两种服务器连接接口，第一种阻塞式，将上面的1-3个步骤全部执行完再退出，一直阻塞直到返回结果。

```c
//连接服务器(阻塞方式)
int ril_sock_connect(ril_socket_t s, const char *host, unsigned short port, 
                     ril_socket_type type);
```

第二种是非阻塞式，它的参数与阻塞式接口一样，但内部只执行1-2步骤，对于连接结果应用程序可以通过查询连接状态接口来得到，这样可以不至于让当前任务阻塞太久。

```c
//连接服务器(非阻塞方式)
int ril_sock_connect_async(ril_socket_t s, const char *host, 
                           unsigned short port, ril_socket_type type);
```

**返回值:**

对于阻塞式接口，如果返回RIL_OK，表示当前socket已经正确连接到服务器；而对于非阻塞式接口，它只是表示成功发送了连接请求，对于连接结果需要使用主动查询。

**参数说明:**

- s : 表示socket描述符，在创建时由RIL提供。
- host：服务器地址。
- port : 服务器端口号。

**使用范例：**

```c
...
int ret;
ril_socket_t sockfd;
sockfd = ril_sock_create(NULL, 256);               //创建socket并设置接收缓冲区为256bytes
if (sockfd != RIL_INVALID_SOCKET) {
    printf("Socket created successfully.\r\n");
} else {
    printf("Socket created failed.\r\n");
}
while (1) {
    if (sockfd != RIL_INVALID_SOCKET && !ril_sock_online(sockfd)) {
        if (!ril_sock_busy(sockfd)) {
            ril_sock_connect_async(sockfd, "www.baidu.com", 1234, RIL_SOCK_TCP);
        }
    }
    
    /* Do something else*/    
}
...
```

### 断开服务器连接

函数原型

```c
int ril_sock_disconnect(ril_socket_t s);
```

**返回值:**

如果执行成功则返回RIL_OK，其它值，异常码。

**参数说明:**

- s : 表示socket描述符，在创建时由RIL提供。

#### 数据发送

与服务器连接工作一样，考虑到发送数据时等待时间可能会比较久，这里也提供了两种数据发送接口。

```c
//发送数据(阻塞方式)
int ril_sock_send(ril_socket_t s, const void *buf, unsigned int len);
```

```c
//发送数据(非阻塞方式)
int ril_sock_send_async(ril_socket_t s, const void *buf, unsigned int len);
```

**返回值:**

对于阻塞式接口，如果返回RIL_OK，表示当前数据已经成功发送到服务器；而对于非阻塞式接口，它只是表示成功数据推送到了模组的缓冲区，对于发送结果需要使用主动查询。

**参数说明:**

- s : 表示socket描述符，在创建时由RIL提供。
- buf：发送缓冲区。
- len: 数据发送长度。

#### 数据接收

对于从模组接收到的数据，RIL会先将它缓冲到指定socket的接收缓冲区内，我们可以通过"ril_sock_recv"读取，它是一个非阻塞式接口，其定义如下：

```c
unsigned int ril_sock_recv(ril_socket_t s, void *buf, unsigned int len);
```

**返回值:**

实际接收到的数据，如果缓冲区为空，则直接返回0。

**参数说明:**

- s : 表示socket描述符，在创建时由RIL提供。
- buf：接收缓冲区。
- len: 接收缓冲区长度。