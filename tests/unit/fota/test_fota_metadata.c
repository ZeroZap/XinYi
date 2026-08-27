#include "xy_fota_metadata.h"
#include "unity.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define METADATA_BASE 0x080A0000U
#define ERASE_SIZE 128U
#define STORAGE_SIZE (2U * ERASE_SIZE)

static uint8_t g_storage[STORAGE_SIZE];
static int g_write_result;
static uint32_t g_fail_write_call;
static uint32_t g_partial_write_size;
static uint32_t g_fail_read_call;
static uint32_t g_read_calls;
static uint32_t g_write_calls;
static uint32_t g_erase_calls;

typedef struct {
    uint32_t magic;
    uint32_t format_version;
    uint32_t generation;
    uint32_t active_version;
    uint32_t min_version;
    uint32_t pending_version;
    uint8_t active_slot;
    uint8_t pending_slot;
    uint8_t boot_attempts;
    uint8_t flags;
    uint32_t crc32;
    uint32_t committed;
} metadata_record_fixture_t;

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
    g_read_calls++;
    if (g_fail_read_call == g_read_calls) {
        return XY_FOTA_FLASH_ERROR;
    }
    memcpy(data, &g_storage[offset], size);
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
    if (g_fail_write_call == 0U || g_write_calls == g_fail_write_call) {
        return g_write_result;
    }
    return XY_FOTA_OK;
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
    g_fail_write_call = 0U;
    g_partial_write_size = 0U;
    g_fail_read_call = 0U;
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

static void test_boot_attempt_policy_rolls_back_after_bounded_failures(void)
{
    xy_fota_metadata_t metadata = initial_metadata();
    bool rollback_required = true;

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_stage_candidate(&metadata, 1U, 4U));
    TEST_ASSERT_EQUAL_UINT32(4U, metadata.pending_version);
    TEST_ASSERT_EQUAL_UINT8(1U, metadata.pending_slot);
    TEST_ASSERT_EQUAL_UINT8(0U, metadata.boot_attempts);
    TEST_ASSERT_BITS_HIGH(XY_FOTA_METADATA_FLAG_PENDING, metadata.flags);

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_record_boot_attempt(&metadata, 3U,
                                                               &rollback_required));
    TEST_ASSERT_FALSE(rollback_required);
    TEST_ASSERT_EQUAL_UINT8(1U, metadata.boot_attempts);
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_record_boot_attempt(&metadata, 3U,
                                                               &rollback_required));
    TEST_ASSERT_FALSE(rollback_required);
    TEST_ASSERT_EQUAL_UINT8(2U, metadata.boot_attempts);
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_record_boot_attempt(&metadata, 3U,
                                                               &rollback_required));
    TEST_ASSERT_TRUE(rollback_required);
    TEST_ASSERT_EQUAL_UINT8(0U, metadata.boot_attempts);
    TEST_ASSERT_EQUAL_UINT8(XY_FOTA_METADATA_NO_SLOT, metadata.pending_slot);
    TEST_ASSERT_EQUAL_UINT32(0U, metadata.pending_version);
    TEST_ASSERT_BITS_LOW(XY_FOTA_METADATA_FLAG_PENDING, metadata.flags);
    TEST_ASSERT_EQUAL_UINT8(0U, metadata.active_slot);
    TEST_ASSERT_EQUAL_UINT32(3U, metadata.active_version);
    TEST_ASSERT_EQUAL_UINT32(3U, metadata.min_version);
}

static void test_boot_attempt_policy_does_not_wrap_counter(void)
{
    xy_fota_metadata_t metadata = initial_metadata();
    bool rollback_required = false;

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_stage_candidate(&metadata, 1U, 4U));
    metadata.boot_attempts = UINT8_MAX;

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_record_boot_attempt(&metadata, UINT8_MAX,
                                                               &rollback_required));
    TEST_ASSERT_TRUE(rollback_required);
    TEST_ASSERT_EQUAL_UINT8(0U, metadata.boot_attempts);
    TEST_ASSERT_EQUAL_UINT8(XY_FOTA_METADATA_NO_SLOT, metadata.pending_slot);
    TEST_ASSERT_EQUAL_UINT32(0U, metadata.pending_version);
    TEST_ASSERT_BITS_LOW(XY_FOTA_METADATA_FLAG_PENDING, metadata.flags);
}

