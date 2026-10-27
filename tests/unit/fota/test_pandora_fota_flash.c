/*
 * Host contract for the Pandora board-owned FOTA internal-Flash adapters.
 *
 * Covers the two board owners that stand between the resident bootloader /
 * metadata journal and the canonical `xy_hal_flash_*` HAL:
 *
 *   - pandora_fota_flash.c          metadata journal backend + boot journal config
 *   - pandora_fota_install_flash.c  application install ops + vector gate
 *
 * The Flash HAL is faked with a RAM-backed 4 KiB metadata window so the real
 * two-slot journal core (`xy_fota_metadata.c`) can drive the board backend
 * end-to-end on the host.
 *
 * Evidence boundary: this is a Host contract only. It does not flash, erase or
 * read a physical part, and it says nothing about erase endurance, power-loss
 * behaviour or real bootloader vector handoff. The single target-only step --
 * fetching the two vector words from the memory-mapped application base -- is
 * deliberately kept out of the host build; only its decision rules
 * (`pandora_fota_application_vectors_check`) are covered here.
 */

#include "unity.h"

#include "pandora_fota_flash.h"
#include "pandora_fota_install_flash.h"
#include "xy_fota.h"
#include "xy_fota_metadata.h"
#include "xy_hal_flash.h"

#include <stdint.h>
#include <string.h>

#define METADATA_WINDOW_SIZE (2U * PANDORA_FOTA_METADATA_ERASE_SIZE)

static uint8_t fake_window[METADATA_WINDOW_SIZE];

static xy_hal_error_t fake_init_result;
static xy_hal_error_t fake_unlock_result;
static xy_hal_error_t fake_lock_result;
static xy_hal_error_t fake_read_result;
static xy_hal_error_t fake_write_result;
static xy_hal_error_t fake_erase_result;

static unsigned int init_calls;
static unsigned int unlock_calls;
static unsigned int lock_calls;
static unsigned int read_calls;
static unsigned int write_calls;
static unsigned int erase_calls;

static unsigned int init_seq;
static unsigned int unlock_seq;
static unsigned int write_seq;
static unsigned int erase_seq;
static unsigned int lock_seq;
static unsigned int read_seq;
static unsigned int op_sequence;

static uint32_t last_read_address;
static uint32_t last_read_size;
static uint32_t last_write_address;
static uint32_t last_write_size;
static uint32_t last_erase_address;
static uint32_t last_erase_size;

static int window_contains(uint32_t address, uint32_t size) {
    uint32_t end = PANDORA_FOTA_METADATA_BASE + METADATA_WINDOW_SIZE;

    return address >= PANDORA_FOTA_METADATA_BASE && address <= end && size <= end - address;
}

/* ------------------------------------------------------------------ fakes */

xy_hal_error_t xy_hal_flash_init(void* flash) {
    (void)flash;
    ++init_calls;
    init_seq = ++op_sequence;
    return fake_init_result;
}

xy_hal_error_t xy_hal_flash_unlock(void* flash) {
    (void)flash;
    ++unlock_calls;
    unlock_seq = ++op_sequence;
    return fake_unlock_result;
}

xy_hal_error_t xy_hal_flash_lock(void* flash) {
    (void)flash;
    ++lock_calls;
    lock_seq = ++op_sequence;
    return fake_lock_result;
}

