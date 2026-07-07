#include "unity.h"
#include "xy_stdlib.h"

#include <stddef.h>
#include <stdlib.h>

void *xy_malloc(size_t size)
{
    return malloc(size);
}

void *xy_calloc(size_t nmemb, size_t size)
{
    return calloc(nmemb, size);
}

void *xy_realloc(void *ptr, size_t size)
{
    return realloc(ptr, size);
}

void xy_free(void *ptr)
{
    free(ptr);
}

void xy_safe_free(void **ptr)
{
    if (ptr && *ptr) {
        xy_free(*ptr);
        *ptr = NULL;
    }
}

#ifdef TEST_CLIB_ALLOC_SHIM_MAIN

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_clib_alloc_shim_allocates_and_zeroes_memory(void)
{
    uint8_t *bytes = (uint8_t *)xy_calloc(4, sizeof(uint8_t));

    TEST_ASSERT_NOT_NULL(bytes);
    for (size_t i = 0; i < 4; ++i) {
        TEST_ASSERT_EQUAL_UINT8(0, bytes[i]);
        bytes[i] = (uint8_t)(i + 1U);
    }

    uint8_t *resized = (uint8_t *)xy_realloc(bytes, 8);
    TEST_ASSERT_NOT_NULL(resized);
    TEST_ASSERT_EQUAL_UINT8(1, resized[0]);
    TEST_ASSERT_EQUAL_UINT8(2, resized[1]);
    TEST_ASSERT_EQUAL_UINT8(3, resized[2]);
    TEST_ASSERT_EQUAL_UINT8(4, resized[3]);

    xy_free(resized);
}

static void test_clib_alloc_shim_safe_free_clears_pointer(void)
{
    void *ptr = xy_malloc(8);

    TEST_ASSERT_NOT_NULL(ptr);
    xy_safe_free(&ptr);
    TEST_ASSERT_NULL(ptr);
    xy_safe_free(&ptr);
    TEST_ASSERT_NULL(ptr);
    xy_safe_free(NULL);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_clib_alloc_shim_allocates_and_zeroes_memory);
    RUN_TEST(test_clib_alloc_shim_safe_free_clears_pointer);
    return UNITY_END();
}

#endif /* TEST_CLIB_ALLOC_SHIM_MAIN */
