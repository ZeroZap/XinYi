#include "xy_factory.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

static void test_factory_public_macros_are_stable(void)
{
    assert(FACTORY_ALIGN_SIZE(0U) == 0U);
    assert(FACTORY_ALIGN_SIZE(1U) == XY_FACTORY_ALIGN_SIZE);
    assert(FACTORY_ALIGN_SIZE(9U) == 16U);
    assert(FACTORY_TLV_HEADER_SIZE == 5U);
    assert(FACTORY_TLV_TOTAL_SIZE(7U) == 12U);
    assert(FACTORY_TOTAL_SIZE(128U) == 256U);
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

    assert(one_shot == incremental);
    assert(one_shot == 0xC1F8U);
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

    assert(factory_verify_entry_crc(NULL, &entry));
    entry.data[0] ^= 0x01U;
    assert(!factory_verify_entry_crc(NULL, &entry));
    assert(!factory_verify_entry_crc(NULL, NULL));
}

static void test_factory_status_strings_cover_public_errors(void)
{
    assert(strcmp(factory_status_str(FACTORY_OK), "OK") == 0);
    assert(strcmp(factory_status_str(FACTORY_ERROR_PARAM), "Parameter error") == 0);
    assert(strcmp(factory_status_str(FACTORY_ERROR_NO_SPACE), "No space left") == 0);
    assert(strcmp(factory_status_str(FACTORY_ERROR_CRC), "CRC verify failed") == 0);
    assert(strcmp(factory_status_str((factory_status_t)1234), "Unknown error") == 0);
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

    assert(factory_init(NULL, NULL) == FACTORY_ERROR_PARAM);
    assert(factory_write(NULL, FACTORY_TYPE_DEVICE_ID, buffer, 1U) == FACTORY_ERROR_PARAM);
    assert(factory_write(&handle, FACTORY_TYPE_DEVICE_ID, buffer, 1U) == FACTORY_ERROR_PARAM);
    assert(factory_read(NULL, FACTORY_TYPE_DEVICE_ID, buffer, &len) == FACTORY_ERROR_PARAM);
    assert(factory_delete(NULL, FACTORY_TYPE_DEVICE_ID) == FACTORY_ERROR_PARAM);
    assert(!factory_exists(NULL, FACTORY_TYPE_DEVICE_ID, &len));
    assert(factory_enum(NULL, types, 2U, &count) == FACTORY_ERROR_PARAM);
    assert(factory_format(NULL) == FACTORY_ERROR_PARAM);
    assert(factory_get_info(NULL, &used, NULL, NULL) == FACTORY_ERROR_PARAM);
    assert(factory_verify_and_repair(NULL) == FACTORY_ERROR_PARAM);
}

int main(void)
{
    test_factory_public_macros_are_stable();
    test_factory_crc16_incremental_matches_one_shot();
    test_factory_verify_entry_crc_accepts_and_rejects_entries();
    test_factory_status_strings_cover_public_errors();
    test_factory_defensive_guards_before_init();
    return 0;
}
