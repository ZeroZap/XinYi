#include "unity.h"

#include "pandora_sys.h"
#include "xy_error.h"
#include "xy_hal_rtc.h"
#include "xy_hal_sys.h"
#include "xy_sys.h"

#include <stdint.h>

/* Retained reset-request record slot owned by the Pandora board backend. */
#define TEST_RESET_REQUEST_INDEX 3U

static uint32_t fake_reset_reason;
static xy_hal_error_t fake_reset_result;
static xy_hal_error_t fake_clear_result;
static xy_hal_error_t fake_uid_result;
static uint32_t fake_uid[3];
static uint32_t fake_backup[XY_HAL_RTC_BACKUP_REGISTER_COUNT];
static xy_hal_error_t fake_backup_read_result;
static xy_hal_error_t fake_backup_write_result;
static unsigned int reset_calls;
static unsigned int clear_calls;
static unsigned int uid_calls;
static unsigned int backup_read_calls;
static unsigned int backup_write_calls;
static unsigned int op_sequence;
static unsigned int backup_write_seq;
static unsigned int reset_seq;

void setUp(void) {
    fake_reset_reason = XY_HAL_SYS_RESET_REASON_NONE;
    fake_reset_result = XY_HAL_OK;
    fake_clear_result = XY_HAL_OK;
    fake_uid_result = XY_HAL_OK;
    fake_uid[0] = UINT32_C(0x001B002E);
    fake_uid[1] = UINT32_C(0x36475013);
    fake_uid[2] = UINT32_C(0x20313556);
    for (unsigned int index = 0U; index < XY_HAL_RTC_BACKUP_REGISTER_COUNT; ++index) {
        fake_backup[index] = 0U;
    }
    fake_backup_read_result = XY_HAL_OK;
    fake_backup_write_result = XY_HAL_OK;
    reset_calls = 0U;
    clear_calls = 0U;
    uid_calls = 0U;
    backup_read_calls = 0U;
    backup_write_calls = 0U;
    op_sequence = 0U;
    backup_write_seq = 0U;
    reset_seq = 0U;
}

void tearDown(void) {}

uint32_t xy_hal_sys_get_reset_reason(void) {
    return fake_reset_reason;
}

xy_hal_error_t xy_hal_sys_clear_reset_reason(void) {
    ++clear_calls;
    return fake_clear_result;
}

xy_hal_error_t xy_hal_sys_software_reset(void) {
    ++reset_calls;
    reset_seq = ++op_sequence;
    return fake_reset_result;
}

xy_hal_error_t xy_hal_rtc_backup_read(uint32_t index, uint32_t *value) {
    ++backup_read_calls;
    ++op_sequence;
    if (fake_backup_read_result != XY_HAL_OK)
        return fake_backup_read_result;
    if (value == NULL || index >= XY_HAL_RTC_BACKUP_REGISTER_COUNT)
        return XY_HAL_ERROR_INVALID_PARAM;
    *value = fake_backup[index];
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_backup_write(uint32_t index, uint32_t value) {
    ++backup_write_calls;
    backup_write_seq = ++op_sequence;
    if (fake_backup_write_result != XY_HAL_OK)
        return fake_backup_write_result;
    if (index >= XY_HAL_RTC_BACKUP_REGISTER_COUNT)
        return XY_HAL_ERROR_INVALID_PARAM;
    fake_backup[index] = value;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_sys_get_unique_id(uint32_t id[3]) {
    ++uid_calls;
    if (fake_uid_result != XY_HAL_OK)
        return fake_uid_result;
    id[0] = fake_uid[0];
    id[1] = fake_uid[1];
    id[2] = fake_uid[2];
    return XY_HAL_OK;
}

void test_init_snapshots_reset_reason_and_clears_hal_flags(void) {
    uint32_t reason = 0U;

    fake_reset_reason = XY_HAL_SYS_RESET_REASON_SOFTWARE | XY_HAL_SYS_RESET_REASON_WATCHDOG;
    xy_sys_init();

    TEST_ASSERT_EQUAL_UINT(1U, clear_calls);
    TEST_ASSERT_EQUAL_UINT(0U, reset_calls);
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_sys_reboot_reason(&reason));
    TEST_ASSERT_EQUAL_UINT32(fake_reset_reason, reason);
}

void test_reset_validates_input_and_propagates_hal_failure(void) {
    fake_reset_result = XY_HAL_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_ERROR_INVALID_PARAM, xy_sys_reset(-1));
    TEST_ASSERT_EQUAL_UINT(0U, reset_calls);
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_sys_reset(1));
    TEST_ASSERT_EQUAL_UINT(1U, reset_calls);
}

