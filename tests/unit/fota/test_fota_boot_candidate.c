#include "unity.h"

#include <stdint.h>
#include <string.h>

#include "xy_fota.h"
#include "xy_fota_boot.h"

#define STORAGE_BASE 0x00100000U
#define EXECUTION_BASE 0x08000000U
#define JOURNAL_MAGIC 0x58424A52U
#define JOURNAL_COMMIT UINT64_C(0x434F4D54434F4D54)
#define JOURNAL_INSTALLED 2U
#define JOURNAL_CONFIRMED 3U
#define JOURNAL_ROLLED_BACK 4U
#define JOURNAL_RESTAGE_AUTHORIZED 5U

typedef struct {
    uint32_t magic;
    uint32_t generation;
    uint32_t state;
    uint32_t image_version;
    uint32_t image_size;
    uint32_t image_crc32;
    uint32_t record_crc32;
    uint32_t boot_attempts;
    uint64_t commit;
} test_boot_journal_record_t;

static uint8_t storage[1024];
static int read_result;
static unsigned int handoff_calls;
static uint8_t handoff_slot;
static uint32_t handoff_version;
static uint8_t execution[1024];
static unsigned int erase_calls;
static unsigned int write_calls;
static int execution_read_result;
static int journal_read_result;
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
    if (execution_read_result != XY_FOTA_OK) {
        return execution_read_result;
    }
    if (address < EXECUTION_BASE || size > sizeof(execution) - (address - EXECUTION_BASE)) {
        return XY_FOTA_FLASH_ERROR;
    }
    memcpy(data, execution + address - EXECUTION_BASE, size);
    return XY_FOTA_OK;
}

static int journal_read(uint32_t address, uint8_t *data, uint32_t size)
{
    if (journal_read_result != XY_FOTA_OK) {
        return journal_read_result;
    }
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

static void write_journal_record(uint32_t slot, uint32_t generation, uint32_t state,
                                 const xy_fota_boot_candidate_header_t *header)
{
    test_boot_journal_record_t record = {
        .magic = JOURNAL_MAGIC,
        .generation = generation,
        .state = state,
        .image_version = header->image_version,
        .image_size = header->image_size,
        .image_crc32 = header->image_crc32,
        .boot_attempts = 0U,
        .commit = JOURNAL_COMMIT,
    };

    record.record_crc32 =
        xy_fota_calc_crc32((const uint8_t *)&record,
                           offsetof(test_boot_journal_record_t, record_crc32));
    memcpy(journal + slot * 256U, &record, sizeof(record));
}

static const test_boot_journal_record_t *latest_journal_record(void)
{
    const test_boot_journal_record_t *first = (const test_boot_journal_record_t *)journal;
    const test_boot_journal_record_t *second =
        (const test_boot_journal_record_t *)(journal + 256U);

    return (int32_t)(second->generation - first->generation) > 0 ? second : first;
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
    execution_read_result = XY_FOTA_OK;
    journal_read_result = XY_FOTA_OK;
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

static void test_rejects_unaligned_install_layout_before_erasing_execution_slot(void)
{
    xy_fota_boot_candidate_header_t header;
    xy_fota_boot_candidate_config_t config = default_config();
    xy_fota_boot_install_ops_t ops = {
        .erase = execution_erase,
        .write = execution_write,
        .read = execution_read,
        .program_granule = 8U,
        .erase_granule = 256U,
    };
    uint8_t image[384];
    uint32_t reset_handler;

    build_candidate(&header, image, sizeof(image));
    config.execution_base += 4U;
    ((xy_fota_boot_candidate_header_t *)storage)->load_address = config.execution_base;
    reset_handler = config.execution_base + 0x101U;
    memcpy(storage + sizeof(header) + sizeof(uint32_t), &reset_handler, sizeof(reset_handler));
    ((xy_fota_boot_candidate_header_t *)storage)->image_crc32 =
        xy_fota_calc_crc32(storage + sizeof(header), sizeof(image));

    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          xy_fota_boot_candidate_install(&config, &ops, NULL));
    TEST_ASSERT_EQUAL_UINT(0U, erase_calls);
    TEST_ASSERT_EQUAL_UINT(0U, write_calls);

    config = default_config();
    build_candidate(&header, image, sizeof(image));
    ops.program_granule = 7U;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          xy_fota_boot_candidate_install(&config, &ops, NULL));
    TEST_ASSERT_EQUAL_UINT(0U, erase_calls);
    TEST_ASSERT_EQUAL_UINT(0U, write_calls);
}

