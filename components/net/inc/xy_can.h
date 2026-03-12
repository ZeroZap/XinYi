/**
 * @file xy_can.h
 * @brief CAN Bus Protocol Stack
 * @version 1.0.0
 * @date 2026-03-01 上午
 */

#ifndef XY_CAN_H
#define XY_CAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief CAN 帧类型
 */
typedef enum {
    XY_CAN_FRAME_DATA = 0,      /**< 数据帧 */
    XY_CAN_FRAME_REMOTE = 1,    /**< 远程帧 */
} xy_can_frame_type_t;

/**
 * @brief CAN 帧 ID 类型
 */
typedef enum {
    XY_CAN_ID_STD = 0,          /**< 标准帧 (11 位) */
    XY_CAN_ID_EXT = 1,          /**< 扩展帧 (29 位) */
} xy_can_id_type_t;

/**
 * @brief CAN 错误码
 */
#define XY_CAN_OK               0
#define XY_CAN_ERROR            (-1)
#define XY_CAN_INVALID_PARAM    (-2)
#define XY_CAN_BUSY             (-3)
#define XY_CAN_TIMEOUT          (-4)
#define XY_CAN_NO_DATA          (-5)
#define XY_CAN_FIFO_FULL        (-6)
#define XY_CAN_FIFO_EMPTY       (-7)

/**
 * @brief CAN 最大数据长度
 */
#define XY_CAN_MAX_DLC          8

/**
 * @brief CAN 消息结构
 */
typedef struct {
    uint32_t id;                        /**< CAN ID */
    xy_can_id_type_t id_type;           /**< ID 类型 */
    xy_can_frame_type_t frame_type;     /**< 帧类型 */
    uint8_t dlc;                        /**< 数据长度 (0-8) */
    uint8_t data[XY_CAN_MAX_DLC];       /**< 数据 */
    uint32_t timestamp;                 /**< 时间戳 (ms) */
} xy_can_msg_t;

/**
 * @brief CAN 配置
 */
typedef struct {
    uint32_t baudrate;              /**< 波特率 */
    uint32_t rx_fifo_size;          /**< 接收 FIFO 大小 */
    uint32_t tx_fifo_size;          /**< 发送 FIFO 大小 */
    bool enable_loopback;           /**< 环回模式 */
    bool enable_silent;             /**< 静默模式 */
} xy_can_config_t;

/**
 * @brief CAN 设备结构
 */
typedef struct {
    xy_can_config_t config;         /**< 配置 */
    void *hw_handle;                /**< 硬件句柄 */
    
    xy_can_msg_t *rx_fifo;          /**< 接收 FIFO */
    uint32_t rx_fifo_size;          /**< FIFO 大小 */
    uint32_t rx_head;               /**< 写指针 */
    uint32_t rx_tail;               /**< 读指针 */
    
    xy_can_msg_t *tx_fifo;          /**< 发送 FIFO */
    uint32_t tx_fifo_size;          /**< FIFO 大小 */
    uint32_t tx_head;               /**< 写指针 */
    uint32_t tx_tail;               /**< 读指针 */
    
    uint32_t tx_count;              /**< 发送计数 */
    uint32_t rx_count;              /**< 接收计数 */
    uint32_t error_count;           /**< 错误计数 */
    
    xy_can_rx_callback_t rx_callback;  /**< 接收回调 */
    void *callback_user_data;       /**< 回调用户数据 */
    
    bool initialized;               /**< 初始化标志 */
} xy_can_t;

/**
 * @brief CAN 回调函数
 */
typedef void (*xy_can_rx_callback_t)(xy_can_t *can, const xy_can_msg_t *msg);

/**
 * @brief 初始化 CAN
 * @param can CAN 设备句柄
 * @param hw_handle 硬件句柄
 * @param config 配置
 * @return XY_CAN_OK 成功，其他值失败
 */
int xy_can_init(xy_can_t *can, void *hw_handle, const xy_can_config_t *config);

/**
 * @brief 反初始化 CAN
 * @param can CAN 设备句柄
 * @return XY_CAN_OK 成功，其他值失败
 */
int xy_can_deinit(xy_can_t *can);

/**
 * @brief 启动 CAN
 * @param can CAN 设备句柄
 * @return XY_CAN_OK 成功，其他值失败
 */
int xy_can_start(xy_can_t *can);

/**
 * @brief 停止 CAN
 * @param can CAN 设备句柄
 * @return XY_CAN_OK 成功，其他值失败
 */
int xy_can_stop(xy_can_t *can);

/**
 * @brief 发送消息
 * @param can CAN 设备句柄
 * @param msg CAN 消息
 * @param timeout 超时时间 (ms)
 * @return XY_CAN_OK 成功，其他值失败
 */
int xy_can_send(xy_can_t *can, const xy_can_msg_t *msg, uint32_t timeout);

/**
 * @brief 接收消息
 * @param can CAN 设备句柄
 * @param msg CAN 消息指针
 * @param timeout 超时时间 (ms)
 * @return XY_CAN_OK 成功，其他值失败
 */
int xy_can_receive(xy_can_t *can, xy_can_msg_t *msg, uint32_t timeout);

/**
 * @brief 注册接收回调
 * @param can CAN 设备句柄
 * @param callback 回调函数
 * @param user_data 用户数据 (传递给回调)
 * @return XY_CAN_OK 成功，其他值失败
 */
int xy_can_register_rx_callback(xy_can_t *can, xy_can_rx_callback_t callback, void *user_data);

/**
 * @brief 注销接收回调
 * @param can CAN 设备句柄
 * @return XY_CAN_OK 成功，其他值失败
 */
int xy_can_unregister_rx_callback(xy_can_t *can);

/**
 * @brief CAN 中断接收处理 (在硬件中断中调用)
 * @param can CAN 设备句柄
 * @param msg 接收到的消息
 */
void xy_can_isr_receive(xy_can_t *can, const xy_can_msg_t *msg);

/**
 * @brief 获取发送计数
 * @param can CAN 设备句柄
 * @return 发送计数
 */
uint32_t xy_can_get_tx_count(const xy_can_t *can);

/**
 * @brief 获取接收计数
 * @param can CAN 设备句柄
 * @return 接收计数
 */
uint32_t xy_can_get_rx_count(const xy_can_t *can);

/**
 * @brief 获取错误计数
 * @param can CAN 设备句柄
 * @return 错误计数
 */
uint32_t xy_can_get_error_count(const xy_can_t *can);

/**
 * @brief 获取 FIFO 使用率
 * @param can CAN 设备句柄
 * @param rx_usage 接收 FIFO 使用率指针
 * @param tx_usage 发送 FIFO 使用率指针
 * @return XY_CAN_OK 成功，其他值失败
 */
int xy_can_get_fifo_usage(const xy_can_t *can, float *rx_usage, float *tx_usage);

#ifdef __cplusplus
}
#endif

#endif /* XY_CAN_H */
