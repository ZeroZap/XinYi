/******************************************************************************
 * @brief    ril操作系统相关移植接口模板
 *
 * Copyright (c) 2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2020-09-21     Morro        Initial version. 
 ******************************************************************************/
#include "os_port.h"
#include "ril_port.h"
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>

/*
 * @brief	   获取当前系统毫秒数
 */
unsigned int ril_get_ms(void)
{
    return os_get_ms();
}

/**
 * @brief	   毫秒延时
 * @params[in] ms    -  延时的毫秒数
 * @return     none
 */
void ril_delay(uint32_t ms)
{
    os_delay(ms);
}

/**
 * @brief	   创建信号量
 * @params[in] value    - 初始值
 * @return     指向一新信号量的指针
 */
ril_sem_t ril_sem_new(int value)
{
    return os_sem_new(value);
}
/**
 * @brief	   等待信号量
 * @params[in] s       - 信号量
 * @params[in] timeout - 等待超时时间(ms为单位)
 * @return     true - 成功获取到信号量, false - 等待超时  
 * @note       如果"timeout"参数非零，线程应该仅在指定的时间内阻塞(以单位为毫秒)。
 *             如果"timeout"参数为零,则任务应该一直阻塞,直到信号量发出信号。
 */
bool ril_sem_wait(ril_sem_t s, unsigned int timeout)
{
    return os_sem_wait(s, timeout);	
}

/**
 * @brief	   发送信号量
 * @params[in] s      - 信号量
 * @return     none
 */ 
void ril_sem_post(ril_sem_t s)
{
    os_sem_post(s);
}

/*
 * @brief	   释放信号量
 * @retval     none
 */  
void ril_sem_free(ril_sem_t s)
{
   os_sem_free(s);
}

/**
 * @brief	   进入临界区
 * @return     none
 */ 
void ril_enter_critical(void)
{
    os_enter_critical();
}

/*
 * @brief	   退入临界区
 * @return     none
 */   
void ril_exit_critical(void)
{
    os_exit_critical();
}

/*
 * @brief	   内存分配
 */  
void *ril_malloc(int nbytes)
{
    return os_mem_malloc(nbytes);
}
/*
 * @brief	   内存释放
 */  
void ril_free(void *p)
{
    os_mem_free(p);
}

/**
 * @brief      调试输出
 * @params[in] level - log等级,参考RIL_LOG_XXX
 * @params[in] fmt   - 格式化输出
 */ 
void ril_log(int level, const char *fmt, ...)
{
    va_list   args;
    ril_enter_critical();
    
    printf("[RIL]:");
    va_start(args, fmt); 
    vprintf(fmt, args);
    va_end(args);
    
    ril_exit_critical();
}

