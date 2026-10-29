#include "xy_hal_uart.h"

#include "stm32l4xx_hal.h"

#define XY_HAL_UART_MAX_INSTANCES 8U

typedef struct {
    UART_HandleTypeDef *handle;
    xy_hal_uart_callback_t callback;
    void *callback_arg;
    /* Receive interrupt state. irq_byte is where the vendor writes the byte, pending is
       the handover the callback reads - the same split as the WCH backend, so a callback
       that wants the byte never has to touch the data register itself. */
    uint8_t irq_byte;
    uint8_t pending_byte;
    uint8_t pending_valid;
    uint8_t irq_armed;
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
    xy_hal_uart_context_t *context = uart_context_find(handle);

    /* Stash the byte before dispatching: the callback reads it through
       xy_hal_uart_rx_byte(), and re-arming here is what keeps the interrupt from firing
       once and going quiet. */
    if (context != NULL) {
        context->pending_byte = context->irq_byte;
        context->pending_valid = 1U;
    }
    uart_callback_dispatch(handle, XY_HAL_UART_EVENT_RX_DONE);
    if (context != NULL && context->irq_armed != 0U) {
        (void)HAL_UART_Receive_IT(handle, &context->irq_byte, 1U);
    }
}


/* Which NVIC line carries this UART. The answer comes back through an out-parameter
   rather than as a sentinel IRQn_Type: a magic "unknown" number inside a legal
   range is how a checked path silently stops checking. */
static int uart_irqn(const UART_HandleTypeDef *handle, IRQn_Type *irqn)
{
#ifdef USART1
    if (handle->Instance == (void *)USART1) {
        *irqn = USART1_IRQn;
        return 1;
    }
#endif
#ifdef USART2
    if (handle->Instance == (void *)USART2) {
        *irqn = USART2_IRQn;
        return 1;
    }
#endif
#ifdef USART3
    if (handle->Instance == (void *)USART3) {
        *irqn = USART3_IRQn;
        return 1;
    }
#endif
#ifdef UART4
    if (handle->Instance == (void *)UART4) {
        *irqn = UART4_IRQn;
        return 1;
    }
#endif
#ifdef UART5
    if (handle->Instance == (void *)UART5) {
        *irqn = UART5_IRQn;
        return 1;
    }
#endif
#ifdef LPUART1
    if (handle->Instance == (void *)LPUART1) {
        *irqn = LPUART1_IRQn;
        return 1;
    }
#endif
    (void)handle;
    (void)irqn;
    return 0;
}

/* Opt-in: supplying the vectors pulls HAL_UART_IRQHandler into the link, and that
 * function references the vendor DMA driver. A board that wants the HAL to own the
 * UART vector defines XY_HAL_UART_PROVIDE_IRQ_VECTORS and links stm32XX_hal_dma.c.
 * Boards that keep their own it.c vectors leave this off - the guard over board
 * interrupt tables checks that such an override forwards to the HAL. */
#if defined(XY_HAL_UART_PROVIDE_IRQ_VECTORS)
/* The vector a board can forget to write. Without one the peripheral raises
   RXNE and the NVIC line fires into nothing: the interrupt exists and the byte
   is never collected. Weak, so a board's own USARTx_IRQHandler (CubeMX writes
   one) still wins - and the board guard checks that an override forwards on. */
static void uart_vector_dispatch(void *instance)
{
    size_t index;

    for (index = 0U; index < XY_HAL_UART_MAX_INSTANCES; index++) {
        if (uart_contexts[index].handle != NULL &&
            uart_contexts[index].handle->Instance == instance) {
            HAL_UART_IRQHandler(uart_contexts[index].handle);
            return;
        }
    }
}

#ifdef USART1
__weak void USART1_IRQHandler(void)
{
    uart_vector_dispatch((void *)USART1);
}
#endif

#ifdef USART2
__weak void USART2_IRQHandler(void)
{
    uart_vector_dispatch((void *)USART2);
}
#endif

#ifdef USART3
__weak void USART3_IRQHandler(void)
{
    uart_vector_dispatch((void *)USART3);
}
#endif

#ifdef UART4
__weak void UART4_IRQHandler(void)
{
    uart_vector_dispatch((void *)UART4);
}
#endif

#ifdef UART5
__weak void UART5_IRQHandler(void)
{
    uart_vector_dispatch((void *)UART5);
}
#endif

#ifdef LPUART1
__weak void LPUART1_IRQHandler(void)
{
    uart_vector_dispatch((void *)LPUART1);
}
#endif

#endif

xy_hal_error_t xy_hal_uart_enable_rx_irq(void *uart, int enable)
{
    xy_hal_uart_context_t *context;
    IRQn_Type irqn;

    if (uart == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    context = uart_context_find(uart);
    if (context == NULL) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (!uart_irqn((UART_HandleTypeDef *)uart, &irqn)) {
        /* No NVIC line is known for this instance, so arming would succeed at the
           peripheral and never reach the CPU. Report it instead of pretending. */
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (enable == 0) {
        context->irq_armed = 0U;
        context->pending_valid = 0U;
        HAL_NVIC_DisableIRQ(irqn);
        return uart_status(HAL_UART_AbortReceive_IT((UART_HandleTypeDef *)uart));
    }

    context->pending_valid = 0U;
    context->irq_armed = 1U;
    /* NVIC before the peripheral: a byte arriving between the two must already have
       somewhere to land. */
    HAL_NVIC_SetPriority(irqn, 5U, 0U);
    HAL_NVIC_EnableIRQ(irqn);
    return uart_status(HAL_UART_Receive_IT((UART_HandleTypeDef *)uart, &context->irq_byte, 1U));
}

void xy_hal_uart_irq_handler(void *uart)
{
    /* On STM32 the vector belongs to the board's interrupt table, which normally calls
       HAL_UART_IRQHandler() itself. This entry point exists for boards that route the
       vector through the HAL instead, so the contract is the same shape as the WCH one:
       one call does the whole interrupt. */
    if (uart != NULL) {
        HAL_UART_IRQHandler((UART_HandleTypeDef *)uart);
    }
}

int xy_hal_uart_rx_byte(void *uart)
{
    xy_hal_uart_context_t *context;

    if (uart == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    context = uart_context_find(uart);
    if (context == NULL || context->pending_valid == 0U) {
        return XY_HAL_ERROR_NOT_FOUND;
    }
    context->pending_valid = 0U;
    return (int)context->pending_byte;
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *handle)
{
    uart_callback_dispatch(handle, XY_HAL_UART_EVENT_ERROR);
}
