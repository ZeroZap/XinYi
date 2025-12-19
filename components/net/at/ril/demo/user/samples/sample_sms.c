/******************************************************************************
 * @brief    RIL SMS 收发演示程序
 *
 * Copyright (c) 2020  <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2021-02-05     Morro        Initial version
 ******************************************************************************/
#include "cli.h"                                    //命令行管理器
#include "ril.h"
#include "ril_core.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
 * @brief   短信接收处理
 */
static void sms_recv_handler(sms_info_t *sms, int data_size)
{
    printf("Receive sms=> \r\nphone:%s\r\nText:%s\r\n",sms->phone, sms->msg);
}
ril_on_notify(RIL_NOTIF_SMS, sms_recv_handler);


/*
 * @brief   sms短信发送命令
 *          命令格式:sms,phone,message text
 * @example sms,18512344321,sms test
 */    
static int do_cmd_sms(struct cli_obj *cli, int argc, char *argv[])
{
    bool res;
    if (!ril_isreg()) {
        cli->print(cli, "unreg to network.");      //未注册到网络,无法发送短信
        return -1;
    }    
    if (argc != 3) {
        cli->print(cli, "Command format error!!!\r\n"
                 "format:sms,phone,message text.\r\n"
                 "Example:sms,18912345678,sms test.\r\n");
        return -1;
    }
    res = ril_sms_send(argv[1], argv[2]);
    cli->print(cli, "sms send %s\r\n", res == RIL_OK ? "OK" : "ERROR");
    return 0;
                      
}cmd_register("sms", do_cmd_sms, "send sms");     //注册短信发送命令
