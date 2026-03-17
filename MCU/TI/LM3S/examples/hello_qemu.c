/**
 * @file hello_qemu.c
 * @brief XinYi on QEMU LM3S6965 测试程序
 * 
 * 使用 ARM 半主机 (Semihosting) 输出到 QEMU 控制台
 */

#include <stdint.h>
#include <stdbool.h>

/* 半主机调用号 */
#define SYS_WRITE0  0x04

/* 半主机调用 */
static inline void semihosting_call(uint32_t op, const void *arg)
{
    __asm__ volatile (
        "mov r0, %0\n"
        "mov r1, %1\n"
        "bkpt 0xAB\n"
        :
        : "r"(op), "r"(arg)
        : "r0", "r1", "memory"
    );
}

/* 半主机字符串输出 */
static void print_str(const char *str)
{
    semihosting_call(SYS_WRITE0, str);
}

/* 简单的延时 */
static void delay_ms(uint32_t ms)
{
    volatile uint32_t count = ms * 50000;
    while (count--);
}

/* GPIO 寄存器 (LM3S6965) */
#define GPIO_PORTF_BASE  0x40025000U
#define GPIO_PORTF_DIR   (*(volatile uint32_t *)(GPIO_PORTF_BASE + 0x400))
#define GPIO_PORTF_DEN   (*(volatile uint32_t *)(GPIO_PORTF_BASE + 0x71C))
#define GPIO_PORTF_DATA  (*(volatile uint32_t *)(GPIO_PORTF_BASE + 0x3FC))

/* LED 引脚 (PF1) */
#define LED_PIN  0x02

int main(void)
{
    print_str("\n================================\n");
    print_str("  XinYi on QEMU LM3S6965!\n");
    print_str("  HAL Test - GPIO Blink\n");
    print_str("================================\n\n");
    
    /* 配置 PF1 为输出 */
    GPIO_PORTF_DIR |= LED_PIN;
    GPIO_PORTF_DEN |= LED_PIN;
    
    print_str("Starting LED blink on PF1...\n\n");
    
    /* LED 闪烁 */
    while (1) {
        /* LED ON */
        GPIO_PORTF_DATA |= LED_PIN;
        print_str("  LED ON\n");
        delay_ms(500);
        
        /* LED OFF */
        GPIO_PORTF_DATA &= ~LED_PIN;
        print_str("  LED OFF\n");
        delay_ms(500);
    }
    
    return 0;
}
