/******************************************************************************
 * @brief    任务管理器
 *
 * Copyright (c) 2020  <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2021-01-03     Morro        Initial version
 ******************************************************************************/

#ifndef _TASK_MANAGER_H_
#define _TASK_MANAGER_H_

#include "os_port.h"
#include "comdef.h"
#include <stdbool.h>

/*模组初始化项*/
typedef struct {
    const char *name;               //模组名称
    void (*init)(void);             //初始化接口
}init_item_t;

/*任务处理项*/
typedef struct {
    const char *name;               //模组名称    
    void (*entry)(void *param);     //任务入口
    unsigned int stack_size;        //栈大小    
    int          prority;           //任务优先级
}task_item_t;

#define __module_initialize(name,func,level)           \
    USED ANONY_TYPE(const init_item_t, init_tbl_##func)\
    SECTION("init.item."level) = {name,func}

/*
 * @brief       任务定义
 * @param[in]  name        - 任务名称 
 * @param[in]  entry       - 任务入口(void func(void *){...})
 * @param[in]  stack_size  - 栈大小
 * @param[in]  prority     - 任务优先级
 */
#define task_define(name, entry, stack_size, prority)       \
    USED ANONY_TYPE(const task_item_t, task_item_##entry)   \
    SECTION("task.item.1") =                                \
    {name, (void (*)(void *))entry, stack_size, prority}

/*
 * @brief       模组初始化注册(优先级system_init > driver_init > module_init)
 * @param[in]   name    - 模组名称 
 * @param[in]   func    - 初始化入口函数(void func(void){...})
 */
#define system_init(name,func)  __module_initialize(name,func,"1")
#define driver_init(name,func)  __module_initialize(name,func,"2")
#define module_init(name,func)  __module_initialize(name,func,"3")

void os_sleep(unsigned int ms);    
    
void os_run(void);

#endif
