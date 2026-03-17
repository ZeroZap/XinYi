/**
 * @file startup.c
 * @brief STM32F405 启动代码和向量表
 */

#include <stdint.h>

extern uint32_t _stack_top;
extern int main(void);
void TIM2_IRQHandler(void);

void Reset_Handler(void);
void NMI_Handler(void);
void HardFault_Handler(void);
void Default_Handler(void);

__attribute__((section(".isr_vector")))
const uint32_t vector_table[] = {
    (uint32_t)&_stack_top,     /* SP */
    (uint32_t)Reset_Handler,   /* Reset */
    (uint32_t)NMI_Handler,     /* NMI */
    (uint32_t)HardFault_Handler, /* HardFault */
    /* ... 跳过其他异常 ... */
    /* TIM2 是 IRQ 28, 在向量表中位置 = 2 + 28 = 30 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* IRQ 0-9 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* IRQ 10-19 */
    0, 0, 0, 0, 0, 0, 0, 0,        /* IRQ 20-27 */
    (uint32_t)TIM2_IRQHandler,     /* IRQ 28 - TIM2 */
};

void Reset_Handler(void)
{
    main();
    while (1);
}

void NMI_Handler(void) { while (1); }
void HardFault_Handler(void) { while (1); }
void Default_Handler(void) { while (1); }
