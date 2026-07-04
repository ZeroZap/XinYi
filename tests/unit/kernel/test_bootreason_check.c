#include "unity.h"

#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#define MTK_BOOTREASON_CHECK_ENABLE 1
#define EXCEPTION_MEMDUMP_MODE 0
#define EXCEPTION_MEMDUMP_MINIDUMP 1

#include "bootreason_check.h"

static uint8_t g_rtc_data[BOOTREASON_FLAG_BYTES];
static bootreason_status_t g_wdt_status;
static bootreason_status_t g_normal_power_status;
static int g_log_call_count;

static void reset_bootreason_state(void);

void setUp(void)
{
    memset(g_rtc_data, 0, sizeof(g_rtc_data));
    g_wdt_status = BOOTREASON_STATUS_ERROR;
    g_normal_power_status = BOOTREASON_STATUS_ERROR;
    g_log_call_count = 0;
    reset_bootreason_state();
}

void tearDown(void)
{
}

void hal_rtc_get_data(uint32_t offset, char *buf, uint32_t len)
{
    TEST_ASSERT_NOT_NULL(buf);
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(sizeof(g_rtc_data), offset + len);
    memcpy(buf, &g_rtc_data[offset], len);
}

void hal_rtc_set_data(uint32_t offset, const char *buf, uint32_t len)
{
    TEST_ASSERT_NOT_NULL(buf);
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(sizeof(g_rtc_data), offset + len);
    memcpy(&g_rtc_data[offset], buf, len);
}

bootreason_status_t bootreason_check_wdt_timeout_reset(void)
{
    return g_wdt_status;
}

bootreason_status_t bootreason_check_normal_power_on(void)
{
    return g_normal_power_status;
}

void log_hal_msgid_info(const char *fmt, int arg_count, ...)
{
    TEST_ASSERT_NOT_NULL(fmt);
    TEST_ASSERT_EQUAL_INT(2, arg_count);
    g_log_call_count++;
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
    TEST_ASSERT_EQUAL_HEX8(BOOTREASON_INIT_FLAG, g_rtc_data[0]);
    TEST_ASSERT_EQUAL_HEX8(0, g_rtc_data[1]);
    TEST_ASSERT_EQUAL_INT(1, g_log_call_count);

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

    setUp();
    g_wdt_status = BOOTREASON_STATUS_OK;
    init_with_flag(BOOTREASON_SLEEP_ENTER_FLAG);
    assert_reason(BOOTREASON_SLEEPERROR);

    setUp();
    g_normal_power_status = BOOTREASON_STATUS_OK;
    init_with_flag(0);
    assert_reason(BOOTREASON_NORMAL);

    setUp();
    init_with_flag(0);
    assert_reason(BOOTREASON_UNKNOWN);
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