xy_hal_error_t xy_hal_flash_read(void* flash, uint32_t address, uint8_t* data, uint32_t size) {
    (void)flash;
    ++read_calls;
    read_seq = ++op_sequence;
    last_read_address = address;
    last_read_size = size;
    if (fake_read_result != XY_HAL_OK) {
        return fake_read_result;
    }
    if (data != NULL && window_contains(address, size)) {
        memcpy(data, &fake_window[address - PANDORA_FOTA_METADATA_BASE], size);
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_flash_write(void* flash, uint32_t address, const uint8_t* data,
                                  uint32_t size) {
    (void)flash;
    ++write_calls;
    write_seq = ++op_sequence;
    last_write_address = address;
    last_write_size = size;
    if (fake_write_result != XY_HAL_OK) {
        return fake_write_result;
    }
    if (data != NULL && window_contains(address, size)) {
        memcpy(&fake_window[address - PANDORA_FOTA_METADATA_BASE], data, size);
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_flash_erase(void* flash, uint32_t address, uint32_t size) {
    (void)flash;
    ++erase_calls;
    erase_seq = ++op_sequence;
    last_erase_address = address;
    last_erase_size = size;
    if (fake_erase_result != XY_HAL_OK) {
        return fake_erase_result;
    }
    if (window_contains(address, size)) {
        memset(&fake_window[address - PANDORA_FOTA_METADATA_BASE], 0xFF, size);
    }
    return XY_HAL_OK;
}

/* ------------------------------------------------------------- fixtures */

static void reset_hal_calls(void) {
    init_calls = 0U;
    unlock_calls = 0U;
    lock_calls = 0U;
    read_calls = 0U;
    write_calls = 0U;
    erase_calls = 0U;
    init_seq = 0U;
    unlock_seq = 0U;
    lock_seq = 0U;
    read_seq = 0U;
    write_seq = 0U;
    erase_seq = 0U;
    op_sequence = 0U;
    last_read_address = 0U;
    last_read_size = 0U;
    last_write_address = 0U;
    last_write_size = 0U;
    last_erase_address = 0U;
    last_erase_size = 0U;
}

static void assert_no_hal_calls(void) {
    TEST_ASSERT_EQUAL_UINT(0U, init_calls);
    TEST_ASSERT_EQUAL_UINT(0U, unlock_calls);
    TEST_ASSERT_EQUAL_UINT(0U, lock_calls);
    TEST_ASSERT_EQUAL_UINT(0U, read_calls);
    TEST_ASSERT_EQUAL_UINT(0U, write_calls);
    TEST_ASSERT_EQUAL_UINT(0U, erase_calls);
}

void setUp(void) {
    memset(fake_window, 0xFF, sizeof(fake_window));
    fake_init_result = XY_HAL_OK;
    fake_unlock_result = XY_HAL_OK;
    fake_lock_result = XY_HAL_OK;
    fake_read_result = XY_HAL_OK;
    fake_write_result = XY_HAL_OK;
    fake_erase_result = XY_HAL_OK;
    reset_hal_calls();
}

void tearDown(void) {}

/* ------------------------------------------------- descriptor contracts */

static void test_metadata_backend_descriptor(void) {
    const xy_fota_metadata_flash_t* backend = pandora_fota_metadata_backend();

    TEST_ASSERT_NOT_NULL(backend);
    TEST_ASSERT_NOT_NULL(backend->ops);
    TEST_ASSERT_EQUAL_HEX32(0x0807E000U, backend->base_addr);
    TEST_ASSERT_EQUAL_HEX32(PANDORA_FOTA_METADATA_ERASE_SIZE, backend->erase_size);
    TEST_ASSERT_EQUAL_HEX32(0x0807E000U, backend->base_addr);
    TEST_ASSERT_NOT_NULL(backend->ops->read);
    TEST_ASSERT_NOT_NULL(backend->ops->write);
    TEST_ASSERT_NOT_NULL(backend->ops->erase);
    /* The journal owners neither own nor release the internal Flash controller. */
    TEST_ASSERT_NULL(backend->ops->init);
    TEST_ASSERT_NULL(backend->ops->deinit);
}

static void test_journal_config_matches_metadata_backend(void) {
    const xy_fota_metadata_flash_t* backend = pandora_fota_metadata_backend();
    xy_fota_boot_journal_config_t journal = pandora_fota_boot_journal_config();

    TEST_ASSERT_EQUAL_HEX32(backend->base_addr, journal.address);
    TEST_ASSERT_EQUAL_HEX32(backend->erase_size, journal.slot_size);
    TEST_ASSERT_NOT_NULL(journal.read);
    TEST_ASSERT_NOT_NULL(journal.write);
    TEST_ASSERT_NOT_NULL(journal.erase);
    TEST_ASSERT_TRUE(journal.read == backend->ops->read);
    TEST_ASSERT_TRUE(journal.write == backend->ops->write);
    TEST_ASSERT_TRUE(journal.erase == backend->ops->erase);
}

static void test_install_ops_descriptor(void) {
    const xy_fota_boot_install_ops_t* ops = pandora_fota_install_ops();

    TEST_ASSERT_NOT_NULL(ops);
    TEST_ASSERT_NOT_NULL(ops->read);
    TEST_ASSERT_NOT_NULL(ops->write);
    TEST_ASSERT_NOT_NULL(ops->erase);
    TEST_ASSERT_EQUAL_UINT32(PANDORA_FOTA_INSTALL_PROGRAM_SIZE, ops->program_granule);
    TEST_ASSERT_EQUAL_UINT32(PANDORA_FOTA_INSTALL_ERASE_SIZE, ops->erase_granule);
    TEST_ASSERT_EQUAL_UINT32(8U, ops->program_granule);
    TEST_ASSERT_EQUAL_UINT32(0x800U, ops->erase_granule);
}

/* --------------------------------------------- metadata journal flash ops */

static void test_metadata_read_rejects_invalid_requests(void) {
    const xy_fota_flash_ops_t* ops = pandora_fota_metadata_backend()->ops;
    uint8_t buffer[8];

    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          ops->read(PANDORA_FOTA_METADATA_BASE, NULL, sizeof(buffer)));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          ops->read(PANDORA_FOTA_METADATA_BASE - 8U, buffer, sizeof(buffer)));
    /* One byte past the two journal slots must not be readable. */
    TEST_ASSERT_EQUAL_INT(
        XY_FOTA_INVALID_PARAM,
        ops->read(PANDORA_FOTA_METADATA_BASE + METADATA_WINDOW_SIZE, buffer, sizeof(buffer)));
    /* Application range is not owned by the metadata backend. */
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          ops->read(PANDORA_FOTA_APP_BASE, buffer, sizeof(buffer)));
    /* Request straddling the end of the journal. */
    TEST_ASSERT_EQUAL_INT(
        XY_FOTA_INVALID_PARAM,
        ops->read(PANDORA_FOTA_METADATA_BASE + METADATA_WINDOW_SIZE - 4U, buffer, sizeof(buffer)));
    assert_no_hal_calls();
}

static void test_metadata_read_propagates_hal_failures(void) {
    const xy_fota_flash_ops_t* ops = pandora_fota_metadata_backend()->ops;
    uint8_t buffer[8];

    fake_init_result = XY_HAL_ERROR_FAIL;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_FLASH_ERROR,
                          ops->read(PANDORA_FOTA_METADATA_BASE, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_UINT(0U, read_calls);

    reset_hal_calls();
    fake_init_result = XY_HAL_OK;
    fake_read_result = XY_HAL_ERROR_IO;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_FLASH_ERROR,
                          ops->read(PANDORA_FOTA_METADATA_BASE, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_UINT(1U, read_calls);
    /* A read-only path must never take the program lock. */
    TEST_ASSERT_EQUAL_UINT(0U, unlock_calls);
    TEST_ASSERT_EQUAL_UINT(0U, lock_calls);
}

static void test_metadata_read_forwards_exact_request(void) {
    const xy_fota_flash_ops_t* ops = pandora_fota_metadata_backend()->ops;
    uint8_t buffer[8] = {0};

    fake_window[0] = 0xA5U;
    fake_window[7] = 0x5AU;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          ops->read(PANDORA_FOTA_METADATA_BASE, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_HEX32(PANDORA_FOTA_METADATA_BASE, last_read_address);
    TEST_ASSERT_EQUAL_UINT32(sizeof(buffer), last_read_size);
    TEST_ASSERT_EQUAL_HEX8(0xA5U, buffer[0]);
    TEST_ASSERT_EQUAL_HEX8(0x5AU, buffer[7]);
    TEST_ASSERT_EQUAL_UINT(1U, read_calls);
    TEST_ASSERT_EQUAL_UINT(0U, unlock_calls);
    TEST_ASSERT_EQUAL_UINT(0U, lock_calls);
}

static void test_metadata_write_rejects_invalid_requests(void) {
    const xy_fota_flash_ops_t* ops = pandora_fota_metadata_backend()->ops;
    const uint8_t payload[16] = {0};

    /* Not 8-byte aligned. */
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          ops->write(PANDORA_FOTA_METADATA_BASE + 4U, payload, 8U));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          ops->write(PANDORA_FOTA_METADATA_BASE, payload, 12U));
    /* NULL payload. */
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM, ops->write(PANDORA_FOTA_METADATA_BASE, NULL, 8U));
    /* Outside the journal window. */
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          ops->write(PANDORA_FOTA_APP_BASE, payload, sizeof(payload)));
    TEST_ASSERT_EQUAL_INT(
        XY_FOTA_INVALID_PARAM,
        ops->write(PANDORA_FOTA_METADATA_BASE + METADATA_WINDOW_SIZE, payload, 8U));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          ops->write(PANDORA_FOTA_METADATA_BASE + METADATA_WINDOW_SIZE - 8U,
                                     payload, sizeof(payload)));
    assert_no_hal_calls();
}

