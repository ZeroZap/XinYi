/**
 * @file main.c
 * @brief STM32F405 I2C EEPROM 测试程序
 * 
 * QEMU olimex-stm32-h405 开发板 I2C EEPROM 测试
 * - 24C256 EEPROM (32KB)
 * - I2C1 总线 (PB6/PB7)
 * - 设备地址：0x50
 */

#include <stdint.h>

/* 简单的内存比较函数 (避免依赖 libc) */
static int my_memcmp(const void *s1, const void *s2, uint32_t n);

/* 字符串长度 */
static uint32_t my_strlen(const char *str)
{
    uint32_t len = 0;
    while (*str++) len++;
    return len;
}

static int my_memcmp(const void *s1, const void *s2, uint32_t n)
{
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;
    
    for (uint32_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }
    return 0;
}

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
#define RCC_APB1ENR_I2C1EN  (1 << 21)
#define RCC_AHB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x30))
#define RCC_AHB1ENR_GPIOBEN (1 << 1)

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
#define I2C_CR1_PE          (1 << 0)
#define I2C_CR1_START       (1 << 8)
#define I2C_CR1_STOP        (1 << 9)
#define I2C_CR1_ACK         (1 << 10)

#define I2C_SR1_SB          (1 << 0)
#define I2C_SR1_ADDR        (1 << 1)
#define I2C_SR1_TXE         (1 << 7)
#define I2C_SR1_RXNE        (1 << 6)
#define I2C_SR1_BTF         (1 << 2)
#define I2C_SR1_AF          (1 << 10)

#define I2C_SR2_MSL         (1 << 0)
#define I2C_SR2_BUSY        (1 << 1)

#define I2C_CR2_FREQ        0x10
#define I2C_CCR_CCR         0x50
#define I2C_TRISE_VALUE     0x11

/* GPIO */
#define GPIO_AF_I2C1        4

/* EEPROM 24C256 */
#define EEPROM_ADDR         0x50
#define EEPROM_SIZE         32768
#define EEPROM_PAGE_SIZE    64

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
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC_APB1ENR |= RCC_APB1ENR_I2C1EN;
    
    GPIOB_MODER &= ~((0x3 << 12) | (0x3 << 14));
    GPIOB_MODER |= ((0x2 << 12) | (0x2 << 14));
    
    GPIOB_OTYPER |= ((1 << 6) | (1 << 7));
    GPIOB_OSPEEDR |= ((0x3 << 12) | (0x3 << 14));
    GPIOB_PUPDR &= ~((0x3 << 12) | (0x3 << 14));
    GPIOB_PUPDR |= ((0x1 << 12) | (0x1 << 14));
    
    GPIOB_AFRL &= ~((0xF << 24) | (0xF << 28));
    GPIOB_AFRL |= ((GPIO_AF_I2C1 << 24) | (GPIO_AF_I2C1 << 28));
    
    I2C1_CR1 &= ~I2C_CR1_PE;
    I2C1_CR2 = I2C_CR2_FREQ;
    I2C1_CCR = I2C_CCR_CCR;
    I2C1_TRISE = I2C_TRISE_VALUE;
    I2C1_CR1 |= I2C_CR1_PE;
    I2C1_CR1 |= I2C_CR1_ACK;
}

/*============================================================================
 *  I2C 底层函数
 *===========================================================================*/

static int i2c_start(void)
{
    I2C1_CR1 |= I2C_CR1_START;
    
    uint32_t timeout = 10000;
    while (!(I2C1_SR1 & I2C_SR1_SB) && timeout--);
    
    return (timeout > 0) ? 0 : -1;
}

static int i2c_stop(void)
{
    I2C1_CR1 |= I2C_CR1_STOP;
    
    uint32_t timeout = 10000;
    while ((I2C1_SR2 & I2C_SR2_BUSY) && timeout--);
    
    delay_us(100);
    return (timeout > 0) ? 0 : -1;
}

