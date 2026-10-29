/**
 * @file xy_hal_uart.c
 * @brief UART HAL STM32U5 Implementation
 * @version 2.0
 * @date 2026-02-28
 */

#include "../../inc/xy_hal_uart.h"

#if defined(STM32U5) || defined(STM32U5xx)

#include "stm32u5xx_hal.h"
#include <string.h>

/* UART context structure */
typedef struct {
    UART_HandleTypeDef *huart;
    xy_hal_uart_callback_t callback;
    void *arg;
    uint8_t initialized;
    /* Receive interrupt state, same names as the L4 backend so the contract guard can
       check both without a second vocabulary. */
    uint8_t irq_byte;
    uint8_t pending_byte;
    uint8_t pending_valid;
    uint8_t irq_armed;
} uart_ctx_t;

/* Maximum UART instances - adjust based on MCU */
#define MAX_UART_INSTANCES 8
static uart_ctx_t g_uart_ctx[MAX_UART_INSTANCES];

/**
 * @brief Find UART context by handle
 */
static uart_ctx_t *find_uart_ctx(void *uart)
{
    for (size_t i = 0; i < MAX_UART_INSTANCES; i++) {
        if (g_uart_ctx[i].huart == uart) {
            return &g_uart_ctx[i];
        }
    }
    return NULL;
}

/**
 * @brief Allocate new UART context
 */
static uart_ctx_t *alloc_uart_ctx(void)
{
    for (size_t i = 0; i < MAX_UART_INSTANCES; i++) {
        if (g_uart_ctx[i].huart == NULL) {
            return &g_uart_ctx[i];
        }
    }
    return NULL;
}

/**
 * @brief Convert XY UART word length to STM32
 */
static uint32_t xy_to_stm32_wordlen(xy_hal_uart_wordlen_t wordlen)
{
    switch (wordlen) {
#ifdef UART_WORDLENGTH_7B
    case XY_HAL_UART_WORDLEN_7B:
        return UART_WORDLENGTH_7B;
#endif
    case XY_HAL_UART_WORDLEN_8B:
        return UART_WORDLENGTH_8B;
    case XY_HAL_UART_WORDLEN_9B:
        return UART_WORDLENGTH_9B;
    default:
        return UART_WORDLENGTH_8B;
    }
}

/**
 * @brief Convert XY UART stop bits to STM32
 */
static uint32_t xy_to_stm32_stopbits(xy_hal_uart_stopbits_t stopbits)
{
    switch (stopbits) {
    case XY_HAL_UART_STOPBITS_1:
        return UART_STOPBITS_1;
#ifdef UART_STOPBITS_1_5
    case XY_HAL_UART_STOPBITS_1_5:
        return UART_STOPBITS_1_5;
#endif
    case XY_HAL_UART_STOPBITS_2:
        return UART_STOPBITS_2;
    default:
        return UART_STOPBITS_1;
    }
}

/**
 * @brief Convert XY UART parity to STM32
 */
static uint32_t xy_to_stm32_parity(xy_hal_uart_parity_t parity)
{
    switch (parity) {
    case XY_HAL_UART_PARITY_NONE:
        return UART_PARITY_NONE;
    case XY_HAL_UART_PARITY_EVEN:
        return UART_PARITY_EVEN;
    case XY_HAL_UART_PARITY_ODD:
        return UART_PARITY_ODD;
    default:
        return UART_PARITY_NONE;
    }
}

/**
 * @brief Convert XY UART flow control to STM32
 */
static uint32_t xy_to_stm32_flowctrl(xy_hal_uart_flowctrl_t flowctrl)
{
    switch (flowctrl) {
    case XY_HAL_UART_FLOWCTRL_NONE:
        return UART_HWCONTROL_NONE;
    case XY_HAL_UART_FLOWCTRL_RTS:
        return UART_HWCONTROL_RTS;
    case XY_HAL_UART_FLOWCTRL_CTS:
        return UART_HWCONTROL_CTS;
    case XY_HAL_UART_FLOWCTRL_RTS_CTS:
        return UART_HWCONTROL_RTS_CTS;
    default:
        return UART_HWCONTROL_NONE;
    }
}

/**
 * @brief Convert XY UART mode to STM32
 */
