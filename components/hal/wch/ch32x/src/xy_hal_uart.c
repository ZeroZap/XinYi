/**
 * @file xy_hal_uart.c
 * @brief WCH CH32V30x UART HAL implementation
 */

#include "xy_hal_uart.h"

#ifdef MCU_CH32

#include "ch32v30x.h"
#include "xy_hal_tick_internal.h"

static uint8_t uart_initialized[8];
static xy_hal_uart_callback_t error_callbacks[8];
static void* error_callback_args[8];

static uint32_t uart_tick_ms(void) {
    return xy_hal_tick_ms_internal();
}

static int uart_valid(void* instance) {
    return instance == USART1 || instance == USART2 || instance == USART3 || instance == UART4 ||
           instance == UART5 || instance == UART6 || instance == UART7 || instance == UART8;
}

static int uart_index(void* instance) {
    if (instance == USART1)
        return 0;
    if (instance == USART2)
        return 1;
    if (instance == USART3)
        return 2;
    if (instance == UART4)
        return 3;
    if (instance == UART5)
        return 4;
    if (instance == UART6)
        return 5;
    if (instance == UART7)
        return 6;
    if (instance == UART8)
        return 7;
    return -1;
}

static void uart_enable_clock(void* instance) {
    if (instance == USART1)
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    else if (instance == USART2)
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    else if (instance == USART3)
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
    else if (instance == UART4)
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);
    else if (instance == UART5)
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5, ENABLE);
    else if (instance == UART6)
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART6, ENABLE);
    else if (instance == UART7)
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART7, ENABLE);
    else if (instance == UART8)
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART8, ENABLE);
}

static uint16_t uart_word_length(xy_hal_uart_wordlen_t value) {
    return value == XY_HAL_UART_WORDLEN_9B ? USART_WordLength_9b : USART_WordLength_8b;
}

static uint16_t uart_stop_bits(xy_hal_uart_stopbits_t value) {
    if (value == XY_HAL_UART_STOPBITS_2)
        return USART_StopBits_2;
    if (value == XY_HAL_UART_STOPBITS_1_5)
        return USART_StopBits_1_5;
    return USART_StopBits_1;
}

static uint16_t uart_parity(xy_hal_uart_parity_t value) {
    if (value == XY_HAL_UART_PARITY_EVEN)
        return USART_Parity_Even;
    if (value == XY_HAL_UART_PARITY_ODD)
        return USART_Parity_Odd;
    return USART_Parity_No;
}

static uint16_t uart_flow(xy_hal_uart_flowctrl_t value) {
    if (value == XY_HAL_UART_FLOWCTRL_RTS)
        return USART_HardwareFlowControl_RTS;
    if (value == XY_HAL_UART_FLOWCTRL_CTS)
        return USART_HardwareFlowControl_CTS;
    if (value == XY_HAL_UART_FLOWCTRL_RTS_CTS)
        return USART_HardwareFlowControl_RTS_CTS;
    return USART_HardwareFlowControl_None;
}

static uint16_t uart_mode(xy_hal_uart_mode_t value) {
    uint16_t mode = 0U;
    if ((value & XY_HAL_UART_MODE_TX) != 0U)
        mode |= USART_Mode_Tx;
    if ((value & XY_HAL_UART_MODE_RX) != 0U)
        mode |= USART_Mode_Rx;
    return mode;
}