static void test_metadata_write_orders_init_unlock_write_lock(void) {
    const xy_fota_flash_ops_t* ops = pandora_fota_metadata_backend()->ops;
    const uint8_t payload[16] = {0x11, 0x22, 0x33, 0x44};

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          ops->write(PANDORA_FOTA_METADATA_BASE, payload, sizeof(payload)));
    TEST_ASSERT_EQUAL_UINT(1U, init_calls);
    TEST_ASSERT_EQUAL_UINT(1U, unlock_calls);
    TEST_ASSERT_EQUAL_UINT(1U, write_calls);
    TEST_ASSERT_EQUAL_UINT(1U, lock_calls);
    TEST_ASSERT_TRUE(init_seq < unlock_seq);
    TEST_ASSERT_TRUE(unlock_seq < write_seq);
    TEST_ASSERT_TRUE(write_seq < lock_seq);
    TEST_ASSERT_EQUAL_HEX32(PANDORA_FOTA_METADATA_BASE, last_write_address);
    TEST_ASSERT_EQUAL_UINT32(sizeof(payload), last_write_size);
    TEST_ASSERT_EQUAL_HEX8(0x33U, fake_window[2]);
    TEST_ASSERT_EQUAL_HEX8(0x44U, fake_window[3]);
}

static void test_metadata_write_stops_before_program_when_begin_fails(void) {
    const xy_fota_flash_ops_t* ops = pandora_fota_metadata_backend()->ops;
    const uint8_t payload[8] = {0};

    fake_init_result = XY_HAL_ERROR_FAIL;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_FLASH_ERROR,
                          ops->write(PANDORA_FOTA_METADATA_BASE, payload, sizeof(payload)));
    TEST_ASSERT_EQUAL_UINT(0U, unlock_calls);
    TEST_ASSERT_EQUAL_UINT(0U, write_calls);
    TEST_ASSERT_EQUAL_UINT(0U, lock_calls);

    reset_hal_calls();
    fake_unlock_result = XY_HAL_ERROR_FAIL;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_FLASH_ERROR,
                          ops->write(PANDORA_FOTA_METADATA_BASE, payload, sizeof(payload)));
    TEST_ASSERT_EQUAL_UINT(0U, write_calls);
    /* flash_begin() failed, so the lock step is never reached. */
    TEST_ASSERT_EQUAL_UINT(0U, lock_calls);
}

