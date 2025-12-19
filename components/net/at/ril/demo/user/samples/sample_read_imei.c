/******************************************************************************
 * @brief    RIL IMEI ∂¡»°—› æ≥Ã–Ú
 *
 * Copyright (c) 2020  <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2021-02-05     Morro        Initial version
 ******************************************************************************/
#include "cli.h"
#include "ril.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * @brief   IMEI∂¡√¸¡Ó
 *          √¸¡Ó∏Ò Ω:imei
 */    
static int do_cmd_imei(struct cli_obj *cli, int argc, char *argv[])
{
    char imei[32];
    if (ril_request(RIL_REQ_GET_IMEI, imei, sizeof(imei)) == RIL_OK) {
        cli->print(cli, "The IMEI of the device is %s\r\n", imei);
    } else
        cli->print(cli, "IMEI read failed.\r\n");
    return 0;
                      
}cmd_register("imei", do_cmd_imei, "read imei");
