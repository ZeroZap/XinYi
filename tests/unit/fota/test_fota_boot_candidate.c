#include "unity.h"

#include <stdint.h>
#include <string.h>

#include "xy_fota.h"
#include "xy_fota_boot.h"

#define STORAGE_BASE 0x00100000U
#define EXECUTION_BASE 0x08000000U

static uint8_t storage[1024];
static int read_result;
static unsigned int handoff_calls;
static uint8_t handoff_slot;
static uint32_t handoff_version;
static uint8_t execution[1024];
static unsigned int erase_calls;
static unsigned int write_calls;
static uint8_t journal[512];
static unsigned int journal_erase_calls;
static unsigned int journal_write_calls;
static int journal_fail_write;

static int candidate_read(uint32_t address, uint8_t *data, uint32_t size)
{
    if (read_result != XY_FOTA_OK) {
        return read_result;
    }
    if (address < STORAGE_BASE || size > sizeof(storage) - (address - STORAGE_BASE)) {
        return XY_FOTA_FLASH_ERROR;
    }
    memcpy(data, &storage[address - STORAGE_BASE], size);
    return XY_FOTA_OK;
}

static int execution_erase(uint32_t address, uint32_t size)
{
    if (address != EXECUTION_BASE || size > sizeof(execution)) {
        return XY_FOTA_FLASH_ERROR;
    }
    memset(execution, 0xFF, size);
    ++erase_calls;
    return XY_FOTA_OK;
}

static int execution_write(uint32_t address, const uint8_t *data, uint32_t size)
{
    if (address < EXECUTION_BASE || size > sizeof(execution) - (address - EXECUTION_BASE)) {
        return XY_FOTA_FLASH_ERROR;
    }
    memcpy(execution + address - EXECUTION_BASE, data, size);
    ++write_calls;
    return XY_FOTA_OK;
}

static int execution_read(uint32_t address, uint8_t *data, uint32_t size)
{
    if (address < EXECUTION_BASE || size > sizeof(execution) - (address - EXECUTION_BASE)) {
        return XY_FOTA_FLASH_ERROR;
    }
    memcpy(data, execution + address - EXECUTION_BASE, size);
    return XY_FOTA_OK;
}

static int journal_read(uint32_t address, uint8_t *data, uint32_t size)
{
    if (address < 0x0807E000U || size > sizeof(journal) - (address - 0x0807E000U)) {
        return XY_FOTA_FLASH_ERROR;
    }
    memcpy(data, journal + address - 0x0807E000U, size);
    return XY_FOTA_OK;
}

static int journal_erase(uint32_t address, uint32_t size)
{
    if (address < 0x0807E000U || size > sizeof(journal) - (address - 0x0807E000U)) {
        return XY_FOTA_FLASH_ERROR;
    }
    memset(journal + address - 0x0807E000U, 0xFF, size);
    ++journal_erase_calls;
    return XY_FOTA_OK;
}

static int journal_write(uint32_t address, const uint8_t *data, uint32_t size)
{
    if (journal_fail_write || address < 0x0807E000U ||
        size > sizeof(journal) - (address - 0x0807E000U)) {
        return XY_FOTA_FLASH_ERROR;
    }
    for (uint32_t index = 0U; index < size; ++index) {
        journal[address - 0x0807E000U + index] &= data[index];
    }
    ++journal_write_calls;
    return XY_FOTA_OK;
}

static int handoff(uint8_t slot, uint32_t version, void *user_data)
{
    TEST_ASSERT_EQUAL_PTR(&handoff_calls, user_data);
    ++handoff_calls;
    handoff_slot = slot;
    handoff_version = version;
    return XY_FOTA_OK;
}

static xy_fota_boot_candidate_config_t default_config(void)
{
    xy_fota_boot_candidate_config_t config = {
        .storage_address = STORAGE_BASE,
        .storage_size = sizeof(storage),
        .execution_base = EXECUTION_BASE,
        .execution_limit = 0x0807E000U,
        .sram_base = 0x20000000U,
        .sram_limit = 0x20018000U,
        .sram2_base = 0x10000000U,
        .sram2_limit = 0x10008000U,
        .read = candidate_read,
    };
    return config;
}

