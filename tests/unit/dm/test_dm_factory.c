#include "xy_factory.h"
#include "unity.h"

#include <stdint.h>
#include <string.h>

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_factory_public_macros_are_stable(void)
{
    TEST_ASSERT_EQUAL_UINT32(0U, FACTORY_ALIGN_SIZE(0U));
    TEST_ASSERT_EQUAL_UINT32(XY_FACTORY_ALIGN_SIZE, FACTORY_ALIGN_SIZE(1U));
    TEST_ASSERT_EQUAL_UINT32(16U, FACTORY_ALIGN_SIZE(9U));
    TEST_ASSERT_EQUAL_UINT32(5U, FACTORY_TLV_HEADER_SIZE);
    TEST_ASSERT_EQUAL_UINT32(12U, FACTORY_TLV_TOTAL_SIZE(7U));
    TEST_ASSERT_EQUAL_UINT32(256U, FACTORY_TOTAL_SIZE(128U));
}

static void test_factory_crc16_incremental_matches_one_shot(void)
{
    const uint8_t payload[] = {0x7E, FACTORY_TYPE_DEVICE_ID, 0x04, 0x00,
                               'X', 'i', 'n', 'Y'};

    const uint16_t one_shot = factory_crc16(payload, sizeof(payload));
    uint16_t incremental = 0xFFFFU;
    incremental = factory_crc16_update(incremental, payload, 2U);
    incremental = factory_crc16_update(incremental, payload + 2U, 2U);
    incremental = factory_crc16_update(incremental, payload + 4U, 4U);

    TEST_ASSERT_EQUAL_HEX16(incremental, one_shot);
    TEST_ASSERT_EQUAL_HEX16(0xC1F8U, one_shot);
}

static void test_factory_verify_entry_crc_accepts_and_rejects_entries(void)
{
    struct __attribute__((packed)) local_entry {
        uint8_t magic;
        uint8_t type;
        uint16_t len;
        uint16_t crc;
        uint8_t data[4];
    } entry = {
        .magic = 0x7EU,
        .type = FACTORY_TYPE_DEVICE_ID,
        .len = 4U,
        .crc = 0U,
        .data = {'X', 'i', 'n', 'Y'},
    };

    entry.crc = factory_crc16(&entry.magic, 1U);
    entry.crc = factory_crc16_update(entry.crc, &entry.type, 1U);
    entry.crc = factory_crc16_update(entry.crc, (const uint8_t *)&entry.len, 2U);
    entry.crc = factory_crc16_update(entry.crc, entry.data, entry.len);

    TEST_ASSERT_TRUE(factory_verify_entry_crc(NULL, &entry));
    entry.data[0] ^= 0x01U;
    TEST_ASSERT_FALSE(factory_verify_entry_crc(NULL, &entry));
    TEST_ASSERT_FALSE(factory_verify_entry_crc(NULL, NULL));
}

static void test_factory_status_strings_cover_public_errors(void)
{
    TEST_ASSERT_EQUAL_STRING("OK", factory_status_str(FACTORY_OK));
    TEST_ASSERT_EQUAL_STRING("Parameter error", factory_status_str(FACTORY_ERROR_PARAM));
    TEST_ASSERT_EQUAL_STRING("No space left", factory_status_str(FACTORY_ERROR_NO_SPACE));
    TEST_ASSERT_EQUAL_STRING("CRC verify failed", factory_status_str(FACTORY_ERROR_CRC));
    TEST_ASSERT_EQUAL_STRING("Unknown error", factory_status_str((factory_status_t)1234));
}

static void test_factory_defensive_guards_before_init(void)
{
    factory_handle_t handle;
    uint8_t buffer[8];
    uint16_t len = sizeof(buffer);
    uint8_t types[2];
    uint8_t count = 0U;
    uint32_t used = 0U;

    memset(&handle, 0, sizeof(handle));

    TEST_ASSERT_EQUAL(FACTORY_ERROR_PARAM, factory_init(NULL, NULL));
    TEST_ASSERT_EQUAL(FACTORY_ERROR_PARAM,
                      factory_write(NULL, FACTORY_TYPE_DEVICE_ID, buffer, 1U));
    TEST_ASSERT_EQUAL(FACTORY_ERROR_PARAM,
                      factory_write(&handle, FACTORY_TYPE_DEVICE_ID, buffer, 1U));
    TEST_ASSERT_EQUAL(FACTORY_ERROR_PARAM,
                      factory_read(NULL, FACTORY_TYPE_DEVICE_ID, buffer, &len));
    TEST_ASSERT_EQUAL(FACTORY_ERROR_PARAM, factory_delete(NULL, FACTORY_TYPE_DEVICE_ID));
    TEST_ASSERT_FALSE(factory_exists(NULL, FACTORY_TYPE_DEVICE_ID, &len));
    TEST_ASSERT_EQUAL(FACTORY_ERROR_PARAM, factory_enum(NULL, types, 2U, &count));
    TEST_ASSERT_EQUAL(FACTORY_ERROR_PARAM, factory_format(NULL));
    TEST_ASSERT_EQUAL(FACTORY_ERROR_PARAM, factory_get_info(NULL, &used, NULL, NULL));
    TEST_ASSERT_EQUAL(FACTORY_ERROR_PARAM, factory_verify_and_repair(NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_factory_public_macros_are_stable);
    RUN_TEST(test_factory_crc16_incremental_matches_one_shot);
    RUN_TEST(test_factory_verify_entry_crc_accepts_and_rejects_entries);
    RUN_TEST(test_factory_status_strings_cover_public_errors);
    RUN_TEST(test_factory_defensive_guards_before_init);
    return UNITY_END();
}
