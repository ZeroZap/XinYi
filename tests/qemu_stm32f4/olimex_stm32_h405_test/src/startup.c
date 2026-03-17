/**
 * @file startup.c
 * @brief STM32F405 启动代码和向量表
 * 
 * QEMU olimex-stm32-h405 (STM32F405RG)
 */

#include <stdint.h>

/* 外部引用 */
extern uint32_t _stack_top;
extern int main(void);

/* 中断处理函数声明 */
void Reset_Handler(void);
void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void Default_Handler(void);

/* 向量表 - 放在 .isr_vector 段 */
__attribute__((section(".isr_vector")))
const uint32_t vector_table[] = {
    (uint32_t)&_stack_top,     /* 初始 SP */
    (uint32_t)Reset_Handler,   /* 复位处理函数 */
    (uint32_t)NMI_Handler,     /* NMI */
    (uint32_t)HardFault_Handler, /* Hard Fault */
    (uint32_t)MemManage_Handler, /* MPU Fault */
    (uint32_t)BusFault_Handler,  /* Bus Fault */
    (uint32_t)UsageFault_Handler,/* Usage Fault */
    /* 其他中断... */
};

/* 复位处理函数 */
void Reset_Handler(void)
{
    main();
    
    /* 死循环 */
    while (1);
}

/* NMI 处理 */
void NMI_Handler(void)
{
    while (1);
}

/* HardFault 处理 */
void HardFault_Handler(void)
{
    while (1);
}

/* MemManage 处理 */
void MemManage_Handler(void)
{
    while (1);
}

/* BusFault 处理 */
void BusFault_Handler(void)
{
    while (1);
}

/* UsageFault 处理 */
void UsageFault_Handler(void)
{
    while (1);
}

/* 默认中断处理 */
void Default_Handler(void)
{
    while (1);
}
