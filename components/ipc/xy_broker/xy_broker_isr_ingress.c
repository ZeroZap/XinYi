#include "xy_broker_isr_ingress.h"

#include <string.h>

int xy_broker_isr_ingress_init(xy_broker_isr_ingress_t *ingress,
                               xy_broker_isr_msg_t *storage, uint16_t capacity,
                               xy_broker_isr_wake_t wake_from_isr, void *wake_context)
{
    if (ingress == NULL || storage == NULL || capacity < 2U || wake_from_isr == NULL) {
        return XY_BROKER_INVALID_PARAM;
    }
    memset(ingress, 0, sizeof(*ingress));
    ingress->storage = storage;
    ingress->capacity = capacity;
    ingress->wake_from_isr = wake_from_isr;
    ingress->wake_context = wake_context;
    return XY_BROKER_OK;
}

int xy_broker_isr_publish(xy_broker_isr_ingress_t *ingress, uint16_t src_server,
                          uint16_t dst_server, uint16_t msg_id, const void *payload,
                          uint16_t payload_len, uint8_t priority)
{
    uint16_t next;
    xy_broker_isr_msg_t *message;

    if (ingress == NULL || ingress->storage == NULL || ingress->capacity < 2U ||
        ingress->wake_from_isr == NULL || payload_len > XY_BROKER_MAX_MSG_SIZE ||
        (payload_len > 0U && payload == NULL)) {
        return XY_BROKER_INVALID_PARAM;
    }
    next = (uint16_t)((ingress->tail + 1U) % ingress->capacity);
    if (next == ingress->head) {
        return XY_BROKER_QUEUE_FULL;
    }

    message = &ingress->storage[ingress->tail];
    message->src_server = src_server;
    message->dst_server = dst_server;
    message->msg_id = msg_id;
    message->payload_len = payload_len;
    message->priority = priority;
    if (payload_len > 0U) {
        memcpy(message->payload, payload, payload_len);
    }
    __asm volatile("" ::: "memory");
    ingress->tail = next;
    if (ingress->wake_from_isr(ingress->wake_context) != XY_BROKER_OK) {
        return XY_BROKER_ERROR;
    }
    return XY_BROKER_OK;
}

int xy_broker_isr_drain_one(xy_broker_isr_ingress_t *ingress)
{
    xy_broker_isr_msg_t *message;
    int result;

    if (ingress == NULL || ingress->storage == NULL || ingress->capacity < 2U) {
        return XY_BROKER_INVALID_PARAM;
    }
    if (ingress->head == ingress->tail) {
        return XY_BROKER_NOT_FOUND;
    }
    message = &ingress->storage[ingress->head];
    result = xy_broker_send_msg(message->src_server, message->dst_server, message->msg_id,
                                message->payload, message->payload_len, message->priority);
    if (result == XY_BROKER_OK) {
        ingress->head = (uint16_t)((ingress->head + 1U) % ingress->capacity);
    }
    return result;
}
