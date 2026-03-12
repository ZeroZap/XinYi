/**
 * @file main.c
 * @brief STM32U5 Demo - XinYi Framework
 * @brief STM32U599QI 开发板示例程序
 * 
 * 功能：
 * - LED blink (PB0)
 * - UART2 串口打印
 * - 低功耗模式演示
 */

#include <stdint.h>
#include <string.h>

/* ==================== 硬件定义 ==================== */

/* RCC 寄存器 */
#define RCC_BASE            0x40021000UL
#define RCC_AHB3ENR         (*(volatile uint32_t *)(RCC_BASE + 0x34UL))
#define RCC_AHB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x30UL))
#define RCC_APB1LENR        (*(volatile uint32_t *)(RCC_BASE + 0x38UL))

/* GPIO 寄存器 */
#define GPIOB_BASE          0x48000400UL
#define GPIOB_MODER         (*(volatile uint32_t *)(GPIOB_BASE + 0x00UL))
#define GPIOB_OTYPER        (*(volatile uint32_t *)(GPIOB_BASE + 0x04UL))
#define GPIOB_OSPEEDR       (*(volatile uint32_t *)(GPIOB_BASE + 0x08UL))
#define GPIOB_PUPDR         (*(volatile uint32_t *)(GPIOB_BASE + 0x0CUL))
#define GPIOB_ODR           (*(volatile uint32_t *)(GPIOB_BASE + 0x14UL))
#define GPIOB_BSRR          (*(volatile uint32_t *)(GPIOB_BASE + 0x18UL))

/* USART2 寄存器 */
#define USART2_BASE         0x40008000UL
#define USART2_CR1          (*(volatile uint32_t *)(USART2_BASE + 0x00UL))
#define USART2_CR2          (*(volatile uint32_t *)(USART2_BASE + 0x04UL))
#define USART2_BRR          (*(volatile uint32_t *)(USART2_BASE + 0x0CUL))
#define USART2_ISR          (*(volatile uint32_t *)(USART2_BASE + 0x1CUL))
#define USART2_TDR          (*(volatile uint32_t *)(USART2_BASE + 0x28UL))
#define USART2_RDR          (*(volatile uint32_t *)(USART2_BASE + 0x24UL))

/* 系统配置 */
#define IOPENR_BASE         0x40022090UL
#define IOPENR              (*(volatile uint32_t *)(IOPENR_BASE))

/* LED 引脚定义 (PB0) */
#define LED_PIN             0
#define LED_ON()            (GPIOB_BSRR = (1UL << LED_PIN))
#define LED_OFF()           (GPIOB_BSRR = (1UL << (LED_PIN + 16)))
#define LED_TOGGLE()        (GPIOB_ODR ^= (1UL << LED_PIN))

/* ==================== 函数声明 ==================== */

static void SystemClock_Config(void);
static void GPIO_Init(void);
static void USART2_Init(void);
static void USART2_SendChar(char c);
static void USART2_SendString(const char *str);
static void Delay_ms(volatile uint32_t ms);

/* ==================== 主函数 ==================== */

int main(void)
{
    /* 系统初始化 */
    SystemClock_Config();
    GPIO_Init();
    USART2_Init();
    
    /* 启动信息 */
    USART2_SendString("\r\n");
    USART2_SendString("================================\r\n");
    USART2_SendString("  XinYi Framework - STM32U5 Demo\r\n");
    USART2_SendString("  MCU: STM32U599QI (Cortex-M33)\r\n");
    USART2_SendString("================================\r\n");
    USART2_SendString("\r\n");
    
    uint32_t count = 0;
    char buffer[64];
    
    /* 主循环 */
    while (1) {
        LED_TOGGLE();
        
        /* 发送计数信息 */
        count++;
        
        /* 简单的整数转字符串 */
        const char *msg = "LED Toggle Count: ";
        USART2_SendString(msg);
        
        /* 发送计数值 (简化版) */
        if (count < 10) {
            USART2_SendChar('0' + count);
        } else if (count < 100) {
            USART2_SendChar('0' + count / 10);
            USART2_SendChar('0' + count % 10);
        } else {
            USART2_SendString("99+");
            count = 0;
        }
        
        USART2_SendString("\r\n");
        
        Delay_ms(500);
    }
    
    return 0;
}

