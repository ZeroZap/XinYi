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
#include "taskManager.h"
#include <stddef.h>

/*
 * @brief       空处理,用于定位段入口
 */
static void nop_process(void) {}
    

const init_item_t init_tbl_start SECTION("init.item.0") = {     
    "", nop_process
};

const init_item_t init_tbl_end SECTION("init.item.4") = {       
    "", nop_process
};


const task_item_t task_tbl_start SECTION("task.item.0") = {     
    0
};

const task_item_t task_tbl_end SECTION("task.item.2") = {       
    0
};

/*
 * @brief       模组初始化
 * @param[in]   none
 * @return      none
 */
static void init_items(void)
{
    const init_item_t *it = &init_tbl_start;
    while (it < &init_tbl_end) {
        it++->init();
    }   
}

/*
 * @brief       创建任务
 * @param[in]   none
 * @return      none
 */
static void create_tasks(void)
{
    const task_item_t *t;
    for (t = &task_tbl_start + 1; t < &task_tbl_end; t++) {
        os_task_create( t->entry, t->name, t->stack_size, t->prority, NULL, NULL);   
    }
}

/*
 * @brief      运行任务
 *              1. 模组初始化优化级 system_init > driver_init > module_init
 *              2. 创建任务
 *              3. 启动任务
 * @param[in]   none
 * @return      none
 */
void os_run(void)
{
    init_items();
    create_tasks(); 
    os_start_kernel();
}
/*
 * @brief       任务休眠
 * @param[in]   ms
 * @return      none
 */
void os_sleep(unsigned int ms)
{
    vTaskDelay(ms);
}

/*
 * @brief       显示任务信息
 */
void os_show_task_info(void)
{
}