static void test_staging_does_not_reset_an_existing_candidate_attempt_count(void)
{
    xy_fota_metadata_t metadata = initial_metadata();
    xy_fota_metadata_t snapshot;
    bool rollback_required = false;

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_stage_candidate(&metadata, 1U, 4U));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_record_boot_attempt(&metadata, 3U,
                                                               &rollback_required));
    TEST_ASSERT_FALSE(rollback_required);
    snapshot = metadata;

    TEST_ASSERT_EQUAL_INT(XY_FOTA_IN_PROGRESS,
                          xy_fota_metadata_stage_candidate(&metadata, 1U, 4U));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &metadata, sizeof(metadata));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_IN_PROGRESS,
                          xy_fota_metadata_stage_candidate(&metadata, 1U, 5U));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &metadata, sizeof(metadata));
}

static void test_boot_attempt_confirmation_advances_floor_and_clears_pending(void)
{
    xy_fota_metadata_t metadata = initial_metadata();
    bool rollback_required = false;

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_stage_candidate(&metadata, 1U, 4U));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_record_boot_attempt(&metadata, 3U,
                                                               &rollback_required));
    TEST_ASSERT_FALSE(rollback_required);
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_metadata_confirm_candidate(&metadata));
    TEST_ASSERT_EQUAL_UINT8(1U, metadata.active_slot);
    TEST_ASSERT_EQUAL_UINT32(4U, metadata.active_version);
    TEST_ASSERT_EQUAL_UINT32(4U, metadata.min_version);
    TEST_ASSERT_EQUAL_UINT8(XY_FOTA_METADATA_NO_SLOT, metadata.pending_slot);
    TEST_ASSERT_EQUAL_UINT32(0U, metadata.pending_version);
    TEST_ASSERT_EQUAL_UINT8(0U, metadata.boot_attempts);
    TEST_ASSERT_BITS_LOW(XY_FOTA_METADATA_FLAG_PENDING, metadata.flags);
}

static void test_boot_attempt_policy_rejects_invalid_transitions(void)
{
    xy_fota_metadata_t metadata = initial_metadata();
    xy_fota_metadata_t snapshot;
    bool rollback_required = false;

    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          xy_fota_metadata_stage_candidate(NULL, 1U, 4U));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          xy_fota_metadata_stage_candidate(&metadata, 0U, 4U));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_VERSION_ERROR,
                          xy_fota_metadata_stage_candidate(&metadata, 1U, 2U));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_NO_IMAGE,
                          xy_fota_metadata_record_boot_attempt(&metadata, 3U,
                                                               &rollback_required));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_NO_IMAGE,
                          xy_fota_metadata_confirm_candidate(&metadata));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          xy_fota_metadata_record_boot_attempt(&metadata, 0U,
                                                               &rollback_required));

    metadata = initial_metadata();
    metadata.active_slot = 2U;
    snapshot = metadata;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          xy_fota_metadata_stage_candidate(&metadata, 1U, 4U));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &metadata, sizeof(metadata));

    metadata = initial_metadata();
    metadata.flags = XY_FOTA_METADATA_FLAG_PENDING;
    metadata.pending_slot = 1U;
    metadata.pending_version = 4U;
    metadata.active_version = metadata.min_version - 1U;
    snapshot = metadata;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          xy_fota_metadata_record_boot_attempt(&metadata, 3U,
                                                               &rollback_required));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &metadata, sizeof(metadata));

    metadata = initial_metadata();
    metadata.flags = XY_FOTA_METADATA_FLAG_PENDING;
    metadata.pending_slot = 1U;
    metadata.pending_version = 4U;
    metadata.flags |= (uint8_t)(1U << 7);
    snapshot = metadata;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          xy_fota_metadata_confirm_candidate(&metadata));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &metadata, sizeof(metadata));
}

