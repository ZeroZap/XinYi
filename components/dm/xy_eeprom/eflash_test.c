#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "eflash.h"

/**
 * @file eflash_test.c
 * @brief Test program for eflash component
 * @version 1.0
 * @date 2025-10-22
 */

/* Test configuration */
#define TEST_PAGE_SIZE  512
#define TEST_PAGE_COUNT 16
#define TEST_TOTAL_SIZE (TEST_PAGE_SIZE * TEST_PAGE_COUNT)

/* Color output for terminal */
#define COLOR_GREEN  "\033[0;32m"
#define COLOR_RED    "\033[0;31m"
#define COLOR_YELLOW "\033[0;33m"
#define COLOR_RESET  "\033[0m"

static void print_test_result(const char *test_name, bool passed)
{
    if (passed) {
        printf(COLOR_GREEN "[PASS]" COLOR_RESET " %s\n", test_name);
    } else {
        printf(COLOR_RED "[FAIL]" COLOR_RESET " %s\n", test_name);
    }
}

static void print_separator(void)
{
    printf("========================================\n");
}

/* Test 1: Basic initialization */
static bool test_init(void)
{
    eflash_handle_t handle = { 0 };
    eflash_config_t config = { .total_size = 0, /* Will be calculated */
                               .page_size  = TEST_PAGE_SIZE,
                               .page_count = TEST_PAGE_COUNT,
                               .write_unit = EFLASH_WRITE_UNIT_32BIT,
                               .auto_erase = false };

    eflash_result_t result = eflash_init(&handle, &config);
    if (result != EFLASH_OK) {
        return false;
    }

    /* Verify configuration */
    if (handle.config.total_size != TEST_TOTAL_SIZE) {
        return false;
    }

    if (handle.memory == NULL || handle.page_erased == NULL) {
        return false;
    }

    if (!handle.initialized) {
        return false;
    }

    eflash_deinit(&handle);
    return true;
}

/* Test 2: Read/Write operations with 32-bit write unit */
static bool test_read_write_32bit(void)
{
    eflash_handle_t handle = { 0 };
    eflash_config_t config = { .page_size  = TEST_PAGE_SIZE,
                               .page_count = TEST_PAGE_COUNT,
                               .write_unit = EFLASH_WRITE_UNIT_32BIT,
                               .auto_erase = true };

    if (eflash_init(&handle, &config) != EFLASH_OK) {
        return false;
    }

    /* Write aligned data (32-bit) */
    uint8_t write_data[32] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                               0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00,
                               0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0,
                               0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45, 0x67, 0x89 };
    uint8_t read_data[32]  = { 0 };

    eflash_result_t result =
        eflash_write(&handle, 0, write_data, sizeof(write_data));
    if (result != EFLASH_OK) {
        eflash_deinit(&handle);
        return false;
    }

    result = eflash_read(&handle, 0, read_data, sizeof(read_data));
    if (result != EFLASH_OK) {
        eflash_deinit(&handle);
        return false;
    }

    if (memcmp(write_data, read_data, sizeof(write_data)) != 0) {
        eflash_deinit(&handle);
        return false;
    }

    eflash_deinit(&handle);
    return true;
}

/* Test 3: 64-bit write unit */
static bool test_write_64bit(void)
{
    eflash_handle_t handle = { 0 };
    eflash_config_t config = { .page_size  = 1024,
                               .page_count = 8,
                               .write_unit = EFLASH_WRITE_UNIT_64BIT,
                               .auto_erase = true };

    if (eflash_init(&handle, &config) != EFLASH_OK) {
        return false;
    }

    /* Write data aligned to 64-bit (8 bytes) */
    uint64_t data        = 0x123456789ABCDEF0ULL;
    uint8_t *data_ptr    = (uint8_t *)&data;
    uint8_t read_data[8] = { 0 };

    eflash_result_t result = eflash_write(&handle, 0, data_ptr, 8);
    if (result != EFLASH_OK) {
        eflash_deinit(&handle);
        return false;
    }

    result = eflash_read(&handle, 0, read_data, 8);
    if (result != EFLASH_OK) {
        eflash_deinit(&handle);
        return false;
    }

    if (memcmp(data_ptr, read_data, 8) != 0) {
        eflash_deinit(&handle);
        return false;
    }

    eflash_deinit(&handle);
    return true;
}

/* Test 4: 128-bit write unit */
static bool test_write_128bit(void)
{
    eflash_handle_t handle = { 0 };
    eflash_config_t config = { .page_size  = 2048,
                               .page_count = 4,
                               .write_unit = EFLASH_WRITE_UNIT_128BIT,
                               .auto_erase = true };

    if (eflash_init(&handle, &config) != EFLASH_OK) {
        return false;
    }

    /* Write data aligned to 128-bit (16 bytes) */
    uint8_t write_data[16] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                               0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF };
    uint8_t read_data[16]  = { 0 };

    eflash_result_t result = eflash_write(&handle, 0, write_data, 16);
    if (result != EFLASH_OK) {
        eflash_deinit(&handle);
        return false;
    }

    result = eflash_read(&handle, 0, read_data, 16);
    if (result != EFLASH_OK) {
        eflash_deinit(&handle);
        return false;
    }

    if (memcmp(write_data, read_data, 16) != 0) {
        eflash_deinit(&handle);
        return false;
    }

    eflash_deinit(&handle);
    return true;
}