xy_hal_error_t xy_hal_uart_init(void* instance, const xy_hal_uart_config_t* config) {
    USART_InitTypeDef init = {0};
    int index;
    if (!uart_valid(instance) || config == NULL || config->baudrate == 0U ||
        config->wordlen == XY_HAL_UART_WORDLEN_7B || config->wordlen > XY_HAL_UART_WORDLEN_9B ||
        config->stopbits > XY_HAL_UART_STOPBITS_2 || config->parity > XY_HAL_UART_PARITY_ODD ||
        config->flowctrl > XY_HAL_UART_FLOWCTRL_RTS_CTS || uart_mode(config->mode) == 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    index = uart_index(instance);
    if (uart_initialized[index] != 0U)
        return XY_HAL_ERROR_ALREADY_INIT;
    uart_enable_clock(instance);
    init.USART_BaudRate = config->baudrate;
    init.USART_WordLength = uart_word_length(config->wordlen);
    init.USART_StopBits = uart_stop_bits(config->stopbits);
    init.USART_Parity = uart_parity(config->parity);
    init.USART_HardwareFlowControl = uart_flow(config->flowctrl);
    init.USART_Mode = uart_mode(config->mode);
    USART_Init(instance, &init);
    USART_Cmd(instance, ENABLE);
    uart_initialized[index] = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_deinit(void* instance) {
    int index = uart_index(instance);
    if (index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (uart_initialized[index] == 0U)
        return XY_HAL_ERROR_NOT_INIT;
    USART_Cmd(instance, DISABLE);
    USART_DeInit(instance);
    uart_initialized[index] = 0U;
    error_callbacks[index] = NULL;
    error_callback_args[index] = NULL;
    return XY_HAL_OK;
}

static xy_hal_error_t uart_wait(void* instance, uint16_t flag, uint32_t timeout) {
    uint32_t start = uart_tick_ms();
    while (USART_GetFlagStatus(instance, flag) == RESET) {
        if (timeout != 0U && (uart_tick_ms() - start) >= timeout)
            return XY_HAL_ERROR_TIMEOUT;
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_send(void* instance, const uint8_t* data, size_t len, uint32_t timeout) {
    size_t i;
    xy_hal_error_t result;
    int index;
    if (!uart_valid(instance) || (len != 0U && data == NULL))
        return XY_HAL_ERROR_INVALID_PARAM;
    index = uart_index(instance);
    if (uart_initialized[index] == 0U)
        return XY_HAL_ERROR_NOT_INIT;
    for (i = 0U; i < len; ++i) {
        result = uart_wait(instance, USART_FLAG_TXE, timeout);
        if (result != XY_HAL_OK)
            return result;
        USART_SendData(instance, data[i]);
    }
    return len == 0U ? XY_HAL_OK : uart_wait(instance, USART_FLAG_TC, timeout);
}

xy_hal_error_t xy_hal_uart_recv(void* instance, uint8_t* data, size_t len, uint32_t timeout) {
    size_t i;
    xy_hal_error_t result;
    int index;
    if (!uart_valid(instance) || (len != 0U && data == NULL))
        return XY_HAL_ERROR_INVALID_PARAM;
    index = uart_index(instance);
    if (uart_initialized[index] == 0U)
        return XY_HAL_ERROR_NOT_INIT;
    for (i = 0U; i < len; ++i) {
        result = uart_wait(instance, USART_FLAG_RXNE, timeout);
        if (result != XY_HAL_OK)
            return result;
        data[i] = (uint8_t)USART_ReceiveData(instance);
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_send_dma(void* instance, const uint8_t* data, size_t len) {
    (void)instance;
    (void)data;
    (void)len;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
xy_hal_error_t xy_hal_uart_recv_dma(void* instance, uint8_t* data, size_t len) {
    (void)instance;
    (void)data;
    (void)len;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
xy_hal_error_t xy_hal_uart_register_callback(void* instance, xy_hal_uart_callback_t callback,
                                             void* arg) {
    int index = uart_index(instance);
    if (index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (uart_initialized[index] == 0U)
        return XY_HAL_ERROR_NOT_INIT;
    error_callbacks[index] = callback;
    error_callback_args[index] = arg;
    return XY_HAL_OK;
}
int xy_hal_uart_available(void* instance) {
    int index = uart_index(instance);
    if (index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (uart_initialized[index] == 0U)
        return XY_HAL_ERROR_NOT_INIT;
    return USART_GetFlagStatus(instance, USART_FLAG_RXNE) == SET ? 1 : 0;
}
xy_hal_error_t xy_hal_uart_flush(void* instance) {
    int index = uart_index(instance);
    if (index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (uart_initialized[index] == 0U)
        return XY_HAL_ERROR_NOT_INIT;
    while (USART_GetFlagStatus(instance, USART_FLAG_RXNE) == SET)
        (void)USART_ReceiveData(instance);
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_uart_error(void* instance) {
    int index = uart_index(instance);
    if (index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (uart_initialized[index] == 0U)
        return XY_HAL_ERROR_NOT_INIT;
    if (error_callbacks[index] != NULL)
        error_callbacks[index](instance, XY_HAL_UART_EVENT_ERROR, error_callback_args[index]);
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_uart_set_error_cb(void* instance, xy_hal_uart_callback_t callback,
                                        void* arg) {
    return xy_hal_uart_register_callback(instance, callback, arg);
}

#else
#error "WCH UART backend requires MCU_CH32"
#endif
