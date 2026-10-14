#include "xy_broker_isr_ingress.h"

#include <string.h>

#define XY_BROKER_ISR_INTEGRITY_SEED 2166136261U
#define XY_BROKER_ISR_INTEGRITY_PRIME 16777619U

static uint32_t integrity_update(uint32_t tag, const void *data, size_t length)
{
    const uint8_t *bytes = data;

    while (length-- > 0U) {
        tag ^= *bytes++;
        tag *= XY_BROKER_ISR_INTEGRITY_PRIME;
    }
    return tag;
}

static uint32_t message_integrity(const xy_broker_isr_msg_t *message)
{
    uint32_t tag = XY_BROKER_ISR_INTEGRITY_SEED;

    tag = integrity_update(tag, &message->src_server, sizeof(message->src_server));
    tag = integrity_update(tag, &message->dst_server, sizeof(message->dst_server));
    tag = integrity_update(tag, &message->msg_id, sizeof(message->msg_id));
    tag = integrity_update(tag, &message->payload_len, sizeof(message->payload_len));
    tag = integrity_update(tag, &message->priority, sizeof(message->priority));
    tag = integrity_update(tag, &message->src_generation, sizeof(message->src_generation));
    tag = integrity_update(tag, &message->dst_generation, sizeof(message->dst_generation));
    return integrity_update(tag, message->payload, message->payload_len);
}

static int ingress_is_live(const xy_broker_isr_ingress_t *ingress)
{
    return ingress != NULL && ingress->self_owner == ingress && ingress->storage != NULL &&
           ingress->storage == ingress->storage_owner && ingress->capacity >= 2U &&
           ingress->capacity == ingress->capacity_owner && ingress->head < ingress->capacity &&
           ingress->tail < ingress->capacity && ingress->wake_from_isr != NULL &&
           ingress->wake_from_isr == ingress->wake_owner &&
           ingress->wake_context == ingress->wake_context_owner;
}

int xy_broker_isr_ingress_init(xy_broker_isr_ingress_t *ingress,
                               xy_broker_isr_msg_t *storage, uint16_t capacity,
                               xy_broker_isr_wake_t wake_from_isr, void *wake_context)
{
    uint32_t broker_generation;

    if (ingress == NULL || storage == NULL || capacity < 2U || wake_from_isr == NULL) {
        return XY_BROKER_INVALID_PARAM;
    }
    broker_generation = xy_broker_get_lifecycle_generation();
    if (broker_generation == 0U) {
        return XY_BROKER_ERROR;
    }
    memset(ingress, 0, sizeof(*ingress));
    ingress->storage = storage;
    ingress->storage_owner = storage;
    ingress->self_owner = ingress;
    ingress->capacity = capacity;
    ingress->capacity_owner = capacity;
    ingress->wake_from_isr = wake_from_isr;
    ingress->wake_owner = wake_from_isr;
    ingress->wake_context = wake_context;
    ingress->wake_context_owner = wake_context;
    ingress->broker_generation = broker_generation;
    return XY_BROKER_OK;
}

int xy_broker_isr_publish(xy_broker_isr_ingress_t *ingress, uint16_t src_server,
                          uint16_t dst_server, uint16_t msg_id, const void *payload,
                          uint16_t payload_len, uint8_t priority)
{
    uint16_t next;
    uint16_t previous_head;
    uint16_t previous_tail;
    uint32_t dst_generation;
    xy_broker_isr_msg_t *message;

    if (!ingress_is_live(ingress) || ingress->wake_from_isr == NULL || src_server == 0U ||
        dst_server == 0U || msg_id == 0U ||
        payload_len > XY_BROKER_MAX_MSG_SIZE || (payload_len > 0U && payload == NULL)) {
        return XY_BROKER_INVALID_PARAM;
    }
    if (ingress->broker_generation != xy_broker_get_lifecycle_generation()) {
        return XY_BROKER_ERROR;
    }
    dst_generation = xy_broker_get_server_generation(dst_server);
    if (dst_generation == 0U) {
        return XY_BROKER_NOT_FOUND;
    }
    previous_head = ingress->head;
    previous_tail = ingress->tail;
    next = (uint16_t)((previous_tail + 1U) % ingress->capacity);
    if (next == ingress->head) {
        return XY_BROKER_QUEUE_FULL;
    }

    message = &ingress->storage[previous_tail];
    message->src_server = src_server;
    message->dst_server = dst_server;
    message->msg_id = msg_id;
    message->payload_len = payload_len;
    message->priority = priority;
    message->src_generation = xy_broker_get_server_generation(src_server);
    message->dst_generation = dst_generation;
    if (payload_len > 0U) {
        memcpy(message->payload, payload, payload_len);
    }
    message->integrity_tag = message_integrity(message);
    __asm volatile("" ::: "memory");
    ingress->tail = next;
    int wake_result = ingress->wake_from_isr(ingress->wake_context);
    if (wake_result != XY_BROKER_OK) {
        ingress->tail = previous_tail;
        return wake_result;
    }
    if (!ingress_is_live(ingress) || ingress->head != previous_head || ingress->tail != next
        || ingress->broker_generation != xy_broker_get_lifecycle_generation()
        || (message->src_generation != 0U
            && message->src_generation != xy_broker_get_server_generation(message->src_server))
        || message->dst_generation != xy_broker_get_server_generation(message->dst_server)) {
        ingress->head = previous_head;
        ingress->tail = previous_tail;
        return XY_BROKER_ERROR;
    }
    return XY_BROKER_OK;
}

int xy_broker_isr_drain_one(xy_broker_isr_ingress_t *ingress)
{
    xy_broker_isr_msg_t *message;
    int result;

    if (!ingress_is_live(ingress)) {
        return XY_BROKER_INVALID_PARAM;
    }
    if (ingress->head == ingress->tail) {
        return XY_BROKER_NOT_FOUND;
    }
    if (ingress->broker_generation != xy_broker_get_lifecycle_generation()) {
        ingress->head = ingress->tail;
        return XY_BROKER_ERROR;
    }
    message = &ingress->storage[ingress->head];
    if (message->src_server == 0U || message->dst_server == 0U || message->msg_id == 0U ||
        message->payload_len > XY_BROKER_MAX_MSG_SIZE) {
        ingress->head = (uint16_t)((ingress->head + 1U) % ingress->capacity);
        return XY_BROKER_ERROR;
    }
    if (message->integrity_tag != message_integrity(message)) {
        ingress->head = (uint16_t)((ingress->head + 1U) % ingress->capacity);
        return XY_BROKER_ERROR;
    }
    if ((message->src_generation != 0U
         && message->src_generation != xy_broker_get_server_generation(message->src_server))
        || message->dst_generation == 0U
        || message->dst_generation != xy_broker_get_server_generation(message->dst_server)) {
        ingress->head = (uint16_t)((ingress->head + 1U) % ingress->capacity);
        return XY_BROKER_NOT_FOUND;
    }
    result = xy_broker_send_msg(message->src_server, message->dst_server, message->msg_id,
                                message->payload, message->payload_len, message->priority);
    if (result == XY_BROKER_OK) {
        ingress->head = (uint16_t)((ingress->head + 1U) % ingress->capacity);
    }
    return result;
}