static void build_candidate(xy_fota_boot_candidate_header_t *header, uint8_t *image,
                            uint32_t image_size)
{
    uint32_t initial_sp = 0x20018000U;
    uint32_t reset_handler = EXECUTION_BASE + 0x101U;

    memset(storage, 0xFF, sizeof(storage));
    memset(image, 0xA5, image_size);
    memcpy(image, &initial_sp, sizeof(initial_sp));
    memcpy(image + sizeof(initial_sp), &reset_handler, sizeof(reset_handler));
    memset(header, 0, sizeof(*header));
    header->magic = XY_FOTA_BOOT_CANDIDATE_MAGIC;
    header->format_version = XY_FOTA_BOOT_CANDIDATE_FORMAT_VERSION;
    header->header_size = sizeof(*header);
    header->image_offset = sizeof(*header);
    header->image_size = image_size;
    header->image_version = 7U;
    header->load_address = EXECUTION_BASE;
    header->image_crc32 = xy_fota_calc_crc32(image, image_size);
    memcpy(storage, header, sizeof(*header));
    memcpy(storage + sizeof(*header), image, image_size);
}

void setUp(void)
{
    memset(storage, 0, sizeof(storage));
    read_result = XY_FOTA_OK;
    handoff_calls = 0U;
    handoff_slot = UINT8_MAX;
    handoff_version = 0U;
    memset(execution, 0, sizeof(execution));
    erase_calls = 0U;
    write_calls = 0U;
    memset(journal, 0xFF, sizeof(journal));
    journal_erase_calls = 0U;
    journal_write_calls = 0U;
    journal_fail_write = 0;
}

void tearDown(void) {}

static void test_validates_header_image_crc_and_cortex_m_vectors_before_handoff(void)
{
    xy_fota_boot_candidate_header_t header;
    xy_fota_boot_candidate_header_t validated;
    xy_fota_boot_candidate_config_t config = default_config();
    uint8_t image[384];

    build_candidate(&header, image, sizeof(image));

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_boot_candidate_validate(&config, &validated));
    TEST_ASSERT_EQUAL_UINT32(sizeof(image), validated.image_size);
    TEST_ASSERT_EQUAL_UINT32(7U, validated.image_version);
    TEST_ASSERT_EQUAL_INT(
        XY_FOTA_OK,
        xy_fota_boot_candidate_handoff(&config, 1U, handoff, &handoff_calls, &validated));
    TEST_ASSERT_EQUAL_UINT(1U, handoff_calls);
    TEST_ASSERT_EQUAL_UINT8(1U, handoff_slot);
    TEST_ASSERT_EQUAL_UINT32(7U, handoff_version);
}

static void test_rejects_malformed_layout_crc_and_vectors_without_handoff(void)
{
    xy_fota_boot_candidate_header_t header;
    xy_fota_boot_candidate_config_t config = default_config();
    uint8_t image[384];
    uint32_t invalid_vector;

    build_candidate(&header, image, sizeof(image));
    ((xy_fota_boot_candidate_header_t *)storage)->image_offset = sizeof(header) - 1U;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          xy_fota_boot_candidate_handoff(&config, 1U, handoff,
                                                         &handoff_calls, NULL));
    TEST_ASSERT_EQUAL_UINT(0U, handoff_calls);

    build_candidate(&header, image, sizeof(image));
    storage[sizeof(header) + 31U] ^= 1U;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_CRC_ERROR, xy_fota_boot_candidate_validate(&config, NULL));

    build_candidate(&header, image, sizeof(image));
    invalid_vector = 0x20000004U;
    memcpy(storage + sizeof(header), &invalid_vector, sizeof(invalid_vector));
    ((xy_fota_boot_candidate_header_t *)storage)->image_crc32 =
        xy_fota_calc_crc32(storage + sizeof(header), sizeof(image));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          xy_fota_boot_candidate_validate(&config, NULL));

    build_candidate(&header, image, sizeof(image));
    invalid_vector = EXECUTION_BASE + 0x100U;
    memcpy(storage + sizeof(header) + sizeof(uint32_t), &invalid_vector,
           sizeof(invalid_vector));
    ((xy_fota_boot_candidate_header_t *)storage)->image_crc32 =
        xy_fota_calc_crc32(storage + sizeof(header), sizeof(image));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          xy_fota_boot_candidate_validate(&config, NULL));
    TEST_ASSERT_EQUAL_UINT(0U, handoff_calls);
}

static void test_rejects_read_failures_and_out_of_range_images(void)
{
    xy_fota_boot_candidate_header_t header;
    xy_fota_boot_candidate_config_t config = default_config();
    uint8_t image[384];

    build_candidate(&header, image, sizeof(image));
    read_result = XY_FOTA_FLASH_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_FLASH_ERROR, xy_fota_boot_candidate_validate(&config, NULL));

    build_candidate(&header, image, sizeof(image));
    read_result = XY_FOTA_OK;
    config.storage_size = sizeof(header) + 100U;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM, xy_fota_boot_candidate_validate(&config, NULL));

    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM, xy_fota_boot_candidate_validate(NULL, NULL));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          xy_fota_boot_candidate_handoff(&config, 1U, NULL, NULL, NULL));
}

