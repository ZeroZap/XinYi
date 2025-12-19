/******************************************************************************
 * @brief    rt-thread 移植接口
 *
 * Copyright (c) 2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2020-09-21     roger.luo    Initial version. 
 ******************************************************************************/

#include "rtthread.h"
#include "ril_port.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

/**
 * @brief	   获取当前系统毫秒数
 */
unsigned int ril_get_ms(void)
{
    return rt_tick_get();
}


/**
 * @brief	   毫秒延时
 * @params[in] ms    -  延时的毫秒数
 * @return     none
 */
void ril_delay(uint32_t ms)
{
    rt_thread_mdelay(ms);
}
/**
 * @brief	   创建信号量
 * @params[in] value    - 初始值
 * @return     指向一新信号量的指针
 */
ril_sem_t ril_sem_new(int value)
{
    return rt_sem_create("ril", value, 0);
}
/**
 * @brief	   等待信号量
 * @params[in] s       - 信号量
 * @params[in] timeout - 等待超时时间(ms为单位)
 * @return     true - 成功获取到信号量, false - 等待超时  
 * @note       如果"timeout"参数非零，线程应该仅在指定的时间内阻塞(以单位为毫秒)。
 *             如果"timeout"参数为零,则任务应该一直阻塞,直到信号量发出信号。
 */
bool ril_sem_wait(ril_sem_t s, uint32_t timeout)
{
    if (timeout == 0)
        timeout = 0xFFFFFFFF - 1;
    return rt_sem_take(s, timeout) == RT_EOK;
}

/**
 * @brief	   发送信号量
 * @params[in] s      - 信号量
 * @return     none
 */  
void ril_sem_post(ril_sem_t s)
{
    rt_sem_release(s);
}

/*
 * @brief	   释放信号量
 * @retval     none
 */  
void ril_sem_free(ril_sem_t s)
{
    rt_sem_delete(s);
}

/**
 * @brief	   进入临界区
 * @return     none
 */ 
void ril_enter_critical(void)
{
    rt_enter_critical();
}

/*
 * @brief	   退入临界区
 * @return     none
 */   
void ril_exit_critical(void)
{
    rt_exit_critical();
}

/**
 * @brief	   内存分配
 * @params[in] nbytes - 分配字节数
 */  
void *ril_malloc(int nbytes)
{
    return rt_malloc((rt_size_t)nbytes);
}
/**
 * @brief	   内存释放
 * @params[in] p - 待释放指针
 */   
void ril_free(void *p)
{
    rt_free(p);
}

/**
 * @brief      调试输出
 * @params[in] level - log等级,参考RIL_LOG_XXX
 * @params[in] fmt   - 格式化输出
 */ 
void ril_debug(const char *fmt, ...)
{
    va_list   args;
    printf("[RIL]:");
    va_start(args, fmt); 
    vprintf(fmt, args);
    va_end(args);
}
