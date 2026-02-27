/**
 * @file xy_hal_can.h
 * @brief CAN (Controller Area Network) Hardware Abstraction Layer
 * @version 2.0
 * @date 2026-02-27
 */

#ifndef XY_HAL_CAN_H
#define XY_HAL_CAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_hal.h"
#include <stdint.h>
#include <stddef.h>

/**
 * @brief CAN 标准帧 ID 最大值
 */
#define XY_HAL_CAN_STD_ID_MAX       (0x7FF)

/**
 * @brief CAN 扩展帧 ID 最大值
 */
#define XY_HAL_CAN_EXT_ID_MAX       (0x1FFFFFFF)

/**
 * @brief CAN 数据长度最大值
 */
#define XY_HAL_CAN_DATA_LEN_MAX     (8)

/**
 * @brief CAN 发送邮箱
 */
typedef enum {
    XY_HAL_CAN_TX_MAILBOX_0 = 0,    /**< 邮箱 0 */
    XY_HAL_CAN_TX_MAILBOX_1,        /**< 邮箱 1 */
    XY_HAL_CAN_TX_MAILBOX_2,        /**< 邮箱 2 */
    XY_HAL_CAN_TX_MAILBOX_NONE,     /**< 无可用邮箱 */
} xy_hal_can_tx_mailbox_t;

/**
 * @brief CAN 帧类型
 */
typedef enum {
    XY_HAL_CAN_FRAME_STANDARD = 0,  /**< 标准帧 */
    XY_HAL_CAN_FRAME_EXTENDED,      /**< 扩展帧 */
} xy_hal_can_frame_type_t;

/**
 * @brief CAN 帧类型 (数据/远程)
 */
typedef enum {
    XY_HAL_CAN_FRAME_TYPE_DATA = 0,     /**< 数据帧 */
    XY_HAL_CAN_FRAME_TYPE_REMOTE,       /**< 远程帧 */
} xy_hal_can_frame_data_type_t;

/**
 * @brief CAN 接收 FIFO
 */
typedef enum {
    XY_HAL_CAN_RX_FIFO_0 = 0,   /**< FIFO 0 */
    XY_HAL_CAN_RX_FIFO_1,       /**< FIFO 1 */
} xy_hal_can_rx_fifo_t;

/**
 * @brief CAN 消息结构
 */
typedef struct {
    uint32_t id;                            /**< CAN ID */
    xy_hal_can_frame_type_t frame_type;     /**< 帧类型 (标准/扩展) */
    xy_hal_can_frame_data_type_t data_type; /**< 数据类型 (数据/远程) */
    uint8_t dlc;                            /**< 数据长度码 (0-8) */
    uint8_t data[XY_HAL_CAN_DATA_LEN_MAX];  /**< 数据 */
    uint8_t fifo;                           /**< 接收 FIFO */
} xy_hal_can_message_t;

/**
 * @brief CAN 工作模式
 */
typedef enum {
    XY_HAL_CAN_MODE_NORMAL = 0,         /**< 正常模式 */
    XY_HAL_CAN_MODE_LOOPBACK,           /**< 环回模式 */
    XY_HAL_CAN_MODE_SILENT,             /**< 静默模式 */
    XY_HAL_CAN_MODE_SILENT_LOOPBACK,    /**< 静默环回模式 */
} xy_hal_can_mode_t;

/**
 * @brief CAN 位时序配置
 */
typedef struct {
    uint32_t prescaler;     /**< 预分频系数 */
    uint8_t bs1;            /**< 时间段 1 (1-1024) */
    uint8_t bs2;            /**< 时间段 2 (1-16) */
    uint8_t sjw;            /**< 同步跳转宽度 (1-4) */
} xy_hal_can_bittiming_t;

/**
 * @brief CAN 配置结构
 */
