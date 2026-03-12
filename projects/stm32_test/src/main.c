/**
 * @brief STM32 Test Project - XinYi Framework
 * @brief 简单的 LED  blink 测试
 */

#include <stdint.h>

/* STM32F4 寄存器定义 (简化版) */
#define RCC_BASE        0x40023800UL
#define GPIOA_BASE      0x40020000UL

#define RCC_AHB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x30UL))
#define GPIOA_MODER     (*(volatile uint32_t *)(GPIOA_BASE + 0x00UL))
#define GPIOA_ODR       (*(volatile uint32_t *)(GPIOA_BASE + 0x14UL))

/* 简单的延时函数 */
static void delay_ms(volatile uint32_t ms)
{
    volatile uint32_t count = ms * 16000;  /* 近似值，取决于系统时钟 */
    while (count--) {
        __asm__ volatile ("nop");
    }
}

int main(void)
{
    /* 使能 GPIOA 时钟 */
    RCC_AHB1ENR |= (1UL << 0);
    
    /* 配置 PA5 为输出模式 (LED 引脚) */
    GPIOA_MODER &= ~(3UL << 10);
    GPIOA_MODER |= (1UL << 10);
    
    /* 主循环：LED blink */
    while (1) {
        GPIOA_ODR |= (1UL << 5);   /* LED ON */
        delay_ms(500);
        GPIOA_ODR &= ~(1UL << 5);  /* LED OFF */
        delay_ms(500);
    }
    
    return 0;
}