static void test_boot_callbacks_persist_handoff_and_confirmation(void)
{
    xy_fota_metadata_t metadata = initial_metadata();
    xy_fota_metadata_t loaded = {0};

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_flash_commit(&backend, &metadata, NULL));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_boot_handoff(1U, 4U, (void *)&backend));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_metadata_flash_load(&backend, &loaded));
    TEST_ASSERT_EQUAL_UINT8(1U, loaded.pending_slot);
    TEST_ASSERT_EQUAL_UINT32(4U, loaded.pending_version);
    TEST_ASSERT_BITS_HIGH(XY_FOTA_METADATA_FLAG_PENDING, loaded.flags);

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_boot_confirm(1U, 4U, (void *)&backend));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_metadata_flash_load(&backend, &loaded));
    TEST_ASSERT_EQUAL_UINT8(1U, loaded.active_slot);
    TEST_ASSERT_EQUAL_UINT32(4U, loaded.active_version);
    TEST_ASSERT_EQUAL_UINT32(4U, loaded.min_version);
    TEST_ASSERT_EQUAL_UINT8(XY_FOTA_METADATA_NO_SLOT, loaded.pending_slot);
    TEST_ASSERT_BITS_LOW(XY_FOTA_METADATA_FLAG_PENDING, loaded.flags);
}

static void test_boot_attempt_callback_persists_count_and_rollback(void)
{
    xy_fota_metadata_t metadata = initial_metadata();
    xy_fota_metadata_t loaded = {0};
    bool rollback_required = true;

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_flash_commit(&backend, &metadata, NULL));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_boot_handoff(1U, 4U, (void *)&backend));

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_boot_attempt(2U, &rollback_required,
                                                        (void *)&backend));
    TEST_ASSERT_FALSE(rollback_required);
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_metadata_flash_load(&backend, &loaded));
    TEST_ASSERT_EQUAL_UINT8(1U, loaded.boot_attempts);
    TEST_ASSERT_EQUAL_UINT8(1U, loaded.pending_slot);

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_boot_attempt(2U, &rollback_required,
                                                        (void *)&backend));
    TEST_ASSERT_TRUE(rollback_required);
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_metadata_flash_load(&backend, &loaded));
    TEST_ASSERT_EQUAL_UINT8(0U, loaded.boot_attempts);
    TEST_ASSERT_EQUAL_UINT8(XY_FOTA_METADATA_NO_SLOT, loaded.pending_slot);
    TEST_ASSERT_BITS_LOW(XY_FOTA_METADATA_FLAG_PENDING, loaded.flags);
    TEST_ASSERT_EQUAL_UINT8(0U, loaded.active_slot);
    TEST_ASSERT_EQUAL_UINT32(3U, loaded.active_version);
}

static void test_boot_attempt_callback_preserves_durable_count_on_commit_failure(void)
{
    xy_fota_metadata_t metadata = initial_metadata();
    xy_fota_metadata_t loaded = {0};
    bool rollback_required = true;

    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          xy_fota_metadata_boot_attempt(2U, &rollback_required, NULL));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_flash_commit(&backend, &metadata, NULL));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_boot_handoff(1U, 4U, (void *)&backend));

    g_write_result = XY_FOTA_FLASH_ERROR;
    g_fail_write_call = g_write_calls + 1U;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_FLASH_ERROR,
                          xy_fota_metadata_boot_attempt(2U, &rollback_required,
                                                        (void *)&backend));
    g_write_result = XY_FOTA_OK;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_metadata_flash_load(&backend, &loaded));
    TEST_ASSERT_EQUAL_UINT8(0U, loaded.boot_attempts);
    TEST_ASSERT_EQUAL_UINT8(1U, loaded.pending_slot);
    TEST_ASSERT_TRUE(rollback_required);
}