typedef struct {
    xy_hal_can_mode_t mode;                 /**< 工作模式 */
    xy_hal_can_bittiming_t bittiming;       /**< 位时序 */
    uint8_t auto_retransmit;                /**< 自动重传：1=使能，0=禁用 */
    uint8_t auto_bus_off_recovery;          /**< 总线关闭自动恢复 */
    uint32_t abom_timeout_ms;               /**< 总线关闭恢复超时 (ms) */
} xy_hal_can_config_t;

/**
 * @brief CAN 过滤器配置
 */
typedef struct {
    uint8_t filter_bank;        /**< 过滤器组 */
    uint8_t filter_mode;        /**< 过滤器模式：0=掩码，1=列表 */
    uint8_t filter_scale;       /**< 过滤器尺度：0=32 位，1=16 位 */
    uint32_t filter_id1;        /**< 过滤器 ID1 */
    uint32_t filter_id2;        /**< 过滤器 ID2 */
    uint32_t filter_mask;       /**< 过滤器掩码 */
    uint8_t fifo_assignment;    /**< FIFO 分配：0=FIFO0, 1=FIFO1 */
    uint8_t enable;             /**< 使能标志 */
} xy_hal_can_filter_config_t;

/**
 * @brief CAN 事件类型
 */
typedef enum {
    XY_HAL_CAN_EVENT_TX_COMPLETE = 0,   /**< 发送完成 */
    XY_HAL_CAN_EVENT_TX_ERROR,          /**< 发送错误 */
    XY_HAL_CAN_EVENT_RX_COMPLETE,       /**< 接收完成 */
    XY_HAL_CAN_EVENT_RX_OVERRUN,        /**< 接收溢出 */
    XY_HAL_CAN_EVENT_BUS_OFF,           /**< 总线关闭 */
    XY_HAL_CAN_EVENT_ERROR,             /**< 错误 */
    XY_HAL_CAN_EVENT_WAKEUP,            /**< 唤醒 */
    XY_HAL_CAN_EVENT_SLEEP,             /**< 睡眠 */
} xy_hal_can_event_t;

/**
 * @brief CAN 错误码
 */
typedef enum {
    XY_HAL_CAN_ERROR_NONE = 0,          /**< 无错误 */
    XY_HAL_CAN_ERROR_STUFF,             /**< 填充错误 */
    XY_HAL_CAN_ERROR_FORM,              /**< 格式错误 */
    XY_HAL_CAN_ERROR_ACK,               /**< ACK 错误 */
    XY_HAL_CAN_ERROR_BIT_RECESSIVE,     /**< 位错误 (隐性) */
    XY_HAL_CAN_ERROR_BIT_DOMINANT,      /**< 位错误 (显性) */
    XY_HAL_CAN_ERROR_CRC,               /**< CRC 错误 */
    XY_HAL_CAN_ERROR_SOFTWARE,          /**< 软件错误 */
} xy_hal_can_error_code_t;

/**
 * @brief CAN 回调类型
 */
typedef void (*xy_hal_can_callback_t)(void *can, xy_hal_can_event_t event, void *arg);

/**
 * @brief CAN 句柄
 */
typedef struct {
    void *instance;                 /**< CAN 实例 */
    xy_hal_can_config_t config;     /**< 配置 */
    uint8_t initialized;            /**< 初始化标志 */
    uint8_t started;                /**< 启动标志 */
} xy_hal_can_handle_t;

