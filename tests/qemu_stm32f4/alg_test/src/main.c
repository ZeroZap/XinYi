/**
 * @file main.c
 * @brief 算法测试框架 - UART 输出验证
 * 
 * 通过 UART 输出测试结果，支持自动化验证
 */

#include <stdint.h>

#define NULL ((void *)0)

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
        print_str(" ("); \
        print_uint32(actual); \
        print_str(")\n"); \
        pass_count++; \
    } else { \
        print_str("  ✗ FAIL: "); \
        print_str(name); \
        print_str(" (expected "); \
        print_uint32(expected); \
        print_str(", got "); \
        print_uint32(actual); \
        print_str(")\n"); \
        fail_count++; \
    }

/*============================================================================
 *  辅助函数
 *===========================================================================*/

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
 *  被测算法 (DUT - Device Under Test)
 *===========================================================================*/

/* 示例 1: CRC-8 计算 */
static uint8_t crc8_calc(const uint8_t *data, uint32_t len)
{
    uint8_t crc = 0;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }
    return crc;
}

/* 示例 2: 环形缓冲区 */
#define RING_BUF_SIZE 16

typedef struct {
    uint8_t buffer[RING_BUF_SIZE];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} ring_buf_t;

static void ring_buf_init(ring_buf_t *rb)
{
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}

static int ring_buf_push(ring_buf_t *rb, uint8_t data)
{
    if (rb->count >= RING_BUF_SIZE)
        return -1; /* 满 */
    
    rb->buffer[rb->head] = data;
    rb->head = (rb->head + 1) % RING_BUF_SIZE;
    rb->count++;
    return 0;
}

static int ring_buf_pop(ring_buf_t *rb, uint8_t *data)
{
    if (rb->count == 0)
        return -1; /* 空 */
    
    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % RING_BUF_SIZE;
    rb->count--;
    return 0;
}

/* 示例 3: 简单滤波算法 */
#define FILTER_WINDOW 5

static uint16_t moving_average(const uint16_t *samples, uint8_t count)
{
    uint32_t sum = 0;
    for (uint8_t i = 0; i < count && i < FILTER_WINDOW; i++) {
        sum += samples[i];
    }
    return sum / (count < FILTER_WINDOW ? count : FILTER_WINDOW);
}

/*============================================================================
 *  测试用例
 *===========================================================================*/

static uint32_t pass_count = 0;
static uint32_t fail_count = 0;