/* ==================== 系统初始化 ==================== */

static void SystemClock_Config(void)
{
    /* STM32U5 默认使用 MSI 时钟 (4MHz)
     * 实际应用中应配置 PLL 到更高频率
     * 此处简化处理，使用默认时钟配置
     */
    
    /* 使能 GPIO 和 USART 时钟使能 */
    /* 实际项目中应使用 HAL 库或完整的时钟配置 */
}

/* ==================== GPIO 初始化 ==================== */

static void GPIO_Init(void)
{
    /* 使能 GPIOB 时钟 */
    RCC_AHB1ENR |= (1UL << 1);  /* GPIOBEN */
    
    /* 配置 PB0 为推挽输出 */
    GPIOB_MODER &= ~(3UL << (LED_PIN * 2));
    GPIOB_MODER |= (1UL << (LED_PIN * 2));  /* 输出模式 */
    
    GPIOB_OTYPER &= ~(1UL << LED_PIN);      /* 推挽输出 */
    
    GPIOB_OSPEEDR &= ~(3UL << (LED_PIN * 2));
    GPIOB_OSPEEDR |= (1UL << (LED_PIN * 2)); /* 中速 */
    
    GPIOB_PUPDR &= ~(3UL << (LED_PIN * 2));
    GPIOB_PUPDR |= (1UL << (LED_PIN * 2));   /* 上拉 */
}

/* ==================== USART2 初始化 ==================== */

static void USART2_Init(void)
{
    /* 使能 USART2 时钟 */
    RCC_APB1LENR |= (1UL << 17);  /* USART2EN */
    
    /* 使能 GPIOA 时钟 (TX: PA2, RX: PA3) */
    RCC_AHB1ENR |= (1UL << 0);  /* GPIOAEN */
    
    /* 配置 PA2/PA3 为复用功能 */
    /* TX: PA2 - AF7 (USART2_TX) */
    /* RX: PA3 - AF7 (USART2_RX) */
    GPIOA_BASE = 0x40020000UL;
    volatile uint32_t *gpioa_moder = (volatile uint32_t *)(GPIOA_BASE + 0x00);
    volatile uint32_t *gpioa_afrl = (volatile uint32_t *)(GPIOA_BASE + 0x20);
    
    /* PA2/PA3 复用模式 */
    *gpioa_moder &= ~((3UL << 4) | (3UL << 6));
    *gpioa_moder |= ((2UL << 4) | (2UL << 6));
    
    /* AF7 选择 */
    *gpioa_afrl &= ~((0xFUL << 8) | (0xFUL << 12));
    *gpioa_afrl |= ((7UL << 8) | (7UL << 12));
    
    /* 配置 USART2: 115200 8N1
     * 假设 PCLK1 = 4MHz, BRR = 4000000 / 115200 ≈ 34.72
     */
    USART2_CR1 &= ~(1UL << 0);  /* UE = 0, 禁用 USART */
    
    USART2_BRR = 35;            /* 波特率设置 */
    
    USART2_CR1 = (1UL << 3) |   /* TE = 1, 发送使能 */
                 (1UL << 2) |   /* RE = 1, 接收使能 */
                 (1UL << 0);    /* UE = 1, 使能 USART */
}

/* ==================== USART2 发送函数 ==================== */

static void USART2_SendChar(char c)
{
    /* 等待发送寄存器为空 */
    while (!(USART2_ISR & (1UL << 7))) {
        /* TXE = 1 表示可以发送 */
    }
    
    USART2_TDR = (uint32_t)c;
}

static void USART2_SendString(const char *str)
{
    while (*str) {
        USART2_SendChar(*str++);
    }
}

/* ==================== 延时函数 ==================== */

static void Delay_ms(volatile uint32_t ms)
{
    /* 简化的软件延时
     * 实际应用中应使用 SysTick 定时器
     */
    volatile uint32_t count = ms * 4000;  /* 近似值，4MHz 时钟 */
    while (count--) {
        __asm__ volatile ("nop");
    }
}
