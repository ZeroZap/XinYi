/**
 * @file test_xy_clib.c
 * @brief XY CLib Component Unit Tests using Unity Framework
 * @version 1.0.0
 * @date 2026-02-28
 */

#include <stdint.h>
#include <string.h>
#include "unity.h"

/* CLib headers */
#include "xy_filter.h"
#include "xy_sort.h"
#include "xy_math.h"
#include "xy_string.h"

/* ==================== Test Fixtures ==================== */

void setUp(void)
{
    /* Called before each test */
}

void tearDown(void)
{
    /* Called after each test */
}

/* ==================== Filter Tests ==================== */

void test_amplitude_limiting_filter(void)
{
    uint16_t last_val = 100;
    uint16_t result;

    /* Normal change - within limit */
    result = xy_filter_amplitude_limiting(105, last_val, 10);
    TEST_ASSERT_EQUAL_UINT16(105, result);

    /* Exceeds limit - should return last value */
    result = xy_filter_amplitude_limiting(120, last_val, 10);
    TEST_ASSERT_EQUAL_UINT16(100, result);

    /* Below limit - negative change */
    result = xy_filter_amplitude_limiting(95, last_val, 10);
    TEST_ASSERT_EQUAL_UINT16(95, result);

    /* Exceeds limit - negative change */
    result = xy_filter_amplitude_limiting(85, last_val, 10);
    TEST_ASSERT_EQUAL_UINT16(100, result);
}

void test_median_filter(void)
{
    uint16_t buffer[5];
    xy_median_filter_t filter;
    uint16_t result;

    /* Initialize median filter */
    xy_filter_median_init(&filter, buffer, 5);

    /* Test with sequence containing noise */
    result = xy_filter_median(&filter, 100);
    TEST_ASSERT_EQUAL_UINT16(100, result);

    result = xy_filter_median(&filter, 102);
    TEST_ASSERT_EQUAL_UINT16(100, result);

    result = xy_filter_median(&filter, 500); /* Noise spike */
    TEST_ASSERT_EQUAL_UINT16(100, result);

    result = xy_filter_median(&filter, 101);
    TEST_ASSERT_EQUAL_UINT16(101, result);

    result = xy_filter_median(&filter, 99);
    TEST_ASSERT_EQUAL_UINT16(101, result);
}

void test_recursive_average_filter(void)
{
    uint16_t buffer[4];
    xy_recursive_average_filter_t filter;
    uint16_t result;

    /* Initialize average filter */
    xy_filter_recursive_average_init(&filter, buffer, 4);

    /* Test with sequence */
    result = xy_filter_recursive_average(&filter, 100);
    TEST_ASSERT_EQUAL_UINT16(100, result);

    result = xy_filter_recursive_average(&filter, 102);
    TEST_ASSERT_EQUAL_UINT16(101, result);

    result = xy_filter_recursive_average(&filter, 98);
    TEST_ASSERT_EQUAL_UINT16(100, result);

    result = xy_filter_recursive_average(&filter, 100);
    TEST_ASSERT_EQUAL_UINT16(100, result);
}

void test_first_order_lag_filter(void)
{
    xy_first_order_lag_filter_t filter;
    uint16_t result;

    /* Initialize lag filter with coefficient 3 (1/8) */
    xy_filter_first_order_lag_init(&filter, 3);

    /* Test with sequence */
    result = xy_filter_first_order_lag(&filter, 100);
    TEST_ASSERT_EQUAL_UINT16(100, result);

    result = xy_filter_first_order_lag(&filter, 102);
    TEST_ASSERT_EQUAL_UINT16(100, result); /* Lag effect */

    result = xy_filter_first_order_lag(&filter, 98);
    TEST_ASSERT_TRUE(result >= 98 && result <= 100);
}

/* ==================== Sort Tests ==================== */

void test_bubble_sort(void)
{
    uint16_t arr[] = { 64, 34, 25, 12, 22, 11, 90, 5 };
    uint16_t len = sizeof(arr) / sizeof(arr[0]);
    uint16_t expected[] = { 5, 11, 12, 22, 25, 34, 64, 90 };

    xy_bubble_sort(arr, len);

    TEST_ASSERT_EQUAL_UINT16_ARRAY(expected, arr, len);
}

void test_selection_sort(void)
{
    uint16_t arr[] = { 64, 34, 25, 12, 22, 11, 90, 5 };
    uint16_t len = sizeof(arr) / sizeof(arr[0]);
    uint16_t expected[] = { 5, 11, 12, 22, 25, 34, 64, 90 };

    xy_selection_sort(arr, len);

    TEST_ASSERT_EQUAL_UINT16_ARRAY(expected, arr, len);
}

void test_insertion_sort(void)
{
    uint16_t arr[] = { 64, 34, 25, 12, 22, 11, 90, 5 };
    uint16_t len = sizeof(arr) / sizeof(arr[0]);
    uint16_t expected[] = { 5, 11, 12, 22, 25, 34, 64, 90 };

    xy_insertion_sort(arr, len);

    TEST_ASSERT_EQUAL_UINT16_ARRAY(expected, arr, len);
}

