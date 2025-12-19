/******************************************************************************
 * @brief    RIL 任务管理
 *
 * Copyright (c) 2020  <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2021-02-05     Morro        Initial version
 ******************************************************************************/
#include "public.h"
#include "ril.h"
#include "module_uart.h"
#include "taskManager.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

/*硬件引脚定义 ---------------------------------------------------------------*/


#define POWER_PIN_PORT     GPIOB
#define POWER_PIN_NUM      GPIO_Pin_12                        //电源脚

#define PWRKEY_PIN_PORT     GPIOB 
#define PWRKEY_PIN_NUM      GPIO_Pin_10                       //启动脚

#define DTR_PIN_PORT       GPIOC
#define DTR_PIN_NUM        GPIO_Pin_2

#define RING_PIN_PORT      GPIOC                              //ring脚(暂时未用)
#define RING_PIN_NUM       GPIO_Pin_3


#define RESET_PIN_PORT      GPIOB                            //复位脚
#define RESET_PIN_NUM       GPIO_Pin_5


/*
 * @brief   模组引脚配置
 */
static void port_init(void)
{
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA , ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB , ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC , ENABLE);
    
    //power  pin
    gpio_conf(POWER_PIN_PORT, GPIO_Mode_OUT, GPIO_PuPd_NOPULL, POWER_PIN_NUM); 
    //power key
    gpio_conf(PWRKEY_PIN_PORT, GPIO_Mode_OUT, GPIO_PuPd_NOPULL,PWRKEY_PIN_NUM); 
    //DTR
    gpio_conf(DTR_PIN_PORT, GPIO_Mode_OUT, GPIO_PuPd_NOPULL,DTR_PIN_NUM);
    
    //RING
    gpio_conf(RING_PIN_PORT, GPIO_Mode_IN, GPIO_PuPd_NOPULL,RING_PIN_NUM);
    
    //RESET
    gpio_conf(RESET_PIN_PORT, GPIO_Mode_OUT, GPIO_PuPd_NOPULL,RESET_PIN_NUM);
}

/* 
 * @brief       io引脚控制
 * @param[in]   p      - 引脚类型
 * @param[in]   isread - 指示是否是读操作
 * @param[in]   level  - 引脚电平(仅适用于写操作)
 * @return      none
 */ 
static int io_ctrl(ril_pin_type p, int isread, int level)
{
    if (!isread) {                            
        switch(p) {                          /* 写操作 */
        case RIL_PIN_RESET:      
            GPIO_WriteBit(RESET_PIN_PORT, RESET_PIN_NUM, (BitAction)level);
            break;
        case RIL_PIN_POWER:
            GPIO_WriteBit(POWER_PIN_PORT, POWER_PIN_NUM, (BitAction)level);
            break;
        case RIL_PIN_PWRKEY:
            GPIO_WriteBit(PWRKEY_PIN_PORT, PWRKEY_PIN_NUM, (BitAction)level);
            break;
        case RIL_PIN_DTR:
            GPIO_WriteBit(DTR_PIN_PORT, DTR_PIN_NUM, (BitAction)level);
            break;       
        }
    } else {                                /* 读操作 */
        switch(p) {
        case RIL_PIN_RING:
            return GPIO_ReadInputDataBit(RING_PIN_PORT, RING_PIN_NUM);
        }
    }
    return level;
}

/*
 * @brief   ril通知处理
 */
static void sim_status_changed_handler(ril_sim_status *sim)
{
    printf("SIM Card status changed to:%d\r\n", *sim);
}ril_on_notify(RIL_NOTIF_SIM, sim_status_changed_handler);

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
    
    result = ril_use_device("EC21");   //选择模组型号
    
    printf("Ril select device %s\r\n", result == RIL_OK ? "OK": "ERROR");
    ril_open();                        //打开设备
    ril_netconn(true);                 //启动网络连接
} system_init("ril", ril_work_init);

task_define("ril main", ril_main_task, 256 ,4);          //定义主任务
task_define("ril at",   ril_atcmd_task, 256, 3);         //AT接收处理任务