/**
 * @brief 初始化 CAN
 * @param handle CAN 句柄
 * @param config CAN 配置
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_init(xy_hal_can_handle_t *handle,
                               const xy_hal_can_config_t *config);

/**
 * @brief 反初始化 CAN
 * @param handle CAN 句柄
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_deinit(xy_hal_can_handle_t *handle);

/**
 * @brief 启动 CAN
 * @param handle CAN 句柄
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_start(xy_hal_can_handle_t *handle);

/**
 * @brief 停止 CAN
 * @param handle CAN 句柄
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_stop(xy_hal_can_handle_t *handle);

/**
 * @brief 配置 CAN 过滤器
 * @param handle CAN 句柄
 * @param config 过滤器配置
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_configure_filter(xy_hal_can_handle_t *handle,
                                           const xy_hal_can_filter_config_t *config);

/**
 * @brief 禁用 CAN 过滤器
 * @param handle CAN 句柄
 * @param filter_bank 过滤器组
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_disable_filter(xy_hal_can_handle_t *handle,
                                         uint8_t filter_bank);

/**
 * @brief 发送 CAN 消息 (阻塞)
 * @param handle CAN 句柄
 * @param message CAN 消息
 * @param timeout 超时时间 (ms)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_send(xy_hal_can_handle_t *handle,
                               const xy_hal_can_message_t *message,
                               uint32_t timeout);

/**
 * @brief 发送 CAN 消息 (非阻塞)
 * @param handle CAN 句柄
 * @param message CAN 消息
 * @param mailbox 使用的邮箱
 * @return XY_HAL_OK 成功，XY_HAL_ERROR_BUSY 邮箱忙，其他值失败
 */
xy_hal_error_t xy_hal_can_send_nb(xy_hal_can_handle_t *handle,
                                  const xy_hal_can_message_t *message,
                                  xy_hal_can_tx_mailbox_t *mailbox);

/**
 * @brief 接收 CAN 消息 (阻塞)
 * @param handle CAN 句柄
 * @param message 接收消息输出
 * @param timeout 超时时间 (ms)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_receive(xy_hal_can_handle_t *handle,
                                  xy_hal_can_message_t *message,
                                  uint32_t timeout);

/**
 * @brief 接收 CAN 消息 (非阻塞)
 * @param handle CAN 句柄
 * @param message 接收消息输出
 * @return XY_HAL_OK 成功，XY_HAL_ERROR_NO_RESOURCE 无消息，其他值失败
 */
xy_hal_error_t xy_hal_can_receive_nb(xy_hal_can_handle_t *handle,
                                     xy_hal_can_message_t *message);

/**
 * @brief 获取发送邮箱状态
 * @param handle CAN 句柄
 * @param mailbox 邮箱号
 * @return 0 空闲，1 挂起，2 发送中，3 发送完成
 */
int xy_hal_can_get_tx_mailbox_status(xy_hal_can_handle_t *handle,
                                     xy_hal_can_tx_mailbox_t mailbox);

/**
 * @brief 获取接收 FIFO 消息数
 * @param handle CAN 句柄
 * @param fifo FIFO 号
 * @return 消息数量
 */
int xy_hal_can_get_rx_fifo_level(xy_hal_can_handle_t *handle,
                                 xy_hal_can_rx_fifo_t fifo);

/**
 * @brief 获取 CAN 错误码
 * @param handle CAN 句柄
 * @return 错误码
 */
xy_hal_can_error_code_t xy_hal_can_get_error_code(xy_hal_can_handle_t *handle);

/**
 * @brief 获取发送错误计数
 * @param handle CAN 句柄
 * @return 错误计数
 */
uint8_t xy_hal_can_get_tx_error_counter(xy_hal_can_handle_t *handle);

/**
 * @brief 获取接收错误计数
 * @param handle CAN 句柄
 * @return 错误计数
 */
uint8_t xy_hal_can_get_rx_error_counter(xy_hal_can_handle_t *handle);

/**
 * @brief 注册 CAN 回调
 * @param handle CAN 句柄
 * @param event 事件类型
 * @param callback 回调函数
 * @param arg 用户参数
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_register_callback(xy_hal_can_handle_t *handle,
                                            xy_hal_can_event_t event,
                                            xy_hal_can_callback_t callback,
                                            void *arg);

/**
 * @brief 进入睡眠模式
 * @param handle CAN 句柄
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_enter_sleep(xy_hal_can_handle_t *handle);

/**
 * @brief 退出睡眠模式
 * @param handle CAN 句柄
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_can_wake_up(xy_hal_can_handle_t *handle);

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_CAN_H */
