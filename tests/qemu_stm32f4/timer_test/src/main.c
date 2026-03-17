/**
 * @file main.c
 * @brief STM32F405 Timer 测试程序
 * 
 * QEMU olimex-stm32-h405 开发板定时器测试
 * - TIM2 基本定时器中断
 * - LED 闪烁 (基于定时器)
 */

#include <stdint.h>

/*============================================================================
 *  半主机调试输出
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
 *  STM32F405 寄存器定义
 *===========================================================================*/

/* RCC */
#define RCC_BASE            0x40023800UL
#define RCC_APB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x40))
#define RCC_APB1ENR_TIM2EN  (1 << 0)   /* TIM2 时钟使能 */
#define RCC_AHB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x30))
#define RCC_AHB1ENR_GPIOCEN (1 << 2)   /* GPIOC 时钟使能 */

/* GPIOC */
#define GPIOC_BASE          0x40020800UL
#define GPIOC_MODER         (*(volatile uint32_t *)(GPIOC_BASE + 0x00))
#define GPIOC_ODR           (*(volatile uint32_t *)(GPIOC_BASE + 0x14))
#define GPIOC_BSRR          (*(volatile uint32_t *)(GPIOC_BASE + 0x18))

/* TIM2 */
#define TIM2_BASE           0x40000000UL
#define TIM2_CR1            (*(volatile uint32_t *)(TIM2_BASE + 0x00))
#define TIM2_DIER           (*(volatile uint32_t *)(TIM2_BASE + 0x0C))
#define TIM2_SR             (*(volatile uint32_t *)(TIM2_BASE + 0x10))
#define TIM2_EGR            (*(volatile uint32_t *)(TIM2_BASE + 0x14))
#define TIM2_CNT            (*(volatile uint32_t *)(TIM2_BASE + 0x24))
#define TIM2_ARR            (*(volatile uint32_t *)(TIM2_BASE + 0x28))
#define TIM2_PSC            (*(volatile uint32_t *)(TIM2_BASE + 0x2C))

/* TIM 控制位 */
#define TIM_CR1_CEN         (1 << 0)   /* 计数器使能 */
#define TIM_DIER_UIE        (1 << 0)   /* 更新中断使能 */
#define TIM_SR_UIF          (1 << 0)   /* 更新中断标志 */
#define TIM_EGR_UG          (1 << 0)   /* 更新生成 */

/* NVIC */
#define NVIC_ISER           (*(volatile uint32_t *)0xE000E100)
#define NVIC_ICER           (*(volatile uint32_t *)0xE000E180)

/* LED */
#define LED_PIN             13

/*============================================================================
 *  全局变量
 *===========================================================================*/

static volatile uint32_t timer_ticks = 0;
static volatile uint8_t led_state = 0;

/*============================================================================
 *  中断处理
 *===========================================================================*/

void TIM2_IRQHandler(void)
{
    if (TIM2_SR & TIM_SR_UIF) {
        TIM2_SR &= ~TIM_SR_UIF;  /* 清除中断标志 */
        timer_ticks++;
        
        /* 每 500ms 翻转 LED */
        if (timer_ticks % 500 == 0) {
            led_state = !led_state;
            if (led_state) {
                GPIOC_BSRR = (1 << LED_PIN);
            } else {
                GPIOC_BSRR = (1 << (LED_PIN + 16));
            }
        }
    }
}

/*============================================================================
 *  延时函数
 *===========================================================================*/

static void delay_ms(uint32_t ms)
{
    volatile uint32_t count = ms * 15000;
    while (count--);
}

/*============================================================================
 *  GPIO 初始化
 *===========================================================================*/

static void gpio_init(void)
{
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    
    GPIOC_MODER &= ~(0x3 << 26);
    GPIOC_MODER |= (0x1 << 26);
}

/*============================================================================
 *  Timer 初始化
 *===========================================================================*/

static void timer_init(void)
{
    /* 使能 TIM2 时钟 */
    RCC_APB1ENR |= RCC_APB1ENR_TIM2EN;
    
    /* 配置预分频器 - 16MHz / 16000 = 1kHz (1ms) */
    TIM2_PSC = 16000 - 1;
    
    /* 配置自动重装载值 - 1ms 中断 */
    TIM2_ARR = 1 - 1;
    
    /* 使能更新中断 */
    TIM2_DIER |= TIM_DIER_UIE;
    
    /* 使能定时器 */
    TIM2_CR1 |= TIM_CR1_CEN;
    
    /* 使能 NVIC 中的 TIM2 中断 (IRQ 28) */
    NVIC_ISER = (1 << 28);
}

/*============================================================================
 *  主函数
 *===========================================================================*/

int main(void)
{
    print_str("\n");
    print_str("========================================\n");
    print_str("  XinYi STM32F4 Timer Test!\n");
    print_str("  MCU: STM32F405RG (Cortex-M4F)\n");
    print_str("  TIM2 @ 1kHz Interrupt\n");
    print_str("  LED: PC13 (500ms blink)\n");
    print_str("========================================\n\n");
    
    /* 初始化 GPIO */
    print_str("[INIT] Initializing GPIOC (PC13)...\n");
    gpio_init();
    print_str("[INIT] GPIO initialized.\n\n");
    
    /* 初始化 Timer */
    print_str("[INIT] Initializing TIM2 (1kHz)...\n");
    timer_init();
    print_str("[INIT] Timer initialized.\n\n");
    
    print_str("[LOOP] Running timer interrupt test...\n");
    print_str("[INFO] LED will blink every 500ms\n\n");
    
    /* 主循环 */
    uint32_t last_ticks = 0;
    while (1) {
        /* 每秒打印一次状态 */
        if (timer_ticks - last_ticks >= 1000) {
            print_str("[TICK] 1 second elapsed (");
            /* 简单数字输出 */
            uint32_t total = timer_ticks;
            char buf[16];
            int i = 0;
            if (total == 0) {
                buf[i++] = '0';
            } else {
                char tmp[16];
                int j = 0;
                while (total > 0) {
                    tmp[j++] = '0' + (total % 10);
                    total /= 10;
                }
                while (j > 0) {
                    buf[i++] = tmp[--j];
                }
            }
            buf[i] = '\0';
            print_str(buf);
            print_str(" ticks)\n");
            last_ticks = timer_ticks;
        }
        
        /* 低功耗等待 - 实际应用中可用 WFI */
        __asm__("nop");
    }
    
    return 0;
}
