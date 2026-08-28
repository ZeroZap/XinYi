/**
 * @file test_dm_nvm.c
 * @brief Focused host tests for DM NVM key-value contracts
 */

#include "xy_nvm.h"
#include "unity.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

void setUp(void)
{
}

void tearDown(void)
{
}

void xy_log_char(char ch)
{
    (void)ch;
}

static uint8_t flash_area[256];
static size_t backend_write_calls;
static size_t backend_fail_write_call;
static size_t backend_partial_write_bytes;

static xy_nvm_status_t backend_read(void *context, size_t offset, void *buf, size_t len)
{
    memcpy(buf, (const uint8_t *)context + offset, len);
    return XY_NVM_OK;
}

static xy_nvm_status_t backend_write(void *context, size_t offset, const void *buf, size_t len)
{
    backend_write_calls++;
    if (backend_write_calls == backend_fail_write_call) {
        size_t written = backend_partial_write_bytes < len ? backend_partial_write_bytes : len;
        memcpy((uint8_t *)context + offset, buf, written);
        return XY_NVM_ERROR;
    }
    memcpy((uint8_t *)context + offset, buf, len);
    return XY_NVM_OK;
}

static xy_nvm_status_t backend_erase(void *context, size_t offset, size_t len)
{
    memset((uint8_t *)context + offset, 0xFF, len);
    return XY_NVM_OK;
}

static const xy_nvm_storage_ops_t backend_ops = {
    .read = backend_read,
    .write = backend_write,
    .erase = backend_erase,
};

static xy_nvm_t make_nvm(void)
{
    memset(flash_area, 0xFF, sizeof(flash_area));
    xy_nvm_t nvm;
    xy_nvm_config_t cfg = {
        .flash_base = flash_area,
        .page_size = 64,
        .num_pages = 4,
    };
    TEST_ASSERT_EQUAL(XY_NVM_OK, xy_nvm_init(&nvm, &cfg));
    return nvm;
}

static void test_init_validation(void)
{
    xy_nvm_t nvm;
    xy_nvm_config_t cfg = {.flash_base = flash_area, .page_size = 64, .num_pages = 4};

    TEST_ASSERT_EQUAL(XY_NVM_ERROR_INVALID_PARAM, xy_nvm_init(NULL, &cfg));
    TEST_ASSERT_EQUAL(XY_NVM_ERROR_INVALID_PARAM, xy_nvm_init(&nvm, NULL));
    cfg.flash_base = NULL;
    TEST_ASSERT_EQUAL(XY_NVM_ERROR_INVALID_PARAM, xy_nvm_init(&nvm, &cfg));

    printf("  [PASS] init validation\n");
}

static void test_set_get_delete_roundtrip(void)
{
    xy_nvm_t nvm = make_nvm();
    const uint8_t payload[] = {0x10, 0x22, 0x33, 0x44};

    TEST_ASSERT_EQUAL(XY_NVM_OK, xy_nvm_set(&nvm, 7, payload, sizeof(payload)));

    xy_nvm_result_t result = xy_nvm_get(&nvm, 7);
    TEST_ASSERT_EQUAL(XY_NVM_OK, result.status);
    TEST_ASSERT_EQUAL_UINT16((uint16_t)sizeof(payload), result.len);
    TEST_ASSERT_EQUAL_MEMORY(payload, result.data, sizeof(payload));

    TEST_ASSERT_EQUAL(XY_NVM_OK, xy_nvm_delete(&nvm, 7));
    result = xy_nvm_get(&nvm, 7);
    TEST_ASSERT_EQUAL(XY_NVM_ERROR_NOT_FOUND, result.status);

    TEST_ASSERT_EQUAL(XY_NVM_OK, xy_nvm_deinit(&nvm));
    printf("  [PASS] set/get/delete roundtrip\n");
}

static void test_restart_recovers_last_complete_value(void)
{
    xy_nvm_t nvm = make_nvm();
    const uint8_t first[] = {0x11, 0x22, 0x33, 0x44};
    const uint8_t second[] = {0xAA, 0xBB, 0xCC, 0xDD};

    TEST_ASSERT_EQUAL(XY_NVM_OK, xy_nvm_set(&nvm, 9, first, sizeof(first)));
    TEST_ASSERT_EQUAL(XY_NVM_OK, xy_nvm_set(&nvm, 9, second, sizeof(second)));

    xy_nvm_t restarted;
    xy_nvm_config_t cfg = {
        .flash_base = flash_area,
        .page_size = 64,
        .num_pages = 4,
    };
    TEST_ASSERT_EQUAL(XY_NVM_OK, xy_nvm_init(&restarted, &cfg));

    xy_nvm_result_t result = xy_nvm_get(&restarted, 9);
    TEST_ASSERT_EQUAL(XY_NVM_OK, result.status);
    TEST_ASSERT_EQUAL_UINT16((uint16_t)sizeof(second), result.len);
    TEST_ASSERT_EQUAL_MEMORY(second, result.data, sizeof(second));

    /* Simulate a torn append: recognizable header with an incomplete payload. */
    flash_area[32] = 0x55;
    flash_area[33] = 0xAA;
    flash_area[34] = 0x55;
    flash_area[35] = 0xAA;
    flash_area[36] = 9;
    flash_area[37] = 0xFF;
    flash_area[38] = 4;
    flash_area[39] = 0;
    flash_area[40] = 0;

    TEST_ASSERT_EQUAL(XY_NVM_OK, xy_nvm_init(&restarted, &cfg));
    result = xy_nvm_get(&restarted, 9);
    TEST_ASSERT_EQUAL(XY_NVM_OK, result.status);
    TEST_ASSERT_EQUAL_MEMORY(second, result.data, sizeof(second));

    printf("  [PASS] restart recovery keeps last complete value\n");
}

