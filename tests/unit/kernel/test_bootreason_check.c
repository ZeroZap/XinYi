#include "unity.h"

#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include "fff.h"

#define MTK_BOOTREASON_CHECK_ENABLE 1
#define EXCEPTION_MEMDUMP_MODE 0
#define EXCEPTION_MEMDUMP_MINIDUMP 1

#include "bootreason_check.h"

DEFINE_FFF_GLOBALS;

static uint8_t g_rtc_data[BOOTREASON_FLAG_BYTES];
static bootreason_status_t g_wdt_status;
static bootreason_status_t g_normal_power_status;

FAKE_VOID_FUNC(hal_rtc_get_data, uint32_t, char *, uint32_t);
FAKE_VOID_FUNC(hal_rtc_set_data, uint32_t, const char *, uint32_t);
FAKE_VALUE_FUNC(bootreason_status_t, bootreason_check_wdt_timeout_reset);
FAKE_VALUE_FUNC(bootreason_status_t, bootreason_check_normal_power_on);
FAKE_VOID_FUNC3_VARARG(log_hal_msgid_info, const char *, int, ...);

static void reset_bootreason_state(void);

static void hal_rtc_get_data_impl(uint32_t offset, char *buf, uint32_t len);
static void hal_rtc_set_data_impl(uint32_t offset, const char *buf, uint32_t len);
static bootreason_status_t bootreason_check_wdt_timeout_reset_impl(void);
static bootreason_status_t bootreason_check_normal_power_on_impl(void);
static void log_hal_msgid_info_impl(const char *fmt, int arg_count, va_list ap);

void setUp(void)
{
    memset(g_rtc_data, 0, sizeof(g_rtc_data));
    g_wdt_status = BOOTREASON_STATUS_ERROR;
    g_normal_power_status = BOOTREASON_STATUS_ERROR;

    RESET_FAKE(hal_rtc_get_data);
    RESET_FAKE(hal_rtc_set_data);
    RESET_FAKE(bootreason_check_wdt_timeout_reset);
    RESET_FAKE(bootreason_check_normal_power_on);
    RESET_FAKE(log_hal_msgid_info);
    FFF_RESET_HISTORY();

    hal_rtc_get_data_fake.custom_fake = hal_rtc_get_data_impl;
    hal_rtc_set_data_fake.custom_fake = hal_rtc_set_data_impl;
    bootreason_check_wdt_timeout_reset_fake.custom_fake = bootreason_check_wdt_timeout_reset_impl;
    bootreason_check_normal_power_on_fake.custom_fake = bootreason_check_normal_power_on_impl;
    log_hal_msgid_info_fake.custom_fake = log_hal_msgid_info_impl;

    reset_bootreason_state();
}

void tearDown(void)
{
}

static void hal_rtc_get_data_impl(uint32_t offset, char *buf, uint32_t len)
{
    TEST_ASSERT_NOT_NULL(buf);
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(sizeof(g_rtc_data), offset + len);
    memcpy(buf, &g_rtc_data[offset], len);
}

static void hal_rtc_set_data_impl(uint32_t offset, const char *buf, uint32_t len)
{
    TEST_ASSERT_NOT_NULL(buf);
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(sizeof(g_rtc_data), offset + len);
    memcpy(&g_rtc_data[offset], buf, len);
}

static bootreason_status_t bootreason_check_wdt_timeout_reset_impl(void)
{
    return g_wdt_status;
}

static bootreason_status_t bootreason_check_normal_power_on_impl(void)
{
    return g_normal_power_status;
}

static void log_hal_msgid_info_impl(const char *fmt, int arg_count, va_list ap)
{
    (void)ap;
    TEST_ASSERT_NOT_NULL(fmt);
    TEST_ASSERT_EQUAL_INT(2, arg_count);
}

#include "bootreason_check.c"

static void reset_bootreason_state(void)
{
    bootreason_lastreason = BOOTREASON_MAX;
}

