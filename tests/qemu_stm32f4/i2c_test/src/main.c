/**
 * @file main.c
 * @brief STM32F405 I2C 测试程序
 * 
 * QEMU olimex-stm32-h405 开发板 I2C 测试
 * - I2C1 主机模式
 * - PB6 (SCL), PB7 (SDA)
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
#define RCC_APB1ENR_I2C1EN  (1 << 21)  /* I2C1 时钟使能 */
#define RCC_AHB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x30))
#define RCC_AHB1ENR_GPIOBEN (1 << 1)   /* GPIOB 时钟使能 */

/* GPIOB */
#define GPIOB_BASE          0x40020400UL
#define GPIOB_MODER         (*(volatile uint32_t *)(GPIOB_BASE + 0x00))
#define GPIOB_AFRL          (*(volatile uint32_t *)(GPIOB_BASE + 0x20))
#define GPIOB_OTYPER        (*(volatile uint32_t *)(GPIOB_BASE + 0x04))
#define GPIOB_OSPEEDR       (*(volatile uint32_t *)(GPIOB_BASE + 0x08))
#define GPIOB_PUPDR         (*(volatile uint32_t *)(GPIOB_BASE + 0x0C))

/* I2C1 */
#define I2C1_BASE           0x40005400UL
#define I2C1_CR1            (*(volatile uint32_t *)(I2C1_BASE + 0x00))
#define I2C1_CR2            (*(volatile uint32_t *)(I2C1_BASE + 0x04))
#define I2C1_OAR1           (*(volatile uint32_t *)(I2C1_BASE + 0x08))
#define I2C1_DR             (*(volatile uint32_t *)(I2C1_BASE + 0x10))
#define I2C1_SR1            (*(volatile uint32_t *)(I2C1_BASE + 0x14))
#define I2C1_SR2            (*(volatile uint32_t *)(I2C1_BASE + 0x18))
#define I2C1_CCR            (*(volatile uint32_t *)(I2C1_BASE + 0x1C))
#define I2C1_TRISE          (*(volatile uint32_t *)(I2C1_BASE + 0x20))

/* I2C 控制位 */
#define I2C_CR1_PE          (1 << 0)   /* I2C 使能 */
#define I2C_CR1_START       (1 << 8)   /* 启动条件 */
#define I2C_CR1_STOP        (1 << 9)   /* 停止条件 */
#define I2C_CR1_ACK         (1 << 10)  /* 应答使能 */

#define I2C_SR1_SB          (1 << 0)   /* 启动完成 */
#define I2C_SR1_ADDR        (1 << 1)   /* 地址匹配 */
#define I2C_SR1_TXE         (1 << 7)   /* 发送缓冲区空 */
#define I2C_SR1_RXNE        (1 << 6)   /* 接收缓冲区非空 */
#define I2C_SR1_BTF         (1 << 2)   /* 字节传输完成 */

#define I2C_SR2_MSL         (1 << 0)   /* 主机模式 */
#define I2C_SR2_BUSY        (1 << 1)   /* 总线忙 */

#define I2C_CR2_FREQ        0x10       /* 外设输入频率 16MHz */
#define I2C_CCR_CCR         0x50       /* 时钟控制 */
#define I2C_TRISE_VALUE     0x11       /* 最大上升时间 */

/* GPIO 复用 */
#define GPIO_AF_I2C1        4          /* PB6/PB7 复用为 I2C1 */

/*============================================================================
 *  延时函数
 *===========================================================================*/

static void delay_ms(uint32_t ms)
{
    volatile uint32_t count = ms * 15000;
    while (count--);
}

static void delay_us(uint32_t us)
{
    volatile uint32_t count = us * 15;
    while (count--);
}

/*============================================================================
 *  I2C 初始化
 *===========================================================================*/

static void i2c_init(void)
{
    /* 1. 使能 GPIOB 和 I2C1 时钟 */
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC_APB1ENR |= RCC_APB1ENR_I2C1EN;
    
    /* 2. 配置 I2C 引脚为开漏复用功能 */
    /* PB6 (SCL), PB7 (SDA) */
    GPIOB_MODER &= ~((0x3 << 12) | (0x3 << 14));
    GPIOB_MODER |= ((0x2 << 12) | (0x2 << 14));
    
    /* 开漏输出 */
    GPIOB_OTYPER |= ((1 << 6) | (1 << 7));
    
    /* 高速 */
    GPIOB_OSPEEDR |= ((0x3 << 12) | (0x3 << 14));
    
    /* 上拉 */
    GPIOB_PUPDR &= ~((0x3 << 12) | (0x3 << 14));
    GPIOB_PUPDR |= ((0x1 << 12) | (0x1 << 14));
    
    /* 配置为 AF4 (I2C1) */
    GPIOB_AFRL &= ~((0xF << 24) | (0xF << 28));
    GPIOB_AFRL |= ((GPIO_AF_I2C1 << 24) | (GPIO_AF_I2C1 << 28));
    
    /* 3. 配置 I2C1 */
    /* 禁用 I2C */
    I2C1_CR1 &= ~I2C_CR1_PE;
    
    /* 配置时钟频率 */
    I2C1_CR2 = I2C_CR2_FREQ;
    
    /* 配置时钟控制 (100kHz) */
    I2C1_CCR = I2C_CCR_CCR;
    
    /* 配置上升时间 */
    I2C1_TRISE = I2C_TRISE_VALUE;
    
    /* 使能 I2C */
    I2C1_CR1 |= I2C_CR1_PE;
    
    /* 使能应答 */
    I2C1_CR1 |= I2C_CR1_ACK;
}

