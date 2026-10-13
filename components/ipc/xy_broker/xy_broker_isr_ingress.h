#ifndef XY_BROKER_ISR_INGRESS_H
#define XY_BROKER_ISR_INGRESS_H

#include "xy_broker.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*xy_broker_isr_wake_t)(void *context);

typedef struct {
    uint16_t src_server;
    uint16_t dst_server;
    uint16_t msg_id;
    uint16_t payload_len;
    uint8_t priority;
    uint32_t src_generation;
    uint32_t dst_generation;
    uint8_t payload[XY_BROKER_MAX_MSG_SIZE];
} xy_broker_isr_msg_t;

typedef struct {
    xy_broker_isr_msg_t *storage;
    xy_broker_isr_msg_t *storage_owner;
    const void *self_owner;
    uint16_t capacity;
    uint16_t capacity_owner;
    volatile uint16_t head;
    volatile uint16_t tail;
    xy_broker_isr_wake_t wake_from_isr;
    void *wake_context;
    uint32_t broker_generation;
} xy_broker_isr_ingress_t;

int xy_broker_isr_ingress_init(xy_broker_isr_ingress_t *ingress,
                               xy_broker_isr_msg_t *storage, uint16_t capacity,
                               xy_broker_isr_wake_t wake_from_isr, void *wake_context);
int xy_broker_isr_publish(xy_broker_isr_ingress_t *ingress, uint16_t src_server,
                          uint16_t dst_server, uint16_t msg_id, const void *payload,
                          uint16_t payload_len, uint8_t priority);
int xy_broker_isr_drain_one(xy_broker_isr_ingress_t *ingress);

#ifdef __cplusplus
}
#endif

#endif
