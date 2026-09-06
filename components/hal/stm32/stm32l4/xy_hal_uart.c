#include "xy_hal_uart.h"

#include "stm32l4xx_hal.h"

#define XY_HAL_UART_MAX_INSTANCES 8U

typedef struct {
    UART_HandleTypeDef *handle;
    xy_hal_uart_callback_t callback;
    void *callback_arg;
} xy_hal_uart_context_t;

static xy_hal_uart_context_t uart_contexts[XY_HAL_UART_MAX_INSTANCES];

static xy_hal_uart_context_t *uart_context_find(void *uart)
{
    for (uint32_t index = 0U; index < XY_HAL_UART_MAX_INSTANCES; ++index) {
        if (uart_contexts[index].handle == uart) {
            return &uart_contexts[index];
        }
    }
    return NULL;
}

static xy_hal_uart_context_t *uart_context_allocate(void)
{
    for (uint32_t index = 0U; index < XY_HAL_UART_MAX_INSTANCES; ++index) {
        if (uart_contexts[index].handle == NULL) {
            return &uart_contexts[index];
        }
    }
    return NULL;
}

static uint32_t uart_word_length(xy_hal_uart_wordlen_t wordlen)
{
    switch (wordlen) {
#ifdef UART_WORDLENGTH_7B
    case XY_HAL_UART_WORDLEN_7B:
        return UART_WORDLENGTH_7B;
#endif
    case XY_HAL_UART_WORDLEN_9B:
        return UART_WORDLENGTH_9B;
    case XY_HAL_UART_WORDLEN_8B:
    default:
        return UART_WORDLENGTH_8B;
    }
}

static uint32_t uart_stop_bits(xy_hal_uart_stopbits_t stopbits)
{
    switch (stopbits) {
#ifdef UART_STOPBITS_1_5
    case XY_HAL_UART_STOPBITS_1_5:
        return UART_STOPBITS_1_5;
#endif
    case XY_HAL_UART_STOPBITS_2:
        return UART_STOPBITS_2;
    case XY_HAL_UART_STOPBITS_1:
    default:
        return UART_STOPBITS_1;
    }
}

static uint32_t uart_parity(xy_hal_uart_parity_t parity)
{
    switch (parity) {
    case XY_HAL_UART_PARITY_EVEN:
        return UART_PARITY_EVEN;
    case XY_HAL_UART_PARITY_ODD:
        return UART_PARITY_ODD;
    case XY_HAL_UART_PARITY_NONE:
    default:
        return UART_PARITY_NONE;
    }
}

static uint32_t uart_flow_control(xy_hal_uart_flowctrl_t flowctrl)
{
    switch (flowctrl) {
    case XY_HAL_UART_FLOWCTRL_RTS:
        return UART_HWCONTROL_RTS;
    case XY_HAL_UART_FLOWCTRL_CTS:
        return UART_HWCONTROL_CTS;
    case XY_HAL_UART_FLOWCTRL_RTS_CTS:
        return UART_HWCONTROL_RTS_CTS;
    case XY_HAL_UART_FLOWCTRL_NONE:
    default:
        return UART_HWCONTROL_NONE;
    }
}

static uint32_t uart_mode(xy_hal_uart_mode_t mode)
{
    switch (mode) {
    case XY_HAL_UART_MODE_TX:
        return UART_MODE_TX;
    case XY_HAL_UART_MODE_RX:
        return UART_MODE_RX;
    case XY_HAL_UART_MODE_TX_RX:
    default:
        return UART_MODE_TX_RX;
    }
}

static xy_hal_error_t uart_status(HAL_StatusTypeDef status)
{
    if (status == HAL_OK) {
        return XY_HAL_OK;
    }
    if (status == HAL_TIMEOUT) {
        return XY_HAL_ERROR_TIMEOUT;
    }
    if (status == HAL_BUSY) {
        return XY_HAL_ERROR_BUSY;
    }
    return XY_HAL_ERROR_IO;
}

