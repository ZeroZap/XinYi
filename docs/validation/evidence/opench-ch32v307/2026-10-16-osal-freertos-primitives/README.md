# openCH FreeRTOS OSAL primitives HIL

The canonical WCH FreeRTOS root build ran a two-thread producer/consumer pipeline using only XinYi OSAL APIs. The producer protected shared state with a mutex, queued twelve monotonic `uint32_t` messages, set an event flag, and delayed between iterations. The consumer waited for the flag, received and validated each message, and checked the mutex-protected state.

WCH-Link program, verify, and reset passed. All twelve ordered transfers completed without queue, event, mutex, or shared-state error markers. An empty message-queue receive mapped to `XY_OS_ERROR_TIMEOUT`; an event wait expired with no requested bit. Four platform/root builds and 278 host tests passed.

This is bounded task-context coverage for thread/delay/mutex/message-queue/event-flags and timeout behavior. It does not establish ISR-safe queue/event operations, priority inheritance measurements, resource exhaustion recovery, nested IRQ safety, or long-duration qualification.