static void test_installs_validated_candidate_and_verifies_execution_slot(void)
{
    xy_fota_boot_candidate_header_t header;
    xy_fota_boot_candidate_header_t installed;
    xy_fota_boot_candidate_config_t config = default_config();
    xy_fota_boot_install_ops_t ops = {
        .erase = execution_erase,
        .write = execution_write,
        .read = execution_read,
        .program_granule = 8U,
        .erase_granule = 256U,
    };
    uint8_t image[384];

    build_candidate(&header, image, sizeof(image));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_boot_candidate_install(&config, &ops, &installed));
    TEST_ASSERT_EQUAL_UINT(1U, erase_calls);
    TEST_ASSERT_GREATER_THAN_UINT(1U, write_calls);
    TEST_ASSERT_EQUAL_MEMORY(image, execution, sizeof(image));
    TEST_ASSERT_EQUAL_UINT32(sizeof(image), installed.image_size);

    storage[sizeof(header) + 20U] ^= 1U;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_CRC_ERROR,
                          xy_fota_boot_candidate_install(&config, &ops, NULL));
    TEST_ASSERT_EQUAL_UINT(1U, erase_calls);
}

static void test_install_journal_skips_same_installed_candidate_after_restart(void)
{
    xy_fota_boot_candidate_header_t header;
    xy_fota_boot_candidate_config_t config = default_config();
    xy_fota_boot_install_ops_t install_ops = {
        .erase = execution_erase,
        .write = execution_write,
        .read = execution_read,
        .program_granule = 8U,
        .erase_granule = 256U,
    };
    xy_fota_boot_journal_config_t journal_config = {
        .address = 0x0807E000U,
        .slot_size = 256U,
        .read = journal_read,
        .erase = journal_erase,
        .write = journal_write,
    };
    uint8_t image[384];
    int installed = 0;

    build_candidate(&header, image, sizeof(image));
    TEST_ASSERT_EQUAL_INT(
        XY_FOTA_OK,
        xy_fota_boot_candidate_install_once(&config, &install_ops, &journal_config, &installed));
    TEST_ASSERT_TRUE(installed);
    TEST_ASSERT_EQUAL_UINT(1U, erase_calls);
    installed = 1;
    TEST_ASSERT_EQUAL_INT(
        XY_FOTA_OK,
        xy_fota_boot_candidate_install_once(&config, &install_ops, &journal_config, &installed));
    TEST_ASSERT_FALSE(installed);
    TEST_ASSERT_EQUAL_UINT(1U, erase_calls);
}

static void test_install_journal_recovers_failed_and_corrupt_records(void)
{
    xy_fota_boot_candidate_header_t header;
    xy_fota_boot_candidate_config_t config = default_config();
    xy_fota_boot_install_ops_t install_ops = {
        .erase = execution_erase,
        .write = execution_write,
        .read = execution_read,
        .program_granule = 8U,
        .erase_granule = 256U,
    };
    xy_fota_boot_journal_config_t journal_config = {
        .address = 0x0807E000U,
        .slot_size = 256U,
        .read = journal_read,
        .erase = journal_erase,
        .write = journal_write,
    };
    uint8_t image[384];
    int installed = 0;

    build_candidate(&header, image, sizeof(image));
    journal_fail_write = 1;
    TEST_ASSERT_EQUAL_INT(
        XY_FOTA_FLASH_ERROR,
        xy_fota_boot_candidate_install_once(&config, &install_ops, &journal_config, &installed));
    TEST_ASSERT_EQUAL_UINT(0U, erase_calls);
    journal_fail_write = 0;
    TEST_ASSERT_EQUAL_INT(
        XY_FOTA_OK,
        xy_fota_boot_candidate_install_once(&config, &install_ops, &journal_config, &installed));
    TEST_ASSERT_TRUE(installed);

    journal[256U + 8U] ^= 1U;
    TEST_ASSERT_EQUAL_INT(
        XY_FOTA_OK,
        xy_fota_boot_candidate_install_once(&config, &install_ops, &journal_config, &installed));
    TEST_ASSERT_TRUE(installed);
    TEST_ASSERT_EQUAL_UINT(2U, erase_calls);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_validates_header_image_crc_and_cortex_m_vectors_before_handoff);
    RUN_TEST(test_rejects_malformed_layout_crc_and_vectors_without_handoff);
    RUN_TEST(test_rejects_read_failures_and_out_of_range_images);
    RUN_TEST(test_installs_validated_candidate_and_verifies_execution_slot);
    RUN_TEST(test_install_journal_skips_same_installed_candidate_after_restart);
    RUN_TEST(test_install_journal_recovers_failed_and_corrupt_records);
    return UNITY_END();
}
