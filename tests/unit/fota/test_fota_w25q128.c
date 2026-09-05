#include "unity.h"

#include <stdint.h>
#include <string.h>

#include "xy_fota.h"
#include "xy_fota_w25q128.h"

static uint32_t erased[8];
static size_t erase_count;
static uint8_t storage[8192];
static xy_w25q128_t flash;
static uint32_t programmed_address[4];
static size_t programmed_length[4];
static size_t program_count;
static xy_w25q128_status_t erase_result;
static xy_w25q128_status_t program_result;
static xy_w25q128_status_t read_result;

xy_w25q128_status_t xy_w25q128_sector_erase(xy_w25q128_t *device, uint32_t address,
                                            uint32_t timeout_ms)
{
    TEST_ASSERT_EQUAL_PTR(&flash, device);
    TEST_ASSERT_GREATER_THAN_UINT32(0U, timeout_ms);
    if (erase_result != XY_W25Q128_OK) {
        return erase_result;
    }
    erased[erase_count++] = address;
    memset(&storage[address - 0x100000U], 0xFF, 4096U);
    return XY_W25Q128_OK;
}

xy_w25q128_status_t xy_w25q128_page_program(xy_w25q128_t *device, uint32_t address,
                                            const uint8_t *data, size_t length,
                                            uint32_t timeout_ms)
{
    TEST_ASSERT_EQUAL_PTR(&flash, device);
    TEST_ASSERT_GREATER_THAN_UINT32(0U, timeout_ms);
    if (program_result != XY_W25Q128_OK) {
        return program_result;
    }
    programmed_address[program_count] = address;
    programmed_length[program_count++] = length;
    memcpy(&storage[address - 0x100000U], data, length);
    return XY_W25Q128_OK;
}

xy_w25q128_status_t xy_w25q128_read(xy_w25q128_t *device, uint32_t address, uint8_t *data,
                                    size_t length)
{
    TEST_ASSERT_EQUAL_PTR(&flash, device);
    if (read_result != XY_W25Q128_OK) {
        return read_result;
    }
    memcpy(data, &storage[address - 0x100000U], length);
    return XY_W25Q128_OK;
}

void setUp(void)
{
    (void)xy_fota_w25q128_ops()->deinit();
    memset(&flash, 0, sizeof(flash));
    flash.initialized = 1U;
    flash.capacity = 16U * 1024U * 1024U;
    memset(storage, 0, sizeof(storage));
    memset(erased, 0, sizeof(erased));
    memset(programmed_address, 0, sizeof(programmed_address));
    memset(programmed_length, 0, sizeof(programmed_length));
    erase_count = 0U;
    program_count = 0U;
    erase_result = XY_W25Q128_OK;
    program_result = XY_W25Q128_OK;
    read_result = XY_W25Q128_OK;
}

void tearDown(void) {}

static void test_adapter_erases_candidate_range_and_splits_page_writes(void)
{
    uint8_t payload[300];
    uint8_t actual[300] = {0};
    const xy_fota_flash_ops_t *ops;

    for (size_t index = 0U; index < sizeof(payload); ++index) {
        payload[index] = (uint8_t)index;
    }
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_w25q128_bind(&flash, 0x100000U, sizeof(storage), 100U));
    ops = xy_fota_w25q128_ops();
    TEST_ASSERT_NOT_NULL(ops);
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, ops->init());
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, ops->erase(0x100000U, 5000U));
    TEST_ASSERT_EQUAL_UINT(2U, erase_count);
    TEST_ASSERT_EQUAL_HEX32(0x100000U, erased[0]);
    TEST_ASSERT_EQUAL_HEX32(0x101000U, erased[1]);
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, ops->write(0x1000F0U, payload, sizeof(payload)));
    TEST_ASSERT_EQUAL_UINT(3U, program_count);
    TEST_ASSERT_EQUAL_HEX32(0x1000F0U, programmed_address[0]);
    TEST_ASSERT_EQUAL_UINT(16U, programmed_length[0]);
    TEST_ASSERT_EQUAL_HEX32(0x100100U, programmed_address[1]);
    TEST_ASSERT_EQUAL_UINT(256U, programmed_length[1]);
    TEST_ASSERT_EQUAL_HEX32(0x100200U, programmed_address[2]);
    TEST_ASSERT_EQUAL_UINT(28U, programmed_length[2]);
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, ops->read(0x1000F0U, actual, sizeof(actual)));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(payload, actual, sizeof(payload));
}

static void test_adapter_rejects_unbound_unaligned_and_out_of_range_access(void)
{
    const xy_fota_flash_ops_t *ops = xy_fota_w25q128_ops();
    uint8_t byte = 0;

    TEST_ASSERT_EQUAL_INT(XY_FOTA_FLASH_ERROR, ops->init());
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          xy_fota_w25q128_bind(&flash, 0x100001U, sizeof(storage), 100U));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_w25q128_bind(&flash, 0x100000U, sizeof(storage), 100U));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM, ops->erase(0x100001U, 4096U));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM, ops->write(0x102000U, &byte, 1U));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM, ops->read(0x0FFFFFU, &byte, 1U));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM, ops->write(0x100000U, NULL, 1U));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, ops->write(0x100000U, NULL, 0U));
}

static void test_adapter_maps_driver_failures_and_stops_immediately(void)
{
    const xy_fota_flash_ops_t *ops = xy_fota_w25q128_ops();
    uint8_t bytes[300] = {0};

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_w25q128_bind(&flash, 0x100000U, sizeof(storage), 100U));
    erase_result = XY_W25Q128_TIMEOUT;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_FLASH_ERROR, ops->erase(0x100000U, 4096U));
    TEST_ASSERT_EQUAL_UINT(0U, erase_count);

    program_result = XY_W25Q128_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_FLASH_ERROR, ops->write(0x1000F0U, bytes, sizeof(bytes)));
    TEST_ASSERT_EQUAL_UINT(0U, program_count);

    read_result = XY_W25Q128_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_FLASH_ERROR, ops->read(0x100000U, bytes, sizeof(bytes)));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_adapter_erases_candidate_range_and_splits_page_writes);
    RUN_TEST(test_adapter_rejects_unbound_unaligned_and_out_of_range_access);
    RUN_TEST(test_adapter_maps_driver_failures_and_stops_immediately);
    return UNITY_END();
}
