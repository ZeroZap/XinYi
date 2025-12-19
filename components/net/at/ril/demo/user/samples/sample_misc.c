/******************************************************************************
 * @brief    RIL  ≤‚ ‘≥Ã–Ú
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

 
static int do_cmd_poweron(struct cli_obj *cli, int argc, char *argv[])
{
    ril_open();
    cli->print(cli, "open\r\n");
    return 0;
                      
}cmd_register("pwr-on", do_cmd_poweron, "power on");



static int do_cmd_poweroff(struct cli_obj *cli, int argc, char *argv[])
{
    ril_close();
    cli->print(cli, "close\r\n");
    return 0;
                      
}cmd_register("pwr-off", do_cmd_poweroff, "power off");



static int do_cmd_pdp_active(struct cli_obj *cli, int argc, char *argv[])
{
    ril_netconn(true);
    cli->print(cli, "pdp active\r\n");
    return 0;
                      
}cmd_register("pdp-active", do_cmd_pdp_active, "pdp active");


static int do_cmd_pdp_deactive(struct cli_obj *cli, int argc, char *argv[])
{
    ril_netconn(false);
    cli->print(cli, "pdp deactive\r\n");
    return 0;
                      
}cmd_register("pdp-deactive", do_cmd_pdp_deactive, "pdp deactive");