static void test_metadata_write_propagates_program_failure_and_still_locks(void) {
    const xy_fota_flash_ops_t* ops = pandora_fota_metadata_backend()->ops;
    const uint8_t payload[8] = {0};

    fake_write_result = XY_HAL_ERROR_FAIL;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_FLASH_ERROR,
                          ops->write(PANDORA_FOTA_METADATA_BASE, payload, sizeof(payload)));
    TEST_ASSERT_EQUAL_UINT(1U, write_calls);
    /* The controller must be re-locked even when programming failed. */
    TEST_ASSERT_EQUAL_UINT(1U, lock_calls);
    TEST_ASSERT_TRUE(write_seq < lock_seq);
}

static void test_metadata_write_reports_lock_failure_after_success(void) {
    const xy_fota_flash_ops_t* ops = pandora_fota_metadata_backend()->ops;
    const uint8_t payload[8] = {0x5AU};

    fake_lock_result = XY_HAL_ERROR_FAIL;
    /* A write that cannot be re-locked is reported as failed, not as success. */
    TEST_ASSERT_EQUAL_INT(XY_FOTA_FLASH_ERROR,
                          ops->write(PANDORA_FOTA_METADATA_BASE, payload, sizeof(payload)));
    TEST_ASSERT_EQUAL_UINT(1U, write_calls);
    TEST_ASSERT_EQUAL_UINT(1U, lock_calls);
    TEST_ASSERT_EQUAL_HEX8(0x5AU, fake_window[0]);
}

static void test_metadata_write_accepts_already_initialised_flash(void) {
    const xy_fota_flash_ops_t* ops = pandora_fota_metadata_backend()->ops;
    const uint8_t payload[8] = {0};

    fake_init_result = XY_HAL_ERROR_ALREADY_INIT;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          ops->write(PANDORA_FOTA_METADATA_BASE, payload, sizeof(payload)));
    TEST_ASSERT_EQUAL_UINT(1U, write_calls);
    TEST_ASSERT_EQUAL_UINT(1U, lock_calls);
}

static void test_metadata_erase_accepts_only_the_two_journal_slots(void) {
    const xy_fota_flash_ops_t* ops = pandora_fota_metadata_backend()->ops;

    /* Neither a sub-sector nor a whole-journal request may reach the HAL. */
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM, ops->erase(PANDORA_FOTA_METADATA_BASE, 0x400U));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM, ops->erase(PANDORA_FOTA_METADATA_BASE + 0x400U,
                                                            PANDORA_FOTA_METADATA_ERASE_SIZE));
    TEST_ASSERT_EQUAL_INT(
        XY_FOTA_INVALID_PARAM,
        ops->erase(PANDORA_FOTA_METADATA_BASE + 2U * PANDORA_FOTA_METADATA_ERASE_SIZE,
                   PANDORA_FOTA_METADATA_ERASE_SIZE));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM, ops->erase(PANDORA_FOTA_METADATA_BASE,
                                                            2U * PANDORA_FOTA_METADATA_ERASE_SIZE));
    assert_no_hal_calls();

    fake_window[0] = 0x00U;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          ops->erase(PANDORA_FOTA_METADATA_BASE, PANDORA_FOTA_METADATA_ERASE_SIZE));
    TEST_ASSERT_EQUAL_HEX32(PANDORA_FOTA_METADATA_BASE, last_erase_address);
    TEST_ASSERT_EQUAL_HEX8(0xFFU, fake_window[0]);
    TEST_ASSERT_EQUAL_UINT(1U, lock_calls);

    reset_hal_calls();
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          ops->erase(PANDORA_FOTA_METADATA_BASE + PANDORA_FOTA_METADATA_ERASE_SIZE,
                                     PANDORA_FOTA_METADATA_ERASE_SIZE));
    TEST_ASSERT_EQUAL_HEX32(PANDORA_FOTA_METADATA_BASE + PANDORA_FOTA_METADATA_ERASE_SIZE,
                            last_erase_address);
    TEST_ASSERT_EQUAL_UINT(1U, erase_calls);
    TEST_ASSERT_EQUAL_UINT(1U, lock_calls);
}