static void test_typed_helpers(void)
{
    xy_nvm_t nvm = make_nvm();

    TEST_ASSERT_EQUAL(XY_NVM_OK, xy_nvm_set_u8(&nvm, 1, 0x5A));
    xy_nvm_result_t r8 = xy_nvm_get_u8(&nvm, 1);
    TEST_ASSERT_EQUAL(XY_NVM_OK, r8.status);
    TEST_ASSERT_EQUAL_UINT16(1U, r8.len);
    TEST_ASSERT_EQUAL_HEX8(0x5A, r8.data[0]);

    TEST_ASSERT_EQUAL(XY_NVM_OK, xy_nvm_set_u16(&nvm, 2, 0x1234U));
    xy_nvm_result_t r16 = xy_nvm_get_u16(&nvm, 2);
    TEST_ASSERT_EQUAL(XY_NVM_OK, r16.status);
    TEST_ASSERT_EQUAL_UINT16((uint16_t)sizeof(uint16_t), r16.len);
    uint16_t v16;
    memcpy(&v16, r16.data, sizeof(v16));
    TEST_ASSERT_EQUAL_HEX16(0x1234U, v16);

    TEST_ASSERT_EQUAL(XY_NVM_OK, xy_nvm_set_u32(&nvm, 3, 0x89ABCDEFUL));
    xy_nvm_result_t r32 = xy_nvm_get_u32(&nvm, 3);
    TEST_ASSERT_EQUAL(XY_NVM_OK, r32.status);
    TEST_ASSERT_EQUAL_UINT16((uint16_t)sizeof(uint32_t), r32.len);
    uint32_t v32;
    memcpy(&v32, r32.data, sizeof(v32));
    TEST_ASSERT_EQUAL_HEX32(0x89ABCDEFUL, v32);

    printf("  [PASS] typed helpers\n");
}

static void test_backend_write_interruption_preserves_last_complete_value(void)
{
    memset(flash_area, 0xFF, sizeof(flash_area));
    backend_write_calls = 0;
    backend_fail_write_call = 0;
    backend_partial_write_bytes = 0;
    xy_nvm_config_t cfg = {
        .flash_base = flash_area,
        .page_size = 64,
        .num_pages = 4,
        .storage_ops = &backend_ops,
        .storage_context = flash_area,
    };
    xy_nvm_t nvm;
    const uint8_t stable[] = {0x11, 0x22};
    const uint8_t replacement[] = {0xAA, 0xBB};

    TEST_ASSERT_EQUAL(XY_NVM_OK, xy_nvm_init(&nvm, &cfg));
    TEST_ASSERT_EQUAL(XY_NVM_OK, xy_nvm_set(&nvm, 12, stable, sizeof(stable)));

    backend_fail_write_call = backend_write_calls + 1U;
    TEST_ASSERT_EQUAL(XY_NVM_ERROR, xy_nvm_set(&nvm, 12, replacement, sizeof(replacement)));

    xy_nvm_t restarted;
    TEST_ASSERT_EQUAL(XY_NVM_OK, xy_nvm_init(&restarted, &cfg));
    xy_nvm_result_t result = xy_nvm_get(&restarted, 12);
    TEST_ASSERT_EQUAL(XY_NVM_OK, result.status);
    TEST_ASSERT_EQUAL_MEMORY(stable, result.data, sizeof(stable));

    backend_fail_write_call = backend_write_calls + 2U;
    TEST_ASSERT_EQUAL(XY_NVM_ERROR, xy_nvm_set(&restarted, 12, replacement, sizeof(replacement)));
    TEST_ASSERT_EQUAL(XY_NVM_OK, xy_nvm_init(&restarted, &cfg));
    result = xy_nvm_get(&restarted, 12);
    TEST_ASSERT_EQUAL(XY_NVM_OK, result.status);
    TEST_ASSERT_EQUAL_MEMORY(stable, result.data, sizeof(stable));
}