/*============================================================================
 *  I2C 读写函数
 *===========================================================================*/

static int i2c_start(void)
{
    /* 发送 START */
    I2C1_CR1 |= I2C_CR1_START;
    
    /* 等待 SB 标志 */
    uint32_t timeout = 10000;
    while (!(I2C1_SR1 & I2C_SR1_SB) && timeout--);
    
    return (timeout > 0) ? 0 : -1;
}

static int i2c_stop(void)
{
    /* 发送 STOP */
    I2C1_CR1 |= I2C_CR1_STOP;
    
    /* 等待总线空闲 */
    uint32_t timeout = 10000;
    while ((I2C1_SR2 & I2C_SR2_BUSY) && timeout--);
    
    return (timeout > 0) ? 0 : -1;
}

static int i2c_write_byte(uint8_t addr, uint8_t data)
{
    /* 发送从机地址 (写) */
    I2C1_DR = (addr << 1) & 0xFE;
    
    /* 等待 ADDR */
    uint32_t timeout = 10000;
    while (!(I2C1_SR1 & I2C_SR1_ADDR) && timeout--);
    
    if (timeout == 0) return -1;
    
    /* 清除 ADDR 标志 (读 SR1 然后读 SR2) */
    (void)I2C1_SR1;
    (void)I2C1_SR2;
    
    /* 等待 TXE */
    timeout = 10000;
    while (!(I2C1_SR1 & I2C_SR1_TXE) && timeout--);
    
    if (timeout == 0) return -1;
    
    /* 发送数据 */
    I2C1_DR = data;
    
    /* 等待 BTF */
    timeout = 10000;
    while (!(I2C1_SR1 & I2C_SR1_BTF) && timeout--);
    
    return (timeout > 0) ? 0 : -1;
}

static void i2c_scan_bus(void)
{
    uint8_t devices_found = 0;
    
    print_str("[SCAN] Scanning I2C bus (0x01-0x7F)...\n");
    
    for (uint8_t addr = 0x01; addr <= 0x7F; addr++) {
        /* 发送 START */
        I2C1_CR1 |= I2C_CR1_START;
        
        /* 等待 SB */
        uint32_t timeout = 1000;
        while (!(I2C1_SR1 & I2C_SR1_SB) && timeout--);
        
        if (timeout > 0) {
            /* 发送地址 */
            I2C1_DR = (addr << 1) & 0xFE;
            
            /* 等待 ADDR 或超时 */
            timeout = 1000;
            while (!(I2C1_SR1 & I2C_SR1_ADDR) && !(I2C1_SR1 & 0x0400) /* AF */ && timeout--);
            
            if (I2C1_SR1 & I2C_SR1_ADDR) {
                /* 设备响应 */
                print_str("[SCAN] Device found at 0x");
                char hex[3];
                hex[0] = (addr >> 4) > 9 ? 'A' + (addr >> 4) - 10 : '0' + (addr >> 4);
                hex[1] = (addr & 0x0F) > 9 ? 'A' + (addr & 0x0F) - 10 : '0' + (addr & 0x0F);
                hex[2] = '\0';
                print_str(hex);
                print_str("\n");
                devices_found++;
                
                /* 清除 ADDR */
                (void)I2C1_SR1;
                (void)I2C1_SR2;
            }
            
            /* 发送 STOP */
            I2C1_CR1 |= I2C_CR1_STOP;
            delay_us(100);
        }
    }
    
    if (devices_found == 0) {
        print_str("[SCAN] No devices found.\n");
        print_str("[INFO] QEMU I2C bus has no slave devices.\n");
    }
}

/*============================================================================
 *  主函数
 *===========================================================================*/

int main(void)
{
    print_str("\n");
    print_str("========================================\n");
    print_str("  XinYi STM32F4 I2C Test!\n");
    print_str("  MCU: STM32F405RG (Cortex-M4F)\n");
    print_str("  I2C1: PB6(SCL)/PB7(SDA)\n");
    print_str("  Mode: Master, 100kHz\n");
    print_str("========================================\n\n");
    
    /* 初始化 I2C */
    print_str("[INIT] Initializing I2C1...\n");
    i2c_init();
    print_str("[INIT] I2C initialized.\n\n");
    
    /* 扫描总线 */
    i2c_scan_bus();
    
    print_str("\n[TEST] I2C Write Test (no slave)...\n");
    
    /* 尝试写入 (无从机) */
    int result = i2c_write_byte(0x50, 0xAA);
    
    if (result == 0) {
        print_str("[WRITE] Success (0x50: 0xAA)\n");
    } else {
        print_str("[WRITE] Timeout/No ACK (expected in QEMU)\n");
    }
    
    i2c_stop();
    
    print_str("\n[INFO] I2C bus ready for slave devices.\n");
    print_str("[LOOP] Idle...\n");
    
    while (1) {
        __asm__("nop");
    }
    
    return 0;
}
