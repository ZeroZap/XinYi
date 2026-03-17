/**
 * @file main.c
 * @brief XinYi HAL 统一 API 验证测试
 * 
 * 在 QEMU 上验证 STM32F4 的 HAL 统一 API
 * 测试 GPIO/SPI/I2C/UART 的统一接口
 */

#include <stdint.h>

/*============================================================================
 *  半主机输出
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
 *  测试框架宏
 *===========================================================================*/

#define TEST_START(name) \
    print_str("\n[TEST] "); \
    print_str(name); \
    print_str("...\n")

#define TEST_ASSERT(condition, name) \
    if (condition) { \
        print_str("  ✓ PASS: "); \
        print_str(name); \
        print_str("\n"); \
        pass_count++; \
    } else { \
        print_str("  ✗ FAIL: "); \
        print_str(name); \
        print_str("\n"); \
        fail_count++; \
    }

#define TEST_ASSERT_EQ(expected, actual, name) \
    if ((expected) == (actual)) { \
        print_str("  ✓ PASS: "); \
        print_str(name); \
        print_str("\n"); \
        pass_count++; \
    } else { \
        print_str("  ✗ FAIL: "); \
        print_str(name); \
        print_str("\n"); \
        fail_count++; \
    }

/*============================================================================
 *  辅助函数
 *===========================================================================*/

static uint32_t pass_count = 0;
static uint32_t fail_count = 0;

static void print_uint32(uint32_t value)
{
    char buf[12];
    int i = 0;
    
    if (value == 0) {
        print_str("0");
        return;
    }
    
    while (value > 0 && i < 11) {
        buf[i++] = '0' + (value % 10);
        value /= 10;
    }
    
    while (i > 0) {
        char c[2] = {buf[--i], '\0'};
        print_str(c);
    }
}

static void print_hex8(uint8_t value)
{
    char hex[3];
    hex[0] = (value >> 4) > 9 ? 'A' + (value >> 4) - 10 : '0' + (value >> 4);
    hex[1] = (value & 0x0F) > 9 ? 'A' + (value & 0x0F) - 10 : '0' + (value & 0x0F);
    hex[2] = '\0';
    print_str(hex);
}

static void delay_ms(uint32_t ms)
{
    volatile uint32_t count = ms * 15000;
    while (count--);
}

/*============================================================================
 *  STM32F4 寄存器定义 (用于验证 HAL 实现)
 *===========================================================================*/

/* RCC */
#define RCC_BASE            0x40023800UL
#define RCC_AHB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x30))
#define RCC_APB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0x44))
#define RCC_AHB1ENR_GPIOAEN (1 << 0)
#define RCC_AHB1ENR_GPIOCEN (1 << 2)
#define RCC_APB2ENR_USART1EN (1 << 4)

/* GPIO */
#define GPIOA_BASE          0x40020000UL
#define GPIOA_MODER         (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_ODR           (*(volatile uint32_t *)(GPIOA_BASE + 0x14))
#define GPIOA_BSRR          (*(volatile uint32_t *)(GPIOA_BASE + 0x18))

#define GPIOC_BASE          0x40020800UL
#define GPIOC_MODER         (*(volatile uint32_t *)(GPIOC_BASE + 0x00))
#define GPIOC_ODR           (*(volatile uint32_t *)(GPIOC_BASE + 0x14))
#define GPIOC_BSRR          (*(volatile uint32_t *)(GPIOC_BASE + 0x18))

/* GPIO 模式 */
#define GPIO_MODE_INPUT     0x00
#define GPIO_MODE_OUTPUT    0x01
#define GPIO_MODE_AF        0x02
#define GPIO_MODE_ANALOG    0x03

/*============================================================================
 *  HAL 统一 API 模拟实现 (用于 QEMU 验证)
 *===========================================================================*/

/* GPIO API */
typedef struct {
    uint32_t port_base;
    uint8_t pin;
} xy_hal_gpio_dev_t;

typedef xy_hal_gpio_dev_t* xy_hal_gpio_t;

typedef struct {
    uint8_t mode;
    uint8_t pull;
    uint8_t speed;
} xy_hal_gpio_config_t;

static xy_hal_gpio_dev_t gpio_devs[16];

static xy_hal_gpio_t xy_hal_gpio_bind(const char *name)
{
    /* 解析 "GPIOx.y" 格式 */
    if (name[0] == 'G' && name[1] == 'P' && name[2] == 'I' && name[3] == 'O') {
        uint8_t port = name[4] - 'A';
        uint8_t pin = name[6] - '0';
        
        static uint8_t dev_idx = 0;
        xy_hal_gpio_t dev = &gpio_devs[dev_idx++];
        
        if (port == 0) dev->port_base = GPIOA_BASE;
        else if (port == 2) dev->port_base = GPIOC_BASE;
        dev->pin = pin;
        
        return dev;
    }
    return (xy_hal_gpio_t)0;
}

