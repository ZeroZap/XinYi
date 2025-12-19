/******************************************************************************
 * @brief    RIL HTTP 文件下载演示程序
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
#include "http_client.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * @brief       http 事件处理
 */
static void http_event(http_event_args_t *e)
{
    unsigned int recvsize = e->offset + e->datalen;
    if (e->state == HTTP_STAT_DATA) {
        printf("%d/%d bytes %.1f%% completed.\r\n", recvsize , e->filesize, 
                 100.0 * recvsize/ e->filesize);
        
        //write(file, e->data, e->datalen);
        
    }    
    if (e->state == HTTP_STAT_DONE)
        printf("\r\nDownload complete, spand time:%d\r\n", e->spand_time);
}

/**
 * @brief   http文件下载命令
 *          命令格式:http,host,port,filename, timeout(s)
 * @example http,123.146.152.12,1234,/ril-demo.hex,120
 */    
static int do_cmd_http(struct cli_obj *cli, int argc, char *argv[])
{
    http_client_t *http;
    const char *host, *file;
    int port, timeout;
    if (argc < 5) {
        cli->print(cli, "Command format error!!!\r\n"
                 "Format:http,host,port,filename, timeout(s)\r\n"
                 "Example:http,123.146.152.12,1234,/ril-demo.hex,120\r\n");
        return -1;
    }
    host = argv[1];
    port = atoi(argv[2]);
    file = argv[3];
    timeout = atoi(argv[4]);
    
    cli->print(cli, "Download file [%s] from [%s].\r\n", file, host);
    
    //创建HTTP客户端
    http = http_client_create(http_event, host, port);
    if (http == NULL) {
        cli->print(cli, "Input error, http client create failed.\r\n");
        return -1;
    }
    http_start_download(http, file, timeout);             //启动HTTP下载
    http_client_destroy(http);                            //销毁客户端
    return 0;
                      
}cmd_register("http", do_cmd_http, "http file download"); //注册http命令
