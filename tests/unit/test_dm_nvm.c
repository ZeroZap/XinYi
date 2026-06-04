/**
 * @file test_dm_nvm.c
 * @brief Focused host tests for DM NVM key-value contracts
 */

#include "xy_nvm.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void xy_log_char(char ch)
{
    (void)ch;
}

static uint8_t flash_area[256];

static xy_nvm_t make_nvm(void)
{
    memset(flash_area, 0xFF, sizeof(flash_area));
    xy_nvm_t nvm;
    xy_nvm_config_t cfg = {
        .flash_base = flash_area,
        .page_size = 64,
        .num_pages = 4,
    };
    assert(xy_nvm_init(&nvm, &cfg) == XY_NVM_OK);
    return nvm;
}

static void test_init_validation(void)
{
    xy_nvm_t nvm;
    xy_nvm_config_t cfg = {.flash_base = flash_area, .page_size = 64, .num_pages = 4};

    assert(xy_nvm_init(NULL, &cfg) == XY_NVM_ERROR_INVALID_PARAM);
    assert(xy_nvm_init(&nvm, NULL) == XY_NVM_ERROR_INVALID_PARAM);
    cfg.flash_base = NULL;
    assert(xy_nvm_init(&nvm, &cfg) == XY_NVM_ERROR_INVALID_PARAM);

    printf("  [PASS] init validation\n");
}

static void test_set_get_delete_roundtrip(void)
{
    xy_nvm_t nvm = make_nvm();
    const uint8_t payload[] = {0x10, 0x22, 0x33, 0x44};

    assert(xy_nvm_set(&nvm, 7, payload, sizeof(payload)) == XY_NVM_OK);

    xy_nvm_result_t result = xy_nvm_get(&nvm, 7);
    assert(result.status == XY_NVM_OK);
    assert(result.len == sizeof(payload));
    assert(memcmp(result.data, payload, sizeof(payload)) == 0);

    assert(xy_nvm_delete(&nvm, 7) == XY_NVM_OK);
    result = xy_nvm_get(&nvm, 7);
    assert(result.status == XY_NVM_ERROR_NOT_FOUND);

    assert(xy_nvm_deinit(&nvm) == XY_NVM_OK);
    printf("  [PASS] set/get/delete roundtrip\n");
}

static void test_typed_helpers(void)
{
    xy_nvm_t nvm = make_nvm();

    assert(xy_nvm_set_u8(&nvm, 1, 0x5A) == XY_NVM_OK);
    xy_nvm_result_t r8 = xy_nvm_get_u8(&nvm, 1);
    assert(r8.status == XY_NVM_OK);
    assert(r8.len == 1);
    assert(r8.data[0] == 0x5A);

    assert(xy_nvm_set_u16(&nvm, 2, 0x1234U) == XY_NVM_OK);
    xy_nvm_result_t r16 = xy_nvm_get_u16(&nvm, 2);
    assert(r16.status == XY_NVM_OK);
    assert(r16.len == sizeof(uint16_t));
    uint16_t v16;
    memcpy(&v16, r16.data, sizeof(v16));
    assert(v16 == 0x1234U);

    assert(xy_nvm_set_u32(&nvm, 3, 0x89ABCDEFUL) == XY_NVM_OK);
    xy_nvm_result_t r32 = xy_nvm_get_u32(&nvm, 3);
    assert(r32.status == XY_NVM_OK);
    assert(r32.len == sizeof(uint32_t));
    uint32_t v32;
    memcpy(&v32, r32.data, sizeof(v32));
    assert(v32 == 0x89ABCDEFUL);

    printf("  [PASS] typed helpers\n");
}

static void test_capacity_stats_and_format(void)
{
    xy_nvm_t nvm = make_nvm();
    uint8_t a[8] = {0};
    uint8_t b[12] = {0};

    assert(xy_nvm_set(&nvm, 10, a, sizeof(a)) == XY_NVM_OK);
    assert(xy_nvm_set(&nvm, 11, b, sizeof(b)) == XY_NVM_OK);

    uint16_t used = 0;
    uint16_t free_bytes = 0;
    xy_nvm_get_stats(&nvm, &used, &free_bytes);
    assert(used == 44U);
    assert(free_bytes == (uint16_t)(sizeof(flash_area) - 44U));

    assert(xy_nvm_format(&nvm) == XY_NVM_OK);
    xy_nvm_get_stats(&nvm, &used, &free_bytes);
    assert(used == 0U);
    assert(free_bytes == sizeof(flash_area));

    printf("  [PASS] stats and format\n");
}

static void test_invalid_lengths_and_full(void)
{
    xy_nvm_t nvm = make_nvm();
    uint8_t too_large[XY_NVM_MAX_DATA_LEN + 1U];
    memset(too_large, 0xA5, sizeof(too_large));

    assert(xy_nvm_set(&nvm, 1, NULL, 1) == XY_NVM_ERROR_INVALID_PARAM);
    assert(xy_nvm_set(&nvm, 1, too_large, sizeof(too_large)) == XY_NVM_ERROR_INVALID_PARAM);

    uint8_t block[XY_NVM_MAX_DATA_LEN];
    memset(block, 0x5A, sizeof(block));
    assert(xy_nvm_set(&nvm, 1, block, sizeof(block)) == XY_NVM_OK);
    assert(xy_nvm_set(&nvm, 2, block, sizeof(block)) == XY_NVM_OK);
    assert(xy_nvm_set(&nvm, 3, block, sizeof(block)) == XY_NVM_OK);
    assert(xy_nvm_set(&nvm, 4, block, sizeof(block)) == XY_NVM_ERROR_FULL);

    printf("  [PASS] invalid lengths and full storage\n");
}

int main(void)
{
    printf("[TEST] DM NVM focused host contracts\n");
    test_init_validation();
    test_set_get_delete_roundtrip();
    test_typed_helpers();
    test_capacity_stats_and_format();
    test_invalid_lengths_and_full();
    printf("[PASS] DM NVM focused host contracts\n");
    return 0;
}
