# openCH FreeRTOS OSAL message-queue lifecycle HIL

The FreeRTOS adapter previously returned zero unconditionally from `xy_os_msgqueue_get_msg_size`; capacity was reconstructed from live queue counters instead of retained object metadata. The adapter now owns a queue context containing the native queue handle, declared capacity, and message size. Creation validates non-zero dimensions, and deletion releases both the native queue and context.

WCH-Link program, verify, and reset passed. Capacity/message-size metadata, count/space transitions, three-item FIFO order, full and empty bounded timeout mapping, reset, delete, and recreation passed without error markers. PC, STM32L4 FreeRTOS, STM32U5 FreeRTOS, and CH32V307 FreeRTOS root builds passed; all 278 host tests passed.

This does not validate ISR-origin queue operations, priority payload semantics, concurrent deletion, allocation failure, or long-duration heap behavior.
