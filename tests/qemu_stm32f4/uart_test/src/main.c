/**
 * @file main.c
 * @brief STM32F405 UART 测试程序
 * 
 * QEMU olimex-stm32-h405 开发板 UART 测试
 * - USART1 串口输出 (半主机 + 寄存器)
 * - 回显测试
 */

#include <stdint.h>

/* 简单的字符串函数实现 (避免依赖 libc) */
static int my_strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

static uint16_t my_strlen(const char *str)
{
    uint16_t len = 0;
    while (*str++) len++;
    return len;
}

/*============================================================================
 *  半主机调试输出 (ARM Semihosting)
 *===========================================================================*/

#define SYS_WRITE0  0x04
#define SYS_READ0   0x06

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

/* RCC - 复位和时钟控制 */
#define RCC_BASE            0x40023800UL
#define RCC_APB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0x44))
#define RCC_APB2ENR_USART1EN (1 << 4)  /* USART1 时钟使能 */
#define RCC_AHB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x30))
#define RCC_AHB1ENR_GPIOAEN (1 << 0)   /* GPIOA 时钟使能 */

/* GPIOA */
#define GPIOA_BASE          0x40020000UL
#define GPIOA_MODER         (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_AFRL          (*(volatile uint32_t *)(GPIOA_BASE + 0x20))
#define GPIOA_AFRH          (*(volatile uint32_t *)(GPIOA_BASE + 0x24))

/* USART1 */
#define USART1_BASE         0x40011000UL
#define USART1_SR           (*(volatile uint32_t *)(USART1_BASE + 0x00))
#define USART1_DR           (*(volatile uint32_t *)(USART1_BASE + 0x04))
#define USART1_BRR          (*(volatile uint32_t *)(USART1_BASE + 0x08))
#define USART1_CR1          (*(volatile uint32_t *)(USART1_BASE + 0x0C))
#define USART1_CR2          (*(volatile uint32_t *)(USART1_BASE + 0x10))
#define USART1_CR3          (*(volatile uint32_t *)(USART1_BASE + 0x14))

/* USART 状态位 */
#define USART_SR_TXE        (1 << 7)   /* 发送数据寄存器空 */
#define USART_SR_TC         (1 << 6)   /* 发送完成 */
#define USART_SR_RXNE       (1 << 5)   /* 读数据寄存器非空 */

/* USART 控制位 */
#define USART_CR1_UE        (1 << 13)  /* USART 使能 */
#define USART_CR1_TE        (1 << 3)   /* 发送使能 */
#define USART_CR1_RE        (1 << 2)   /* 接收使能 */

/* GPIO 复用功能 */
#define GPIO_AF_USART1      7          /* PA9/PA10 复用为 USART1 */

/*============================================================================
 *  延时函数
 *===========================================================================*/

static void delay_ms(uint32_t ms)
{
    volatile uint32_t count = ms * 15000;
    while (count--);
}

/*============================================================================
 *  UART 初始化
 *===========================================================================*/

static void uart_init(uint32_t baudrate)
{
    /* 1. 使能 GPIOA 和 USART1 时钟 */
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC_APB2ENR |= RCC_APB2ENR_USART1EN;
    
    /* 2. 配置 PA9 (TX) 和 PA10 (RX) 为复用功能 */
    /* MODER: 10 = 复用模式 (bits 18:19 for PA9, bits 20:21 for PA10) */
    GPIOA_MODER &= ~((0x3 << 18) | (0x3 << 20));
    GPIOA_MODER |= ((0x2 << 18) | (0x2 << 20));
    
    /* AFRH: 设置 PA9/PA10 为 AF7 (USART1) */
    /* PA9: bits 4:7, PA10: bits 8:11 */
    GPIOA_AFRH &= ~((0xF << 4) | (0xF << 8));
    GPIOA_AFRH |= ((GPIO_AF_USART1 << 4) | (GPIO_AF_USART1 << 8));
    
    /* 3. 配置 USART1 */
    /* 禁用 USART 进行配置 */
    USART1_CR1 &= ~USART_CR1_UE;
    
    /* 设置波特率 - QEMU STM32F4 默认 16MHz */
    /* BRR = f_clk / baudrate = 16000000 / 115200 ≈ 139 (0x8B) */
    uint32_t brr = 16000000 / baudrate;
    USART1_BRR = brr;
    
    /* 使能 TX 和 RX */
    USART1_CR1 |= USART_CR1_TE | USART_CR1_RE;
    
    /* 使能 USART */
    USART1_CR1 |= USART_CR1_UE;
    
    /* 等待发送完成 */
    while (!(USART1_SR & USART_SR_TC));
}

