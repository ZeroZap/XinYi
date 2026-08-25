#include "unity.h"

#include "xy_error.h"
#include "xy_sys.h"

#include <stdint.h>

void setUp(void)
{
}

void tearDown(void)
{
}

void test_default_reset_fails_closed(void)
{
    TEST_ASSERT_EQUAL_INT(XY_ERROR_NOT_SUPPORTED, xy_sys_reset(1));
}

void test_default_system_queries_fail_closed_and_preserve_outputs(void)
{
    uint32_t reboot_reason = UINT32_C(0x11223344);
    uint8_t chip_id[12] = {0xA5};
    uint8_t mac_addr[6] = {0x5A};
    char sw_version[16] = "sw-sentinel";
    char hw_version[16] = "hw-sentinel";

    TEST_ASSERT_EQUAL_INT(XY_ERROR_NOT_SUPPORTED, xy_sys_reboot_reason(&reboot_reason));
    TEST_ASSERT_EQUAL_HEX32(UINT32_C(0x11223344), reboot_reason);

    TEST_ASSERT_EQUAL_INT(XY_ERROR_NOT_SUPPORTED, xy_sys_get_chip_id(chip_id));
    TEST_ASSERT_EQUAL_HEX8(0xA5, chip_id[0]);

    TEST_ASSERT_EQUAL_INT(XY_ERROR_NOT_SUPPORTED, xy_sys_get_mac_addr(mac_addr));
    TEST_ASSERT_EQUAL_HEX8(0x5A, mac_addr[0]);

    TEST_ASSERT_EQUAL_INT(XY_ERROR_NOT_SUPPORTED, xy_sys_get_sw_ver(sw_version));
    TEST_ASSERT_EQUAL_STRING("sw-sentinel", sw_version);

    TEST_ASSERT_EQUAL_INT(XY_ERROR_NOT_SUPPORTED, xy_sys_get_hw_ver(hw_version));
    TEST_ASSERT_EQUAL_STRING("hw-sentinel", hw_version);
}

void test_default_system_queries_reject_null_without_claiming_success(void)
{
    TEST_ASSERT_EQUAL_INT(XY_ERROR_NOT_SUPPORTED, xy_sys_reboot_reason(NULL));
    TEST_ASSERT_EQUAL_INT(XY_ERROR_NOT_SUPPORTED, xy_sys_get_chip_id(NULL));
    TEST_ASSERT_EQUAL_INT(XY_ERROR_NOT_SUPPORTED, xy_sys_get_mac_addr(NULL));
    TEST_ASSERT_EQUAL_INT(XY_ERROR_NOT_SUPPORTED, xy_sys_get_sw_ver(NULL));
    TEST_ASSERT_EQUAL_INT(XY_ERROR_NOT_SUPPORTED, xy_sys_get_hw_ver(NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_default_reset_fails_closed);
    RUN_TEST(test_default_system_queries_fail_closed_and_preserve_outputs);
    RUN_TEST(test_default_system_queries_reject_null_without_claiming_success);
    return UNITY_END();
}