static void test_metadata_erase_propagates_hal_failure(void) {
    const xy_fota_flash_ops_t* ops = pandora_fota_metadata_backend()->ops;

    fake_erase_result = XY_HAL_ERROR_FAIL;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_FLASH_ERROR,
                          ops->erase(PANDORA_FOTA_METADATA_BASE, PANDORA_FOTA_METADATA_ERASE_SIZE));
    TEST_ASSERT_EQUAL_UINT(1U, erase_calls);
    TEST_ASSERT_EQUAL_UINT(1U, lock_calls);

    reset_hal_calls();
    fake_unlock_result = XY_HAL_ERROR_FAIL;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_FLASH_ERROR,
                          ops->erase(PANDORA_FOTA_METADATA_BASE, PANDORA_FOTA_METADATA_ERASE_SIZE));
    TEST_ASSERT_EQUAL_UINT(0U, erase_calls);
    TEST_ASSERT_EQUAL_UINT(0U, lock_calls);
}

/* --------------------------------------------------- install flash ops */

static void test_install_read_rejects_invalid_requests(void) {
    const xy_fota_boot_install_ops_t* ops = pandora_fota_install_ops();
    uint8_t buffer[8];

    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM, ops->read(PANDORA_FOTA_APP_BASE, NULL, 8U));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          ops->read(PANDORA_FOTA_APP_BASE - 8U, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          ops->read(PANDORA_FOTA_EXECUTION_LIMIT + 8U, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          ops->read(PANDORA_FOTA_EXECUTION_LIMIT - 4U, buffer, sizeof(buffer)));
    assert_no_hal_calls();

    /* The last aligned granule before the metadata region is still readable. */
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          ops->read(PANDORA_FOTA_EXECUTION_LIMIT - 8U, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_HEX32(PANDORA_FOTA_EXECUTION_LIMIT - 8U, last_read_address);
}

static void test_install_read_propagates_failures(void) {
    const xy_fota_boot_install_ops_t* ops = pandora_fota_install_ops();
    uint8_t buffer[8];

    fake_init_result = XY_HAL_ERROR_FAIL;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_FLASH_ERROR,
                          ops->read(PANDORA_FOTA_APP_BASE, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_UINT(0U, read_calls);

    reset_hal_calls();
    fake_init_result = XY_HAL_OK;
    fake_read_result = XY_HAL_ERROR_IO;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_FLASH_ERROR,
                          ops->read(PANDORA_FOTA_APP_BASE, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_UINT(1U, read_calls);
    TEST_ASSERT_EQUAL_UINT(0U, lock_calls);
}

static void test_install_write_rejects_unaligned_or_out_of_range(void) {
    const xy_fota_boot_install_ops_t* ops = pandora_fota_install_ops();
    const uint8_t payload[16] = {0};

    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          ops->write(PANDORA_FOTA_APP_BASE + 4U, payload, 8U));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM, ops->write(PANDORA_FOTA_APP_BASE, payload, 12U));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM, ops->write(PANDORA_FOTA_APP_BASE, NULL, 8U));
    /* Before the application slot, and past the metadata reservation. */
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          ops->write(PANDORA_FOTA_APP_BASE - 8U, payload, 8U));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          ops->write(PANDORA_FOTA_EXECUTION_LIMIT, payload, 8U));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          ops->write(PANDORA_FOTA_EXECUTION_LIMIT - 8U, payload, sizeof(payload)));
    assert_no_hal_calls();
}

static void test_install_write_orders_init_unlock_write_lock(void) {
    const xy_fota_boot_install_ops_t* ops = pandora_fota_install_ops();
    const uint8_t payload[16] = {0};

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, ops->write(PANDORA_FOTA_APP_BASE, payload, sizeof(payload)));
    TEST_ASSERT_EQUAL_UINT(1U, init_calls);
    TEST_ASSERT_EQUAL_UINT(1U, unlock_calls);
    TEST_ASSERT_EQUAL_UINT(1U, write_calls);
    TEST_ASSERT_EQUAL_UINT(1U, lock_calls);
    TEST_ASSERT_TRUE(init_seq < unlock_seq);
    TEST_ASSERT_TRUE(unlock_seq < write_seq);
    TEST_ASSERT_TRUE(write_seq < lock_seq);
    TEST_ASSERT_EQUAL_HEX32(PANDORA_FOTA_APP_BASE, last_write_address);
    TEST_ASSERT_EQUAL_UINT32(sizeof(payload), last_write_size);
}

