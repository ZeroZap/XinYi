/**
 * @file xy_hal_uart_pc.c
 * @brief PC/Linux simulation layer for UART HAL
 */

#include "xy_hal_uart.h"
#include <stdio.h>
#include <string.h>

int xy_hal_uart_send(void *huart, const uint8_t *data, size_t len, uint32_t timeout)
{
    (void)huart;
    (void)timeout;
    
    // Simulation: print to stdout
    if (data && len > 0) {
        fwrite(data, 1, len, stdout);
        fflush(stdout);
    }
    return (int)len;
}

int xy_hal_uart_recv(void *huart, uint8_t *data, size_t len, uint32_t timeout)
{
    (void)huart;
    (void)timeout;
    
    // Simulation: no data available
    if (data && len > 0) {
        memset(data, 0, len);
    }
    return (int)len;
}
