/**
 * @file main.c
 * @brief STM32F405 (olimex-stm32-h405) QEMU 测试程序
 * 
 * QEMU olimex-stm32-h405 开发板测试
 * - LED 闪烁 (GPIO)
 * - 半主机串口输出
 */

#include <stdint.h>

/*============================================================================
 *  半主机调试输出 (ARM Semihosting)
 *===========================================================================*/

#define SYS_WRITE0  0x04

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

static void print_str(const char *str)
{
    semihosting_call(SYS_WRITE0, str);
}

/*============================================================================
 *  STM32F405 寄存器定义 (简化版)
 *===========================================================================*/

/* RCC - 复位和时钟控制 */
#define RCC_BASE            0x40023800UL
#define RCC_AHB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x30))
#define RCC_AHB1ENR_GPIOCEN (1 << 2)  /* GPIOC 时钟使能 */

/* GPIOC */
#define GPIOC_BASE          0x40020800UL
#define GPIOC_MODER         (*(volatile uint32_t *)(GPIOC_BASE + 0x00))
#define GPIOC_OTYPER        (*(volatile uint32_t *)(GPIOC_BASE + 0x04))
#define GPIOC_OSPEEDR       (*(volatile uint32_t *)(GPIOC_BASE + 0x08))
#define GPIOC_PUPDR         (*(volatile uint32_t *)(GPIOC_BASE + 0x0C))
#define GPIOC_ODR           (*(volatile uint32_t *)(GPIOC_BASE + 0x14))
#define GPIOC_BSRR          (*(volatile uint32_t *)(GPIOC_BASE + 0x18))

/* LED 引脚 - olimex-stm32-h405 的 LED 在 PC13 */
#define LED_GREEN_PIN       13  /* PC13 - 绿色 LED */

/*============================================================================
 *  延时函数
 *===========================================================================*/

static void delay_ms(uint32_t ms)
{
    volatile uint32_t count = ms * 15000;  /* STM32F4 @ 16MHz (QEMU) */
    while (count--);
}

/*============================================================================
 *  GPIO 初始化
 *===========================================================================*/

static void gpio_init(void)
{
    /* 使能 GPIOC 时钟 */
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    
    /* 配置 PC13 为推挽输出 */
    /* MODER: 11 = 输出模式 (bits 26:27 for PC13) */
    GPIOC_MODER &= ~(0x3 << 26);
    GPIOC_MODER |= (0x1 << 26);
    
    /* OTYPER: 0 = 推挽 (bit 13 for PC13) */
    GPIOC_OTYPER &= ~(1 << 13);
    
    /* OSPEEDR: 11 = 高速 (bits 26:27 for PC13) */
    GPIOC_OSPEEDR &= ~(0x3 << 26);
    GPIOC_OSPEEDR |= (0x3 << 26);
    
    /* PUPDR: 00 = 无上拉/下拉 (bits 26:27 for PC13) */
    GPIOC_PUPDR &= ~(0x3 << 26);
}

static void led_on(void)
{
    GPIOC_BSRR = (1 << LED_GREEN_PIN);  /* 置位 */
}

static void led_off(void)
{
    GPIOC_BSRR = (1 << (LED_GREEN_PIN + 16));  /* 复位 */
}

static void led_toggle(void)
{
    GPIOC_ODR ^= (1 << LED_GREEN_PIN);
}

/*============================================================================
 *  主函数
 *===========================================================================*/

int main(void)
{
    print_str("\n");
    print_str("========================================\n");
    print_str("  XinYi on QEMU OLIMEX-STM32-H405!\n");
    print_str("  MCU: STM32F405RG (Cortex-M4F)\n");
    print_str("  Test: GPIO LED Blink\n");
    print_str("========================================\n\n");
    
    /* 初始化 GPIO */
    print_str("[INIT] Configuring GPIOC (PC13)...\n");
    gpio_init();
    print_str("[INIT] GPIO initialized.\n\n");
    
    print_str("[LOOP] Starting LED blink (PC13)...\n\n");
    
    /* LED 闪烁循环 */
    while (1) {
        /* LED ON */
        led_on();
        print_str("  [LED] GREEN ON (PC13)\n");
        delay_ms(500);
        
        /* LED OFF */
        led_off();
        print_str("  [LED] GREEN OFF\n");
        delay_ms(500);
    }
    
    return 0;
}
