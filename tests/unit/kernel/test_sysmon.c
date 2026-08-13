#include "xy_sysmon.h"
#include "xy_stdio.h"

#include <stdint.h>
#include <string.h>

#include "unity.h"

static uint32_t fake_tick;
static char print_log[1024];
static size_t print_log_len;

uint8_t heap_area[256];
uint8_t *_heap_start = &heap_area[0];
uint8_t *_heap_end = &heap_area[sizeof(heap_area)];

uint32_t xy_os_tick_get(void)
{
    return fake_tick;
}

static void capture_print(char *str)
{
    size_t len;
    size_t copy_len;

    if (!str) {
        return;
    }

    len = strlen(str);
    copy_len = len;
    if (copy_len > sizeof(print_log) - print_log_len - 1U) {
        copy_len = sizeof(print_log) - print_log_len - 1U;
    }
    memcpy(&print_log[print_log_len], str, copy_len);
    print_log_len += copy_len;
    print_log[print_log_len] = '\0';
}

void setUp(void)
{
    fake_tick = 1000U;
    print_log_len = 0U;
    print_log[0] = '\0';
    xy_stdio_printf_init(capture_print);
}

void tearDown(void)
{
}

static void test_sysmon_init_stats_and_simple_getters(void)
{
    xy_sys_stats_t stats;

    memset(&stats, 0xA5, sizeof(stats));
    TEST_ASSERT_EQUAL_INT(XY_SYSMON_INVALID_PARAM, xy_sysmon_get_stats(NULL));

    TEST_ASSERT_EQUAL_INT(XY_SYSMON_OK, xy_sysmon_init());
    fake_tick = 1234U;
    TEST_ASSERT_EQUAL_INT(XY_SYSMON_OK, xy_sysmon_get_stats(&stats));

    TEST_ASSERT_EQUAL_FLOAT(0.0F, stats.cpu_usage);
    TEST_ASSERT_EQUAL_UINT32(sizeof(heap_area), stats.heap_total);
    TEST_ASSERT_EQUAL_UINT32(0U, stats.heap_used);
    TEST_ASSERT_EQUAL_UINT32(0U, stats.heap_max_used);
    TEST_ASSERT_EQUAL_FLOAT(0.0F, stats.heap_usage);
    TEST_ASSERT_EQUAL_UINT32(0U, stats.stack_total);
    TEST_ASSERT_EQUAL_UINT32(0U, stats.stack_used);
    TEST_ASSERT_EQUAL_UINT32(0U, stats.stack_peak);
    TEST_ASSERT_EQUAL_FLOAT(0.0F, stats.stack_usage);
    TEST_ASSERT_EQUAL_UINT32(0U, stats.task_count);
    TEST_ASSERT_EQUAL_UINT32(0U, stats.task_max);
    TEST_ASSERT_EQUAL_UINT32(234U, stats.uptime);
    TEST_ASSERT_EQUAL_UINT32(1000U, stats.tick_rate);

    TEST_ASSERT_EQUAL_FLOAT(0.0F, xy_sysmon_get_cpu_usage());
    TEST_ASSERT_EQUAL_FLOAT(0.0F, xy_sysmon_get_heap_usage());
    TEST_ASSERT_EQUAL_FLOAT(0.0F, xy_sysmon_get_stack_usage());
    TEST_ASSERT_EQUAL_UINT32(234U, xy_sysmon_get_uptime());
    TEST_ASSERT_EQUAL_UINT32(0U, xy_sysmon_get_task_count());
}

static void test_sysmon_heap_usage_handles_zero_sized_heap(void)
{
    xy_sys_stats_t stats;
    uint8_t *saved_heap_end = _heap_end;

    TEST_ASSERT_EQUAL_INT(XY_SYSMON_OK, xy_sysmon_init());
    _heap_end = _heap_start;

    TEST_ASSERT_EQUAL_INT(XY_SYSMON_OK, xy_sysmon_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(0U, stats.heap_total);
    TEST_ASSERT_EQUAL_FLOAT(0.0F, stats.heap_usage);
    TEST_ASSERT_EQUAL_FLOAT(0.0F, xy_sysmon_get_heap_usage());

    _heap_end = saved_heap_end;
}

static void test_sysmon_print_helpers_emit_portable_task_contract(void)
{
    TEST_ASSERT_EQUAL_INT(XY_SYSMON_OK, xy_sysmon_init());
    fake_tick = 1500U;

    xy_sysmon_print_status();

    TEST_ASSERT_NOT_NULL(strstr(print_log, "=== System Status ==="));
    TEST_ASSERT_NOT_NULL(strstr(print_log, "Heap:"));
    TEST_ASSERT_NOT_NULL(strstr(print_log, "Uptime:"));
    TEST_ASSERT_NOT_NULL(strstr(print_log, "=== Task List ==="));
    TEST_ASSERT_NOT_NULL(strstr(print_log, "Total tasks:"));
    TEST_ASSERT_NOT_NULL(strstr(print_log, "RTOS specific"));
}

static void test_sysmon_alarm_registration_is_host_stubbed_contract(void)
{
    TEST_ASSERT_EQUAL_INT(XY_SYSMON_INVALID_PARAM,
                          xy_sysmon_register_alarm(NULL, 75.0F, NULL));
    TEST_ASSERT_EQUAL_STRING("", print_log);

    TEST_ASSERT_EQUAL_INT(XY_SYSMON_OK,
                          xy_sysmon_register_alarm("heap", 75.0F, NULL));
    TEST_ASSERT_NOT_NULL(strstr(print_log, "Sysmon alarm registered: heap"));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_sysmon_init_stats_and_simple_getters);
    RUN_TEST(test_sysmon_heap_usage_handles_zero_sized_heap);
    RUN_TEST(test_sysmon_print_helpers_emit_portable_task_contract);
    RUN_TEST(test_sysmon_alarm_registration_is_host_stubbed_contract);
    return UNITY_END();
}
