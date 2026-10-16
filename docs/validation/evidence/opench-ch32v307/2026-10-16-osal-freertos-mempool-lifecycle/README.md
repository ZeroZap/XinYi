# openCH FreeRTOS OSAL memory-pool lifecycle HIL

The FreeRTOS pool previously accepted foreign, interior, and duplicate frees, allowing free-list corruption and `free_count` overflow. It also lost the dynamically allocated pool-memory pointer, so delete released only the control object. The pool now records its memory range and ownership, rejects overflow/undersized/misaligned creation inputs, validates block range/alignment, detects duplicate frees, and releases owned pool storage on deletion.

WCH-Link program, verify, and reset passed. External-memory metadata, three-block exhaustion, foreign/interior free rejection, duplicate-free rejection, release/reallocation, external storage deletion, and dynamic pool creation/deletion passed without error markers. Four platform/root builds and all 278 host tests passed.

This does not qualify concurrent delete/free races, ISR usage, blocking fairness, deliberate heap exhaustion, or long-duration fragmentation.