void test_reset_records_request_reason_before_software_reset(void) {
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_sys_reset(XY_SYS_RESET_REQUEST_FOTA_HANDOFF));
    TEST_ASSERT_EQUAL_UINT(1U, backup_write_calls);
    TEST_ASSERT_EQUAL_UINT(1U, reset_calls);
    /* The reason must be durable before NVIC_SystemReset() diverges. */
    TEST_ASSERT_TRUE(backup_write_seq < reset_seq);
    TEST_ASSERT_EQUAL_UINT32(XY_SYS_RESET_REQUEST_FOTA_HANDOFF,
                             fake_backup[TEST_RESET_REQUEST_INDEX] & 0xFFU);
    TEST_ASSERT_NOT_EQUAL(0U, fake_backup[TEST_RESET_REQUEST_INDEX]);
}

void test_reset_rejects_reason_outside_retained_encoding(void) {
    TEST_ASSERT_EQUAL_INT(XY_ERROR_INVALID_PARAM, xy_sys_reset(256));
    TEST_ASSERT_EQUAL_UINT(0U, backup_write_calls);
    TEST_ASSERT_EQUAL_UINT(0U, reset_calls);
}

void test_reset_reports_record_failure_without_skipping_the_reset(void) {
    fake_backup_write_result = XY_HAL_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_sys_reset(XY_SYS_RESET_REQUEST_APP_RESTART));
    /* Availability wins: a caller must not be stranded on a failed record write. */
    TEST_ASSERT_EQUAL_UINT(1U, reset_calls);
}

void test_init_latches_retained_request_and_consumes_the_record(void) {
    uint32_t request = UINT32_C(0xFFFFFFFF);
    xy_hal_error_t record_result;

    /* First boot records the request the board is about to reset with. */
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_sys_reset(XY_SYS_RESET_REQUEST_APP_RESTART));
    record_result = fake_backup[TEST_RESET_REQUEST_INDEX] == 0U ? XY_HAL_ERROR : XY_HAL_OK;
    TEST_ASSERT_EQUAL(XY_HAL_OK, record_result);
    backup_write_calls = 0U;

    /* Next boot: the reason survives the reset and is reported once. */
    xy_sys_init();
    TEST_ASSERT_EQUAL_UINT(1U, backup_read_calls);
    TEST_ASSERT_EQUAL_UINT(1U, backup_write_calls);
    TEST_ASSERT_EQUAL_UINT32(0U, fake_backup[TEST_RESET_REQUEST_INDEX]);
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_sys_reset_request(&request));
    TEST_ASSERT_EQUAL_UINT32(XY_SYS_RESET_REQUEST_APP_RESTART, request);
}

void test_init_reports_unspecified_request_when_record_is_absent_or_foreign(void) {
    uint32_t request = UINT32_C(0xFFFFFFFF);

    xy_sys_init();
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_sys_reset_request(&request));
    TEST_ASSERT_EQUAL_UINT32(XY_SYS_RESET_REQUEST_UNSPECIFIED, request);

    /* A stale word from another writer must not be decoded as a SYS request. */
    fake_backup[TEST_RESET_REQUEST_INDEX] = UINT32_C(0xDEADBEEF);
    request = UINT32_C(0xFFFFFFFF);
    xy_sys_init();
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_sys_reset_request(&request));
    TEST_ASSERT_EQUAL_UINT32(XY_SYS_RESET_REQUEST_UNSPECIFIED, request);
    TEST_ASSERT_EQUAL_UINT32(0U, fake_backup[TEST_RESET_REQUEST_INDEX]);
}

