/******************************************************************************
 * @brief    AT模组OS相关移植接口
 *
 * Copyright (c) 2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2020-01-02     Morro        Initial version
 ******************************************************************************/
#ifndef _AT_UTIL_H_
#define _AT_UTIL_H_

#include "ril_port.h"
#include <stdbool.h>


typedef ril_sem_t at_sem_t;                                /*信号量*/

/*
 * @brief	   获取当前系统毫秒数
 */
static inline unsigned int at_get_ms(void)
{
    return ril_get_ms();
}
/*
 * @brief	   超时判断
 * @retval     true | false
 */
static inline bool at_istimeout(unsigned int start_time, unsigned int timeout)
{
    return ril_istimeout(start_time, timeout);
}

/*
 * @brief	   毫秒延时
 * @retval     none
 */
static inline void at_delay(unsigned int ms)
{
    ril_delay(ms);
}
/*
 * @brief	   创建信号量
 * @retval     none
 */
static inline at_sem_t at_sem_new(int value)
{
    return ril_sem_new(value);
}
/*
 * @brief	   等待信号量
 * @retval     none
 */
static inline bool at_sem_wait(at_sem_t s, unsigned int timeout)
{
    return ril_sem_wait(s, timeout);
}

/*
 * @brief	   释放信号量
 * @retval     none
 */  
static inline void at_sem_post(at_sem_t s)
{
    ril_sem_post(s);
}

///*
// * @brief	   释放信号量
// * @retval     none
// */  
//static inline void at_sem_free(ril_sem_t s)
//{
//    ril_sem_free(s);
//}

#endif