static void test_install_write_stops_before_program_when_begin_fails(void) {
    const xy_fota_boot_install_ops_t* ops = pandora_fota_install_ops();
    const uint8_t payload[8] = {0};

    fake_unlock_result = XY_HAL_ERROR_FAIL;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_FLASH_ERROR,
                          ops->write(PANDORA_FOTA_APP_BASE, payload, sizeof(payload)));
    TEST_ASSERT_EQUAL_UINT(1U, init_calls);
    TEST_ASSERT_EQUAL_UINT(0U, write_calls);
    TEST_ASSERT_EQUAL_UINT(0U, lock_calls);
}

static void test_install_write_reports_lock_failure(void) {
    const xy_fota_boot_install_ops_t* ops = pandora_fota_install_ops();
    const uint8_t payload[8] = {0};

    fake_lock_result = XY_HAL_ERROR_FAIL;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_FLASH_ERROR,
                          ops->write(PANDORA_FOTA_APP_BASE, payload, sizeof(payload)));
    TEST_ASSERT_EQUAL_UINT(1U, write_calls);
    TEST_ASSERT_EQUAL_UINT(1U, lock_calls);
}

static void test_install_erase_rejects_partial_or_unaligned_requests(void) {
    const xy_fota_boot_install_ops_t* ops = pandora_fota_install_ops();

    /* Zero-size, sub-sector and misaligned erases must never reach the HAL. */
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM, ops->erase(PANDORA_FOTA_APP_BASE, 0U));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          ops->erase(PANDORA_FOTA_APP_BASE, PANDORA_FOTA_INSTALL_ERASE_SIZE / 2U));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          ops->erase(PANDORA_FOTA_APP_BASE + PANDORA_FOTA_INSTALL_ERASE_SIZE / 2U,
                                     PANDORA_FOTA_INSTALL_ERASE_SIZE));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          ops->erase(PANDORA_FOTA_APP_BASE - PANDORA_FOTA_INSTALL_ERASE_SIZE,
                                     PANDORA_FOTA_INSTALL_ERASE_SIZE));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM,
                          ops->erase(PANDORA_FOTA_EXECUTION_LIMIT - PANDORA_FOTA_INSTALL_ERASE_SIZE,
                                     2U * PANDORA_FOTA_INSTALL_ERASE_SIZE));
    assert_no_hal_calls();
}

static void test_install_erase_accepts_aligned_sectors(void) {
    const xy_fota_boot_install_ops_t* ops = pandora_fota_install_ops();

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          ops->erase(PANDORA_FOTA_APP_BASE, PANDORA_FOTA_INSTALL_ERASE_SIZE));
    TEST_ASSERT_EQUAL_HEX32(PANDORA_FOTA_APP_BASE, last_erase_address);
    TEST_ASSERT_EQUAL_UINT(1U, erase_calls);
    TEST_ASSERT_EQUAL_UINT(1U, lock_calls);

    reset_hal_calls();
    TEST_ASSERT_EQUAL_INT(
        XY_FOTA_OK, ops->erase(PANDORA_FOTA_EXECUTION_LIMIT - 2U * PANDORA_FOTA_INSTALL_ERASE_SIZE,
                               2U * PANDORA_FOTA_INSTALL_ERASE_SIZE));
    TEST_ASSERT_EQUAL_UINT32(2U * PANDORA_FOTA_INSTALL_ERASE_SIZE, last_erase_size);
    TEST_ASSERT_EQUAL_UINT(1U, erase_calls);
    TEST_ASSERT_EQUAL_UINT(1U, lock_calls);
}

static void test_install_erase_propagates_hal_failure(void) {
    const xy_fota_boot_install_ops_t* ops = pandora_fota_install_ops();

    fake_erase_result = XY_HAL_ERROR_FAIL;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_FLASH_ERROR,
                          ops->erase(PANDORA_FOTA_APP_BASE, PANDORA_FOTA_INSTALL_ERASE_SIZE));
    TEST_ASSERT_EQUAL_UINT(1U, erase_calls);
    TEST_ASSERT_EQUAL_UINT(1U, lock_calls);
}

/* ------------------------------------------------- application vector gate */

static void test_vectors_check_accepts_sram1_stack(void) {
    /* Pandora application: SP 0x20018000, Thumb reset 0x0800ad15. */
    TEST_ASSERT_EQUAL_INT(1, pandora_fota_application_vectors_check(0x20018000U, 0x0800AD15U));
    TEST_ASSERT_EQUAL_INT(1, pandora_fota_application_vectors_check(0x20000008U, 0x08008001U));
    TEST_ASSERT_EQUAL_INT(1, pandora_fota_application_vectors_check(0x20010000U, 0x0807DFF9U));
}