void test_init_reports_unspecified_request_when_backup_read_fails(void) {
    uint32_t request = UINT32_C(0xFFFFFFFF);

    fake_backup[TEST_RESET_REQUEST_INDEX] = UINT32_C(0x58595201);
    fake_backup_read_result = XY_HAL_ERROR;
    xy_sys_init();

    TEST_ASSERT_EQUAL_INT(XY_OK, xy_sys_reset_request(&request));
    TEST_ASSERT_EQUAL_UINT32(XY_SYS_RESET_REQUEST_UNSPECIFIED, request);
}

void test_reset_request_requires_output(void) {
    TEST_ASSERT_EQUAL_INT(XY_ERROR_INVALID_PARAM, xy_sys_reset_request(NULL));
}

void test_chip_id_requires_output_and_preserves_output_on_hal_failure(void) {
    uint32_t chip_id[3] = {UINT32_C(0xAAAAAAAA), UINT32_C(0xBBBBBBBB), UINT32_C(0xCCCCCCCC)};

    TEST_ASSERT_EQUAL_INT(XY_ERROR_INVALID_PARAM, xy_sys_get_chip_id(NULL));
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_sys_get_chip_id(chip_id));
    TEST_ASSERT_EQUAL_UINT32(fake_uid[0], chip_id[0]);
    TEST_ASSERT_EQUAL_UINT32(fake_uid[1], chip_id[1]);
    TEST_ASSERT_EQUAL_UINT32(fake_uid[2], chip_id[2]);
    TEST_ASSERT_EQUAL_UINT(1U, uid_calls);

    fake_uid_result = XY_HAL_ERROR;
    chip_id[0] = UINT32_C(0xAAAAAAAA);
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_sys_get_chip_id(chip_id));
    TEST_ASSERT_EQUAL_UINT32(UINT32_C(0xAAAAAAAA), chip_id[0]);
}

void test_mac_addr_requires_output_and_preserves_on_hal_failure(void) {
    uint8_t mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

    TEST_ASSERT_EQUAL_INT(XY_ERROR_INVALID_PARAM, xy_sys_get_mac_addr(NULL));

    TEST_ASSERT_EQUAL_INT(XY_OK, xy_sys_get_mac_addr(mac));
    TEST_ASSERT_EQUAL_UINT8(0x2E, mac[0]);
    TEST_ASSERT_EQUAL_UINT8(0x00, mac[1]);
    TEST_ASSERT_EQUAL_UINT8(0x1B, mac[2]);
    TEST_ASSERT_EQUAL_UINT8(0x00, mac[3]);
    TEST_ASSERT_EQUAL_UINT8(0x13, mac[4]);
    TEST_ASSERT_EQUAL_UINT8(0x50, mac[5]);
    /* unicast: bit 0 clear; locally administered: bit 1 set */
    TEST_ASSERT_EQUAL_UINT8(0x00, mac[0] & 0x01U);
    TEST_ASSERT_EQUAL_UINT8(0x02, mac[0] & 0x02U);
    TEST_ASSERT_EQUAL_UINT(1U, uid_calls);

    fake_uid_result = XY_HAL_ERROR;
    mac[0] = 0xAA;
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_sys_get_mac_addr(mac));
    TEST_ASSERT_EQUAL_UINT8(0xAA, mac[0]);
}

