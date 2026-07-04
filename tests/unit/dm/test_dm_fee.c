#include "unity.h"
#include "xy_fee.h"

#include <stdint.h>
#include <string.h>

#define FLASH_SIZE 4096U
#define PAGE_SIZE  1024U
#define FEE_PAGES  2U
#define CACHE_SIZE 128U
#define GRAN       8U

static uint8_t g_flash[FLASH_SIZE];
static uint8_t g_cache[CACHE_SIZE];
static uint8_t g_work[FEE_WORK_SIZE(GRAN)];
static fee_handle_t g_fee;
static uint32_t g_erase_count;
static uint32_t g_write_count;
static uint32_t g_read_count;

static int fake_erase(uint32_t addr)
{
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(FLASH_SIZE - PAGE_SIZE, addr);
    memset(&g_flash[addr], 0xFF, PAGE_SIZE);
    g_erase_count++;
    return 0;
}

static int fake_write(uint32_t addr, const uint8_t *data, uint16_t len)
{
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(FLASH_SIZE, addr + len);
    for (uint16_t i = 0; i < len; ++i) {
        g_flash[addr + i] &= data[i];
    }
    g_write_count++;
    return 0;
}

static int fake_read(uint32_t addr, uint8_t *data, uint16_t len)
{
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(FLASH_SIZE, addr + len);
    memcpy(data, &g_flash[addr], len);
    g_read_count++;
    return 0;
}

static const fee_flash_ops_t g_flash_ops = {
    .erase = fake_erase,
    .write = fake_write,
    .read = fake_read,
};

static fee_config_t make_config(void)
{
    fee_config_t config = {
        .flash_base = (uint8_t *)0,
        .pages_per_fee_page = FEE_PAGES,
        .flash_page_size = PAGE_SIZE,
        .cache_size = CACHE_SIZE,
        .write_granularity = GRAN,
        .max_erase_count = 10000,
        .flash_ops = &g_flash_ops,
    };
    return config;
}

static void init_fee(void)
{
    fee_config_t config = make_config();
    TEST_ASSERT_EQUAL(FEE_OK, fee_init(&g_fee, &config, g_cache, g_work));
}

void setUp(void)
{
    memset(g_flash, 0xFF, sizeof(g_flash));
    memset(g_cache, 0x00, sizeof(g_cache));
    memset(g_work, 0x00, sizeof(g_work));
    memset(&g_fee, 0x00, sizeof(g_fee));
    g_erase_count = 0;
    g_write_count = 0;
    g_read_count = 0;
}

void tearDown(void)
{
}

static void test_init_formats_empty_flash_and_clears_cache(void)
{
    init_fee();

    TEST_ASSERT_EQUAL_UINT8(0, g_fee.active_page);
    TEST_ASSERT_EQUAL_UINT16(0, g_fee.write_offset);
    uint8_t erased[CACHE_SIZE];
    memset(erased, 0xFF, sizeof(erased));
    TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(erased, g_cache, CACHE_SIZE,
                                          "empty flash init should mirror erased EEPROM");
    TEST_ASSERT_EQUAL_UINT32(FEE_PAGES, g_erase_count);
}

static void test_write_updates_cache_and_read_returns_latest_bytes(void)
{
    init_fee();

    const uint8_t first[] = {0x11, 0x22, 0x33, 0x44, 0x55};
    const uint8_t second[] = {0xAA, 0xBB, 0xCC};
    uint8_t out[sizeof(first)] = {0};

    TEST_ASSERT_EQUAL(FEE_OK, fee_write(&g_fee, 4, first, sizeof(first)));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(first, &g_cache[4], sizeof(first));

    TEST_ASSERT_EQUAL(FEE_OK, fee_write(&g_fee, 6, second, sizeof(second)));
    TEST_ASSERT_EQUAL(FEE_OK, fee_read(&g_fee, 4, out, sizeof(out)));

    const uint8_t expected[] = {0x11, 0x22, 0xAA, 0xBB, 0xCC};
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, out, sizeof(expected));
}

static void test_reinit_rebuilds_cache_from_flash_records(void)
{
    init_fee();

    const uint8_t data0[] = {0x10, 0x20, 0x30, 0x40};
    const uint8_t data1[] = {0xA1, 0xA2, 0xA3, 0xA4};
    TEST_ASSERT_EQUAL(FEE_OK, fee_write(&g_fee, 0, data0, sizeof(data0)));
    TEST_ASSERT_EQUAL(FEE_OK, fee_write(&g_fee, 32, data1, sizeof(data1)));

    uint8_t recovered[CACHE_SIZE];
    uint8_t work[FEE_WORK_SIZE(GRAN)];
    fee_handle_t recovered_fee;
    fee_config_t config = make_config();
    memset(recovered, 0x00, sizeof(recovered));
    memset(work, 0x00, sizeof(work));
    memset(&recovered_fee, 0x00, sizeof(recovered_fee));

    TEST_ASSERT_EQUAL(FEE_OK, fee_init(&recovered_fee, &config, recovered, work));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data0, &recovered[0], sizeof(data0));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data1, &recovered[32], sizeof(data1));
}

static void test_bounds_and_zero_length_contracts(void)
{
    init_fee();

    uint8_t byte = 0x5A;
    TEST_ASSERT_EQUAL(FEE_ERROR_PARAM, fee_write(&g_fee, CACHE_SIZE, &byte, 1));
    TEST_ASSERT_EQUAL(FEE_ERROR_PARAM, fee_read(&g_fee, CACHE_SIZE, &byte, 1));
    TEST_ASSERT_EQUAL(FEE_OK, fee_write(&g_fee, 8, NULL, 0));
    TEST_ASSERT_EQUAL(FEE_OK, fee_read(&g_fee, 8, NULL, 0));
    TEST_ASSERT_EQUAL(FEE_ERROR_PARAM, fee_write(&g_fee, 8, NULL, 1));
    TEST_ASSERT_EQUAL(FEE_ERROR_PARAM, fee_read(&g_fee, 8, NULL, 1));
}

static void test_manual_gc_preserves_virtual_eeprom_contents(void)
{
    init_fee();

    uint8_t expected[CACHE_SIZE];
    memset(expected, 0xFF, sizeof(expected));
    for (uint8_t i = 0; i < 24; ++i) {
        uint8_t data[] = {(uint8_t)(0x80U + i), (uint8_t)(0x40U + i)};
        uint16_t addr = (uint16_t)((i * 3U) % (CACHE_SIZE - sizeof(data)));
        TEST_ASSERT_EQUAL(FEE_OK, fee_write(&g_fee, addr, data, sizeof(data)));
        memcpy(&expected[addr], data, sizeof(data));
    }

    TEST_ASSERT_EQUAL(FEE_OK, fee_gc(&g_fee));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, g_cache, CACHE_SIZE);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_formats_empty_flash_and_clears_cache);
    RUN_TEST(test_write_updates_cache_and_read_returns_latest_bytes);
    RUN_TEST(test_reinit_rebuilds_cache_from_flash_records);
    RUN_TEST(test_bounds_and_zero_length_contracts);
    RUN_TEST(test_manual_gc_preserves_virtual_eeprom_contents);
    return UNITY_END();
}
