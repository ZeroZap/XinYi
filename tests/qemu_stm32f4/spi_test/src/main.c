/**
 * @file main.c
 * @brief STM32F405 SPI 测试程序
 * 
 * QEMU olimex-stm32-h405 开发板 SPI 测试
 * - SPI1 主机模式
 * - PA5 (SCK), PA6 (MISO), PA7 (MOSI)
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
#define RCC_APB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0x44))
#define RCC_APB2ENR_SPI1EN  (1 << 12)  /* SPI1 时钟使能 */
#define RCC_AHB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x30))
#define RCC_AHB1ENR_GPIOAEN (1 << 0)   /* GPIOA 时钟使能 */

/* GPIOA */
#define GPIOA_BASE          0x40020000UL
#define GPIOA_MODER         (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_AFRL          (*(volatile uint32_t *)(GPIOA_BASE + 0x20))
#define GPIOA_OSPEEDR       (*(volatile uint32_t *)(GPIOA_BASE + 0x08))

/* SPI1 */
#define SPI1_BASE           0x40013000UL
#define SPI1_CR1            (*(volatile uint32_t *)(SPI1_BASE + 0x00))
#define SPI1_CR2            (*(volatile uint32_t *)(SPI1_BASE + 0x04))
#define SPI1_SR             (*(volatile uint32_t *)(SPI1_BASE + 0x08))
#define SPI1_DR             (*(volatile uint32_t *)(SPI1_BASE + 0x0C))
#define SPI1_CRCPR          (*(volatile uint32_t *)(SPI1_BASE + 0x10))

/* SPI 控制位 */
#define SPI_CR1_SPE         (1 << 6)   /* SPI 使能 */
#define SPI_CR1_MSTR        (1 << 2)   /* 主机模式 */
#define SPI_CR1_BR          (7 << 3)   /* 波特率预分频 */
#define SPI_CR1_SSM         (1 << 9)   /* 软件 NSS 管理 */
#define SPI_CR1_SSI         (1 << 8)   /* 内部 NSS */

#define SPI_SR_TXE          (1 << 1)   /* 发送缓冲区空 */
#define SPI_SR_RXNE         (1 << 0)   /* 接收缓冲区非空 */
#define SPI_SR_BSY          (1 << 7)   /* 忙标志 */

/* GPIO 复用 */
#define GPIO_AF_SPI1        5          /* PA5/PA6/PA7 复用为 SPI1 */

/*============================================================================
 *  延时函数
 *===========================================================================*/

static void delay_ms(uint32_t ms)
{
    volatile uint32_t count = ms * 15000;
    while (count--);
}

/*============================================================================
 *  SPI 初始化
 *===========================================================================*/

static void spi_init(void)
{
    /* 1. 使能 GPIOA 和 SPI1 时钟 */
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC_APB2ENR |= RCC_APB2ENR_SPI1EN;
    
    /* 2. 配置 SPI 引脚为复用功能 */
    /* PA5 (SCK), PA6 (MISO), PA7 (MOSI) - 复用模式 */
    GPIOA_MODER &= ~((0x3 << 10) | (0x3 << 12) | (0x3 << 14));
    GPIOA_MODER |= ((0x2 << 10) | (0x2 << 12) | (0x2 << 14));
    
    /* 配置为 AF5 (SPI1) */
    GPIOA_AFRL &= ~((0xF << 20) | (0xF << 24) | (0xF << 28));
    GPIOA_AFRL |= ((GPIO_AF_SPI1 << 20) | (GPIO_AF_SPI1 << 24) | (GPIO_AF_SPI1 << 28));
    
    /* 高速 */
    GPIOA_OSPEEDR |= ((0x3 << 10) | (0x3 << 12) | (0x3 << 14));
    
    /* 3. 配置 SPI1 */
    /* 禁用 SPI */
    SPI1_CR1 &= ~SPI_CR1_SPE;
    
    /* 配置：主机模式，软件 NSS，波特率预分频 */
    SPI1_CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR;
    
    /* 8 位数据帧 */
    SPI1_CR2 = 0;
    
    /* 使能 SPI */
    SPI1_CR1 |= SPI_CR1_SPE;
}

/*============================================================================
 *  SPI 收发函数
 *===========================================================================*/

static uint8_t spi_transfer(uint8_t data)
{
    /* 等待 TXE */
    while (!(SPI1_SR & SPI_SR_TXE));
    
    /* 发送数据 */
    SPI1_DR = data;
    
    /* 等待接收完成 */
    while (!(SPI1_SR & SPI_SR_RXNE));
    
    /* 等待空闲 */
    while (SPI1_SR & SPI_SR_BSY);
    
    /* 读取数据 */
    return (uint8_t)(SPI1_DR & 0xFF);
}

/*============================================================================
 *  主函数
 *===========================================================================*/

int main(void)
{
    uint8_t tx_data, rx_data;
    
    print_str("\n");
    print_str("========================================\n");
    print_str("  XinYi STM32F4 SPI Test!\n");
    print_str("  MCU: STM32F405RG (Cortex-M4F)\n");
    print_str("  SPI1: PA5(SCK)/PA6(MISO)/PA7(MOSI)\n");
    print_str("  Mode: Master, 8-bit, ~1MHz\n");
    print_str("========================================\n\n");
    
    /* 初始化 SPI */
    print_str("[INIT] Initializing SPI1...\n");
    spi_init();
    print_str("[INIT] SPI initialized.\n\n");
    
    print_str("[TEST] SPI Loopback Test\n");
    print_str("[INFO] Sending 0x00-0x0F pattern...\n\n");
    
    /* SPI 数据传输测试 */
    for (uint8_t i = 0; i < 16; i++) {
        tx_data = i;
        rx_data = spi_transfer(tx_data);
        
        /* 输出结果 */
        print_str("[SPI] TX: 0x");
        /* 输出十六进制 */
        char hex[3];
        hex[0] = (tx_data >> 4) > 9 ? 'A' + (tx_data >> 4) - 10 : '0' + (tx_data >> 4);
        hex[1] = (tx_data & 0x0F) > 9 ? 'A' + (tx_data & 0x0F) - 10 : '0' + (tx_data & 0x0F);
        hex[2] = '\0';
        print_str(hex);
        
        print_str(" RX: 0x");
        hex[0] = (rx_data >> 4) > 9 ? 'A' + (rx_data >> 4) - 10 : '0' + (rx_data >> 4);
        hex[1] = (rx_data & 0x0F) > 9 ? 'A' + (rx_data & 0x0F) - 10 : '0' + (rx_data & 0x0F);
        hex[2] = '\0';
        print_str(hex);
        
        if (tx_data == rx_data) {
            print_str(" ✓\n");
        } else {
            print_str(" ✗ MISMATCH!\n");
        }
        
        delay_ms(100);
    }
    
    print_str("\n[TEST] Complete!\n");
    print_str("[INFO] In QEMU, MISO is not connected.\n");
    print_str("[INFO] RX shows 0x00 or 0xFF (floating).\n\n");
    
    /* 持续测试 */
    print_str("[LOOP] Continuous SPI transfer...\n");
    uint8_t pattern = 0xAA;
    while (1) {
        rx_data = spi_transfer(pattern);
        pattern = ~pattern;  /* 翻转模式 */
        delay_ms(500);
    }
    
    return 0;
}