void test_mac_addr_normalizes_unicast_and_locally_administered_bits(void) {
    uint8_t mac[6];

    fake_uid[0] = UINT32_C(0x000000FF); /* low byte 0xFF: bit 0 (multicast) set */
    fake_uid[1] = UINT32_C(0x00000000);
    fake_uid[2] = UINT32_C(0x00000000);

    TEST_ASSERT_EQUAL_INT(XY_OK, xy_sys_get_mac_addr(mac));
    TEST_ASSERT_EQUAL_UINT8(0xFC, mac[0] & 0xFCU); /* upper 6 bits preserved */
    TEST_ASSERT_EQUAL_UINT8(0x02, mac[0] & 0x03U); /* bit 0 clear, bit 1 set */
}

/* One reset has one cause from the boot decision's point of view, but the HAL
 * returns a bitmask. The board captured these masks on real boots: a software
 * reset reported 0x5 and a watchdog reset 0x9, i.e. an internal reset also
 * asserts the NRST flag. So the pin bit alone can never outrank the internal
 * cause that produced it, and a power event must never be reported as POWER_ON. */
void test_reset_kind_attribution_uses_the_documented_priority(void) {
    static const struct {
        uint32_t reason;
        pandora_reset_kind_t expected;
        const char *meaning;
    } cases[] = {
        {XY_HAL_SYS_RESET_REASON_NONE, PANDORA_RESET_KIND_POWER_ON, "no flag set at all"},
        {XY_HAL_SYS_RESET_REASON_EXTERNAL_PIN, PANDORA_RESET_KIND_EXTERNAL_PIN, "pin only"},
        {XY_HAL_SYS_RESET_REASON_EXTERNAL_PIN | XY_HAL_SYS_RESET_REASON_SOFTWARE,
         PANDORA_RESET_KIND_SOFTWARE, "software reset as captured (0x5)"},
        {XY_HAL_SYS_RESET_REASON_EXTERNAL_PIN | XY_HAL_SYS_RESET_REASON_WATCHDOG,
         PANDORA_RESET_KIND_WATCHDOG, "watchdog reset as captured (0x9)"},
        {XY_HAL_SYS_RESET_REASON_BROWNOUT, PANDORA_RESET_KIND_BROWNOUT, "brownout only"},
        {XY_HAL_SYS_RESET_REASON_EXTERNAL_PIN | XY_HAL_SYS_RESET_REASON_BROWNOUT,
         PANDORA_RESET_KIND_BROWNOUT, "brownout also asserts the pin flag"},
        {XY_HAL_SYS_RESET_REASON_LOW_POWER, PANDORA_RESET_KIND_LOW_POWER, "low-power only"},
        {XY_HAL_SYS_RESET_REASON_EXTERNAL_PIN | XY_HAL_SYS_RESET_REASON_LOW_POWER,
         PANDORA_RESET_KIND_LOW_POWER, "low-power also asserts the pin flag"},
        {XY_HAL_SYS_RESET_REASON_BROWNOUT | XY_HAL_SYS_RESET_REASON_LOW_POWER,
         PANDORA_RESET_KIND_BROWNOUT, "brownout outranks low-power"},
        {XY_HAL_SYS_RESET_REASON_SOFTWARE | XY_HAL_SYS_RESET_REASON_BROWNOUT,
         PANDORA_RESET_KIND_SOFTWARE, "requested reset outranks the power event"},
        {XY_HAL_SYS_RESET_REASON_WATCHDOG | XY_HAL_SYS_RESET_REASON_SOFTWARE |
             XY_HAL_SYS_RESET_REASON_BROWNOUT,
         PANDORA_RESET_KIND_WATCHDOG, "watchdog outranks software"},
        {XY_HAL_SYS_RESET_REASON_WATCHDOG | XY_HAL_SYS_RESET_REASON_LOW_POWER,
         PANDORA_RESET_KIND_WATCHDOG, "watchdog outranks low-power"},
        {UINT32_C(1) << 5U, PANDORA_RESET_KIND_UNKNOWN, "unknown bit is not a power-on"},
        {UINT32_C(0xFFFFFFFF), PANDORA_RESET_KIND_UNKNOWN, "unknown bits are not a power-on"},
    };

    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        TEST_ASSERT_EQUAL_INT_MESSAGE(cases[index].expected,
                                      pandora_reset_kind_from_reason(cases[index].reason),
                                      cases[index].meaning);
    }
}

