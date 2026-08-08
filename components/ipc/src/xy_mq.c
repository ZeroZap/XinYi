/**
 * @file xy_mq.c
 * @brief Message Queue Implementation
 * @version 2.0.0
 * @date 2026-03-02
 */

#include "xy_mq.h"
#include "xy_log.h"
#include "xy_os.h"
#include <string.h>
#include <stdlib.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

static uint32_t mq_slot_size(const xy_mq_t *mq)
{
    return (uint32_t)sizeof(xy_mq_msg_t) + mq->config.msg_size;
}

static uint8_t *mq_slot_at(const xy_mq_t *mq, uint16_t index)
{
    return mq->buffer + ((uint32_t)index * mq_slot_size(mq));
}

int xy_mq_init(xy_mq_t *mq, const xy_mq_config_t *config)
{
    if (!mq || !config || config->msg_size == 0 || config->max_msgs == 0) {
        return XY_MQ_INVALID_PARAM;
    }
    
    memset(mq, 0, sizeof(*mq));
    memcpy(&mq->config, config, sizeof(xy_mq_config_t));
    
    /* 分配缓冲区 */
    uint32_t size = ((uint32_t)sizeof(xy_mq_msg_t) + config->msg_size) * config->max_msgs;
    mq->buffer = malloc(size);
    if (!mq->buffer) {
        return XY_MQ_NO_MEM;
    }
    memset(mq->buffer, 0, size);
    
    mq->head = 0;
    mq->tail = 0;
    mq->count = 0;
    mq->initialized = true;
    
    xy_log_i("MQ initialized: size=%d, max_msgs=%d, priority=%d\n",
             config->msg_size, config->max_msgs, config->priority_enabled);
    return XY_MQ_OK;
}

int xy_mq_deinit(xy_mq_t *mq)
{
    if (!mq) {
        return XY_MQ_INVALID_PARAM;
    }
    
    if (mq->buffer) {
        free(mq->buffer);
        mq->buffer = NULL;
    }
    
    mq->initialized = false;
    return XY_MQ_OK;
}

int xy_mq_send(xy_mq_t *mq, const xy_mq_msg_t *msg, uint32_t timeout)
{
    uint32_t start;
    
    if (!mq || !msg || !mq->initialized) {
        return XY_MQ_INVALID_PARAM;
    }
    
    if (msg->len > mq->config.msg_size || (msg->len > 0 && !msg->data)) {
        return XY_MQ_INVALID_PARAM;
    }
    
    /* 检查队列是否满 */
    if (mq->count >= mq->config.max_msgs) {
        if (mq->config.priority_enabled && msg->priority == XY_MQ_PRIORITY_URGENT) {
            /* 紧急消息：丢弃最低优先级消息 */
            mq->tail = (mq->tail + 1) % mq->config.max_msgs;
            mq->count--;
            mq->drop_count++;
        } else if (mq->config.overwrite_old) {
            /* 覆盖旧消息 */
            mq->tail = (mq->tail + 1) % mq->config.max_msgs;
            mq->count--;
            mq->drop_count++;
        } else {
            if (timeout == 0) {
                return XY_MQ_FULL;
            }

            /* 等待空间 */
            start = xy_os_tick_get();
            while (mq->count >= mq->config.max_msgs) {
                if ((xy_os_tick_get() - start) > timeout) {
                    return XY_MQ_TIMEOUT;
                }
                xy_os_delay(1);
            }
        }
    }
    
    /* 复制消息 */
    uint8_t *slot = mq_slot_at(mq, mq->head);
    xy_mq_msg_t *stored = (xy_mq_msg_t *)slot;
    uint8_t *dest = slot + sizeof(*stored);
    *stored = *msg;
    stored->data = NULL;
    if (msg->len > 0) {
        memcpy(dest, msg->data, msg->len);
    }
    
    /* 更新写指针 */
    mq->head = (mq->head + 1) % mq->config.max_msgs;
    mq->count++;
    mq->send_count++;
    
    return XY_MQ_OK;
}

int xy_mq_recv(xy_mq_t *mq, xy_mq_msg_t *msg, uint32_t timeout)
{
    uint32_t start;
    
    if (!mq || !msg || !mq->initialized) {
        return XY_MQ_INVALID_PARAM;
    }

    uint16_t caller_len = msg->len;
    uint8_t *caller_data = msg->data;
    if (caller_len > 0 && !caller_data) {
        return XY_MQ_INVALID_PARAM;
    }
    
    /* 等待消息 */
    if (mq->count == 0 && timeout == 0) {
        return XY_MQ_EMPTY;
    }

    start = xy_os_tick_get();
    while (mq->count == 0) {
        if ((xy_os_tick_get() - start) > timeout) {
            return XY_MQ_TIMEOUT;
        }
        xy_os_delay(1);
    }
    
    /* 复制消息 */
    uint8_t *slot = mq_slot_at(mq, mq->tail);
    const xy_mq_msg_t *stored = (const xy_mq_msg_t *)slot;
    uint8_t *src = slot + sizeof(*stored);
    uint16_t copy_len = caller_len < stored->len ? caller_len : stored->len;
    msg->id = stored->id;
    msg->priority = stored->priority;
    msg->timestamp = stored->timestamp;
    msg->len = stored->len;
    msg->data = caller_data;
    if (copy_len > 0) {
        memcpy(caller_data, src, copy_len);
    }
    
    /* 更新读指针 */
    mq->tail = (mq->tail + 1) % mq->config.max_msgs;
    mq->count--;
    mq->recv_count++;
    
    return XY_MQ_OK;
}

int xy_mq_try_send(xy_mq_t *mq, const xy_mq_msg_t *msg)
{
    return xy_mq_send(mq, msg, 0);
}

int xy_mq_try_recv(xy_mq_t *mq, xy_mq_msg_t *msg)
{
    return xy_mq_recv(mq, msg, 0);
}

uint16_t xy_mq_get_count(const xy_mq_t *mq)
{
    if (!mq) {
        return 0;
    }
    return mq->count;
}

uint16_t xy_mq_get_free(const xy_mq_t *mq)
{
    if (!mq) {
        return 0;
    }
    return mq->config.max_msgs - mq->count;
}

int xy_mq_clear(xy_mq_t *mq)
{
    if (!mq) {
        return XY_MQ_INVALID_PARAM;
    }
    
    mq->head = 0;
    mq->tail = 0;
    mq->count = 0;
    
    return XY_MQ_OK;
}

int xy_mq_get_stats(const xy_mq_t *mq, uint32_t *send, uint32_t *recv, uint32_t *drop)
{
    if (!mq) {
        return XY_MQ_INVALID_PARAM;
    }
    
    if (send) *send = mq->send_count;
    if (recv) *recv = mq->recv_count;
    if (drop) *drop = mq->drop_count;
    
    return XY_MQ_OK;
}
