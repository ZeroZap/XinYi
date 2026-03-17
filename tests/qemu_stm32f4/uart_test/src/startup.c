/**
 * @file startup.c
 * @brief STM32F405 启动代码和向量表
 */

#include <stdint.h>

extern uint32_t _stack_top;
extern int main(void);

void Reset_Handler(void);
void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void Default_Handler(void);

__attribute__((section(".isr_vector")))
const uint32_t vector_table[] = {
    (uint32_t)&_stack_top,
    (uint32_t)Reset_Handler,
    (uint32_t)NMI_Handler,
    (uint32_t)HardFault_Handler,
    (uint32_t)MemManage_Handler,
    (uint32_t)BusFault_Handler,
    (uint32_t)UsageFault_Handler,
};

void Reset_Handler(void)
{
    main();
    while (1);
}

void NMI_Handler(void) { while (1); }
void HardFault_Handler(void) { while (1); }
void MemManage_Handler(void) { while (1); }
void BusFault_Handler(void) { while (1); }
void UsageFault_Handler(void) { while (1); }
void Default_Handler(void) { while (1); }
