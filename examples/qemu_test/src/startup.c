/**
 * QEMU Cortex-M3 最小启动代码
 */

#include <stdint.h>

/* 函数声明 */
void Reset_Handler(void);
void Default_Handler(void);

/* 向量表 */
__attribute__((section(".isr_vector")))
const uint32_t vector_table[] = {
    0x20008000,              /* 初始 SP (32KB RAM 顶部) */
    (uint32_t)Reset_Handler, /* 复位处理函数 */
};

/* 复位处理函数 */
void Reset_Handler(void)
{
    /* 简单死循环 - QEMU 可以运行 */
    while (1) {
        __asm__("nop");
    }
}

/* 默认中断处理函数 */
void Default_Handler(void) {
    while (1);
}