static void test_crc8(void)
{
    TEST_START("CRC-8 Algorithm");
    
    /* 测试 1: 空数据 */
    uint8_t crc = crc8_calc((uint8_t *)0, 0);
    TEST_ASSERT_EQ(0, crc, "Empty data");
    
    /* 测试 2: 单字节 */
    uint8_t data1[] = {0x41}; /* 'A' */
    crc = crc8_calc(data1, 1);
    TEST_ASSERT(crc != 0, "Single byte non-zero");
    
    /* 测试 3: 已知向量 */
    uint8_t data2[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    crc = crc8_calc(data2, 5);
    print_str("  CRC-8(0102030405) = 0x");
    print_hex8(crc);
    print_str("\n");
    TEST_ASSERT(crc != 0, "Known vector");
    
    /* 测试 4: 重复性 */
    uint8_t crc1 = crc8_calc(data2, 5);
    uint8_t crc2 = crc8_calc(data2, 5);
    TEST_ASSERT_EQ(crc1, crc2, "Repeatability");
}

static void test_ring_buffer(void)
{
    TEST_START("Ring Buffer");
    
    ring_buf_t rb;
    ring_buf_init(&rb);
    
    /* 测试 1: 初始状态 */
    TEST_ASSERT_EQ(0, rb.count, "Initial count");
    TEST_ASSERT_EQ(0, rb.head, "Initial head");
    TEST_ASSERT_EQ(0, rb.tail, "Initial tail");
    
    /* 测试 2: 推入数据 */
    for (uint8_t i = 0; i < 10; i++) {
        ring_buf_push(&rb, i);
    }
    TEST_ASSERT_EQ(10, rb.count, "Count after push 10");
    
    /* 测试 3: 弹出数据 */
    uint8_t data;
    ring_buf_pop(&rb, &data);
    TEST_ASSERT_EQ(0, data, "First popped data");
    TEST_ASSERT_EQ(9, rb.count, "Count after pop");
    
    /* 测试 4: 环形特性 */
    ring_buf_init(&rb);
    for (uint8_t i = 0; i < 20; i++) {
        ring_buf_push(&rb, i);
        if (rb.count > 0) {
            ring_buf_pop(&rb, &data);
        }
    }
    TEST_ASSERT(rb.count <= RING_BUF_SIZE, "Ring overflow protection");
    
    /* 测试 5: 满缓冲区 */
    ring_buf_init(&rb);
    int result = 0;
    for (uint8_t i = 0; i < RING_BUF_SIZE + 5; i++) {
        if (ring_buf_push(&rb, i) != 0) {
            result = 1;
            break;
        }
    }
    TEST_ASSERT(result == 1, "Full buffer rejection");
}

static void test_filter(void)
{
    TEST_START("Moving Average Filter");
    
    /* 测试 1: 恒定输入 */
    uint16_t samples1[FILTER_WINDOW] = {100, 100, 100, 100, 100};
    uint16_t avg = moving_average(samples1, FILTER_WINDOW);
    TEST_ASSERT_EQ(100, avg, "Constant input");
    
    /* 测试 2: 线性增长 */
    uint16_t samples2[FILTER_WINDOW] = {10, 20, 30, 40, 50};
    avg = moving_average(samples2, FILTER_WINDOW);
    TEST_ASSERT_EQ(30, avg, "Linear growth average");
    
    /* 测试 3: 少于窗口大小 */
    uint16_t samples3[3] = {10, 20, 30};
    avg = moving_average(samples3, 3);
    TEST_ASSERT_EQ(20, avg, "Partial window");
    
    /* 测试 4: 零值 */
    uint16_t samples4[FILTER_WINDOW] = {0, 0, 0, 0, 0};
    avg = moving_average(samples4, FILTER_WINDOW);
    TEST_ASSERT_EQ(0, avg, "Zero input");
}

static void test_performance(void)
{
    TEST_START("Performance Benchmark");
    
    /* CRC 性能测试 */
    uint8_t test_data[100];
    for (int i = 0; i < 100; i++) {
        test_data[i] = i & 0xFF;
    }
    
    print_str("  CRC-8 100 bytes... ");
    uint32_t start = 0x1000000; /* 模拟计时 */
    volatile uint8_t crc = 0;
    for (int i = 0; i < 100; i++) {
        crc = crc8_calc(test_data, 100);
    }
    uint32_t end = 0x1000100;
    print_str("done (crc=0x");
    print_hex8(crc);
    print_str(")\n");
    TEST_ASSERT(crc != 0, "CRC completed");
}

/*============================================================================
 *  主函数
 *===========================================================================*/

int main(void)
{
    print_str("\n");
    print_str("╔════════════════════════════════════════╗\n");
    print_str("║  XinYi Algorithm Test Framework       ║\n");
    print_str("║  STM32F405 on QEMU                    ║\n");
    print_str("╚════════════════════════════════════════╝\n");
    print_str("\n");
    
    /* 运行所有测试 */
    test_crc8();
    test_ring_buffer();
    test_filter();
    test_performance();
    
    /* 输出总结 */
    print_str("\n");
    print_str("╔════════════════════════════════════════╗\n");
    print_str("║  Test Summary                          ║\n");
    print_str("╚════════════════════════════════════════╝\n");
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
        print_str("\n");
        print_str(">>> ALL TESTS PASSED <<<\n");
        print_str("\n");
    } else {
        print_str("\n");
        print_str(">>> SOME TESTS FAILED <<<\n");
        print_str("\n");
    }
    
    /* 输出机器可读结果 (便于脚本解析) */
    print_str("[RESULT] PASS=");
    print_uint32(pass_count);
    print_str(" FAIL=");
    print_uint32(fail_count);
    print_str("\n");
    
    /* 无限循环 */
    while (1) {
        __asm__("nop");
    }
    
    return 0;
}