void test_quick_sort(void)
{
    uint16_t arr[] = { 64, 34, 25, 12, 22, 11, 90, 5 };
    uint16_t len = sizeof(arr) / sizeof(arr[0]);
    uint16_t expected[] = { 5, 11, 12, 22, 25, 34, 64, 90 };

    xy_quick_sort(arr, len);

    TEST_ASSERT_EQUAL_UINT16_ARRAY(expected, arr, len);
}

void test_shell_sort(void)
{
    uint16_t arr[] = { 64, 34, 25, 12, 22, 11, 90, 5 };
    uint16_t len = sizeof(arr) / sizeof(arr[0]);
    uint16_t expected[] = { 5, 11, 12, 22, 25, 34, 64, 90 };

    xy_shell_sort(arr, len);

    TEST_ASSERT_EQUAL_UINT16_ARRAY(expected, arr, len);
}

void test_heap_sort(void)
{
    uint16_t arr[] = { 64, 34, 25, 12, 22, 11, 90, 5 };
    uint16_t len = sizeof(arr) / sizeof(arr[0]);
    uint16_t expected[] = { 5, 11, 12, 22, 25, 34, 64, 90 };

    xy_heap_sort(arr, len);

    TEST_ASSERT_EQUAL_UINT16_ARRAY(expected, arr, len);
}

void test_binary_insertion_sort(void)
{
    uint16_t arr[] = { 64, 34, 25, 12, 22, 11, 90, 5 };
    uint16_t len = sizeof(arr) / sizeof(arr[0]);
    uint16_t expected[] = { 5, 11, 12, 22, 25, 34, 64, 90 };

    xy_binary_insertion_sort(arr, len);

    TEST_ASSERT_EQUAL_UINT16_ARRAY(expected, arr, len);
}

void test_binary_search(void)
{
    uint16_t arr[] = { 5, 11, 12, 22, 25, 34, 64, 90 };
    uint16_t len = sizeof(arr) / sizeof(arr[0]);
    int16_t result;

    /* Search for existing element */
    result = xy_binary_search(arr, len, 22);
    TEST_ASSERT_EQUAL_INT16(3, result);

    /* Search for first element */
    result = xy_binary_search(arr, len, 5);
    TEST_ASSERT_EQUAL_INT16(0, result);

    /* Search for last element */
    result = xy_binary_search(arr, len, 90);
    TEST_ASSERT_EQUAL_INT16(7, result);

    /* Search for non-existing element */
    result = xy_binary_search(arr, len, 50);
    TEST_ASSERT_EQUAL_INT16(-1, result);
}

/* ==================== Math Tests ==================== */

void test_clamp(void)
{
    int32_t result;

    /* Value within range */
    result = XY_CLAMP(50, 0, 100);
    TEST_ASSERT_EQUAL_INT32(50, result);

    /* Value below minimum */
    result = XY_CLAMP(-10, 0, 100);
    TEST_ASSERT_EQUAL_INT32(0, result);

    /* Value above maximum */
    result = XY_CLAMP(150, 0, 100);
    TEST_ASSERT_EQUAL_INT32(100, result);
}

void test_min_max(void)
{
    int32_t result;

    /* Test MIN */
    result = XY_MIN(10, 20);
    TEST_ASSERT_EQUAL_INT32(10, result);

    result = XY_MIN(-5, 5);
    TEST_ASSERT_EQUAL_INT32(-5, result);

    /* Test MAX */
    result = XY_MAX(10, 20);
    TEST_ASSERT_EQUAL_INT32(20, result);

    result = XY_MAX(-5, 5);
    TEST_ASSERT_EQUAL_INT32(5, result);
}

void test_swap(void)
{
    int32_t a = 10, b = 20;

    XY_SWAP(a, b);

    TEST_ASSERT_EQUAL_INT32(20, a);
    TEST_ASSERT_EQUAL_INT32(10, b);
}

void test_bit_operations(void)
{
    uint32_t mask;
    uint8_t value;

    /* Test BIT macro */
    mask = XY_BIT(0);
    TEST_ASSERT_EQUAL_UINT32(1, mask);

    mask = XY_BIT(5);
    TEST_ASSERT_EQUAL_UINT32(32, mask);

    mask = XY_BIT(31);
    TEST_ASSERT_EQUAL_UINT32(0x80000000, mask);

    /* Test BIT_SET */
    value = 0x00;
    BIT_SET(value, 3);
    TEST_ASSERT_EQUAL_UINT8(0x08, value);

    /* Test BIT_CLEAR */
    value = 0xFF;
    BIT_CLEAR(value, 3);
    TEST_ASSERT_EQUAL_UINT8(0xF7, value);

    /* Test BIT_IS_SET */
    value = 0x0A;
    TEST_ASSERT_TRUE(BIT_IS_SET(value, 1));
    TEST_ASSERT_FALSE(BIT_IS_SET(value, 2));
}