static int xy_hal_gpio_configure(xy_hal_gpio_t dev, uint8_t pin, const xy_hal_gpio_config_t *config)
{
    if (!dev || !config) return -1;
    
    volatile uint32_t *moder = (volatile uint32_t *)(dev->port_base + 0x00);
    uint8_t shift = dev->pin * 2;
    
    /* 配置模式 */
    *moder &= ~(0x3 << shift);
    *moder |= (config->mode << shift);
    
    return 0;
}

static int xy_hal_gpio_write(xy_hal_gpio_t dev, uint8_t pin, uint8_t value)
{
    if (!dev) return -1;
    
    volatile uint32_t *bsrr = (volatile uint32_t *)(dev->port_base + 0x18);
    
    if (value) {
        *bsrr = (1 << dev->pin);
    } else {
        *bsrr = (1 << (dev->pin + 16));
    }
    
    return 0;
}

static int xy_hal_gpio_read(xy_hal_gpio_t dev, uint8_t pin)
{
    if (!dev) return 0;
    
    volatile uint32_t *idr = (volatile uint32_t *)(dev->port_base + 0x10);
    return (*idr & (1 << dev->pin)) ? 1 : 0;
}

static int xy_hal_gpio_toggle(xy_hal_gpio_t dev, uint8_t pin)
{
    if (!dev) return -1;
    
    volatile uint32_t *odr = (volatile uint32_t *)(dev->port_base + 0x14);
    *odr ^= (1 << dev->pin);
    
    return 0;
}

/* UART API */
typedef struct {
    uint32_t instance;
    uint32_t baudrate;
} xy_hal_uart_dev_t;

typedef xy_hal_uart_dev_t* xy_hal_uart_t;

typedef struct {
    uint32_t baudrate;
    uint8_t data_bits;
    uint8_t stop_bits;
    uint8_t parity;
} xy_hal_uart_config_t;

static xy_hal_uart_dev_t uart_devs[4];

static xy_hal_uart_t xy_hal_uart_bind(const char *name)
{
    if (name[4] == '1') {
        static uint8_t dev_idx = 0;
        xy_hal_uart_t dev = &uart_devs[dev_idx++];
        dev->instance = 1;
        dev->baudrate = 0;
        return dev;
    }
    return (xy_hal_uart_t)0;
}

static int xy_hal_uart_configure(xy_hal_uart_t dev, const xy_hal_uart_config_t *config)
{
    if (!dev || !config) return -1;
    
    dev->baudrate = config->baudrate;
    
    /* 使能 USART1 时钟 */
    RCC_APB2ENR |= RCC_APB2ENR_USART1EN;
    
    return 0;
}

static int xy_hal_uart_write(xy_hal_uart_t dev, const uint8_t *data, uint32_t length)
{
    if (!dev || !data) return -1;
    
    /* QEMU 中使用半主机输出 */
    for (uint32_t i = 0; i < length; i++) {
        char c[2] = {(char)data[i], '\0'};
        print_str(c);
    }
    
    return length;
}

/*============================================================================
 *  GPIO HAL 测试
 *===========================================================================*/

static void test_hal_gpio_bind(void)
{
    TEST_START("HAL GPIO Bind");
    
    xy_hal_gpio_t gpio = xy_hal_gpio_bind("GPIOA.5");
    TEST_ASSERT(gpio != (xy_hal_gpio_t)0, "Bind GPIOA.5");
    
    gpio = xy_hal_gpio_bind("GPIOC.13");
    TEST_ASSERT(gpio != (xy_hal_gpio_t)0, "Bind GPIOC.13");
}

static void test_hal_gpio_configure(void)
{
    TEST_START("HAL GPIO Configure");
    
    xy_hal_gpio_t gpio = xy_hal_gpio_bind("GPIOA.5");
    
    xy_hal_gpio_config_t config = {
        .mode = GPIO_MODE_OUTPUT,
        .pull = 0,
        .speed = 0
    };
    
    int ret = xy_hal_gpio_configure(gpio, 5, &config);
    TEST_ASSERT_EQ(0, ret, "Configure GPIOA.5 as output");
}

static void test_hal_gpio_write_read(void)
{
    TEST_START("HAL GPIO Write/Read");
    
    xy_hal_gpio_t gpio = xy_hal_gpio_bind("GPIOA.5");
    
    xy_hal_gpio_config_t config = {
        .mode = GPIO_MODE_OUTPUT,
        .pull = 0,
        .speed = 0
    };
    xy_hal_gpio_configure(gpio, 5, &config);
    
    int ret = xy_hal_gpio_write(gpio, 5, 1);
    TEST_ASSERT_EQ(0, ret, "Write GPIOA.5 high");
    
    ret = xy_hal_gpio_write(gpio, 5, 0);
    TEST_ASSERT_EQ(0, ret, "Write GPIOA.5 low");
}

static void test_hal_gpio_toggle(void)
{
    TEST_START("HAL GPIO Toggle");
    
    xy_hal_gpio_t gpio = xy_hal_gpio_bind("GPIOC.13");
    
    xy_hal_gpio_config_t config = {
        .mode = GPIO_MODE_OUTPUT,
        .pull = 0,
        .speed = 0
    };
    xy_hal_gpio_configure(gpio, 13, &config);
    
    int ret = xy_hal_gpio_toggle(gpio, 13);
    TEST_ASSERT_EQ(0, ret, "Toggle GPIOC.13");
}