static void test_vectors_check_accepts_sram2_stack(void) {
    TEST_ASSERT_EQUAL_INT(1, pandora_fota_application_vectors_check(0x10000008U, 0x0800AD15U));
    TEST_ASSERT_EQUAL_INT(1, pandora_fota_application_vectors_check(0x10008000U, 0x0800AD15U));
}

static void test_vectors_check_rejects_bad_stack_pointer(void) {
    /* SRAM base itself is excluded; both SRAM windows are upper-bounded. */
    TEST_ASSERT_EQUAL_INT(0, pandora_fota_application_vectors_check(0x20000000U, 0x0800AD15U));
    TEST_ASSERT_EQUAL_INT(0, pandora_fota_application_vectors_check(0x20018008U, 0x0800AD15U));
    TEST_ASSERT_EQUAL_INT(0, pandora_fota_application_vectors_check(0x10000000U, 0x0800AD15U));
    TEST_ASSERT_EQUAL_INT(0, pandora_fota_application_vectors_check(0x10008008U, 0x0800AD15U));
    /* 8-byte alignment is mandatory for the initial stack pointer. */
    TEST_ASSERT_EQUAL_INT(0, pandora_fota_application_vectors_check(0x2000FFF4U, 0x0800AD15U));
    TEST_ASSERT_EQUAL_INT(0, pandora_fota_application_vectors_check(0x10000004U, 0x0800AD15U));
    /* Erased Flash (0xFFFFFFFF) is not a stack pointer. */
    TEST_ASSERT_EQUAL_INT(0, pandora_fota_application_vectors_check(0xFFFFFFFFU, 0xFFFFFFFFU));
    TEST_ASSERT_EQUAL_INT(0, pandora_fota_application_vectors_check(0U, 0x0800AD15U));
}

static void test_vectors_check_rejects_non_thumb_reset(void) {
    TEST_ASSERT_EQUAL_INT(0, pandora_fota_application_vectors_check(0x20018000U, 0x0800AD14U));
}

static void test_vectors_check_rejects_out_of_range_reset(void) {
    /* Below the application slot. */
    TEST_ASSERT_EQUAL_INT(0, pandora_fota_application_vectors_check(0x20018000U, 0x08007FF9U));
    TEST_ASSERT_EQUAL_INT(0, pandora_fota_application_vectors_check(0x20018000U, 0x08007FFFU));
    /* At or above the metadata reservation / execution limit. */
    TEST_ASSERT_EQUAL_INT(0, pandora_fota_application_vectors_check(0x20018000U, 0x0807E001U));
    TEST_ASSERT_EQUAL_INT(0, pandora_fota_application_vectors_check(0x20018000U, 0x0807FFFFU));
    /* Reset handler inside the metadata window. */
    TEST_ASSERT_EQUAL_INT(0, pandora_fota_application_vectors_check(0x20018000U, 0x0807F001U));
}

/* --------------------------- board backend driving the real journal core */

static xy_fota_metadata_t make_metadata(uint32_t active_version) {
    xy_fota_metadata_t metadata;

    memset(&metadata, 0, sizeof(metadata));
    metadata.active_version = active_version;
    metadata.min_version = 1U;
    metadata.active_slot = 0U;
    metadata.pending_slot = XY_FOTA_METADATA_NO_SLOT;
    return metadata;
}

static void test_metadata_journal_reports_no_image_on_blank_flash(void) {
    const xy_fota_metadata_flash_t* backend = pandora_fota_metadata_backend();
    xy_fota_metadata_t metadata;

    /* validate() only accepts the descriptor; it must not touch the part. */
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_metadata_flash_validate(backend));
    assert_no_hal_calls();
    /* A blank journal is an empty image, not a valid record. */
    TEST_ASSERT_EQUAL_INT(XY_FOTA_NO_IMAGE, xy_fota_metadata_flash_load(backend, &metadata));
    /* Both journal slots must be examined before declaring an empty journal. */
    TEST_ASSERT_EQUAL_UINT(2U, read_calls);
    TEST_ASSERT_EQUAL_HEX32(PANDORA_FOTA_METADATA_BASE + PANDORA_FOTA_METADATA_ERASE_SIZE,
                            last_read_address);
}

