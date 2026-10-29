/**
 * @file xy_hal_irq_pc.c
 * @brief Interrupt entry points for the host backends
 *
 * These used to discard their argument and return, so a registered callback could never be reached
 * on the host no matter how correct the registration was. Each one now delivers through the backend
 * that owns the state.
 */

#include "xy_hal_dma.h"
#include "xy_hal_timer.h"

/* Delivered by the backends; declared here rather than in a public header because no consumer
 * outside the host layer calls them. */
void xy_hal_timer_pc_irq(void *timer);
void xy_hal_dma_pc_irq(void *dma);

void xy_hal_timer_irq_handler(void *timer)
{
    xy_hal_timer_pc_irq(timer);
}

void xy_hal_dma_irq_handler(void *dma)
{
    xy_hal_dma_pc_irq(dma);
}
