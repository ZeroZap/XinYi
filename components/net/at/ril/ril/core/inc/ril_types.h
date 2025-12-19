/******************************************************************************
 * @brief    ril 基本数据类型定义
 *
 * Copyright (c) 2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2020-05-27     Morro        Initial version. 
 ******************************************************************************/
#ifndef _RIL_TYPE_H_
#define _RIL_TYPE_H_

#include <stdbool.h>

/**
 * GSM 频段定义
 */
#define BAND_GSM_900         0x01         
#define BAND_GSM_1800        0x02
#define BAND_GSM_850         0x04
#define BAND_GSM_1900        0x08
#define BAND_GSM_ALL         0x0F

/**
 * 错误码定义
 */
#define RIL_OK               0                   /* 正确无误 */
#define RIL_ERROR           -1                   /* 通用错误 */
#define RIL_TIMEOUT         -2                   /* 执行超时 */
#define RIL_FAILED          -3                   /* 执行失败 */
#define RIL_NOIMPL          -4                   /* 接口未实现 */
#define RIL_ABORT           -5                   /* 已终止 */     
#define RIL_NOMEM           -6                   /* 内存不足 */
#define RIL_REJECT          -7                   /* 操作被拒*/
#define RIL_INVALID         -8                   /* 无效参数*/  
#define RIL_ONGOING         -9                   /* 进行中*/
#define RIL_FILE_NOT_FOUND  -10                  /* 未找到文件*/


/*RIL通知类型定义 -------------------------------------------------------------*/
typedef enum {
    /**
     * @brief       sim卡状态更新 
     * @param[in]   data -> ril_sim_status *
     */
    RIL_NOTIF_SIM = 0,                               
    /**
     * @brief       网络注册状态更新
     * @param[in]   data ->  ril_netreg_status *
     */    
    RIL_NOTIF_NETREG,                                
    /**
     * @brief       网络连接状态更新
     * @param[in]   data ->  ril_netconn_status *
     */     
    RIL_NOTIF_NETCONN,                               
    /**
     * @brief       收到短信
     * @param[]
     * @param[in]   data -> sms_info_t *
     */
    RIL_NOTIF_SMS,
    /**
     * @brief       来电
     * @param[in]   data -> char *phone
     */     
    RIL_NOTIF_RING,
    /**
     * @brief       命令执行超时
     */     
    RIL_NOTIF_TIMEOUT,
    /**
     * @brief       未知错误(影响设备正常工作)
     */    
    RIL_NOTIF_ERROR,
    /**
     * @brief       用户自定义通知，一般用于特殊的URC码
     */    
    RIL_NOTIF_CUSTOM,
    
    RIL_NOTIF_MAX
}ril_notify_type;

/**
 * @brief 通知管理项
 */
typedef struct {
    ril_notify_type type;
    void (*handler)(void *data, int data_size);    
}ril_notify_item_t;