static void test_boot_callbacks_fail_closed_on_mismatch_or_flash_failure(void)
{
    xy_fota_metadata_t metadata = initial_metadata();
    xy_fota_metadata_t loaded = {0};

    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          xy_fota_metadata_boot_handoff(1U, 4U, NULL));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_flash_commit(&backend, &metadata, NULL));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_boot_handoff(1U, 4U, (void *)&backend));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_VERSION_ERROR,
                          xy_fota_metadata_boot_confirm(1U, 5U, (void *)&backend));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          xy_fota_metadata_boot_confirm(0U, 4U, (void *)&backend));

    g_write_result = XY_FOTA_FLASH_ERROR;
    g_fail_write_call = g_write_calls + 2U;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_FLASH_ERROR,
                          xy_fota_metadata_boot_confirm(1U, 4U, (void *)&backend));
    g_write_result = XY_FOTA_OK;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_metadata_flash_load(&backend, &loaded));
    TEST_ASSERT_EQUAL_UINT8(0U, loaded.active_slot);
    TEST_ASSERT_EQUAL_UINT8(1U, loaded.pending_slot);
    TEST_ASSERT_EQUAL_UINT32(4U, loaded.pending_version);
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

    invalid_backend = backend;
    invalid_backend.base_addr = UINT32_MAX - ERASE_SIZE + 1U;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          xy_fota_metadata_flash_load(&invalid_backend, &metadata));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          xy_fota_metadata_flash_commit(&invalid_backend, &metadata, NULL));
    TEST_ASSERT_EQUAL_UINT32(0U, g_read_calls);
    TEST_ASSERT_EQUAL_UINT32(0U, g_erase_calls);
    TEST_ASSERT_EQUAL_UINT32(0U, g_write_calls);

    TEST_ASSERT_EQUAL_INT(XY_FOTA_NO_IMAGE,
                          xy_fota_metadata_flash_load(&backend, &metadata));
}

static void test_metadata_commit_rejects_inconsistent_boot_state(void)
{
    xy_fota_metadata_t metadata = initial_metadata();

    metadata.active_slot = 2U;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          xy_fota_metadata_flash_commit(&backend, &metadata, NULL));
    TEST_ASSERT_EQUAL_UINT32(0U, g_erase_calls);
    TEST_ASSERT_EQUAL_UINT32(0U, g_write_calls);

    metadata = initial_metadata();
    metadata.pending_slot = 1U;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          xy_fota_metadata_flash_commit(&backend, &metadata, NULL));

    metadata = initial_metadata();
    metadata.min_version = metadata.active_version + 1U;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          xy_fota_metadata_flash_commit(&backend, &metadata, NULL));

    metadata = initial_metadata();
    metadata.flags = XY_FOTA_METADATA_FLAG_PENDING;
    metadata.pending_slot = 0U;
    metadata.pending_version = 4U;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          xy_fota_metadata_flash_commit(&backend, &metadata, NULL));

    metadata.pending_slot = 1U;
    metadata.pending_version = 2U;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          xy_fota_metadata_flash_commit(&backend, &metadata, NULL));

    metadata = initial_metadata();
    metadata.flags = (uint8_t)(XY_FOTA_METADATA_FLAG_PENDING | (1U << 7));
    metadata.pending_slot = 1U;
    metadata.pending_version = 4U;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          xy_fota_metadata_flash_commit(&backend, &metadata, NULL));

    TEST_ASSERT_EQUAL_UINT32(0U, g_erase_calls);
    TEST_ASSERT_EQUAL_UINT32(0U, g_write_calls);
}

static void test_metadata_commit_roundtrip_and_generation(void)
{
    xy_fota_metadata_t metadata = initial_metadata();
    xy_fota_metadata_t loaded = {0};
    xy_fota_metadata_t committed = {0};

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_flash_commit(&backend, &metadata, &committed));
    TEST_ASSERT_EQUAL_UINT32(1U, committed.generation);
    TEST_ASSERT_EQUAL_UINT32(2U, g_write_calls);
    TEST_ASSERT_EQUAL_UINT32(1U, g_erase_calls);
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_metadata_flash_load(&backend, &loaded));
    TEST_ASSERT_EQUAL_UINT32(1U, loaded.generation);
    TEST_ASSERT_EQUAL_UINT32(3U, loaded.active_version);
    TEST_ASSERT_EQUAL_UINT8(0U, loaded.active_slot);

    metadata.active_version = 4U;
    metadata.min_version = 4U;
    metadata.active_slot = 1U;
    metadata.pending_slot = 0U;
    metadata.pending_version = 5U;
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