static void test_install_once_rejects_invalid_layout_before_journaling(void)
{
    xy_fota_boot_candidate_header_t header;
    xy_fota_boot_candidate_config_t config = default_config();
    xy_fota_boot_install_ops_t install_ops = {
        .erase = execution_erase,
        .write = execution_write,
        .read = execution_read,
        .program_granule = 7U,
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
    int installed = 1;

    build_candidate(&header, image, sizeof(image));

    TEST_ASSERT_EQUAL_INT(
        XY_FOTA_INVALID_PARAM,
        xy_fota_boot_candidate_install_once(&config, &install_ops, &journal_config, &installed));
    TEST_ASSERT_FALSE(installed);
    TEST_ASSERT_EQUAL_UINT(0U, erase_calls);
    TEST_ASSERT_EQUAL_UINT(0U, write_calls);
    TEST_ASSERT_EQUAL_UINT(0U, journal_erase_calls);
    TEST_ASSERT_EQUAL_UINT(0U, journal_write_calls);
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

static void test_install_journal_revalidates_execution_slot_before_skipping_install(void)
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

    execution[100U] ^= 1U;
    installed = 0;
    TEST_ASSERT_EQUAL_INT(
        XY_FOTA_OK,
        xy_fota_boot_candidate_install_once(&config, &install_ops, &journal_config, &installed));
    TEST_ASSERT_TRUE(installed);
    TEST_ASSERT_EQUAL_UINT(2U, erase_calls);
    TEST_ASSERT_EQUAL_MEMORY(image, execution, sizeof(image));

    execution_read_result = XY_FOTA_FLASH_ERROR;
    installed = 1;
    TEST_ASSERT_EQUAL_INT(
        XY_FOTA_FLASH_ERROR,
        xy_fota_boot_candidate_install_once(&config, &install_ops, &journal_config, &installed));
    TEST_ASSERT_FALSE(installed);
    TEST_ASSERT_EQUAL_UINT(2U, erase_calls);
}

static void test_install_journal_rejects_ambiguous_valid_generations_without_flash_changes(void)
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
    int installed = 1;

    build_candidate(&header, image, sizeof(image));
    write_journal_record(0U, 7U, JOURNAL_INSTALLED, &header);
    write_journal_record(1U, 7U, JOURNAL_INSTALLED, &header);
    ((test_boot_journal_record_t *)(journal + 256U))->image_version++;
    ((test_boot_journal_record_t *)(journal + 256U))->record_crc32 = xy_fota_calc_crc32(
        journal + 256U, offsetof(test_boot_journal_record_t, record_crc32));

    TEST_ASSERT_EQUAL_INT(
        XY_FOTA_FLASH_ERROR,
        xy_fota_boot_candidate_install_once(&config, &install_ops, &journal_config, &installed));
    TEST_ASSERT_FALSE(installed);
    TEST_ASSERT_EQUAL_UINT(0U, erase_calls);
    TEST_ASSERT_EQUAL_UINT(0U, journal_erase_calls);
    TEST_ASSERT_EQUAL_UINT(0U, journal_write_calls);

    write_journal_record(0U, 1U, JOURNAL_INSTALLED, &header);
    write_journal_record(1U, 0x80000001U, JOURNAL_INSTALLED, &header);
    installed = 1;
    TEST_ASSERT_EQUAL_INT(
        XY_FOTA_FLASH_ERROR,
        xy_fota_boot_candidate_install_once(&config, &install_ops, &journal_config, &installed));
    TEST_ASSERT_FALSE(installed);
    TEST_ASSERT_EQUAL_UINT(0U, erase_calls);
    TEST_ASSERT_EQUAL_UINT(0U, journal_erase_calls);
    TEST_ASSERT_EQUAL_UINT(0U, journal_write_calls);
}

