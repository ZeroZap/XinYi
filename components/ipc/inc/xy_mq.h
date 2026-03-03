/**
 * @file xy_mq.h
 * @brief Lightweight Message Queue for IPC
 * @version 1.0.0
 * @date 2026-03-01 自主任务
 */

#ifndef XY_MQ_H
#define XY_MQ_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 错误码
 */
#define XY_MQ_OK              0
#define XY_MQ_ERROR           (-1)
#define XY_MQ_INVALID_PARAM   (-2)
#define XY_MQ_FULL            (-3)
#define XY_MQ_EMPTY           (-4)
#define XY_MQ_TIMEOUT         (-5)
#define XY_MQ_NO_MEM          (-6)

/**
 * @brief 消息队列配置
 */
typedef struct {
    uint16_t msg_size;        /**< 消息大小 (字节) */
    uint16_t max_msgs;        /**< 最大消息数 */
    bool overwrite_old;       /**< 满时是否覆盖旧消息 */
} xy_mq_config_t;

/**
 * @brief 消息队列句柄
 */
typedef struct {
    xy_mq_config_t config;    /**< 配置 */
    uint8_t *buffer;          /**< 消息缓冲区 */
    uint16_t head;            /**< 写指针 */
    uint16_t tail;            /**< 读指针 */
    uint16_t count;           /**< 当前消息数 */
    uint32_t send_count;      /**< 发送计数 */
    uint32_t recv_count;      /**< 接收计数 */
    uint32_t drop_count;      /**< 丢弃计数 */
    bool initialized;         /**< 初始化标志 */
} xy_mq_t;

/**
 * @brief 初始化消息队列
 */
int xy_mq_init(xy_mq_t *mq, const xy_mq_config_t *config);

/**
 * @brief 反初始化
 */
int xy_mq_deinit(xy_mq_t *mq);

/**
 * @brief 发送消息
 */
int xy_mq_send(xy_mq_t *mq, const void *msg, uint32_t timeout);

/**
 * @brief 接收消息
 */
int xy_mq_recv(xy_mq_t *mq, void *msg, uint32_t timeout);

/**
 * @brief 尝试发送 (不等待)
 */
int xy_mq_try_send(xy_mq_t *mq, const void *msg);

/**
 * @brief 尝试接收 (不等待)
 */
int xy_mq_try_recv(xy_mq_t *mq, void *msg);

/**
 * @brief 获取消息数量
 */
uint16_t xy_mq_get_count(const xy_mq_t *mq);

/**
 * @brief 获取剩余空间
 */
uint16_t xy_mq_get_free(const xy_mq_t *mq);

/**
 * @brief 清空队列
 */
int xy_mq_clear(xy_mq_t *mq);

/**
 * @brief 获取统计信息
 */
int xy_mq_get_stats(const xy_mq_t *mq, uint32_t *send, uint32_t *recv, uint32_t *drop);

#ifdef __cplusplus
}
#endif

#endif