void test_array_size(void)
{
    int32_t arr[] = { 1, 2, 3, 4, 5 };
    size_t size;

    size = XY_ARRAY_SIZE(arr);
    TEST_ASSERT_EQUAL(5, size);
}

/* ==================== String Tests ==================== */

void test_xy_strlen(void)
{
    const char *str1 = "Hello";
    const char *str2 = "";
    size_t len;

    len = xy_strlen(str1);
    TEST_ASSERT_EQUAL(5, len);

    len = xy_strlen(str2);
    TEST_ASSERT_EQUAL(0, len);
}

void test_xy_strcpy(void)
{
    char src[] = "Hello, World!";
    char dst[32];
    char *result;

    result = xy_strcpy(dst, src);

    TEST_ASSERT_EQUAL_STRING(src, dst);
    TEST_ASSERT_EQUAL_PTR(dst, result);
}

void test_xy_strncpy(void)
{
    char src[] = "Hello, World!";
    char dst[32];
    char *result;

    /* Copy full string */
    result = xy_strncpy(dst, src, 13);
    TEST_ASSERT_EQUAL_STRING(src, dst);
    TEST_ASSERT_EQUAL_PTR(dst, result);

    /* Copy partial string */
    xy_memset(dst, 0, sizeof(dst));
    result = xy_strncpy(dst, src, 5);
    TEST_ASSERT_EQUAL_STRING("Hello", dst);
}

void test_xy_strcat(void)
{
    char str1[32] = "Hello";
    const char *str2 = ", World!";
    char *result;

    result = xy_strcat(str1, str2);

    TEST_ASSERT_EQUAL_STRING("Hello, World!", str1);
    TEST_ASSERT_EQUAL_PTR(str1, result);
}

void test_xy_strcmp(void)
{
    const char *str1 = "Hello";
    const char *str2 = "Hello";
    const char *str3 = "World";
    int result;

    /* Equal strings */
    result = xy_strcmp(str1, str2);
    TEST_ASSERT_EQUAL(0, result);

    /* Different strings */
    result = xy_strcmp(str1, str3);
    TEST_ASSERT_TRUE(result != 0);
}

void test_xy_memset(void)
{
    uint8_t buffer[16];
    void *result;

    result = xy_memset(buffer, 0xFF, sizeof(buffer));

    TEST_ASSERT_EQUAL_PTR(buffer, result);
    for (size_t i = 0; i < sizeof(buffer); i++) {
        TEST_ASSERT_EQUAL_UINT8(0xFF, buffer[i]);
    }
}

void test_xy_memcpy(void)
{
    uint8_t src[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
    uint8_t dst[8];
    void *result;

    result = xy_memcpy(dst, src, sizeof(src));

    TEST_ASSERT_EQUAL_PTR(dst, result);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(src, dst, sizeof(src));
}

void test_xy_memcmp(void)
{
    uint8_t buf1[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
    uint8_t buf2[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
    uint8_t buf3[] = { 0x01, 0x02, 0x03, 0x04, 0x06 };
    int result;

    /* Equal buffers */
    result = xy_memcmp(buf1, buf2, sizeof(buf1));
    TEST_ASSERT_EQUAL(0, result);

    /* Different buffers */
    result = xy_memcmp(buf1, buf3, sizeof(buf1));
    TEST_ASSERT_TRUE(result != 0);
}

/* ==================== Main ==================== */

int main(void)
{
    UNITY_BEGIN();

    /* Filter Tests */
    RUN_TEST(test_amplitude_limiting_filter);
    RUN_TEST(test_median_filter);
    RUN_TEST(test_recursive_average_filter);
    RUN_TEST(test_first_order_lag_filter);

    /* Sort Tests */
    RUN_TEST(test_bubble_sort);
    RUN_TEST(test_selection_sort);
    RUN_TEST(test_insertion_sort);
    RUN_TEST(test_quick_sort);
    RUN_TEST(test_shell_sort);
    RUN_TEST(test_heap_sort);
    RUN_TEST(test_binary_insertion_sort);
    RUN_TEST(test_binary_search);

    /* Math Tests */
    RUN_TEST(test_clamp);
    RUN_TEST(test_min_max);
    RUN_TEST(test_swap);
    RUN_TEST(test_bit_operations);
    RUN_TEST(test_array_size);

    /* String Tests */
    RUN_TEST(test_xy_strlen);
    RUN_TEST(test_xy_strcpy);
    RUN_TEST(test_xy_strncpy);
    RUN_TEST(test_xy_strcat);
    RUN_TEST(test_xy_strcmp);
    RUN_TEST(test_xy_memset);
    RUN_TEST(test_xy_memcpy);
    RUN_TEST(test_xy_memcmp);

    return UNITY_END();
}