static void test_partial_program_interruption_allows_restart_and_retry(void)
{
    const uint8_t stable[] = {0x11, 0x22};
    const uint8_t replacement[] = {0xAA, 0xBB};
    xy_nvm_config_t cfg = {
        .flash_base = flash_area,
        .page_size = 64,
        .num_pages = 4,
        .storage_ops = &backend_ops,
        .storage_context = flash_area,
    };

    for (size_t failed_call = 1U; failed_call <= 2U; failed_call++) {
        size_t write_size = failed_call == 1U ? 12U : sizeof(replacement);
        for (size_t partial = 0U; partial < write_size; partial++) {
            memset(flash_area, 0xFF, sizeof(flash_area));
            backend_write_calls = 0U;
            backend_fail_write_call = 0U;
            backend_partial_write_bytes = 0U;

            xy_nvm_t nvm;
            TEST_ASSERT_EQUAL(XY_NVM_OK, xy_nvm_init(&nvm, &cfg));
            TEST_ASSERT_EQUAL(XY_NVM_OK, xy_nvm_set(&nvm, 12, stable, sizeof(stable)));

            backend_fail_write_call = backend_write_calls + failed_call;
            backend_partial_write_bytes = partial;
            TEST_ASSERT_EQUAL(XY_NVM_ERROR,
                              xy_nvm_set(&nvm, 12, replacement, sizeof(replacement)));

            xy_nvm_t restarted;
            TEST_ASSERT_EQUAL(XY_NVM_OK, xy_nvm_init(&restarted, &cfg));
            xy_nvm_result_t result = xy_nvm_get(&restarted, 12);
            TEST_ASSERT_EQUAL(XY_NVM_OK, result.status);
            TEST_ASSERT_EQUAL_MEMORY(stable, result.data, sizeof(stable));

            backend_fail_write_call = 0U;
            backend_partial_write_bytes = 0U;
            TEST_ASSERT_EQUAL(XY_NVM_OK,
                              xy_nvm_set(&restarted, 12, replacement, sizeof(replacement)));
            result = xy_nvm_get(&restarted, 12);
            TEST_ASSERT_EQUAL(XY_NVM_OK, result.status);
            TEST_ASSERT_EQUAL_MEMORY(replacement, result.data, sizeof(replacement));
        }
    }
}

static void test_capacity_stats_and_format(void)
{
    xy_nvm_t nvm = make_nvm();
    uint8_t a[8] = {0};
    uint8_t b[12] = {0};

    TEST_ASSERT_EQUAL(XY_NVM_OK, xy_nvm_set(&nvm, 10, a, sizeof(a)));
    TEST_ASSERT_EQUAL(XY_NVM_OK, xy_nvm_set(&nvm, 11, b, sizeof(b)));

    uint16_t used = 0;
    uint16_t free_bytes = 0;
    xy_nvm_get_stats(&nvm, &used, &free_bytes);
    TEST_ASSERT_EQUAL_UINT16(44U, used);
    TEST_ASSERT_EQUAL_UINT16((uint16_t)(sizeof(flash_area) - 44U), free_bytes);

    TEST_ASSERT_EQUAL(XY_NVM_OK, xy_nvm_format(&nvm));
    xy_nvm_get_stats(&nvm, &used, &free_bytes);
    TEST_ASSERT_EQUAL_UINT16(0U, used);
    TEST_ASSERT_EQUAL_UINT16((uint16_t)sizeof(flash_area), free_bytes);

    printf("  [PASS] stats and format\n");
}

static void test_invalid_lengths_and_full(void)
{
    xy_nvm_t nvm = make_nvm();
    uint8_t too_large[XY_NVM_MAX_DATA_LEN + 1U];
    memset(too_large, 0xA5, sizeof(too_large));

    TEST_ASSERT_EQUAL(XY_NVM_ERROR_INVALID_PARAM, xy_nvm_set(&nvm, 1, NULL, 1));
    TEST_ASSERT_EQUAL(XY_NVM_ERROR_INVALID_PARAM,
                      xy_nvm_set(&nvm, 1, too_large, sizeof(too_large)));

    uint8_t block[XY_NVM_MAX_DATA_LEN];
    memset(block, 0x5A, sizeof(block));
    TEST_ASSERT_EQUAL(XY_NVM_OK, xy_nvm_set(&nvm, 1, block, sizeof(block)));
    TEST_ASSERT_EQUAL(XY_NVM_OK, xy_nvm_set(&nvm, 2, block, sizeof(block)));
    TEST_ASSERT_EQUAL(XY_NVM_OK, xy_nvm_set(&nvm, 3, block, sizeof(block)));
    TEST_ASSERT_EQUAL(XY_NVM_ERROR_FULL, xy_nvm_set(&nvm, 4, block, sizeof(block)));

    printf("  [PASS] invalid lengths and full storage\n");
}

int main(void)
{
    printf("[TEST] DM NVM focused host contracts\n");
    UNITY_BEGIN();
    RUN_TEST(test_init_validation);
    RUN_TEST(test_set_get_delete_roundtrip);
    RUN_TEST(test_restart_recovers_last_complete_value);
    RUN_TEST(test_backend_write_interruption_preserves_last_complete_value);
    RUN_TEST(test_partial_program_interruption_allows_restart_and_retry);
    RUN_TEST(test_typed_helpers);
    RUN_TEST(test_capacity_stats_and_format);
    RUN_TEST(test_invalid_lengths_and_full);
    printf("[PASS] DM NVM focused host contracts\n");
    return UNITY_END();
}
