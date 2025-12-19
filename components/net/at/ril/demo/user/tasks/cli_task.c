/******************************************************************************
 * @brief    命令行任务
 *
 * Copyright (c) 2020  <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2020-09-21     Morro        Initial version
 ******************************************************************************/
#include "taskManager.h"
#include "cli.h"
#include "tty.h"
#include <stdio.h>

static cli_obj_t cli;                               /*命令行对象 */

/*
 * @brief   命令行任务
 */
static void cli_task(void *params)
{
    cli_port_t p = {tty.write, tty.read};           /*读写接口 */
    
    cli_init(&cli, &p);                             /*初始化命令行对象 */
    
    cli_enable(&cli);    
    while (1) {
        cli_process(&cli);
        os_sleep(20);
    }
}

task_define("cli", cli_task, 512, 5);