/*============================================================================
 *  UART 收发函数
 *===========================================================================*/

static void uart_putc(char c)
{
    /* 等待 TXE */
    while (!(USART1_SR & USART_SR_TXE));
    
    /* 发送数据 */
    USART1_DR = (uint32_t)c;
    
    /* 等待 TC */
    while (!(USART1_SR & USART_SR_TC));
}

static char uart_getc(void)
{
    /* 等待 RXNE */
    while (!(USART1_SR & USART_SR_RXNE));
    
    /* 读取数据 */
    return (char)(USART1_DR & 0xFF);
}

static void uart_puts(const char *str)
{
    while (*str) {
        uart_putc(*str++);
    }
}

static void uart_read_line(char *buf, uint16_t max_len)
{
    uint16_t i = 0;
    char c;
    
    while (i < max_len - 1) {
        c = uart_getc();
        uart_putc(c);  /* 回显 */
        
        if (c == '\r' || c == '\n') {
            uart_putc('\n');
            break;
        }
        
        buf[i++] = c;
    }
    buf[i] = '\0';
}

/*============================================================================
 *  主函数
 *===========================================================================*/

int main(void)
{
    char rx_buf[64];
    
    print_str("\n");
    print_str("========================================\n");
    print_str("  XinYi STM32F4 UART Test!\n");
    print_str("  MCU: STM32F405RG (Cortex-M4F)\n");
    print_str("  USART1 @ 115200 8N1\n");
    print_str("========================================\n\n");
    
    /* 初始化 UART */
    print_str("[INIT] Initializing USART1 (PA9/PA10)...\n");
    uart_init(115200);
    print_str("[INIT] UART initialized.\n\n");
    
    print_str("[TEST] UART Echo Test\n");
    print_str("[TEST] Type something and press Enter:\n\n");
    
    /* 回显测试 */
    while (1) {
        print_str("> ");
        uart_puts("> ");
        
        /* 读取一行 */
        uart_read_line(rx_buf, sizeof(rx_buf));
        
        /* 处理命令 */
        if (my_strcmp(rx_buf, "help") == 0) {
            print_str("\n[CMD] Available commands:\n");
            uart_puts("\n[CMD] Available commands:\n");
            print_str("  help  - Show this help\n");
            uart_puts("  help  - Show this help\n");
            print_str("  hello - Say hello\n");
            uart_puts("  hello - Say hello\n");
            print_str("  test  - Run UART test\n");
            uart_puts("  test  - Run UART test\n");
            print_str("\n");
            uart_puts("\n");
        } else if (my_strcmp(rx_buf, "hello") == 0) {
            print_str("\n[CMD] Hello from STM32F405!\n");
            uart_puts("\n[CMD] Hello from STM32F405!\n");
            print_str("\n");
            uart_puts("\n");
        } else if (my_strcmp(rx_buf, "test") == 0) {
            print_str("\n[TEST] Running UART loopback test...\n");
            uart_puts("\n[TEST] Running UART loopback test...\n");
            for (int i = 0; i < 10; i++) {
                char test_char = 'A' + i;
                uart_putc(test_char);
                delay_ms(100);
            }
            print_str("\n[TEST] Complete!\n\n");
            uart_puts("\n[TEST] Complete!\n\n");
        } else if (my_strlen(rx_buf) > 0) {
            /* 未知命令 - 回显 */
            print_str("\n[ECHO] You typed: ");
            uart_puts("\n[ECHO] You typed: ");
            uart_puts(rx_buf);
            print_str("\n\n");
            uart_puts("\n\n");
        }
    }
    
    return 0;
}