static uint32_t xy_to_stm32_mode(xy_hal_uart_mode_t mode)
{
    switch (mode) {
    case XY_HAL_UART_MODE_TX:
        return UART_MODE_TX;
    case XY_HAL_UART_MODE_RX:
        return UART_MODE_RX;
    case XY_HAL_UART_MODE_TX_RX:
        return UART_MODE_TX_RX;
    default:
        return UART_MODE_TX_RX;
    }
}

xy_hal_error_t xy_hal_uart_init(void *uart, const xy_hal_uart_config_t *config)
{
    if (!uart || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)uart;

    /* Check if already initialized */
    uart_ctx_t *ctx = find_uart_ctx(uart);
    if (ctx != NULL && ctx->initialized) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }

    /* Allocate context if not exists */
    if (ctx == NULL) {
        ctx = alloc_uart_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
    }

    huart->Init.BaudRate   = config->baudrate;
    huart->Init.WordLength = xy_to_stm32_wordlen(config->wordlen);
    huart->Init.StopBits   = xy_to_stm32_stopbits(config->stopbits);
    huart->Init.Parity     = xy_to_stm32_parity(config->parity);
    huart->Init.Mode       = xy_to_stm32_mode(config->mode);
    huart->Init.HwFlowCtl  = xy_to_stm32_flowctrl(config->flowctrl);
    huart->Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(huart) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    /* Initialize context */
    ctx->huart      = huart;
    ctx->callback   = NULL;
    ctx->arg        = NULL;
    ctx->initialized = 1;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_deinit(void *uart)
{
    if (!uart) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    uart_ctx_t *ctx = find_uart_ctx(uart);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_UART_DeInit((UART_HandleTypeDef *)uart) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    ctx->initialized = 0;
    ctx->huart       = NULL;
    ctx->callback    = NULL;
    ctx->arg         = NULL;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_send(void *uart, const uint8_t *data, size_t len,
                     uint32_t timeout)
{
    if (!uart || !data || len == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    uart_ctx_t *ctx = find_uart_ctx(uart);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    HAL_StatusTypeDef status = HAL_UART_Transmit(
        (UART_HandleTypeDef *)uart, (uint8_t *)data, (uint16_t)len, timeout);

    if (status != HAL_OK) {
        if (status == HAL_TIMEOUT) {
            return XY_HAL_ERROR_TIMEOUT;
        }
        return XY_HAL_ERROR_IO;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_recv(void *uart, uint8_t *data, size_t len, uint32_t timeout)
{
    if (!uart || !data || len == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    uart_ctx_t *ctx = find_uart_ctx(uart);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    HAL_StatusTypeDef status = HAL_UART_Receive(
        (UART_HandleTypeDef *)uart, data, (uint16_t)len, timeout);

    if (status != HAL_OK) {
        if (status == HAL_TIMEOUT) {
            return XY_HAL_ERROR_TIMEOUT;
        }
        return XY_HAL_ERROR_IO;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_send_dma(void *uart, const uint8_t *data,
                                    size_t len)
{
    if (!uart || !data || len == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    uart_ctx_t *ctx = find_uart_ctx(uart);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    HAL_StatusTypeDef status = HAL_UART_Transmit_DMA(
        (UART_HandleTypeDef *)uart, (uint8_t *)data, (uint16_t)len);

    if (status != HAL_OK) {
        return XY_HAL_ERROR_BUSY;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_recv_dma(void *uart, uint8_t *data, size_t len)
{
    if (!uart || !data || len == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    uart_ctx_t *ctx = find_uart_ctx(uart);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    HAL_StatusTypeDef status = HAL_UART_Receive_DMA(
        (UART_HandleTypeDef *)uart, data, (uint16_t)len);

    if (status != HAL_OK) {
        return XY_HAL_ERROR_BUSY;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_register_callback(void *uart,
                                             xy_hal_uart_callback_t callback,
                                             void *arg)
{
    if (!uart) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    uart_ctx_t *ctx = find_uart_ctx(uart);
    if (ctx == NULL) {
        ctx = alloc_uart_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
        ctx->huart = (UART_HandleTypeDef *)uart;
    }

    ctx->callback = callback;
    ctx->arg      = arg;

    return XY_HAL_OK;
}

int xy_hal_uart_available(void *uart)
{
    if (!uart) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)uart;

    /* Return number of bytes in RX buffer */
    if (huart->RxState == HAL_UART_STATE_BUSY_RX) {
        return (int)(huart->RxXferSize - huart->RxXferCount);
    }
    return 0;
}

xy_hal_error_t xy_hal_uart_flush(void *uart)
{
    if (!uart) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)uart;

    /* Wait for transmission to complete */
    uint32_t timeout = HAL_GetTick() + 1000;
    while (huart->gState != HAL_UART_STATE_READY) {
        if (HAL_GetTick() >= timeout) {
            return XY_HAL_ERROR_TIMEOUT;
        }
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_error(void *uart)
{
    if (!uart) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    /* Placeholder for UART error pin control if needed */
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_set_error_cb(void *uart,
                                        xy_hal_uart_callback_t callback,
                                        void *arg)
{
    /* Use same callback as main callback for simplicity */
    return xy_hal_uart_register_callback(uart, callback, arg);
}

/* ==================== HAL Callbacks ==================== */

/**
 * @brief UART TX complete callback
 *
 * The half-complete callbacks used to report TX_DONE and RX_DONE here. For a one byte
 * receive the half transfer never happens at all, so RX_DONE could never fire - the
 * backend looked complete and carried nothing. Completion is reported from the completion
 * callbacks, and the half callbacks are left to the vendor's weak no-ops rather than
 * dressing them up as something they are not.
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    uart_ctx_t *ctx = find_uart_ctx(huart);
    if (ctx && ctx->callback) {
        ctx->callback(huart, XY_HAL_UART_EVENT_TX_DONE, ctx->arg);
    }
}

/**
 * @brief UART RX complete callback
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    uart_ctx_t *ctx = find_uart_ctx(huart);

    if (ctx != NULL) {
        ctx->pending_byte = ctx->irq_byte;
        ctx->pending_valid = 1U;
    }
    if (ctx && ctx->callback) {
        ctx->callback(huart, XY_HAL_UART_EVENT_RX_DONE, ctx->arg);
    }
    if (ctx != NULL && ctx->irq_armed != 0U) {
        (void)HAL_UART_Receive_IT(huart, &ctx->irq_byte, 1U);
    }
}

static xy_hal_error_t uart_it_status(HAL_StatusTypeDef status)
{
    if (status == HAL_OK) {
        return XY_HAL_OK;
    }
    if (status == HAL_BUSY) {
        return XY_HAL_ERROR_BUSY;
    }
    return XY_HAL_ERROR_IO;
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

    for (index = 0U; index < MAX_UART_INSTANCES; index++) {
        if (g_uart_ctx[index].huart != NULL &&
            g_uart_ctx[index].huart->Instance == instance) {
            HAL_UART_IRQHandler(g_uart_ctx[index].huart);
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
    uart_ctx_t *ctx;
    IRQn_Type irqn;

    if (uart == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = find_uart_ctx(uart);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (!uart_irqn((UART_HandleTypeDef *)uart, &irqn)) {
        /* No NVIC line is known for this instance, so arming would succeed at the
           peripheral and never reach the CPU. Report it instead of pretending. */
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (enable == 0) {
        ctx->irq_armed = 0U;
        ctx->pending_valid = 0U;
        HAL_NVIC_DisableIRQ(irqn);
        return uart_it_status(HAL_UART_AbortReceive_IT((UART_HandleTypeDef *)uart));
    }

    ctx->pending_valid = 0U;
    ctx->irq_armed = 1U;
    /* NVIC before the peripheral: a byte arriving between the two must already have
       somewhere to land. */
    HAL_NVIC_SetPriority(irqn, 5U, 0U);
    HAL_NVIC_EnableIRQ(irqn);
    return uart_it_status(HAL_UART_Receive_IT((UART_HandleTypeDef *)uart, &ctx->irq_byte, 1U));
}

void xy_hal_uart_irq_handler(void *uart)
{
    if (uart != NULL) {
        HAL_UART_IRQHandler((UART_HandleTypeDef *)uart);
    }
}

int xy_hal_uart_rx_byte(void *uart)
{
    uart_ctx_t *ctx;

    if (uart == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = find_uart_ctx(uart);
    if (ctx == NULL || ctx->pending_valid == 0U) {
        return XY_HAL_ERROR_NOT_FOUND;
    }
    ctx->pending_valid = 0U;
    return (int)ctx->pending_byte;
}

/**
 * @brief UART error callback
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    uart_ctx_t *ctx = find_uart_ctx(huart);
    if (ctx && ctx->callback) {
        ctx->callback(huart, XY_HAL_UART_EVENT_ERROR, ctx->arg);
    }
}

#endif /* STM32U5 || STM32U5xx */
