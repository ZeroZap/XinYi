/******************************************************************************
 * @brief      平台相关
 *
 * Copyright (c) 2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 ******************************************************************************/
#include "tty.h"
#include "public.h"
#include "taskManager.h"
#include "comdef.h"
#include <config.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/*
 * @brief	   重定向printf
 */
int fputc(int c, FILE *f)
{       
    tty.write(&c, 1);
    while (tty.tx_isfull()) {}                           //防止丢LOG
    return c;
}

/*******************************************************************************
* @brief	   硬件驱动初始化
* @param[in]   none
* @return 	   none
*******************************************************************************/
void hw_board_init(void)
{        
    RCC_ClocksTypeDef rcc;
    
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  /*设置优先级分组 --------*/
    SystemCoreClockUpdate();  
	NVIC_SetPriority(SysTick_IRQn, 0);		         // Set SysTick IRQ priority      
    NVIC_EnableIRQ(SysTick_IRQn);

    tty.init(115200);
    printf("Program build at : %s %s\r\n", __DATE__, __TIME__);
    RCC_GetClocksFreq(&rcc);
    printf("System clock:%d Hz\r\n", SystemCoreClock);
    printf("HCLK:%d Hz, PCLK1:%d Hz, PCLK2:%d Hz, SYSCLK:%d Hz\r\n", 
           rcc.HCLK_Frequency, rcc.PCLK1_Frequency, rcc.PCLK2_Frequency, rcc.SYSCLK_Frequency);    
    printf("Reset type:%08x\r\n", RCC->CSR);   
    
}system_init("sys", hw_board_init);