static void test_ambiguous_generation_order_fails_closed(void)
{
    xy_fota_metadata_t metadata = initial_metadata();
    xy_fota_metadata_t loaded = {
        .generation = 0xA5A5A5A5U,
        .active_version = 0x5A5A5A5AU,
    };
    metadata_record_fixture_t *first;
    metadata_record_fixture_t *second;
    uint8_t snapshot[STORAGE_SIZE];
    uint32_t erase_calls;
    uint32_t write_calls;

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_flash_commit(&backend, &metadata, NULL));
    metadata.active_version = 4U;
    metadata.min_version = 4U;
    metadata.active_slot = 1U;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_flash_commit(&backend, &metadata, NULL));

    first = (metadata_record_fixture_t *)&g_storage[0];
    second = (metadata_record_fixture_t *)&g_storage[ERASE_SIZE];
    first->generation = 1U;
    first->crc32 = xy_fota_calc_crc32((const uint8_t *)first,
                                      (uint32_t)offsetof(metadata_record_fixture_t, crc32));
    second->generation = 0x80000001U;
    second->crc32 = xy_fota_calc_crc32((const uint8_t *)second,
                                       (uint32_t)offsetof(metadata_record_fixture_t, crc32));

    TEST_ASSERT_EQUAL_INT(XY_FOTA_FLASH_ERROR,
                          xy_fota_metadata_flash_load(&backend, &loaded));
    TEST_ASSERT_EQUAL_UINT32(0xA5A5A5A5U, loaded.generation);
    TEST_ASSERT_EQUAL_UINT32(0x5A5A5A5AU, loaded.active_version);

    memcpy(snapshot, g_storage, sizeof(snapshot));
    erase_calls = g_erase_calls;
    write_calls = g_write_calls;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_FLASH_ERROR,
                          xy_fota_metadata_flash_commit(&backend, &metadata, NULL));
    TEST_ASSERT_EQUAL_UINT32(erase_calls, g_erase_calls);
    TEST_ASSERT_EQUAL_UINT32(write_calls, g_write_calls);
    TEST_ASSERT_EQUAL_MEMORY(snapshot, g_storage, sizeof(snapshot));
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

static void test_semantically_invalid_newest_copy_falls_back_to_previous_generation(void)
{
    xy_fota_metadata_t metadata = initial_metadata();
    xy_fota_metadata_t loaded = {0};
    metadata_record_fixture_t *newest;

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_flash_commit(&backend, &metadata, NULL));
    metadata.active_version = 4U;
    metadata.min_version = 4U;
    metadata.active_slot = 1U;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_flash_commit(&backend, &metadata, NULL));

    newest = (metadata_record_fixture_t *)&g_storage[ERASE_SIZE];
    newest->min_version = newest->active_version + 1U;
    newest->crc32 = xy_fota_calc_crc32((const uint8_t *)newest,
                                       (uint32_t)offsetof(metadata_record_fixture_t, crc32));

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_metadata_flash_load(&backend, &loaded));
    TEST_ASSERT_EQUAL_UINT32(1U, loaded.generation);
    TEST_ASSERT_EQUAL_UINT32(3U, loaded.active_version);
    TEST_ASSERT_EQUAL_UINT8(0U, loaded.active_slot);
}

static void test_load_reports_flash_error_when_no_copy_can_be_read(void)
{
    xy_fota_metadata_t loaded = {
        .active_version = 0xA5A5A5A5U,
        .active_slot = 0x5AU,
    };

    g_fail_read_call = 1U;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_FLASH_ERROR,
                          xy_fota_metadata_flash_load(&backend, &loaded));
    TEST_ASSERT_EQUAL_UINT32(0xA5A5A5A5U, loaded.active_version);
    TEST_ASSERT_EQUAL_UINT8(0x5AU, loaded.active_slot);
}

