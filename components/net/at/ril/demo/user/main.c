/******************************************************************************
 * @brief     主程序入口
 *
 * Copyright (c) 2020~2021, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2020-09-21     Morro        Initial version
 * 2021-01-03     Morro        使用taskManager管理任务
 ******************************************************************************/
#include "taskManager.h"

int main(void)
{
    os_run();
    return 0;
}