static void test_metadata_journal_commit_and_load_via_board_backend(void) {
    const xy_fota_metadata_flash_t* backend = pandora_fota_metadata_backend();
    xy_fota_metadata_t committed;
    xy_fota_metadata_t loaded;
    xy_fota_metadata_t metadata = make_metadata(7U);

    memset(&committed, 0, sizeof(committed));
    memset(&loaded, 0, sizeof(loaded));

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_flash_commit(backend, &metadata, &committed));
    TEST_ASSERT_EQUAL_UINT32(7U, committed.active_version);
    TEST_ASSERT_EQUAL_UINT32(1U, committed.generation);
    /* The first commit lands in slot 0, i.e. the board metadata base. */
    TEST_ASSERT_EQUAL_HEX32(PANDORA_FOTA_METADATA_BASE, last_erase_address);
    TEST_ASSERT_EQUAL_UINT32(PANDORA_FOTA_METADATA_ERASE_SIZE, last_erase_size);

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_metadata_flash_validate(backend));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_metadata_flash_load(backend, &loaded));
    TEST_ASSERT_EQUAL_UINT32(committed.active_version, loaded.active_version);
    TEST_ASSERT_EQUAL_UINT32(committed.generation, loaded.generation);
    TEST_ASSERT_EQUAL_UINT8(committed.active_slot, loaded.active_slot);
}

static void test_metadata_journal_alternates_slots(void) {
    const xy_fota_metadata_flash_t* backend = pandora_fota_metadata_backend();
    xy_fota_metadata_t committed;
    xy_fota_metadata_t loaded;
    xy_fota_metadata_t metadata = make_metadata(3U);

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_flash_commit(backend, &metadata, &committed));
    TEST_ASSERT_EQUAL_HEX32(PANDORA_FOTA_METADATA_BASE, last_erase_address);

    metadata = make_metadata(5U);
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_metadata_flash_commit(backend, &metadata, &committed));
    /* The second commit must rotate to slot 1, not overwrite the live slot. */
    TEST_ASSERT_EQUAL_HEX32(PANDORA_FOTA_METADATA_BASE + PANDORA_FOTA_METADATA_ERASE_SIZE,
                            last_erase_address);
    TEST_ASSERT_EQUAL_UINT32(2U, committed.generation);

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_metadata_flash_load(backend, &loaded));
    TEST_ASSERT_EQUAL_UINT32(5U, loaded.active_version);
    TEST_ASSERT_EQUAL_UINT32(2U, loaded.generation);
    /* The rotation must leave slot 0's previous record intact, not erase it. */
    uint8_t slot0[8] = {0};
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          backend->ops->read(PANDORA_FOTA_METADATA_BASE, slot0, sizeof(slot0)));
    TEST_ASSERT_TRUE(slot0[0] != 0xFFU || slot0[1] != 0xFFU || slot0[2] != 0xFFU ||
                     slot0[3] != 0xFFU);
}

/* ------------------------------------------------------------------- main */

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_metadata_backend_descriptor);
    RUN_TEST(test_journal_config_matches_metadata_backend);
    RUN_TEST(test_install_ops_descriptor);

    RUN_TEST(test_metadata_read_rejects_invalid_requests);
    RUN_TEST(test_metadata_read_propagates_hal_failures);
    RUN_TEST(test_metadata_read_forwards_exact_request);
    RUN_TEST(test_metadata_write_rejects_invalid_requests);
    RUN_TEST(test_metadata_write_orders_init_unlock_write_lock);
    RUN_TEST(test_metadata_write_stops_before_program_when_begin_fails);
    RUN_TEST(test_metadata_write_propagates_program_failure_and_still_locks);
    RUN_TEST(test_metadata_write_reports_lock_failure_after_success);
    RUN_TEST(test_metadata_write_accepts_already_initialised_flash);
    RUN_TEST(test_metadata_erase_accepts_only_the_two_journal_slots);
    RUN_TEST(test_metadata_erase_propagates_hal_failure);

    RUN_TEST(test_install_read_rejects_invalid_requests);
    RUN_TEST(test_install_read_propagates_failures);
    RUN_TEST(test_install_write_rejects_unaligned_or_out_of_range);
    RUN_TEST(test_install_write_orders_init_unlock_write_lock);
    RUN_TEST(test_install_write_stops_before_program_when_begin_fails);
    RUN_TEST(test_install_write_reports_lock_failure);
    RUN_TEST(test_install_erase_rejects_partial_or_unaligned_requests);
    RUN_TEST(test_install_erase_accepts_aligned_sectors);
    RUN_TEST(test_install_erase_propagates_hal_failure);

    RUN_TEST(test_vectors_check_accepts_sram1_stack);
    RUN_TEST(test_vectors_check_accepts_sram2_stack);
    RUN_TEST(test_vectors_check_rejects_bad_stack_pointer);
    RUN_TEST(test_vectors_check_rejects_non_thumb_reset);
    RUN_TEST(test_vectors_check_rejects_out_of_range_reset);

    RUN_TEST(test_metadata_journal_reports_no_image_on_blank_flash);
    RUN_TEST(test_metadata_journal_commit_and_load_via_board_backend);
    RUN_TEST(test_metadata_journal_alternates_slots);

    return UNITY_END();
}
