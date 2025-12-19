/******************************************************************************
 * @brief    RIL TFTP 文件下载演示程序
 *
 * Copyright (c) 2020  <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2021-02-05     Morro        Initial version
 ******************************************************************************/
#include "public.h"
#include "cli.h"
#include "tftp_client.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * @brief       tftp 事件处理
 */
static void tftp_event(tftp_event_args_t *e)
{
    unsigned int recvsize = e->offset + e->datalen;
    if (e->state == TFTP_STAT_DATA) {
        printf("%d/%d bytes %.1f%% completed.\r\n", recvsize , e->filesize, 
                 100.0 * recvsize/ e->filesize);
        
        //write(file, e->data, e->datalen);
        
    }    
    if (e->state == TFTP_STAT_DONE)
        printf("\r\nDownload complete, spand time:%d\r\n", e->spand_time);
}

/**
 * @brief   tftp文件下载命令
 *          命令格式:tftp,host,port,filename,timeout
 * @example tftp,123.146.152.12,1234,/ril-demo.hex,100
 */    
static int do_cmd_tftp(struct cli_obj *cli, int argc, char *argv[])
{
    tftp_client_t *tftp;
    const char *host, *file;
    int port, timeout;
    if (argc < 5) {
        cli->print(cli, "Command format error!!!\r\n"
                 "Format:tftp,host,port,filename\r\n"
                 "Example:tftp,123.146.152.12,1234,/ril-demo.hex\r\n");
        return -1;
    }
    host = argv[1];
    port = atoi(argv[2]);
    file = argv[3];
    timeout = atoi(argv[4]);
    cli->print(cli, "Download file [%s] from [%s].\r\n", file, host);
    
    //创建TFTP客户端
    tftp = tftp_client_create(tftp_event, host, port);
    if (tftp == NULL) {
        cli->print(cli, "Input error, tftp client create failed.\r\n");
        return -1;
    }
    tftp_start_download(tftp, file, timeout);             //启动TFTP下载
    tftp_client_destroy(tftp);                            //销毁客户端
    return 0;
                      
}cmd_register("tftp", do_cmd_tftp, "tftp file download"); //注册tftp命令