static void test_install_journal_confirms_first_candidate_boot_attempt(void)
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
    int rollback_required = 1;

    build_candidate(&header, image, sizeof(image));
    TEST_ASSERT_EQUAL_INT(
        XY_FOTA_OK,
        xy_fota_boot_candidate_install_once(&config, &install_ops, &journal_config, &installed));
    TEST_ASSERT_TRUE(installed);
    TEST_ASSERT_EQUAL_INT(
        XY_FOTA_OK,
        xy_fota_boot_candidate_record_attempt(&config, &install_ops, &journal_config, 3U,
                                              &rollback_required));
    TEST_ASSERT_FALSE(rollback_required);
    TEST_ASSERT_EQUAL_UINT32(JOURNAL_INSTALLED, latest_journal_record()->state);
    TEST_ASSERT_EQUAL_UINT32(1U, latest_journal_record()->boot_attempts);

    TEST_ASSERT_EQUAL_INT(
        XY_FOTA_OK,
        xy_fota_boot_candidate_confirm(&config, &install_ops, &journal_config));
    TEST_ASSERT_EQUAL_UINT32(JOURNAL_CONFIRMED, latest_journal_record()->state);
    TEST_ASSERT_EQUAL_UINT32(0U, latest_journal_record()->boot_attempts);

    installed = 1;
    TEST_ASSERT_EQUAL_INT(
        XY_FOTA_OK,
        xy_fota_boot_candidate_install_once(&config, &install_ops, &journal_config, &installed));
    TEST_ASSERT_FALSE(installed);
    TEST_ASSERT_EQUAL_UINT(1U, erase_calls);

    rollback_required = 1;
    TEST_ASSERT_EQUAL_INT(
        XY_FOTA_OK,
        xy_fota_boot_candidate_record_attempt(&config, &install_ops, &journal_config, 3U,
                                              &rollback_required));
    TEST_ASSERT_FALSE(rollback_required);
    TEST_ASSERT_EQUAL_UINT32(JOURNAL_CONFIRMED, latest_journal_record()->state);
    TEST_ASSERT_EQUAL_UINT32(0U, latest_journal_record()->boot_attempts);
}

static void test_install_journal_treats_legacy_reserved_field_as_zero_attempts(void)
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
    int rollback_required = 1;

    build_candidate(&header, image, sizeof(image));
    memcpy(execution, image, sizeof(image));
    write_journal_record(0U, 7U, JOURNAL_INSTALLED, &header);
    ((test_boot_journal_record_t *)journal)->boot_attempts = UINT32_MAX;
    ((test_boot_journal_record_t *)journal)->record_crc32 = xy_fota_calc_crc32(
        journal, offsetof(test_boot_journal_record_t, record_crc32));

    TEST_ASSERT_EQUAL_INT(
        XY_FOTA_OK,
        xy_fota_boot_candidate_record_attempt(&config, &install_ops, &journal_config, 3U,
                                              &rollback_required));
    TEST_ASSERT_FALSE(rollback_required);
    TEST_ASSERT_EQUAL_UINT32(1U, latest_journal_record()->boot_attempts);
}

static void test_install_journal_rolls_back_after_bounded_unconfirmed_boots(void)
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
    int rollback_required = 0;

    build_candidate(&header, image, sizeof(image));
    TEST_ASSERT_EQUAL_INT(
        XY_FOTA_OK,
        xy_fota_boot_candidate_install_once(&config, &install_ops, &journal_config, &installed));
    TEST_ASSERT_EQUAL_INT(
        XY_FOTA_OK,
        xy_fota_boot_candidate_record_attempt(&config, &install_ops, &journal_config, 2U,
                                              &rollback_required));
    TEST_ASSERT_FALSE(rollback_required);
    TEST_ASSERT_EQUAL_UINT32(1U, latest_journal_record()->boot_attempts);

    TEST_ASSERT_EQUAL_INT(
        XY_FOTA_OK,
        xy_fota_boot_candidate_record_attempt(&config, &install_ops, &journal_config, 2U,
                                              &rollback_required));
    TEST_ASSERT_TRUE(rollback_required);
    TEST_ASSERT_EQUAL_UINT32(JOURNAL_ROLLED_BACK, latest_journal_record()->state);
    TEST_ASSERT_EQUAL_UINT32(2U, latest_journal_record()->boot_attempts);

    TEST_ASSERT_EQUAL_INT(
        XY_FOTA_NO_IMAGE,
        xy_fota_boot_candidate_confirm(&config, &install_ops, &journal_config));
    installed = 1;
    TEST_ASSERT_EQUAL_INT(
        XY_FOTA_VERSION_ERROR,
        xy_fota_boot_candidate_install_once(&config, &install_ops, &journal_config, &installed));
    TEST_ASSERT_FALSE(installed);
    TEST_ASSERT_EQUAL_UINT(1U, erase_calls);
}

