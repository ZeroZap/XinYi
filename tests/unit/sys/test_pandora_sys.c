#include "unity.h"

#include "xy_error.h"
#include "xy_hal_sys.h"
#include "xy_sys.h"

#include <stdint.h>

static uint32_t fake_reset_reason;
static xy_hal_error_t fake_reset_result;
static xy_hal_error_t fake_clear_result;
static xy_hal_error_t fake_uid_result;
static uint32_t fake_uid[3];
static unsigned int reset_calls;
static unsigned int clear_calls;
static unsigned int uid_calls;

void setUp(void) {
    fake_reset_reason = XY_HAL_SYS_RESET_REASON_NONE;
    fake_reset_result = XY_HAL_OK;
    fake_clear_result = XY_HAL_OK;
    fake_uid_result = XY_HAL_OK;
    fake_uid[0] = UINT32_C(0x001B002E);
    fake_uid[1] = UINT32_C(0x36475013);
    fake_uid[2] = UINT32_C(0x20313556);
    reset_calls = 0U;
    clear_calls = 0U;
    uid_calls = 0U;
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
    return fake_reset_result;
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

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_init_snapshots_reset_reason_and_clears_hal_flags);
    RUN_TEST(test_reset_validates_input_and_propagates_hal_failure);
    RUN_TEST(test_chip_id_requires_output_and_preserves_output_on_hal_failure);
    return UNITY_END();
}