xy_hal_error_t xy_hal_uart_init(void *uart, const xy_hal_uart_config_t *config)
{
    UART_HandleTypeDef *handle = uart;
    xy_hal_uart_context_t *context;

    if (handle == NULL || config == NULL || config->baudrate == 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (uart_context_find(uart) != NULL) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }
    context = uart_context_allocate();
    if (context == NULL) {
        return XY_HAL_ERROR_NO_RESOURCE;
    }

    handle->Init.BaudRate = config->baudrate;
    handle->Init.WordLength = uart_word_length(config->wordlen);
    handle->Init.StopBits = uart_stop_bits(config->stopbits);
    handle->Init.Parity = uart_parity(config->parity);
    handle->Init.Mode = uart_mode(config->mode);
    handle->Init.HwFlowCtl = uart_flow_control(config->flowctrl);
    handle->Init.OverSampling = UART_OVERSAMPLING_16;
    handle->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(handle) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    context->handle = handle;
    context->callback = NULL;
    context->callback_arg = NULL;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_deinit(void *uart)
{
    xy_hal_uart_context_t *context = uart_context_find(uart);
    if (uart == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (context == NULL) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (HAL_UART_DeInit((UART_HandleTypeDef *)uart) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
    context->handle = NULL;
    context->callback = NULL;
    context->callback_arg = NULL;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_send(void *uart, const uint8_t *data, size_t len, uint32_t timeout)
{
    if (uart == NULL || data == NULL || len == 0U || len > UINT16_MAX) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (uart_context_find(uart) == NULL) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return uart_status(HAL_UART_Transmit(uart, (uint8_t *)data, (uint16_t)len, timeout));
}

xy_hal_error_t xy_hal_uart_recv(void *uart, uint8_t *data, size_t len, uint32_t timeout)
{
    if (uart == NULL || data == NULL || len == 0U || len > UINT16_MAX) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (uart_context_find(uart) == NULL) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return uart_status(HAL_UART_Receive(uart, data, (uint16_t)len, timeout));
}

xy_hal_error_t xy_hal_uart_send_dma(void *uart, const uint8_t *data, size_t len)
{
    if (uart == NULL || data == NULL || len == 0U || len > UINT16_MAX) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (uart_context_find(uart) == NULL) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return uart_status(HAL_UART_Transmit_DMA(uart, (uint8_t *)data, (uint16_t)len));
}

xy_hal_error_t xy_hal_uart_recv_dma(void *uart, uint8_t *data, size_t len)
{
    if (uart == NULL || data == NULL || len == 0U || len > UINT16_MAX) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (uart_context_find(uart) == NULL) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return uart_status(HAL_UART_Receive_DMA(uart, data, (uint16_t)len));
}

xy_hal_error_t xy_hal_uart_register_callback(void *uart, xy_hal_uart_callback_t callback, void *arg)
{
    xy_hal_uart_context_t *context;
    if (uart == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    context = uart_context_find(uart);
    if (context == NULL) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    context->callback = callback;
    context->callback_arg = arg;
    return XY_HAL_OK;
}

int xy_hal_uart_available(void *uart)
{
    return uart_context_find(uart) == NULL ? XY_HAL_ERROR_NOT_INIT : 0;
}

xy_hal_error_t xy_hal_uart_flush(void *uart)
{
    UART_HandleTypeDef *handle = uart;
    if (uart == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (uart_context_find(uart) == NULL) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return handle->gState == HAL_UART_STATE_READY ? XY_HAL_OK : XY_HAL_ERROR_BUSY;
}

xy_hal_error_t xy_hal_uart_error(void *uart)
{
    if (uart == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (uart_context_find(uart) == NULL) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return HAL_UART_GetError((UART_HandleTypeDef *)uart) == HAL_UART_ERROR_NONE ? XY_HAL_OK
                                                                                : XY_HAL_ERROR_IO;
}

xy_hal_error_t xy_hal_uart_set_error_cb(void *uart, xy_hal_uart_callback_t callback, void *arg)
{
    return xy_hal_uart_register_callback(uart, callback, arg);
}

static void uart_callback_dispatch(UART_HandleTypeDef *handle, xy_hal_uart_event_t event)
{
    xy_hal_uart_context_t *context = uart_context_find(handle);
    if (context != NULL && context->callback != NULL) {
        context->callback(handle, event, context->callback_arg);
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *handle)
{
    uart_callback_dispatch(handle, XY_HAL_UART_EVENT_TX_DONE);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *handle)
{
    uart_callback_dispatch(handle, XY_HAL_UART_EVENT_RX_DONE);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *handle)
{
    uart_callback_dispatch(handle, XY_HAL_UART_EVENT_ERROR);
}