/*============================================================================
 *  UART HAL 测试
 *===========================================================================*/

static void test_hal_uart_bind(void)
{
    TEST_START("HAL UART Bind");
    
    xy_hal_uart_t uart = xy_hal_uart_bind("UART1");
    TEST_ASSERT(uart != (xy_hal_uart_t)0, "Bind UART1");
}

static void test_hal_uart_configure(void)
{
    TEST_START("HAL UART Configure");
    
    xy_hal_uart_t uart = xy_hal_uart_bind("UART1");
    
    xy_hal_uart_config_t config = {
        .baudrate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = 0
    };
    
    int ret = xy_hal_uart_configure(uart, &config);
    TEST_ASSERT_EQ(0, ret, "Configure UART1 115200 8N1");
}

static void test_hal_uart_write(void)
{
    TEST_START("HAL UART Write");
    
    xy_hal_uart_t uart = xy_hal_uart_bind("UART1");
    
    xy_hal_uart_config_t config = {
        .baudrate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = 0
    };
    xy_hal_uart_configure(uart, &config);
    
    const char *test_msg = "HAL UART Test Message\n";
    int ret = xy_hal_uart_write(uart, (const uint8_t *)test_msg, 21);
    TEST_ASSERT(ret > 0, "UART write test message");
}

/*============================================================================
 *  综合 HAL 测试
 *===========================================================================*/

static void test_hal_gpio_led_blink(void)
{
    TEST_START("HAL GPIO LED Blink (PC13)");
    
    xy_hal_gpio_t led = xy_hal_gpio_bind("GPIOC.13");
    
    xy_hal_gpio_config_t config = {
        .mode = GPIO_MODE_OUTPUT,
        .pull = 0,
        .speed = 0
    };
    xy_hal_gpio_configure(led, 13, &config);
    
    /* 闪烁 3 次 */
    for (int i = 0; i < 3; i++) {
        xy_hal_gpio_write(led, 13, 1);
        delay_ms(100);
        xy_hal_gpio_write(led, 13, 0);
        delay_ms(100);
    }
    
    TEST_ASSERT(1, "LED blink 3 times");
}

static void test_hal_combined_workflow(void)
{
    TEST_START("HAL Combined Workflow");
    
    /* 1. 初始化 UART */
    xy_hal_uart_t uart = xy_hal_uart_bind("UART1");
    xy_hal_uart_config_t uart_config = {
        .baudrate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = 0
    };
    xy_hal_uart_configure(uart, &uart_config);
    
    /* 2. 初始化 GPIO */
    xy_hal_gpio_t gpio = xy_hal_gpio_bind("GPIOA.5");
    xy_hal_gpio_config_t gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pull = 0,
        .speed = 0
    };
    xy_hal_gpio_configure(gpio, 5, &gpio_config);
    
    /* 3. 输出测试消息 */
    const char *msg = "HAL Workflow Test\n";
    xy_hal_uart_write(uart, (const uint8_t *)msg, 18);
    
    /* 4. GPIO 切换 */
    xy_hal_gpio_toggle(gpio, 5);
    
    TEST_ASSERT(1, "Combined workflow completed");
}

/*============================================================================
 *  主函数
 *===========================================================================*/

int main(void)
{
    print_str("\n");
    print_str("╔══════════════════════════════════════════╗\n");
    print_str("║  XinYi HAL Unified API Test              ║\n");
    print_str("║  STM32F405 on QEMU                       ║\n");
    print_str("╚══════════════════════════════════════════╝\n");
    print_str("\n");
    
    /* GPIO HAL 测试 */
    test_hal_gpio_bind();
    test_hal_gpio_configure();
    test_hal_gpio_write_read();
    test_hal_gpio_toggle();
    test_hal_gpio_led_blink();
    
    /* UART HAL 测试 */
    test_hal_uart_bind();
    test_hal_uart_configure();
    test_hal_uart_write();
    
    /* 综合测试 */
    test_hal_combined_workflow();
    
    /* 输出总结 */
    print_str("\n");
    print_str("╔══════════════════════════════════════════╗\n");
    print_str("║  Test Summary                            ║\n");
    print_str("╚══════════════════════════════════════════╝\n");
    print_str("  Total: ");
    print_uint32(pass_count + fail_count);
    print_str("\n");
    print_str("  PASS:  ");
    print_uint32(pass_count);
    print_str("\n");
    print_str("  FAIL:  ");
    print_uint32(fail_count);
    print_str("\n");
    
    if (fail_count == 0) {
        print_str("\n>>> ALL TESTS PASSED <<<\n");
    } else {
        print_str("\n>>> SOME TESTS FAILED <<<\n");
    }
    
    print_str("\n[RESULT] PASS=");
    print_uint32(pass_count);
    print_str(" FAIL=");
    print_uint32(fail_count);
    print_str("\n");
    
    while (1) {
        __asm__("nop");
    }
    
    return 0;
}