static void test_attempt_and_confirmation_fail_closed_without_durable_commit(void)
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
    int rollback_required = 1;
    unsigned int durable_writes;

    build_candidate(&header, image, sizeof(image));
    TEST_ASSERT_EQUAL_INT(
        XY_FOTA_OK,
        xy_fota_boot_candidate_install_once(&config, &install_ops, &journal_config, &installed));
    durable_writes = journal_write_calls;
    journal_fail_write = 1;
    TEST_ASSERT_EQUAL_INT(
        XY_FOTA_FLASH_ERROR,
        xy_fota_boot_candidate_record_attempt(&config, &install_ops, &journal_config, 3U,
                                              &rollback_required));
    TEST_ASSERT_TRUE(rollback_required);
    TEST_ASSERT_EQUAL_UINT(durable_writes, journal_write_calls);

    journal_fail_write = 0;
    journal_read_result = XY_FOTA_FLASH_ERROR;
    TEST_ASSERT_EQUAL_INT(
        XY_FOTA_FLASH_ERROR,
        xy_fota_boot_candidate_confirm(&config, &install_ops, &journal_config));
    TEST_ASSERT_EQUAL_UINT(durable_writes, journal_write_calls);
}

static void test_restage_requires_exact_authorization_without_side_effects(void)
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
    xy_fota_boot_restage_authorization_t authorization;
    uint8_t image[384];
    uint8_t journal_before[sizeof(journal)];
    uint8_t execution_before[sizeof(execution)];

    build_candidate(&header, image, sizeof(image));
    write_journal_record(0U, 7U, JOURNAL_ROLLED_BACK, &header);
    memcpy(journal_before, journal, sizeof(journal));
    memcpy(execution_before, execution, sizeof(execution));
    memset(&authorization, 0, sizeof(authorization));

    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          xy_fota_boot_candidate_authorize_restage(
                              &config, &install_ops, &journal_config, &authorization));
    TEST_ASSERT_EQUAL_MEMORY(journal_before, journal, sizeof(journal));
    TEST_ASSERT_EQUAL_MEMORY(execution_before, execution, sizeof(execution));
    TEST_ASSERT_EQUAL_UINT(0U, journal_erase_calls);
    TEST_ASSERT_EQUAL_UINT(0U, journal_write_calls);
    TEST_ASSERT_EQUAL_UINT(0U, erase_calls);
    TEST_ASSERT_EQUAL_UINT(0U, write_calls);

    authorization.magic = XY_FOTA_BOOT_RESTAGE_AUTHORIZATION_MAGIC;
    authorization.format_version = XY_FOTA_BOOT_RESTAGE_AUTHORIZATION_VERSION;
    authorization.size = sizeof(authorization);
    authorization.image_version = header.image_version;
    authorization.image_size = header.image_size;
    authorization.image_crc32 = header.image_crc32 ^ 1U;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          xy_fota_boot_candidate_authorize_restage(
                              &config, &install_ops, &journal_config, &authorization));
    TEST_ASSERT_EQUAL_MEMORY(journal_before, journal, sizeof(journal));
    TEST_ASSERT_EQUAL_MEMORY(execution_before, execution, sizeof(execution));
}

static void test_restage_flash_failure_preserves_rollback_and_success_allows_install(void)
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
    xy_fota_boot_restage_authorization_t authorization;
    uint8_t image[384];
    uint8_t execution_before[sizeof(execution)];
    int installed = 0;

    build_candidate(&header, image, sizeof(image));
    write_journal_record(0U, 7U, JOURNAL_ROLLED_BACK, &header);
    authorization.magic = XY_FOTA_BOOT_RESTAGE_AUTHORIZATION_MAGIC;
    authorization.format_version = XY_FOTA_BOOT_RESTAGE_AUTHORIZATION_VERSION;
    authorization.size = sizeof(authorization);
    authorization.image_version = header.image_version;
    authorization.image_size = header.image_size;
    authorization.image_crc32 = header.image_crc32;
    memcpy(execution_before, execution, sizeof(execution));

    journal_fail_write = 1;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_FLASH_ERROR,
                          xy_fota_boot_candidate_authorize_restage(
                              &config, &install_ops, &journal_config, &authorization));
    TEST_ASSERT_EQUAL_MEMORY(execution_before, execution, sizeof(execution));
    TEST_ASSERT_EQUAL_UINT32(JOURNAL_ROLLED_BACK,
                             ((const test_boot_journal_record_t *)journal)->state);
    TEST_ASSERT_EQUAL_UINT(0U, erase_calls);
    TEST_ASSERT_EQUAL_UINT(0U, write_calls);
    installed = 1;
    TEST_ASSERT_EQUAL_INT(
        XY_FOTA_VERSION_ERROR,
        xy_fota_boot_candidate_install_once(&config, &install_ops, &journal_config, &installed));
    TEST_ASSERT_FALSE(installed);

    journal_fail_write = 0;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_boot_candidate_authorize_restage(
                              &config, &install_ops, &journal_config, &authorization));
    TEST_ASSERT_EQUAL_UINT32(JOURNAL_RESTAGE_AUTHORIZED, latest_journal_record()->state);
    TEST_ASSERT_EQUAL_INT(
        XY_FOTA_OK,
        xy_fota_boot_candidate_install_once(&config, &install_ops, &journal_config, &installed));
    TEST_ASSERT_TRUE(installed);
    TEST_ASSERT_EQUAL_MEMORY(image, execution, sizeof(image));
    TEST_ASSERT_EQUAL_UINT32(JOURNAL_INSTALLED, latest_journal_record()->state);
}