static int i2c_write_byte(uint8_t data)
{
    I2C1_DR = data;
    
    uint32_t timeout = 10000;
    while (!(I2C1_SR1 & I2C_SR1_TXE) && timeout--);
    
    return (timeout > 0) ? 0 : -1;
}

static uint8_t i2c_read_byte_nack(void)
{
    I2C1_CR1 &= ~I2C_CR1_ACK;
    
    uint32_t timeout = 10000;
    while (!(I2C1_SR1 & I2C_SR1_RXNE) && timeout--);
    
    return (uint8_t)(I2C1_DR & 0xFF);
}

static int i2c_wait_addr(void)
{
    uint32_t timeout = 10000;
    while (!(I2C1_SR1 & I2C_SR1_ADDR) && !(I2C1_SR1 & I2C_SR1_AF) && timeout--);
    
    if (I2C1_SR1 & I2C_SR1_AF) {
        I2C1_SR1 &= ~I2C_SR1_AF;
        return -1;
    }
    
    if (timeout > 0) {
        (void)I2C1_SR1;
        (void)I2C1_SR2;
        return 0;
    }
    
    return -1;
}

/*============================================================================
 *  EEPROM 操作函数
 *===========================================================================*/

static int eeprom_write_byte(uint16_t addr, uint8_t data)
{
    if (i2c_start() != 0) return -1;
    
    if (i2c_write_byte((EEPROM_ADDR << 1) & 0xFE) != 0) {
        i2c_stop();
        return -1;
    }
    
    if (i2c_wait_addr() != 0) {
        i2c_stop();
        return -1;
    }
    
    if (i2c_write_byte((addr >> 8) & 0xFF) != 0) {
        i2c_stop();
        return -1;
    }
    
    if (i2c_write_byte(addr & 0xFF) != 0) {
        i2c_stop();
        return -1;
    }
    
    if (i2c_write_byte(data) != 0) {
        i2c_stop();
        return -1;
    }
    
    i2c_stop();
    
    delay_ms(10);
    
    return 0;
}

static int eeprom_read_byte(uint16_t addr, uint8_t *data)
{
    if (i2c_start() != 0) return -1;
    
    if (i2c_write_byte((EEPROM_ADDR << 1) & 0xFE) != 0) {
        i2c_stop();
        return -1;
    }
    
    if (i2c_wait_addr() != 0) {
        i2c_stop();
        return -1;
    }
    
    if (i2c_write_byte((addr >> 8) & 0xFF) != 0) {
        i2c_stop();
        return -1;
    }
    
    if (i2c_write_byte(addr & 0xFF) != 0) {
        i2c_stop();
        return -1;
    }
    
    if (i2c_start() != 0) return -1;
    
    if (i2c_write_byte((EEPROM_ADDR << 1) | 0x01) != 0) {
        i2c_stop();
        return -1;
    }
    
    if (i2c_wait_addr() != 0) {
        i2c_stop();
        return -1;
    }
    
    *data = i2c_read_byte_nack();
    
    i2c_stop();
    
    return 0;
}

static int eeprom_write_page(uint16_t addr, const uint8_t *data, uint8_t len)
{
    if (len > EEPROM_PAGE_SIZE) len = EEPROM_PAGE_SIZE;
    
    if (i2c_start() != 0) return -1;
    
    if (i2c_write_byte((EEPROM_ADDR << 1) & 0xFE) != 0) {
        i2c_stop();
        return -1;
    }
    
    if (i2c_wait_addr() != 0) {
        i2c_stop();
        return -1;
    }
    
    if (i2c_write_byte((addr >> 8) & 0xFF) != 0) {
        i2c_stop();
        return -1;
    }
    
    if (i2c_write_byte(addr & 0xFF) != 0) {
        i2c_stop();
        return -1;
    }
    
    for (uint8_t i = 0; i < len; i++) {
        if (i2c_write_byte(data[i]) != 0) {
            i2c_stop();
            return -1;
        }
    }
    
    i2c_stop();
    
    delay_ms(10);
    
    return 0;
}

