/******************************************************************************
 * @brief     ril操作系统相关移植接口
 *
 * Copyright (c) 2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2020-09-21     Morro        Initial version. 
 ******************************************************************************/

#ifndef _RIL_PORT_H_
#define _RIL_PORT_H_

#include <stdint.h>
#include <stdbool.h>

/* log等级定义 ---------------------------------------------------------------*/
#define RIL_LOG_DBG                      0                   /* debug信息 */
#define RIL_LOG_INFO                     1                   /* 正常状态信息 */
#define RIL_LOG_WARN                     2                   /* 警告信息 */
#define RIL_LOG_ERR                      3                   /* 异常信息 */ 

typedef void *ril_sem_t;                                     /* 信号量*/

/**
 * @brief	   获取当前系统毫秒数
 */
unsigned int ril_get_ms(void);

/**
 * @brief	   超时判断
 * @params[in] start_time - 起始时间
 * @params[in] timeout    - 超时时间(ms) 
 * @return     true | false
 */
static inline bool ril_istimeout(unsigned int start_time, unsigned int timeout)
{
	return ril_get_ms() - start_time > timeout;
}

/**
 * @brief	   毫秒延时
 * @params[in] ms    -  延时的毫秒数
 * @return     none
 */
void ril_delay(unsigned int ms);

/**
 * @brief	   新建信号量
 * @params[in] value    - 初始值
 * @return     指向一新信号量的指针
 */
ril_sem_t ril_sem_new(int value);

/**
 * @brief	   等待信号量
 * @params[in] s       - 信号量
 * @params[in] timeout - 等待超时时间
 * @return     true - 成功获取到信号量, false - 等待超时  
 */
bool ril_sem_wait(ril_sem_t s, unsigned int timeout);

/**
 * @brief	   发送信号量
 * @params[in] s      - 信号量
 * @return     none
 */  
void ril_sem_post(ril_sem_t s);

/**
 * @brief	   释放信号量(暂时未用)
 * @return     none
 */  
void ril_sem_free(ril_sem_t s);


/**
 * @brief	   进入临界区
 * @return     none
 */  
void ril_enter_critical(void);

/**
 * @brief	   退入临界区
 * @return     none
 */  
void ril_exit_critical(void);

/**
 * @brief	   内存分配
 * @params[in] nbytes - 分配字节数
 */  
void *ril_malloc(int nbytes);

/**
 * @brief	   内存释放
 * @params[in] p - 待释放指针
 */  
void ril_free(void *p);

/**
 * @brief      调试输出
 * @params[in] level - log等级,参考RIL_LOG_XXX
 * @params[in] fmt   - 格式化输出
 */ 
void ril_log(int level, const char *fmt, ...);

#endif