static void test_reviewed_restage_requires_exact_source_commit(void)
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
    const uint32_t expected_commit[5] = {1U, 2U, 3U, 4U, 5U};
    xy_fota_boot_reviewed_restage_authorization_t authorization = {
        .candidate = {
            .magic = XY_FOTA_BOOT_RESTAGE_AUTHORIZATION_MAGIC,
            .format_version = XY_FOTA_BOOT_RESTAGE_AUTHORIZATION_VERSION,
            .size = sizeof(xy_fota_boot_restage_authorization_t),
        },
        .source_commit = {1U, 2U, 3U, 4U, 6U},
    };
    uint8_t image[384];

    build_candidate(&header, image, sizeof(image));
    write_journal_record(0U, 7U, JOURNAL_ROLLED_BACK, &header);
    authorization.candidate.image_version = header.image_version;
    authorization.candidate.image_size = header.image_size;
    authorization.candidate.image_crc32 = header.image_crc32;
    TEST_ASSERT_EQUAL_INT(
        XY_FOTA_INVALID_PARAM,
        xy_fota_boot_candidate_authorize_reviewed_restage(
            &config, &install_ops, &journal_config, &authorization, expected_commit));
    TEST_ASSERT_EQUAL_UINT(0U, journal_write_calls);
    authorization.source_commit[4] = 5U;
    TEST_ASSERT_EQUAL_INT(
        XY_FOTA_OK, xy_fota_boot_candidate_authorize_reviewed_restage(
                        &config, &install_ops, &journal_config, &authorization, expected_commit));
    TEST_ASSERT_EQUAL_UINT32(JOURNAL_RESTAGE_AUTHORIZED, latest_journal_record()->state);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_validates_header_image_crc_and_cortex_m_vectors_before_handoff);
    RUN_TEST(test_rejects_malformed_layout_crc_and_vectors_without_handoff);
    RUN_TEST(test_rejects_read_failures_and_out_of_range_images);
    RUN_TEST(test_installs_validated_candidate_and_verifies_execution_slot);
    RUN_TEST(test_rejects_unaligned_install_layout_before_erasing_execution_slot);
    RUN_TEST(test_install_once_rejects_invalid_layout_before_journaling);
    RUN_TEST(test_install_journal_skips_same_installed_candidate_after_restart);
    RUN_TEST(test_install_journal_recovers_failed_and_corrupt_records);
    RUN_TEST(test_install_journal_revalidates_execution_slot_before_skipping_install);
    RUN_TEST(test_install_journal_rejects_ambiguous_valid_generations_without_flash_changes);
    RUN_TEST(test_install_journal_confirms_first_candidate_boot_attempt);
    RUN_TEST(test_install_journal_treats_legacy_reserved_field_as_zero_attempts);
    RUN_TEST(test_install_journal_rolls_back_after_bounded_unconfirmed_boots);
    RUN_TEST(test_attempt_and_confirmation_fail_closed_without_durable_commit);
    RUN_TEST(test_restage_requires_exact_authorization_without_side_effects);
    RUN_TEST(test_restage_flash_failure_preserves_rollback_and_success_allows_install);
    RUN_TEST(test_reviewed_restage_requires_exact_source_commit);
    return UNITY_END();
}
