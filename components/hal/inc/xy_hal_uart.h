/**
 * @file xy_hal_uart.h
 * @brief UART Hardware Abstraction Layer
 * @version 2.0
 * @date 2025-10-26
 */

#ifndef XY_HAL_UART_H
#define XY_HAL_UART_H
#include "xy_hal_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_hal.h"
#include <stddef.h>
#include <stdint.h>

/* UART Word Length */
typedef enum {
  XY_HAL_UART_WORDLEN_7B = 0, /**< 7-bit word length */
  XY_HAL_UART_WORDLEN_8B,     /**< 8-bit word length */
  XY_HAL_UART_WORDLEN_9B,     /**< 9-bit word length */
} xy_hal_uart_wordlen_t;

/* UART Stop Bits */
typedef enum {
  XY_HAL_UART_STOPBITS_1 = 0, /**< 1 stop bit */
  XY_HAL_UART_STOPBITS_1_5,   /**< 1.5 stop bits */
  XY_HAL_UART_STOPBITS_2,     /**< 2 stop bits */
} xy_hal_uart_stopbits_t;

/* UART Parity */
typedef enum {
  XY_HAL_UART_PARITY_NONE = 0, /**< No parity */
  XY_HAL_UART_PARITY_EVEN,     /**< Even parity */
  XY_HAL_UART_PARITY_ODD,      /**< Odd parity */
} xy_hal_uart_parity_t;

/* UART Flow Control */
typedef enum {
  XY_HAL_UART_FLOWCTRL_NONE = 0, /**< No flow control */
  XY_HAL_UART_FLOWCTRL_RTS,      /**< RTS flow control */
  XY_HAL_UART_FLOWCTRL_CTS,      /**< CTS flow control */
  XY_HAL_UART_FLOWCTRL_RTS_CTS,  /**< RTS/CTS flow control */
} xy_hal_uart_flowctrl_t;

/* UART Mode */
typedef enum {
  XY_HAL_UART_MODE_TX = 0x01,    /**< TX only */
  XY_HAL_UART_MODE_RX = 0x02,    /**< RX only */
  XY_HAL_UART_MODE_TX_RX = 0x03, /**< TX and RX */
} xy_hal_uart_mode_t;

/* UART Configuration Structure */
typedef struct {
  uint32_t baudrate;               /**< Baudrate (e.g., 115200) */
  xy_hal_uart_wordlen_t wordlen;   /**< Word length */
  xy_hal_uart_stopbits_t stopbits; /**< Stop bits */
  xy_hal_uart_parity_t parity;     /**< Parity */
  xy_hal_uart_flowctrl_t flowctrl; /**< Flow control */
  xy_hal_uart_mode_t mode;         /**< UART mode */
} xy_hal_uart_config_t;

/* UART Event Types */
typedef enum {
  XY_HAL_UART_EVENT_RX_DONE = 0, /**< RX complete */
  XY_HAL_UART_EVENT_TX_DONE,     /**< TX complete */
  XY_HAL_UART_EVENT_ERROR,       /**< Error occurred */
} xy_hal_uart_event_t;

/* UART callback */
typedef void (*xy_hal_uart_callback_t)(void *uart, xy_hal_uart_event_t event,
                                       void *arg);

/**
 * @brief Initialize UART
 * @param uart UART instance
 * @param config UART configuration
 * @return XY_HAL_OK on success, XY_HAL_ERROR_INVALID_PARAM if parameters
 * invalid, XY_HAL_ERROR_FAIL if initialization failed
 */
xy_hal_error_t xy_hal_uart_init(void *uart, const xy_hal_uart_config_t *config);

/**
 * @brief Deinitialize UART
 * @param uart UART instance
 * @return XY_HAL_OK on success, XY_HAL_ERROR_INVALID_PARAM if parameters
 * invalid
 */
xy_hal_error_t xy_hal_uart_deinit(void *uart);

/**
 * @brief Send data via UART (blocking)
 *
 * @param timeout is the budget for the whole transfer, exactly as for xy_hal_uart_recv.
 *        Zero means wait indefinitely. len must be non-zero and no larger than UINT16_MAX;
 *        both are rejected as XY_HAL_ERROR_INVALID_PARAM on every backend.
 * @param uart UART instance
 * @param data Data buffer
 * @param len Data length
 * @param timeout Timeout in milliseconds
 * @return XY_HAL_OK on success, XY_HAL_ERROR_* on error
 */
xy_hal_error_t xy_hal_uart_send(void *uart, const uint8_t *data, size_t len,
                     uint32_t timeout);