/* Test 5: Page erase */
static bool test_erase_page(void)
{
    eflash_handle_t handle = { 0 };
    eflash_config_t config = { .page_size  = TEST_PAGE_SIZE,
                               .page_count = TEST_PAGE_COUNT,
                               .write_unit = EFLASH_WRITE_UNIT_32BIT,
                               .auto_erase = false };

    if (eflash_init(&handle, &config) != EFLASH_OK) {
        return false;
    }

    /* Erase page 0 (already erased, but test the function) */
    if (eflash_erase_page(&handle, 0) != EFLASH_OK) {
        eflash_deinit(&handle);
        return false;
    }

    /* Check if page is erased */
    if (!eflash_is_page_erased(&handle, 0)) {
        eflash_deinit(&handle);
        return false;
    }

    /* Write some data */
    uint8_t write_data[4] = { 0x12, 0x34, 0x56, 0x78 };
    if (eflash_write(&handle, 0, write_data, 4) != EFLASH_OK) {
        eflash_deinit(&handle);
        return false;
    }

    /* Page should not be erased after write */
    if (eflash_is_page_erased(&handle, 0)) {
        eflash_deinit(&handle);
        return false;
    }

    /* Erase page again */
    if (eflash_erase_page(&handle, 0) != EFLASH_OK) {
        eflash_deinit(&handle);
        return false;
    }

    /* Verify page is erased (all 0xFF) */
    uint8_t read_data[4] = { 0 };
    eflash_read(&handle, 0, read_data, 4);
    if (read_data[0] != 0xFF || read_data[1] != 0xFF || read_data[2] != 0xFF
        || read_data[3] != 0xFF) {
        eflash_deinit(&handle);
        return false;
    }

    eflash_deinit(&handle);
    return true;
}

/* Test 6: Multiple pages */
static bool test_multiple_pages(void)
{
    eflash_handle_t handle = { 0 };
    eflash_config_t config = { .page_size  = 512,
                               .page_count = 16,
                               .write_unit = EFLASH_WRITE_UNIT_32BIT,
                               .auto_erase = true };

    if (eflash_init(&handle, &config) != EFLASH_OK) {
        return false;
    }

    /* Write data across multiple pages */
    uint8_t write_data[1024];
    for (int i = 0; i < 1024; i++) {
        write_data[i] = i & 0xFF;
    }

    /* Write at offset that spans 3 pages */
    uint32_t address       = 256; /* Middle of page 0 */
    eflash_result_t result = eflash_write(&handle, address, write_data, 1024);
    if (result != EFLASH_OK) {
        eflash_deinit(&handle);
        return false;
    }

    /* Read back and verify */
    uint8_t read_data[1024] = { 0 };
    result                  = eflash_read(&handle, address, read_data, 1024);
    if (result != EFLASH_OK) {
        eflash_deinit(&handle);
        return false;
    }

    if (memcmp(write_data, read_data, 1024) != 0) {
        eflash_deinit(&handle);
        return false;
    }

    eflash_deinit(&handle);
    return true;
}

/* Test 7: Alignment check */
static bool test_alignment(void)
{
    eflash_handle_t handle = { 0 };
    eflash_config_t config = { .page_size  = TEST_PAGE_SIZE,
                               .page_count = TEST_PAGE_COUNT,
                               .write_unit = EFLASH_WRITE_UNIT_32BIT,
                               .auto_erase = true };

    if (eflash_init(&handle, &config) != EFLASH_OK) {
        return false;
    }

    /* Try to write unaligned data (should fail) */
    uint8_t data[5]        = { 1, 2, 3, 4, 5 };
    eflash_result_t result = eflash_write(&handle, 0, data, 5);

    /* Should return alignment error */
    if (result != EFLASH_ERROR_ALIGNMENT) {
        eflash_deinit(&handle);
        return false;
    }

    /* Try to write to unaligned address (should fail) */
    uint8_t aligned_data[4] = { 1, 2, 3, 4 };
    result                  = eflash_write(&handle, 1, aligned_data, 4);

    if (result != EFLASH_ERROR_ALIGNMENT) {
        eflash_deinit(&handle);
        return false;
    }

    eflash_deinit(&handle);
    return true;
}