#define __ril_on_notify(type, handler) \
USED ANONY_TYPE(const ril_notify_item_t ,__ril_notify_item_##handler)\
    SECTION("_section.ril.notify.1") UNUSED  = {type, \
    (void (*)(void *, int))handler };

/* ril请求码定义 --------------------------------------------------------------*/
typedef enum {
    /**
     * @brief       获取sim卡状态
     * @param[out]  data -> ril_sim_status *
     */
    RIL_REQ_GET_SIM_STATUS = 0,                           
    /**
     * @brief       获取网络注册状态
     * @param[out]  data -> ril_netreg_status *
     */
    RIL_REQ_GET_REG_STATUS,                           
    /**
     * @brief       获取网络连接状态
     * @param[out]  data -> ril_netconn_status *
     */
    RIL_REQ_GET_CONN_STATUS,                           
    
    /**
     * @brief       获取CSQ值
     * @param[out]  data -> ril_csq_t *
     */    
    RIL_REQ_GET_CSQ,                                   
    
    /**
     * @brief       获取IMEI号
     * @param[out]  data ->  char *imei
     */    
    RIL_REQ_GET_IMEI,                                  
    /**
     * @brief       获取IMSI号
     * @param[out]  data ->  char *imsi
     */       
    RIL_REQ_GET_IMSI,                                
    /**
     * @brief       获取硬件版本号
     * @param[out]  data ->  char *hwver
     */        
    RIL_REQ_GET_HWVER,                                
    /**
     * @brief       获取软件版本号
     * @param[out]  data ->  char *swver
     */       
    RIL_REQ_GET_SWVER,                               
    /**
     * @brief       获取型号
     * @param[out]  data ->  char *model
     */      
    RIL_REQ_GET_MODEL,                                 
    /**
     * @brief       打电话
     * @param[out]  data -> const char *phone
     */     
    RIL_REQ_DIAL,
    /**
     * @brief       挂电话     
     */     
    RIL_REQ_HANGUP,
    /**
     * @brief       接听电话     
     */     
    RIL_REQ_ANSWER,
    /**
     * @brief       获取IP地址
     * @param[out]  data -> char * ip
     */    
    RIL_REQ_GET_IPADDR,     
    /**
     * @brief       设置DNS
     * @param[out]  data[0] -> (char *)主DNS, data[1] -> (char *)备DNS
     */    
    RIL_REQ_SET_DNS      
}ril_request_code;  


/*sim 卡状态 -----------------------------------------------------------------*/
typedef enum {
    SIM_UNKNOW = 0,
    SIM_CHKING,                                        /* 正在检测 */
    SIM_ABSENT,                                        /* 未找到卡 */
    SIM_PIN,                                           /* 需要pin码解锁 */
    SIM_PUK,                                           /* 需要puk码解锁 */
    SIM_READY                                          /* 正常使用 */
}ril_sim_status;

/*网络注册状态*/
typedef enum {
    NETREG_UNREG = 0,                                  /* 未注册*/
    NETREG_REG,                                        /* 已注册*/
    NETREG_REGISTING,                                  /* 正在注册*/
    NETREG_DENIED,                                     /* 注册被拒绝*/
    NETREG_UNKNOWN,                                    /* 未知代码*/
    NETREG_ROAMING                                     /* 漫游状态*/
}ril_netreg_status;

/*网络连接状态*/
typedef enum { 
    NETCONN_OFFLINE = 0,                              /* 离线状态*/      
    NETCONN_PENDING,                                  /* 请求中*/
    NETCONN_ONLINE                                    /* 已上线*/
}ril_netconn_status;

/* 信号质量 */
typedef struct {
    int rssi;      /* Valid values are (0-31, 99) as defined in TS 27.007 8.5 */
    int error_rate;/* bit error rate (0-7, 99) as defined in TS 27.007 8.5 */
}ril_csq_t;

/*ril状态 --------------------------------------------------------------------*/
typedef struct {
    ril_sim_status     sim;                             /* sim卡状态 */
    ril_netreg_status  netreg;                          /* 网络注册状态 */
    ril_netconn_status conn;                            /* 网络连接状态 */
}ril_status_t;

/* 短信信息 -------------------------------------------------------------------*/
typedef struct {
    char               phone[16];                       /* 源手机号*/
    unsigned int       len;                             /* 消息长度*/
    unsigned char      msg[0];                          /* 消息体*/
}sms_info_t;

/*ril适配器管脚类型 ----------------------------------------------------------*/
typedef enum {
    RIL_PIN_RESET,                                      /* 复位 */
    RIL_PIN_POWER,                                      /* 电源 */
    RIL_PIN_PWRKEY,                                     /* 启动(Power key) */
    RIL_PIN_DTR,                                        /* 数据就绪 */
    RIL_PIN_RING,                                       /* ring指示*/ 
}ril_pin_type;

/*小区信息 -------------------------------------------------------------------*/
typedef struct  {
    int mcc;    /* 3-digit Mobile Country Code, 0..999, INT_MAX if unknown  */
    int mnc;    /* 2 or 3-digit Mobile Network Code, 0..999, INT_MAX if unknown  */
    int lac;    /* 16-bit Location Area Code, 0..65535, INT_MAX if unknown  */
    int cid;    /* 28-bit UMTS Cell Identity described in TS 25.331, 0..268435455, INT_MAX if unknown  */
    int psc;    /* 9-bit UMTS Primary Scrambling Code described in TS 25.331, 0..511, INT_MAX if unknown */
}cell_info_t;

/*ril接口适配器 --------------------------------------------------------------*/
typedef struct {
    /**
     * @brief       引脚控制接口
     * @param[in]   pin    - 引脚类型
     * @param[in]   isread - 指示是否进行读操作
     * @param[in]   level  - 0/1写入的电平值(非读操作有效)
     * @return      当前引脚电平
     */
    int (*pin_ctrl)(ril_pin_type pin, int isread, int level);
    /**
     * @brief       数据写接口
     * @param[in]   buf    - 数据缓冲区
     * @param[in]   len    - 数据长度
     * @return      实际成功写入的数据长度
     */
    unsigned int (*write)(const void *buf, unsigned int len);
    /**
     * @brief       数据读接口
     * @param[out]  buf    - 数据缓冲区
     * @param[in]   len    - 期待读取的数据长度
     * @return      实际读取的数据长度    
     */    
    unsigned int (*read) (void *buf, unsigned int len);
}ril_adapter_t;

/*ril配置参数 ----------------------------------------------------------------*/
typedef struct {
    struct {
        const char    *apn;                               /* 接入点名称*/
        const char    *user;                              /* 用户名*/
        const char    *passwd;                            /* 密码 */
    } apn;
}ril_config_t;

#endif
