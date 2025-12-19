/******************************************************************************
 * @brief    模组串口通信
 *
 * Copyright (c) 2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2020-09-21     Morro        Initial version. 
 ******************************************************************************/

#ifndef	_MODULE_UART_H_
#define	_MODULE_UART_H_

#include <stdio.h>
#include <stdbool.h>

/*收发缓冲区定义 -------------------------------------------------------------*/
#define      MODULE_UART_RXBUF_SIZE		 1024
#define      MODULE_UART_TXBUF_SIZE		 2048

void module_uart_init(int baudrate);
unsigned int module_uart_read(void *buf, unsigned int len);
unsigned int module_uart_write(const void *buf, unsigned int len);
void         module_uart_clear(void);

#endif	
