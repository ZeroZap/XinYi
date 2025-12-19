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

#include "module_uart.h"
#include "ringbuffer.h"
#include "public.h"
#include <string.h>

#if (MODULE_UART_RXBUF_SIZE & (MODULE_UART_RXBUF_SIZE - 1)) != 0 
    #error "MODULE_UART_RXBUF_SIZE must be power of 2!"
#endif

#if (MODULE_UART_TXBUF_SIZE & (MODULE_UART_TXBUF_SIZE - 1)) != 0 
    #error "MODULE_UART_RXBUF_SIZE must be power of 2!"
#endif

static unsigned char rxbuf[MODULE_UART_RXBUF_SIZE];   /*接收缓冲区 ------------*/
static unsigned char txbuf[MODULE_UART_TXBUF_SIZE];   /*发送缓冲区 ------------*/

static ring_buf_t rbsend, rbrecv;                     /*收发缓冲区管理 --------*/

/**
 * @brief	    模组通信串口初始化
 * @param[in]   baudrate - 通信波特率
 * @return 	    none
 */
void module_uart_init(int baudrate)
{
    ring_buf_init(&rbsend, txbuf, sizeof(txbuf));    /*初始化环形缓冲区 */
    ring_buf_init(&rbrecv, rxbuf, sizeof(rxbuf)); 

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA , ENABLE);
    
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);
    
    gpio_conf(GPIOA, GPIO_Mode_AF, GPIO_PuPd_NOPULL, GPIO_Pin_2 | GPIO_Pin_3);
    
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    
    uart_conf(USART2, baudrate);                    /*串口配置*/
    
    nvic_conf(USART2_IRQn, 1, 1);
}

/**
 * @brief	    向串口发送缓冲区内写入数据并启动发送
 * @param[in]   buf       -  数据缓存
 * @param[in]   len       -  数据长度
 * @return 	    实际写入长度(如果此时缓冲区满,则返回len)
 */
unsigned int module_uart_write(const void *buf, unsigned int len)
{   
    unsigned int ret;
    ret = ring_buf_put(&rbsend, (unsigned char *)buf, len);  
    USART_ITConfig(USART2, USART_IT_TXE, ENABLE);    
    return ret; 
}

/**
 * @brief	    读取串口接收缓冲区的数据
 * @param[in]   buf       -  数据缓存
 * @param[in]   len       -  数据长度
 * @return 	    (实际读取长度)如果接收缓冲区的有效数据大于len则返回len否则返回缓冲
 *              区有效数据的长度
 */
unsigned int module_uart_read(void *buf, unsigned int len)
{
    return ring_buf_get(&rbrecv, (unsigned char *)buf, len);
}


/**
 * @brief	    module清空接收缓冲区
 * @param[in]   none
 * @return 	    none
 */
void module_uart_clear(void)
{
    ring_buf_clr(&rbrecv);
}

/**
 * @brief	    串口2收发中断
 * @param[in]   none
 * @return 	    none
 */
void USART2_IRQHandler(void)
{    
    unsigned char data;
    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {   //接收处理
        data = USART_ReceiveData(USART2);
        ring_buf_put(&rbrecv, &data, 1);          
    }
    if (USART_GetITStatus(USART2, USART_IT_TXE) != RESET) {     //发送处理
        if (ring_buf_get(&rbsend, &data, 1))     
            USART_SendData(USART2, data);
        else {
            USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
        }
    }
    /*异常处理 ---------------------------------------------------------------*/
    if (USART_GetITStatus(USART2, USART_IT_FE) != RESET)
        data = USART_ReceiveData(USART2);
    else if (USART_GetITStatus(USART2, USART_IT_NE) != RESET)
        data = USART_ReceiveData(USART2);
    else if (USART_GetITStatus(USART2, USART_IT_ORE_RX) != RESET)
        data = USART_ReceiveData(USART2);    
    
}