static void test_commit_refuses_to_overwrite_when_journal_scan_has_read_error(void)
{
    xy_fota_metadata_t metadata = initial_metadata();
    xy_fota_metadata_t loaded = {0};
    uint8_t snapshot[STORAGE_SIZE];

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_flash_commit(&backend, &metadata, NULL));
    memcpy(snapshot, g_storage, sizeof(snapshot));

    metadata.active_version = 4U;
    metadata.min_version = 4U;
    metadata.active_slot = 1U;
    g_fail_read_call = g_read_calls + 2U;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_FLASH_ERROR,
                          xy_fota_metadata_flash_commit(&backend, &metadata, NULL));
    TEST_ASSERT_EQUAL_UINT32(1U, g_erase_calls);
    TEST_ASSERT_EQUAL_MEMORY(snapshot, g_storage, sizeof(snapshot));

    g_fail_read_call = 0U;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_metadata_flash_load(&backend, &loaded));
    TEST_ASSERT_EQUAL_UINT32(3U, loaded.active_version);
    TEST_ASSERT_EQUAL_UINT8(0U, loaded.active_slot);
}

static void test_commit_cleans_target_when_readback_fails(void)
{
    xy_fota_metadata_t metadata = initial_metadata();
    xy_fota_metadata_t loaded = {0};

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_flash_commit(&backend, &metadata, NULL));

    metadata.active_version = 4U;
    metadata.min_version = 4U;
    metadata.active_slot = 1U;
    g_fail_read_call = g_read_calls + 3U;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_FLASH_ERROR,
                          xy_fota_metadata_flash_commit(&backend, &metadata, NULL));
    TEST_ASSERT_EQUAL_UINT32(3U, g_erase_calls);

    g_fail_read_call = 0U;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_metadata_flash_load(&backend, &loaded));
    TEST_ASSERT_EQUAL_UINT32(1U, loaded.generation);
    TEST_ASSERT_EQUAL_UINT32(3U, loaded.active_version);
    TEST_ASSERT_EQUAL_UINT8(0U, loaded.active_slot);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_metadata_guards_and_empty_flash_fail_closed);
    RUN_TEST(test_metadata_commit_rejects_inconsistent_boot_state);
    RUN_TEST(test_metadata_commit_roundtrip_and_generation);
    RUN_TEST(test_partial_write_preserves_previous_committed_record);
    RUN_TEST(test_ambiguous_generation_order_fails_closed);
    RUN_TEST(test_corrupt_newest_copy_falls_back_to_previous_generation);
    RUN_TEST(test_semantically_invalid_newest_copy_falls_back_to_previous_generation);
    RUN_TEST(test_load_reports_flash_error_when_no_copy_can_be_read);
    RUN_TEST(test_commit_refuses_to_overwrite_when_journal_scan_has_read_error);
    RUN_TEST(test_commit_cleans_target_when_readback_fails);
    RUN_TEST(test_boot_attempt_policy_rolls_back_after_bounded_failures);
    RUN_TEST(test_boot_attempt_policy_does_not_wrap_counter);
    RUN_TEST(test_staging_does_not_reset_an_existing_candidate_attempt_count);
    RUN_TEST(test_boot_attempt_confirmation_advances_floor_and_clears_pending);
    RUN_TEST(test_boot_attempt_policy_rejects_invalid_transitions);
    RUN_TEST(test_boot_callbacks_persist_handoff_and_confirmation);
    RUN_TEST(test_boot_attempt_callback_persists_count_and_rollback);
    RUN_TEST(test_boot_attempt_callback_preserves_durable_count_on_commit_failure);
    RUN_TEST(test_boot_callbacks_fail_closed_on_mismatch_or_flash_failure);
    return UNITY_END();
}