static void init_with_flag(uint8_t flag)
{
    g_rtc_data[BOOTREASON_FLAG_OFFSET] = flag;
    bootreason_init();
}

static void assert_reason(bootreason_reason_t expected)
{
    bootreason_reason_t reason = BOOTREASON_MAX;
    TEST_ASSERT_EQUAL_INT(BOOTREASON_STATUS_OK, bootreason_get_reason(&reason));
    TEST_ASSERT_EQUAL_INT(expected, reason);
}

void test_flag_setters_update_rtc_flags_without_clobbering_existing_bits(void)
{
    bootreason_set_flag_exception_reset();
    TEST_ASSERT_BITS_HIGH(BOOTREASON_EXCEPTION_RESET_FLAG, g_rtc_data[0]);

    bootreason_set_flag_assert_reset();
    TEST_ASSERT_BITS_HIGH(BOOTREASON_EXCEPTION_RESET_FLAG | BOOTREASON_ASSERT_RESET_FLAG,
                                g_rtc_data[0]);

    bootreason_set_flag_wdt_sw_reset();
    bootreason_set_flag_soft_reset();
    bootreason_set_flag_xoff_reset();
    bootreason_set_flag_enter_sleep();

    TEST_ASSERT_BITS_HIGH(BOOTREASON_EXCEPTION_RESET_FLAG |
                                    BOOTREASON_ASSERT_RESET_FLAG |
                                    BOOTREASON_WDT_SW_RESET_FLAG |
                                    BOOTREASON_SOFT_RESET_FLAG |
                                    BOOTREASON_XOFF_RESET_FLAG |
                                    BOOTREASON_SLEEP_ENTER_FLAG,
                                g_rtc_data[0]);

    bootreason_clear_flag_exit_sleep();
    TEST_ASSERT_BITS_LOW(BOOTREASON_SLEEP_ENTER_FLAG, g_rtc_data[0]);
    TEST_ASSERT_BITS_HIGH(BOOTREASON_EXCEPTION_RESET_FLAG |
                                    BOOTREASON_ASSERT_RESET_FLAG |
                                    BOOTREASON_WDT_SW_RESET_FLAG |
                                    BOOTREASON_SOFT_RESET_FLAG |
                                    BOOTREASON_XOFF_RESET_FLAG,
                                g_rtc_data[0]);
}