/**
 * @brief Receive data via UART (blocking)
 *
 * @param timeout is the budget for the whole transfer, not per byte. Receiving 4 bytes with
 *        a 10 ms timeout gives up after about 10 ms, not 40 ms - a backend that waits per
 *        byte makes the same call mean different things on different silicon.
 *        Zero means wait indefinitely.
 * @param len must be non-zero: a zero-length receive is a caller mistake, not a success, and
 *        is rejected as XY_HAL_ERROR_INVALID_PARAM on every backend.
 *
 * @param uart UART instance
 * @param data Data buffer
 * @param len Buffer length
 * @return XY_HAL_OK on success, XY_HAL_ERROR_TIMEOUT if the budget ran out,
 *         XY_HAL_ERROR_INVALID_PARAM / XY_HAL_ERROR_NOT_INIT on bad input
 */
xy_hal_error_t xy_hal_uart_recv(void *uart, uint8_t *data, size_t len, uint32_t timeout);

/**
 * @brief Send data via UART (non-blocking with DMA)
 * @param uart UART instance
 * @param data Data buffer
 * @param len Data length
 * @return XY_HAL_OK on success, XY_HAL_ERROR_* on error
 */
xy_hal_error_t xy_hal_uart_send_dma(void *uart, const uint8_t *data,
                                    size_t len);

/**
 * @brief Receive data via UART (non-blocking with DMA)
 * @param uart UART instance
 * @param data Data buffer
 * @param len Buffer length
 * @return XY_HAL_OK on success, XY_HAL_ERROR_* on error
 */
xy_hal_error_t xy_hal_uart_recv_dma(void *uart, uint8_t *data, size_t len);

/**
 * @brief Register UART callback
 * @param uart UART instance
 * @param callback Callback function
 * @param arg User argument
 * @return XY_HAL_OK on success, XY_HAL_ERROR_INVALID_PARAM if parameters
 * invalid
 */
xy_hal_error_t xy_hal_uart_register_callback(void *uart,
                                             xy_hal_uart_callback_t callback,
                                             void *arg);

/**
 * @brief Enable or disable the receive interrupt for a UART
 *
 * With the receive interrupt on, the platform feeds incoming bytes to the
 * registered callback as XY_HAL_UART_EVENT_RX_DONE. The interrupt line is owned by
 * the HAL, exactly as xy_hal_timer_enable_irq() owns the timer's: a caller that asks
 * for the interrupt must not have to program the NVIC itself.
 *
 * @param uart UART instance
 * @param enable Non-zero to enable, zero to disable
 * @return XY_HAL_OK on success, XY_HAL_ERROR_NOT_SUPPORTED where the platform has no
 *         receive interrupt path yet
 */
xy_hal_error_t xy_hal_uart_enable_rx_irq(void *uart, int enable);

/**
 * @brief Take the byte the last receive interrupt delivered
 *
 * xy_hal_uart_irq_handler() consumes the data register itself - leaving RXNE set
 * would re-enter the interrupt forever - so the byte is handed over here instead of
 * being read a second time by the callback. Valid inside the
 * XY_HAL_UART_EVENT_RX_DONE callback; consumes the stashed byte.
 *
 * @param uart UART instance
 * @return The byte (0..255), or XY_HAL_ERROR_NOT_FOUND when there is none pending
 */
int xy_hal_uart_rx_byte(void *uart);

/**
 * @brief Deliver a pending UART interrupt to the HAL
 *
 * Platform interrupt handlers call this with the instance that fired. It clears the
 * flags it consumes and invokes the registered callback; it is safe to call when
 * nothing is pending.
 *
 * @param uart UART instance
 */
void xy_hal_uart_irq_handler(void *uart);


/**
 * @brief Get number of bytes available in RX buffer
 * @param uart UART instance
 * @return Number of bytes available, XY_HAL_ERROR_INVALID_PARAM on error
 */
int xy_hal_uart_available(void *uart);

/**
 * @brief Flush UART buffers
 * @param uart UART instance
 * @return XY_HAL_OK on success, XY_HAL_ERROR_INVALID_PARAM if parameters
 * invalid
 */
xy_hal_error_t xy_hal_uart_flush(void *uart);

/**
 * @brief Control UART error pin (if available)
 * @param uart UART instance
 * @return XY_HAL_OK on success, XY_HAL_ERROR_INVALID_PARAM if parameters
 * invalid
 */
xy_hal_error_t xy_hal_uart_error(void *uart);

/**
 * @brief Set SPI error callback
 * @param uart UART instance
 * @param callback Callback function
 * @param arg User argument
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_uart_set_error_cb(void *uart,
                                        xy_hal_uart_callback_t callback,
                                        void *arg);

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_UART_H */