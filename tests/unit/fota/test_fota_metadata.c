#include "xy_fota_metadata.h"
#include "unity.h"

#include <stdint.h>
#include <string.h>

#define METADATA_BASE 0x080A0000U
#define ERASE_SIZE 128U
#define STORAGE_SIZE (2U * ERASE_SIZE)

static uint8_t g_storage[STORAGE_SIZE];
static int g_write_result;
static uint32_t g_partial_write_size;
static uint32_t g_read_calls;
static uint32_t g_write_calls;
static uint32_t g_erase_calls;

static uint32_t storage_offset(uint32_t addr)
{
    TEST_ASSERT_GREATER_OR_EQUAL_HEX32(METADATA_BASE, addr);
    TEST_ASSERT_LESS_THAN_UINT32(METADATA_BASE + STORAGE_SIZE, addr);
    return addr - METADATA_BASE;
}

static int flash_read(uint32_t addr, uint8_t *data, uint32_t size)
{
    uint32_t offset = storage_offset(addr);

    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(STORAGE_SIZE, offset + size);
    memcpy(data, &g_storage[offset], size);
    g_read_calls++;
    return XY_FOTA_OK;
}

static int flash_write(uint32_t addr, const uint8_t *data, uint32_t size)
{
    uint32_t offset = storage_offset(addr);
    uint32_t write_size = size;

    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(STORAGE_SIZE, offset + size);
    if (g_partial_write_size > 0U && g_partial_write_size < write_size) {
        write_size = g_partial_write_size;
    }
    memcpy(&g_storage[offset], data, write_size);
    g_write_calls++;
    return g_write_result;
}

static int flash_erase(uint32_t addr, uint32_t size)
{
    uint32_t offset = storage_offset(addr);

    TEST_ASSERT_EQUAL_UINT32(ERASE_SIZE, size);
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(STORAGE_SIZE, offset + size);
    memset(&g_storage[offset], 0xFF, size);
    g_erase_calls++;
    return XY_FOTA_OK;
}

static const xy_fota_flash_ops_t flash_ops = {
    .write = flash_write,
    .read = flash_read,
    .erase = flash_erase,
};

static const xy_fota_metadata_flash_t backend = {
    .ops = &flash_ops,
    .base_addr = METADATA_BASE,
    .erase_size = ERASE_SIZE,
};

void setUp(void)
{
    memset(g_storage, 0xFF, sizeof(g_storage));
    g_write_result = XY_FOTA_OK;
    g_partial_write_size = 0U;
    g_read_calls = 0U;
    g_write_calls = 0U;
    g_erase_calls = 0U;
}

void tearDown(void)
{
}

static xy_fota_metadata_t initial_metadata(void)
{
    xy_fota_metadata_t metadata = {
        .active_version = 3U,
        .min_version = 3U,
        .active_slot = 0U,
        .pending_slot = XY_FOTA_METADATA_NO_SLOT,
        .flags = 0U,
    };
    return metadata;
}

static void test_metadata_guards_and_empty_flash_fail_closed(void)
{
    xy_fota_metadata_t metadata = initial_metadata();
    xy_fota_metadata_flash_t invalid_backend = backend;

    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          xy_fota_metadata_flash_load(NULL, &metadata));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          xy_fota_metadata_flash_load(&backend, NULL));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          xy_fota_metadata_flash_commit(&backend, NULL, NULL));

    invalid_backend.erase_size = 1U;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          xy_fota_metadata_flash_commit(&invalid_backend, &metadata, NULL));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_NO_IMAGE,
                          xy_fota_metadata_flash_load(&backend, &metadata));
}

static void test_metadata_commit_roundtrip_and_generation(void)
{
    xy_fota_metadata_t metadata = initial_metadata();
    xy_fota_metadata_t loaded = {0};
    xy_fota_metadata_t committed = {0};

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_flash_commit(&backend, &metadata, &committed));
    TEST_ASSERT_EQUAL_UINT32(1U, committed.generation);
    TEST_ASSERT_EQUAL_UINT32(1U, g_write_calls);
    TEST_ASSERT_EQUAL_UINT32(1U, g_erase_calls);
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_metadata_flash_load(&backend, &loaded));
    TEST_ASSERT_EQUAL_UINT32(1U, loaded.generation);
    TEST_ASSERT_EQUAL_UINT32(3U, loaded.active_version);
    TEST_ASSERT_EQUAL_UINT8(0U, loaded.active_slot);

    metadata.active_version = 4U;
    metadata.min_version = 4U;
    metadata.active_slot = 1U;
    metadata.pending_slot = 0U;
    metadata.flags = XY_FOTA_METADATA_FLAG_PENDING;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_flash_commit(&backend, &metadata, &committed));
    TEST_ASSERT_EQUAL_UINT32(2U, committed.generation);
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_metadata_flash_load(&backend, &loaded));
    TEST_ASSERT_EQUAL_UINT32(2U, loaded.generation);
    TEST_ASSERT_EQUAL_UINT32(4U, loaded.active_version);
    TEST_ASSERT_EQUAL_UINT32(4U, loaded.min_version);
    TEST_ASSERT_EQUAL_UINT8(1U, loaded.active_slot);
    TEST_ASSERT_EQUAL_UINT8(0U, loaded.pending_slot);
    TEST_ASSERT_EQUAL_HEX8(XY_FOTA_METADATA_FLAG_PENDING, loaded.flags);
}

static void test_partial_write_preserves_previous_committed_record(void)
{
    xy_fota_metadata_t metadata = initial_metadata();
    xy_fota_metadata_t loaded = {0};

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_flash_commit(&backend, &metadata, NULL));

    metadata.active_version = 9U;
    metadata.min_version = 9U;
    metadata.active_slot = 1U;
    g_partial_write_size = 12U;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_FLASH_ERROR,
                          xy_fota_metadata_flash_commit(&backend, &metadata, NULL));

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_metadata_flash_load(&backend, &loaded));
    TEST_ASSERT_EQUAL_UINT32(1U, loaded.generation);
    TEST_ASSERT_EQUAL_UINT32(3U, loaded.active_version);
    TEST_ASSERT_EQUAL_UINT8(0U, loaded.active_slot);
}

static void test_corrupt_newest_copy_falls_back_to_previous_generation(void)
{
    xy_fota_metadata_t metadata = initial_metadata();
    xy_fota_metadata_t loaded = {0};

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_flash_commit(&backend, &metadata, NULL));
    metadata.active_version = 4U;
    metadata.min_version = 4U;
    metadata.active_slot = 1U;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_flash_commit(&backend, &metadata, NULL));

    g_storage[ERASE_SIZE + 8U] ^= 0x80U;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_metadata_flash_load(&backend, &loaded));
    TEST_ASSERT_EQUAL_UINT32(1U, loaded.generation);
    TEST_ASSERT_EQUAL_UINT32(3U, loaded.active_version);
    TEST_ASSERT_EQUAL_UINT8(0U, loaded.active_slot);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_metadata_guards_and_empty_flash_fail_closed);
    RUN_TEST(test_metadata_commit_roundtrip_and_generation);
    RUN_TEST(test_partial_write_preserves_previous_committed_record);
    RUN_TEST(test_corrupt_newest_copy_falls_back_to_previous_generation);
    return UNITY_END();
}