static int eeprom_read_block(uint16_t addr, uint8_t *data, uint16_t len)
{
    if (i2c_start() != 0) return -1;
    
    if (i2c_write_byte((EEPROM_ADDR << 1) & 0xFE) != 0) {
        i2c_stop();
        return -1;
    }
    
    if (i2c_wait_addr() != 0) {
        i2c_stop();
        return -1;
    }
    
    if (i2c_write_byte((addr >> 8) & 0xFF) != 0) {
        i2c_stop();
        return -1;
    }
    
    if (i2c_write_byte(addr & 0xFF) != 0) {
        i2c_stop();
        return -1;
    }
    
    if (i2c_start() != 0) return -1;
    
    if (i2c_write_byte((EEPROM_ADDR << 1) | 0x01) != 0) {
        i2c_stop();
        return -1;
    }
    
    if (i2c_wait_addr() != 0) {
        i2c_stop();
        return -1;
    }
    
    I2C1_CR1 |= I2C_CR1_ACK;
    
    for (uint16_t i = 0; i < len - 1; i++) {
        uint32_t timeout = 10000;
        while (!(I2C1_SR1 & I2C_SR1_RXNE) && timeout--);
        if (timeout == 0) {
            i2c_stop();
            return -1;
        }
        data[i] = (uint8_t)(I2C1_DR & 0xFF);
    }
    
    I2C1_CR1 &= ~I2C_CR1_ACK;
    
    uint32_t timeout = 10000;
    while (!(I2C1_SR1 & I2C_SR1_RXNE) && timeout--);
    if (timeout > 0) {
        data[len - 1] = (uint8_t)(I2C1_DR & 0xFF);
    }
    
    i2c_stop();
    
    return 0;
}

/*============================================================================
 *  辅助函数
 *===========================================================================*/

static void print_hex(uint8_t value)
{
    char hex[3];
    hex[0] = (value >> 4) > 9 ? 'A' + (value >> 4) - 10 : '0' + (value >> 4);
    hex[1] = (value & 0x0F) > 9 ? 'A' + (value & 0x0F) - 10 : '0' + (value & 0x0F);
    hex[2] = '\0';
    print_str(hex);
}