void test_reset_kind_names_are_stable_and_distinct(void) {
    TEST_ASSERT_EQUAL_STRING("UNKNOWN", pandora_reset_kind_name(PANDORA_RESET_KIND_UNKNOWN));
    TEST_ASSERT_EQUAL_STRING("POWER_ON", pandora_reset_kind_name(PANDORA_RESET_KIND_POWER_ON));
    TEST_ASSERT_EQUAL_STRING("EXTERNAL_PIN",
                             pandora_reset_kind_name(PANDORA_RESET_KIND_EXTERNAL_PIN));
    TEST_ASSERT_EQUAL_STRING("BROWNOUT", pandora_reset_kind_name(PANDORA_RESET_KIND_BROWNOUT));
    TEST_ASSERT_EQUAL_STRING("SOFTWARE", pandora_reset_kind_name(PANDORA_RESET_KIND_SOFTWARE));
    TEST_ASSERT_EQUAL_STRING("WATCHDOG", pandora_reset_kind_name(PANDORA_RESET_KIND_WATCHDOG));
    TEST_ASSERT_EQUAL_STRING("LOW_POWER", pandora_reset_kind_name(PANDORA_RESET_KIND_LOW_POWER));
    /* An out-of-range value must not index past the name table. */
    TEST_ASSERT_EQUAL_STRING("UNKNOWN", pandora_reset_kind_name((pandora_reset_kind_t)99));
}

void test_init_latches_the_kind_decoded_from_the_snapshot(void) {
    fake_reset_reason = XY_HAL_SYS_RESET_REASON_EXTERNAL_PIN | XY_HAL_SYS_RESET_REASON_WATCHDOG;
    xy_sys_init();
    TEST_ASSERT_EQUAL_INT(PANDORA_RESET_KIND_WATCHDOG, pandora_sys_reset_kind());

    fake_reset_reason = XY_HAL_SYS_RESET_REASON_NONE;
    xy_sys_init();
    TEST_ASSERT_EQUAL_INT(PANDORA_RESET_KIND_POWER_ON, pandora_sys_reset_kind());
}

void test_init_reports_unknown_kind_when_the_hal_returns_an_unrecognized_bit(void) {
    fake_reset_reason = UINT32_C(1) << 5U;
    xy_sys_init();
    TEST_ASSERT_EQUAL_INT(PANDORA_RESET_KIND_UNKNOWN, pandora_sys_reset_kind());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_init_snapshots_reset_reason_and_clears_hal_flags);
    RUN_TEST(test_reset_validates_input_and_propagates_hal_failure);
    RUN_TEST(test_reset_records_request_reason_before_software_reset);
    RUN_TEST(test_reset_rejects_reason_outside_retained_encoding);
    RUN_TEST(test_reset_reports_record_failure_without_skipping_the_reset);
    RUN_TEST(test_init_latches_retained_request_and_consumes_the_record);
    RUN_TEST(test_init_reports_unspecified_request_when_record_is_absent_or_foreign);
    RUN_TEST(test_init_reports_unspecified_request_when_backup_read_fails);
    RUN_TEST(test_reset_request_requires_output);
    RUN_TEST(test_chip_id_requires_output_and_preserves_output_on_hal_failure);
    RUN_TEST(test_mac_addr_requires_output_and_preserves_on_hal_failure);
    RUN_TEST(test_mac_addr_normalizes_unicast_and_locally_administered_bits);
    RUN_TEST(test_reset_kind_attribution_uses_the_documented_priority);
    RUN_TEST(test_reset_kind_names_are_stable_and_distinct);
    RUN_TEST(test_init_latches_the_kind_decoded_from_the_snapshot);
    RUN_TEST(test_init_reports_unknown_kind_when_the_hal_returns_an_unrecognized_bit);
    return UNITY_END();
}
