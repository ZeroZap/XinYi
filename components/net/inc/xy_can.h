/**
 * @file xy_can.h
 * @brief XinYi CAN protocol public API.
 */

#ifndef XY_CAN_H
#define XY_CAN_H

#include "xy_typedef.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** CAN API status codes. */
typedef enum {
    XY_CAN_OK = 0,
    XY_CAN_ERROR = -1,
    XY_CAN_INVALID_PARAM = -2,
    XY_CAN_TIMEOUT = -3,
    XY_CAN_FIFO_FULL = -4,
    XY_CAN_FIFO_EMPTY = -5,
} xy_can_status_t;

/** CAN message structure. */
typedef struct {
    uint32_t id;      /**< 11-bit or 29-bit CAN identifier. */
    uint8_t data[8];  /**< Payload bytes. */
    uint8_t len;      /**< Payload length, 0..8. */
    uint8_t rtr;      /**< Remote transmission request flag. */
} xy_can_msg_t;

/** CAN runtime configuration. */
typedef struct {
    uint32_t baudrate;
    uint32_t rx_fifo_size;
    uint32_t tx_fifo_size;
    uint32_t flags;
} xy_can_config_t;

struct xy_can;
typedef struct xy_can xy_can_t;

typedef void (*xy_can_rx_callback_t)(xy_can_t *can, const xy_can_msg_t *msg);

/** CAN instance state. */
struct xy_can {
    xy_can_config_t config;
    void *hw_handle;

    xy_can_msg_t *rx_fifo;
    xy_can_msg_t *tx_fifo;
    uint32_t rx_fifo_size;
    uint32_t tx_fifo_size;
    uint32_t rx_head;
    uint32_t rx_tail;
    uint32_t tx_head;
    uint32_t tx_tail;

    uint32_t tx_count;
    uint32_t rx_count;
    uint32_t error_count;

    bool initialized;
    bool started;

    xy_can_rx_callback_t rx_callback;
    void *callback_user_data;
};

int xy_can_init(xy_can_t *can, void *hw_handle, const xy_can_config_t *config);
int xy_can_deinit(xy_can_t *can);
int xy_can_start(xy_can_t *can);
int xy_can_stop(xy_can_t *can);
int xy_can_send(xy_can_t *can, const xy_can_msg_t *msg, uint32_t timeout);
int xy_can_receive(xy_can_t *can, xy_can_msg_t *msg, uint32_t timeout);
int xy_can_register_rx_callback(xy_can_t *can, xy_can_rx_callback_t callback,
                                void *user_data);
int xy_can_unregister_rx_callback(xy_can_t *can);
void xy_can_isr_receive(xy_can_t *can, const xy_can_msg_t *msg);
uint32_t xy_can_get_tx_count(const xy_can_t *can);
uint32_t xy_can_get_rx_count(const xy_can_t *can);
uint32_t xy_can_get_error_count(const xy_can_t *can);
int xy_can_get_fifo_usage(const xy_can_t *can, float *rx_usage, float *tx_usage);

#ifdef __cplusplus
}
#endif

#endif /* XY_CAN_H */
