/******************************************************************************
 * Copyright (C) 2021, Xiaohua Semiconductor Co., Ltd. All rights reserved.
 *
 * This software component is licensed by XHSC under BSD 3-Clause license
 * (the "License"); You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                    opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************/

/******************************************************************************
 * @file   main.c
 *
 * @brief  Source file for FLASH example
 *
 * @author MADS Team 
 *
 ******************************************************************************/

/******************************************************************************
 * Include files
 ******************************************************************************/
#include "ddl.h"
#include "flash.h"

/******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
//#define RUN_IN_RAM 1    //need to cfg *.icf

/******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
uint32_t           u32TestBuffer[128] = {0};
/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/


/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/

/**
 ******************************************************************************
 ** \brief  Main function of project
 **
 ** \return uint32_t return value, if needed
 **
 ** This sample
 **
 ******************************************************************************/

int32_t main(void)
{
		///< 内核函数，SysTick配置，定时1s，系统时钟默认RCH 4MHz
    SysTick_Config(SystemCoreClock);
#if 0
    uint32_t          u32Addr         = 0x1FE00;

    ///< STEP-1: 确保初始化正确执行后方能进行FLASH编程操作，FLASH初始化（编程时间,休眠模式配置）
    while(Ok != Flash_Init(1, TRUE))
    {
        ///<用户可以根据自身使用需求添加超时处理代码
    }

    ///< STEP-2 FLASH 操作区解锁(Sector252~255)
    while(Ok != Flash_LockSet(FlashLock1, 0x80000000))
    {
        ///<用户可以根据自身使用需求添加超时处理代码
    }
    
    
//< ==> 8位编程     
    ///< STEP-3 FLASH目标扇区擦除
    while(Ok != Flash_OpModeConfig(FlashSectorEraseMode))
    {
        ///<用户可以根据自身使用需求添加超时处理代码
    }
    if(Ok != Flash_SectorErase(u32Addr))
    {
        while(1)
        {
            ///<用户可以根据自身使用需求添加超时处理代码
        }
    }
    ///< STEP-4 FLASH 字节写、校验    
    while(Ok != Flash_OpModeConfig(FlashWriteMode))
    {
        ///<用户可以根据自身使用需求添加超时处理代码
    }
    if (Ok != Flash_Write8(u32Addr, (uint8_t *)u32TestBuffer, 512))
    {
        while(1)
        {
            ///<用户可以根据自身使用需求添加超时处理代码
        }
    }

    
    
//< ==> 16位编程    
    ///< STEP-3 FLASH目标扇区擦除
    while(Ok != Flash_OpModeConfig(FlashSectorEraseMode))
    {
        ///<用户可以根据自身使用需求添加超时处理代码
    }
    if(Ok != Flash_SectorErase(u32Addr))
    {
        while(1)
        {
            ///<用户可以根据自身使用需求添加超时处理代码
        }
    }
    ///< STEP-4 FLASH 半字写、校验
    while(Ok != Flash_OpModeConfig(FlashWriteMode))
    {
        ///<用户可以根据自身使用需求添加超时处理代码
    }
    if (Ok != Flash_Write16(u32Addr, (uint16_t *)u32TestBuffer, 256))
    {
        while(1)
        {
            ///<用户可以根据自身使用需求添加超时处理代码
        }
    }


    
//< ==> 32位编程    
    ///< STEP-3 FLASH目标扇区擦除
    while(Ok != Flash_OpModeConfig(FlashSectorEraseMode))
    {
        ///<用户可以根据自身使用需求添加超时处理代码
    }
    while(Ok != Flash_SectorErase(u32Addr))
    {
        while(1)
        {
            ///<用户可以根据自身使用需求添加超时处理代码
        }
    }
    
    ///< STEP-4 FLASH 字写、校验
    while(Ok != Flash_OpModeConfig(FlashWriteMode))
    {
        ///<用户可以根据自身使用需求添加超时处理代码
    }
    if (Ok != Flash_Write32(u32Addr, u32TestBuffer, 128))
    {
        while(1)
        {
            ///<用户可以根据自身使用需求添加超时处理代码
        }
    }


    ///< FLASH 读模式
    ///< 注意！！！当不对FLASH进行操作时，应当将FLASH操作状态设置为【读模式】
    while(Ok != Flash_OpModeConfig(FlashReadMode))
    {
        ///<用户可以根据自身使用需求添加超时处理代码
    }
    ///< FLASH 全部加锁
    while(Ok != Flash_LockAll())
    {
        ///<用户可以根据自身使用需求添加超时处理代码
    }
    
    while (1);
#endif
}

/******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
