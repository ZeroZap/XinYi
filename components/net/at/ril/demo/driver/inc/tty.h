/******************************************************************************
 * @brief    tty串口打印驱动
 *
 * Copyright (c) 2016 <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2016-05-17     Morro        Initial version
 ******************************************************************************/

#ifndef	_TTY_H_
#define	_TTY_H_

#include <stdbool.h>

#define TTY_RXBUF_SIZE		 256
#define TTY_TXBUF_SIZE		 2048

/*接口声明 --------------------------------------------------------------------*/
typedef struct {
    void (*init)(int baudrate);                                   
    unsigned int (*write)(const void *buf, unsigned int len);    
    unsigned int (*read)(void *buf, unsigned int len);           
    bool (*tx_isfull)(void);                                    /*发送缓冲区满*/
    bool (*rx_isempty)(void);                                   /*接收缓冲区空*/
}tty_t;

extern const tty_t tty;

#endif
