#ifndef __USB2P_CFG_H__
#define __USB2P_CFG_H__

#define USE_USB2PIN
#define USE_USB2UART
#define USE_USB2I2C
#define USE_USB2SPI
#define USE_USB2PWM
#define USE_USB2ADC
#define USE_USB2DAC
#define USE_USB2CAN
#define USE_USB21WIRE
#define USE_USB2LINK

#define USB2P_RTOS_NONE     0
#define USB2P_RTOS_FREERTOS 1
#define USB2P_RTOS_RTTHREAD 2
#define USB2P_RTOS_UCOS     3
#define USB2P_RTOS_THREADX  4
#define USB2P_RTOS_CMSIS    5
#define USB2P_RTOS_CUSTOM   6

#define USB2P_RTOS USB2P_RTOS_NONE /**< select rtos type. */

#define USB2P_MAX_PORTS    8  /**< max ports number. */
#define USB2P_MAX_PKG_SIZE 64 /**< max pkg size, include header. */

#if USB2P_RTOS == USB2P_RTOS_NONE
#include <stdlib.h>
#define usb2p_mem_malloc(size) malloc(size)
#define usb2p_mem_free(ptr)    free(ptr)
#elif USB2P_RTOS == USB2P_RTOS_FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#define usb2p_mem_malloc(size) pvPortMalloc(size)
#define usb2p_mem_free(ptr)    vPortFree(ptr)
#elif USB2P_RTOS == USB2P_RTOS_RTTHREAD
#include <rtthread.h>
#define usb2p_mem_malloc(size) rt_malloc(size)
#define usb2p_mem_free(ptr)    rt_free(ptr)
#elif USB2P_RTOS == USB2P_RTOS_UCOS
#include <ucos_ii.h>

#elif USB2P_RTOS == USB2P_RTOS_THREADX
#include "tx_api.h"

#elif USB2P_RTOS == USB2P_RTOS_CMSIS
#include "cmsis_os2.h"

#elif USB2P_RTOS == USB2P_RTOS_CUSTOM
#include "your_rtos.h"
#define usb2p_mem_malloc(size) your_rtos_malloc(size)
#define usb2p_mem_free(ptr)    your_rtos_free(ptr)
#endif /* USB2P_RTOS */

#define usb2p_printf printf

#endif /* __USB2P_CFG_H__ */