static void print_uint16(uint16_t value)
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
    uint8_t write_data[] = "Hello EEPROM! This is a test message from XinYi HAL.";
    uint8_t read_data[64];
    uint8_t verify_byte;
    int errors = 0;
    
    print_str("\n");
    print_str("========================================\n");
    print_str("  XinYi STM32F4 I2C EEPROM Test!\n");
    print_str("  MCU: STM32F405RG (Cortex-M4F)\n");
    print_str("  EEPROM: 24C256 (32KB)\n");
    print_str("  Interface: I2C1 (PB6/PB7)\n");
    print_str("  Address: 0x50\n");
    print_str("========================================\n\n");
    
    print_str("[INIT] Initializing I2C1...\n");
    i2c_init();
    print_str("[INIT] I2C initialized.\n\n");
    
    print_str("[TEST 1] Single Byte Write/Read\n");
    print_str("----------------------------------------\n");
    
    uint16_t test_addr = 0x0000;
    uint8_t test_byte = 0xA5;
    
    print_str("Writing 0x");
    print_hex(test_byte);
    print_str(" to address 0x");
    print_hex((test_addr >> 8) & 0xFF);
    print_hex(test_addr & 0xFF);
    print_str("... ");
    
    if (eeprom_write_byte(test_addr, test_byte) == 0) {
        print_str("OK\n");
    } else {
        print_str("FAIL\n");
        errors++;
    }
    
    print_str("Reading from address 0x");
    print_hex((test_addr >> 8) & 0xFF);
    print_hex(test_addr & 0xFF);
    print_str("... ");
    
    if (eeprom_read_byte(test_addr, &verify_byte) == 0) {
        print_str("0x");
        print_hex(verify_byte);
        if (verify_byte == test_byte) {
            print_str(" ✓\n");
        } else {
            print_str(" ✗ MISMATCH!\n");
            errors++;
        }
    } else {
        print_str("FAIL\n");
        errors++;
    }
    
    print_str("\n");
    
    print_str("[TEST 2] Page Write (64 bytes)\n");
    print_str("----------------------------------------\n");
    
    test_addr = 0x0100;
    print_str("Writing ");
    print_uint16(my_strlen((const char *)write_data));
    print_str(" bytes to address 0x");
    print_hex((test_addr >> 8) & 0xFF);
    print_hex(test_addr & 0xFF);
    print_str("... ");
    
    if (eeprom_write_page(test_addr, write_data, my_strlen((const char *)write_data)) == 0) {
        print_str("OK\n");
    } else {
        print_str("FAIL\n");
        errors++;
    }
    
    print_str("\n");
    
    print_str("[TEST 3] Block Read & Verify\n");
    print_str("----------------------------------------\n");
    
    print_str("Reading ");
    print_uint16(my_strlen((const char *)write_data));
    print_str(" bytes from address 0x");
    print_hex((test_addr >> 8) & 0xFF);
    print_hex(test_addr & 0xFF);
    print_str("... ");
    
    if (eeprom_read_block(test_addr, read_data, my_strlen((const char *)write_data)) == 0) {
        print_str("OK\n\n");
        
        print_str("Data read from EEPROM:\n  \"");
        for (uint16_t i = 0; i < sizeof(write_data) - 1; i++) {
            if (read_data[i] >= 32 && read_data[i] <= 126) {
                char c[2] = {(char)read_data[i], '\0'};
                print_str(c);
            } else {
                print_str(".");
            }
        }
        print_str("\"\n\n");
        
        print_str("Verification: ");
        if (my_memcmp(write_data, read_data, my_strlen((const char *)write_data)) == 0) {
            print_str("✓ PASS\n");
        } else {
            print_str("✗ FAIL - Data mismatch!\n");
            errors++;
        }
    } else {
        print_str("FAIL\n");
        errors++;
    }
    
    print_str("\n");
    
    print_str("[TEST 4] Multiple Address Test\n");
    print_str("----------------------------------------\n");
    
    uint16_t test_addresses[] = {0x0000, 0x0100, 0x1000, 0x7FFF};
    uint8_t test_patterns[] = {0xAA, 0x55, 0xF0, 0x0F};
    
    for (uint8_t i = 0; i < 4; i++) {
        print_str("Address 0x");
        print_hex((test_addresses[i] >> 8) & 0xFF);
        print_hex(test_addresses[i] & 0xFF);
        print_str(": Write 0x");
        print_hex(test_patterns[i]);
        print_str("... ");
        
        if (eeprom_write_byte(test_addresses[i], test_patterns[i]) == 0) {
            if (eeprom_read_byte(test_addresses[i], &verify_byte) == 0) {
                if (verify_byte == test_patterns[i]) {
                    print_str("✓\n");
                } else {
                    print_str("✗ Read mismatch\n");
                    errors++;
                }
            } else {
                print_str("✗ Read fail\n");
                errors++;
            }
        } else {
            print_str("✗ Write fail\n");
            errors++;
        }
    }
    
    print_str("\n");
    print_str("========================================\n");
    print_str("  Test Summary\n");
    print_str("========================================\n");
    
    if (errors == 0) {
        print_str("  Result: ALL TESTS PASSED ✓\n");
    } else {
        print_str("  Result: ");
        print_uint16(errors);
        print_str(" ERRORS ✗\n");
    }
    
    print_str("\n");
    print_str("[INFO] In QEMU, EEPROM data is stored in\n");
    print_str("[INFO] the eeprom.bin file (if specified).\n");
    print_str("[INFO] Data persists across QEMU restarts.\n\n");
    
    while (1) {
        __asm__("nop");
    }
    
    return 0;
}