/* Test 8: Erase all */
static bool test_erase_all(void)
{
    eflash_handle_t handle = { 0 };
    eflash_config_t config = { .page_size  = TEST_PAGE_SIZE,
                               .page_count = TEST_PAGE_COUNT,
                               .write_unit = EFLASH_WRITE_UNIT_32BIT,
                               .auto_erase = true };

    if (eflash_init(&handle, &config) != EFLASH_OK) {
        return false;
    }

    /* Write data to multiple locations */
    uint8_t data[4] = { 0xAA, 0xBB, 0xCC, 0xDD };
    eflash_write(&handle, 0, data, 4);
    eflash_write(&handle, 512, data, 4);
    eflash_write(&handle, 1024, data, 4);

    /* Erase all */
    if (eflash_erase_all(&handle) != EFLASH_OK) {
        eflash_deinit(&handle);
        return false;
    }

    /* Verify all pages are erased */
    for (uint32_t i = 0; i < handle.config.page_count; i++) {
        if (!eflash_is_page_erased(&handle, i)) {
            eflash_deinit(&handle);
            return false;
        }
    }

    /* Read back and verify all 0xFF */
    uint8_t read_data[4] = { 0 };
    eflash_read(&handle, 0, read_data, 4);
    if (read_data[0] != 0xFF || read_data[1] != 0xFF || read_data[2] != 0xFF
        || read_data[3] != 0xFF) {
        eflash_deinit(&handle);
        return false;
    }

    eflash_deinit(&handle);
    return true;
}

/* Test 9: Out of range access */
static bool test_out_of_range(void)
{
    eflash_handle_t handle = { 0 };
    eflash_config_t config = { .page_size  = TEST_PAGE_SIZE,
                               .page_count = TEST_PAGE_COUNT,
                               .write_unit = EFLASH_WRITE_UNIT_32BIT,
                               .auto_erase = true };

    if (eflash_init(&handle, &config) != EFLASH_OK) {
        return false;
    }

    uint8_t data[4] = { 1, 2, 3, 4 };

    /* Try to write beyond flash size */
    eflash_result_t result = eflash_write(&handle, TEST_TOTAL_SIZE, data, 4);
    if (result != EFLASH_ERROR_OUT_OF_RANGE) {
        eflash_deinit(&handle);
        return false;
    }

    /* Try to read beyond flash size */
    result = eflash_read(&handle, TEST_TOTAL_SIZE - 2, data, 4);
    if (result != EFLASH_ERROR_OUT_OF_RANGE) {
        eflash_deinit(&handle);
        return false;
    }

    eflash_deinit(&handle);
    return true;
}

/* Test 10: Get info */
static bool test_get_info(void)
{
    eflash_handle_t handle = { 0 };
    eflash_config_t config = { .page_size  = 1024,
                               .page_count = 32,
                               .write_unit = EFLASH_WRITE_UNIT_64BIT,
                               .auto_erase = false };

    if (eflash_init(&handle, &config) != EFLASH_OK) {
        return false;
    }

    eflash_config_t info;
    if (eflash_get_info(&handle, &info) != EFLASH_OK) {
        eflash_deinit(&handle);
        return false;
    }

    if (info.page_size != 1024 || info.page_count != 32
        || info.write_unit != EFLASH_WRITE_UNIT_64BIT
        || info.total_size != 32768) {
        eflash_deinit(&handle);
        return false;
    }

    eflash_deinit(&handle);
    return true;
}

/* Main test function */
int main(void)
{
    printf(COLOR_YELLOW "\n");
    printf("===========================================\n");
    printf("   EFLASH Component Test Suite\n");
    printf("===========================================\n");
    printf(COLOR_RESET "\n");

    int total_tests  = 0;
    int passed_tests = 0;

    /* Run all tests */
    struct {
        const char *name;
        bool (*func)(void);
    } tests[] = {
        { "Test 1: Basic Initialization", test_init },
        { "Test 2: Read/Write (32-bit)", test_read_write_32bit },
        { "Test 3: Write Unit (64-bit)", test_write_64bit },
        { "Test 4: Write Unit (128-bit)", test_write_128bit },
        { "Test 5: Page Erase", test_erase_page },
        { "Test 6: Multiple Pages", test_multiple_pages },
        { "Test 7: Alignment Check", test_alignment },
        { "Test 8: Erase All", test_erase_all },
        { "Test 9: Out of Range", test_out_of_range },
        { "Test 10: Get Info", test_get_info },
    };

    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        total_tests++;
        bool result = tests[i].func();
        print_test_result(tests[i].name, result);
        if (result) {
            passed_tests++;
        }
    }

    print_separator();
    printf("\n");
    printf("Test Summary: %d/%d tests passed\n", passed_tests, total_tests);

    if (passed_tests == total_tests) {
        printf(COLOR_GREEN "All tests PASSED!" COLOR_RESET "\n\n");
        return 0;
    } else {
        printf(COLOR_RED "Some tests FAILED!" COLOR_RESET "\n\n");
        return 1;
    }
}
