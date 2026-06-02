/**
 * @file xy_device_async.h
 * @brief Optional queued async helper API for devices that use xy_device_t::data.
 * @version 1.0.0
 * @date 2026-03-15
 *
 * @note The generic xy_device_async_read/write dispatch APIs are declared in
 *       xy_device.h. This header exposes a small optional helper state machine
 *       with a distinct xy_device_async_*_ex namespace so it does not collide
 *       with the generic dispatch API.
 */

#ifndef XY_DEVICE_ASYNC_H
#define XY_DEVICE_ASYNC_H

#include "xy_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Async Operation Types ==================== */

/**
 * @brief Async operation type.
 */
typedef enum {
    XY_DEVICE_ASYNC_OP_NONE = 0,
    XY_DEVICE_ASYNC_OP_READ,
    XY_DEVICE_ASYNC_OP_WRITE,
    XY_DEVICE_ASYNC_OP_IOCTL,
} xy_device_async_op_t;

/**
 * @brief Async operation state.
 */
typedef enum {
    XY_DEVICE_ASYNC_STATE_IDLE = 0,
    XY_DEVICE_ASYNC_STATE_PENDING,
    XY_DEVICE_ASYNC_STATE_COMPLETED,
    XY_DEVICE_ASYNC_STATE_ERROR,
} xy_device_async_state_t;

/* ==================== Async Callback ==================== */

/**
 * @brief Async helper completion callback.
 * @param dev Device handle.
 * @param op Operation type.
 * @param result Operation result.
 * @param user_data User data.
 */
typedef void (*xy_device_async_callback_t)(xy_device_t *dev,
                                           xy_device_async_op_t op,
                                           int result,
                                           void *user_data);

/* ==================== Async Request Structure ==================== */

/**
 * @brief Async helper request.
 */
typedef struct {
    xy_device_async_op_t op;             /**< Operation type */
    xy_device_async_state_t state;       /**< Operation state */
    void *buffer;                        /**< Data buffer */
    size_t length;                       /**< Data length */
    size_t transferred;                  /**< Transferred byte count */
    int error_code;                      /**< Error code */
    xy_device_async_callback_t callback; /**< Completion callback */
    void *user_data;                     /**< User data */
    uint32_t timeout_ms;                 /**< Timeout in milliseconds */
    uint32_t start_time;                 /**< Start tick */
} xy_device_async_request_t;

/**
 * @brief Async helper operation hooks.
 */
typedef struct {
    int (*read)(xy_device_t *dev, void *buffer, size_t length,
                xy_device_async_callback_t callback, void *user_data,
                uint32_t timeout_ms);
    int (*write)(xy_device_t *dev, const void *buffer, size_t length,
                 xy_device_async_callback_t callback, void *user_data,
                 uint32_t timeout_ms);
    int (*poll)(xy_device_t *dev);
    int (*ready)(xy_device_t *dev, bool for_write);
} xy_device_async_ops_t;

/**
 * @brief Async helper context stored by the caller.
 */
typedef struct {
    xy_device_async_request_t request;   /**< Current request */
    const xy_device_async_ops_t *ops;    /**< Optional backend hooks */
    bool initialized;                    /**< Initialization flag */
    bool is_busy;                        /**< Busy flag */
} xy_device_async_context_t;

/* ==================== Async Helper API ==================== */

/**
 * @brief Initialize async helper state.
 * @param ctx Async helper context owned by the caller.
 * @param ops Optional backend hooks.
 * @return XY_DEVICE_OK on success.
 */
int xy_device_async_init_ex(xy_device_async_context_t *ctx,
                            const xy_device_async_ops_t *ops);

/**
 * @brief Start an async read through the optional helper backend.
 */
int xy_device_async_read_ex(xy_device_t *dev, xy_device_async_context_t *ctx,
                            void *buffer, size_t length,
                            xy_device_async_callback_t callback, void *user_data,
                            uint32_t timeout_ms);

/**
 * @brief Start an async write through the optional helper backend.
 */
int xy_device_async_write_ex(xy_device_t *dev, xy_device_async_context_t *ctx,
                             const void *buffer, size_t length,
                             xy_device_async_callback_t callback, void *user_data,
                             uint32_t timeout_ms);

/**
 * @brief Cancel the current helper operation.
 */
int xy_device_async_cancel_ex(xy_device_t *dev, xy_device_async_context_t *ctx);

/**
 * @brief Get helper operation state.
 */
int xy_device_async_get_state_ex(const xy_device_async_context_t *ctx,
                                 xy_device_async_state_t *state);

/**
 * @brief Get transferred byte count.
 */
int xy_device_async_get_transferred_ex(const xy_device_async_context_t *ctx,
                                       size_t *transferred);

/**
 * @brief Poll helper operation completion.
 * @return 1=completed, 0=pending, negative=error.
 */
int xy_device_async_poll_ex(xy_device_t *dev, xy_device_async_context_t *ctx);

/**
 * @brief Wait for helper operation completion.
 */
int xy_device_async_wait_ex(xy_device_t *dev, xy_device_async_context_t *ctx,
                            uint32_t timeout_ms);

/**
 * @brief Check helper readiness.
 */
int xy_device_async_ready_ex(xy_device_t *dev, xy_device_async_context_t *ctx,
                             bool for_write);

#ifdef __cplusplus
}
#endif

#endif /* XY_DEVICE_ASYNC_H */
