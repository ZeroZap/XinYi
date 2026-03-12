/**
 * @brief STM32F407xx 启动文件
 * @brief 简化的启动代码，用于 XinYi Framework 测试
 */

#include <stdint.h>

/* 链接脚本中定义的符号 */
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;
extern uint32_t _estack;

/* 主函数声明 */
extern int main(void);

/* 复位处理函数 */
void Reset_Handler(void)
{
    uint32_t *src, *dst;
    
    /* 初始化 .data 段 */
    src = &_sidata;
    dst = &_sdata;
    while (dst < &_edata) {
        *dst++ = *src++;
    }
    
    /* 清零 .bss 段 */
    dst = &_sbss;
    while (dst < &_ebss) {
        *dst++ = 0;
    }
    
    /* 调用主函数 */
    main();
    
    /* 如果 main 返回，进入死循环 */
    while (1) {
        __asm__ volatile ("wfi");
    }
}

/* 默认中断处理函数 */
void Default_Handler(void)
{
    while (1) {
        __asm__ volatile ("wfi");
    }
}

/* 弱定义的中断处理函数 */
void NMI_Handler(void)           __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)      __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void)      __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void)       __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void)     __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)            __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void)       __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)         __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)        __attribute__((weak, alias("Default_Handler")));

/* 中断向量表 */
__attribute__((section(".isr_vector")))
const uint32_t vector_table[] = {
    (uint32_t)&_estack,          /* 栈顶指针 */
    (uint32_t)Reset_Handler,     /* 复位处理函数 */
    (uint32_t)NMI_Handler,       /* NMI */
    (uint32_t)HardFault_Handler, /* Hard Fault */
    (uint32_t)MemManage_Handler, /* MPU Fault */
    (uint32_t)BusFault_Handler,  /* Bus Fault */
    (uint32_t)UsageFault_Handler,/* Usage Fault */
    0, 0, 0, 0,                  /* Reserved */
    (uint32_t)SVC_Handler,       /* SVCall */
    (uint32_t)DebugMon_Handler,  /* Debug Monitor */
    0,                           /* Reserved */
    (uint32_t)PendSV_Handler,    /* PendSV */
    (uint32_t)SysTick_Handler,   /* SysTick */
};