void test_init_prioritizes_assert_exception_xoff_and_reset_flags(void)
{
    init_with_flag(BOOTREASON_ASSERT_RESET_FLAG | BOOTREASON_EXCEPTION_RESET_FLAG);
    assert_reason(BOOTREASON_ASSERT);
    TEST_ASSERT_EQUAL_UINT(2, hal_rtc_get_data_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(2, hal_rtc_set_data_fake.call_count);
    TEST_ASSERT_EQUAL_UINT32(BOOTREASON_FLAG_OFFSET, hal_rtc_get_data_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT32(BOOTREASON_FLAG_BYTES, hal_rtc_get_data_fake.arg2_val);
    TEST_ASSERT_EQUAL_HEX8(BOOTREASON_INIT_FLAG, g_rtc_data[0]);
    TEST_ASSERT_EQUAL_HEX8(0, g_rtc_data[1]);
    TEST_ASSERT_EQUAL_UINT(1, log_hal_msgid_info_fake.call_count);

    setUp();
    init_with_flag(BOOTREASON_EXCEPTION_RESET_FLAG | BOOTREASON_XOFF_RESET_FLAG);
    assert_reason(BOOTREASON_XOFF_RESET);

    setUp();
    init_with_flag(BOOTREASON_EXCEPTION_RESET_FLAG);
    assert_reason(BOOTREASON_EXCEPTION);

    setUp();
    init_with_flag(BOOTREASON_WDT_SW_RESET_FLAG);
    assert_reason(BOOTREASON_WATCHDOG_RESET);

    setUp();
    init_with_flag(BOOTREASON_SOFT_RESET_FLAG);
    assert_reason(BOOTREASON_SOFT_RESET);
}

void test_init_reports_watchdog_sleeperror_normal_and_unknown_paths(void)
{
    g_wdt_status = BOOTREASON_STATUS_OK;
    init_with_flag(0);
    assert_reason(BOOTREASON_WATCHDOG);
    TEST_ASSERT_EQUAL_UINT(1, bootreason_check_wdt_timeout_reset_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(0, bootreason_check_normal_power_on_fake.call_count);

    setUp();
    g_wdt_status = BOOTREASON_STATUS_OK;
    init_with_flag(BOOTREASON_SLEEP_ENTER_FLAG);
    assert_reason(BOOTREASON_SLEEPERROR);
    TEST_ASSERT_EQUAL_UINT(1, bootreason_check_wdt_timeout_reset_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(0, bootreason_check_normal_power_on_fake.call_count);

    setUp();
    g_normal_power_status = BOOTREASON_STATUS_OK;
    init_with_flag(0);
    assert_reason(BOOTREASON_NORMAL);
    TEST_ASSERT_EQUAL_UINT(1, bootreason_check_wdt_timeout_reset_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(1, bootreason_check_normal_power_on_fake.call_count);

    setUp();
    init_with_flag(0);
    assert_reason(BOOTREASON_UNKNOWN);
    TEST_ASSERT_EQUAL_UINT(1, bootreason_check_wdt_timeout_reset_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(1, bootreason_check_normal_power_on_fake.call_count);
}

void test_get_reason_without_init_reads_current_rtc_state(void)
{
    g_rtc_data[BOOTREASON_FLAG_OFFSET] = BOOTREASON_SOFT_RESET_FLAG;
    assert_reason(BOOTREASON_SOFT_RESET);
}

void test_get_info_initializes_payloads_and_returns_error_for_reset_only_reasons(void)
{
    bootreason_info_t info;

    init_with_flag(BOOTREASON_SOFT_RESET_FLAG);
    memset(&info, 0xA5, sizeof(info));

    TEST_ASSERT_EQUAL_INT(BOOTREASON_STATUS_ERROR, bootreason_get_info(&info));
    TEST_ASSERT_EQUAL_INT(BOOTREASON_SOFT_RESET, info.reason);
    TEST_ASSERT_NULL(info.panic_file.data);
    TEST_ASSERT_EQUAL_UINT32(0, info.panic_file.len);
    TEST_ASSERT_NULL(info.registers.data);
    TEST_ASSERT_EQUAL_UINT32(0, info.registers.len);
    TEST_ASSERT_NULL(info.stack.data);
    TEST_ASSERT_EQUAL_UINT32(0, info.stack.len);
    TEST_ASSERT_NULL(info.custom.data);
    TEST_ASSERT_EQUAL_UINT32(0, info.custom.len);
}

void test_get_info_accepts_normal_reason_without_minidump(void)
{
    bootreason_info_t info;

    g_normal_power_status = BOOTREASON_STATUS_OK;
    init_with_flag(0);
    memset(&info, 0xA5, sizeof(info));

    TEST_ASSERT_EQUAL_INT(BOOTREASON_STATUS_OK, bootreason_get_info(&info));
    TEST_ASSERT_EQUAL_INT(BOOTREASON_NORMAL, info.reason);
    TEST_ASSERT_NULL(info.custom.data);
    TEST_ASSERT_EQUAL_UINT32(0, info.custom.len);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_flag_setters_update_rtc_flags_without_clobbering_existing_bits);
    RUN_TEST(test_init_prioritizes_assert_exception_xoff_and_reset_flags);
    RUN_TEST(test_init_reports_watchdog_sleeperror_normal_and_unknown_paths);
    RUN_TEST(test_get_reason_without_init_reads_current_rtc_state);
    RUN_TEST(test_get_info_initializes_payloads_and_returns_error_for_reset_only_reasons);
    RUN_TEST(test_get_info_accepts_normal_reason_without_minidump);
    return UNITY_END();
}
