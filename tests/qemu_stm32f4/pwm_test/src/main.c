/**
 * @file main.c
 * @brief STM32F405 PWM 测试程序
 * 
 * QEMU olimex-stm32-h405 开发板 PWM 测试
 * - TIM3 通道 1 PWM 输出
 * - PA6 (TIM3_CH1)
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
#define RCC_APB1ENR_TIM3EN  (1 << 1)   /* TIM3 时钟使能 */
#define RCC_AHB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x30))
#define RCC_AHB1ENR_GPIOAEN (1 << 0)   /* GPIOA 时钟使能 */

/* GPIOA */
#define GPIOA_BASE          0x40020000UL
#define GPIOA_MODER         (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_AFRL          (*(volatile uint32_t *)(GPIOA_BASE + 0x20))
#define GPIOA_OSPEEDR       (*(volatile uint32_t *)(GPIOA_BASE + 0x08))

/* TIM3 */
#define TIM3_BASE           0x40000400UL
#define TIM3_CR1            (*(volatile uint32_t *)(TIM3_BASE + 0x00))
#define TIM3_CR2            (*(volatile uint32_t *)(TIM3_BASE + 0x04))
#define TIM3_SMCR           (*(volatile uint32_t *)(TIM3_BASE + 0x08))
#define TIM3_DIER           (*(volatile uint32_t *)(TIM3_BASE + 0x0C))
#define TIM3_SR             (*(volatile uint32_t *)(TIM3_BASE + 0x10))
#define TIM3_EGR            (*(volatile uint32_t *)(TIM3_BASE + 0x14))
#define TIM3_CCMR1          (*(volatile uint32_t *)(TIM3_BASE + 0x18))
#define TIM3_CCER           (*(volatile uint32_t *)(TIM3_BASE + 0x20))
#define TIM3_PSC            (*(volatile uint32_t *)(TIM3_BASE + 0x28))
#define TIM3_ARR            (*(volatile uint32_t *)(TIM3_BASE + 0x2C))
#define TIM3_CCR1           (*(volatile uint32_t *)(TIM3_BASE + 0x34))

/* TIM 控制位 */
#define TIM_CR1_CEN         (1 << 0)   /* 计数器使能 */
#define TIM_CR1_ARPE        (1 << 7)   /* 自动重装载预装载使能 */

#define TIM_CCMR1_OC1M      (6 << 4)   /* PWM 模式 1 */
#define TIM_CCMR1_OC1PE     (1 << 3)   /* 预装载使能 */

#define TIM_CCER_CC1E       (1 << 0)   /* 通道 1 输出使能 */
#define TIM_CCER_CC1P       (1 << 1)   /* 通道 1 极性 */

/* GPIO 复用 */
#define GPIO_AF_TIM3        2          /* PA6 复用为 TIM3_CH1 */

/*============================================================================
 *  延时函数
 *===========================================================================*/

static void delay_ms(uint32_t ms)
{
    volatile uint32_t count = ms * 15000;
    while (count--);
}

/*============================================================================
 *  PWM 初始化
 *===========================================================================*/

static void pwm_init(uint16_t period, uint16_t duty)
{
    /* 1. 使能 GPIOA 和 TIM3 时钟 */
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC_APB1ENR |= RCC_APB1ENR_TIM3EN;
    
    /* 2. 配置 PA6 为复用功能 (TIM3_CH1) */
    GPIOA_MODER &= ~(0x3 << 12);
    GPIOA_MODER |= (0x2 << 12);  /* 复用模式 */
    
    GPIOA_AFRL &= ~(0xF << 24);
    GPIOA_AFRL |= (GPIO_AF_TIM3 << 24);  /* AF2 */
    
    GPIOA_OSPEEDR |= (0x3 << 12);  /* 高速 */
    
    /* 3. 配置 TIM3 PWM */
    /* 禁用定时器 */
    TIM3_CR1 &= ~TIM_CR1_CEN;
    
    /* 配置预分频器 - 16MHz / 16 = 1MHz 计数频率 */
    TIM3_PSC = 16 - 1;
    
    /* 配置自动重装载值 (周期) - 1MHz / period = PWM 频率 */
    /* period=1000 → 1kHz PWM */
    TIM3_ARR = period - 1;
    
    /* 配置占空比 */
    TIM3_CCR1 = duty;
    
    /* 配置通道 1 为 PWM 模式 1 */
    TIM3_CCMR1 &= ~(0xFF << 0);
    TIM3_CCMR1 |= TIM_CCMR1_OC1M | TIM_CCMR1_OC1PE;
    
    /* 使能通道 1 输出 */
    TIM3_CCER |= TIM_CCER_CC1E;
    
    /* 使能自动重装载预装载 */
    TIM3_CR1 |= TIM_CR1_ARPE;
    
    /* 使能定时器 */
    TIM3_CR1 |= TIM_CR1_CEN;
}

/*============================================================================
 *  PWM 占空比设置
 *===========================================================================*/

static void pwm_set_duty(uint16_t period, uint16_t duty)
{
    if (duty > period) duty = period;
    TIM3_CCR1 = duty;
}

/*============================================================================
 *  打印数字
 *===========================================================================*/

static void print_uint(uint16_t value)
{
    char buf[6];
    int i = 0;
    
    if (value == 0) {
        print_str("0");
        return;
    }
    
    while (value > 0 && i < 5) {
        buf[i++] = '0' + (value % 10);
        value /= 10;
    }
    
    while (i > 0) {
        char c[2] = {buf[--i], '\0'};
        print_str(c);
    }
}

/*============================================================================
 *  主函数
 *===========================================================================*/

int main(void)
{
    uint16_t period = 1000;  /* 1kHz PWM */
    uint16_t duty;
    
    print_str("\n");
    print_str("========================================\n");
    print_str("  XinYi STM32F4 PWM Test!\n");
    print_str("  MCU: STM32F405RG (Cortex-M4F)\n");
    print_str("  TIM3: PA6 (TIM3_CH1)\n");
    print_str("  PWM: 1kHz, Variable Duty\n");
    print_str("========================================\n\n");
    
    /* 初始化 PWM (0% 占空比) */
    print_str("[INIT] Initializing TIM3 PWM...\n");
    pwm_init(period, 0);
    print_str("[INIT] PWM initialized (1kHz).\n\n");
    
    print_str("[TEST] PWM Breathing Effect...\n");
    print_str("[INFO] Duty cycle: 0% → 100% → 0%\n\n");
    
    /* 呼吸灯效果 - 渐变 */
    while (1) {
        /* 渐亮 0% → 100% */
        print_str("[PWM] Fading IN...\n");
        for (duty = 0; duty <= period; duty += 50) {
            pwm_set_duty(period, duty);
            
            print_str("  Duty: ");
            print_uint(duty * 100 / period);
            print_str("%\n");
            
            delay_ms(100);
        }
        
        /* 渐灭 100% → 0% */
        print_str("[PWM] Fading OUT...\n");
        for (duty = period; duty > 0; duty -= 50) {
            pwm_set_duty(period, duty);
            
            print_str("  Duty: ");
            print_uint(duty * 100 / period);
            print_str("%\n");
            
            delay_ms(100);
        }
        
        print_str("\n");
    }
    
    return 0;
}
