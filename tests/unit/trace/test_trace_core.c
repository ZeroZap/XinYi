#include <stdint.h>
#include <string.h>

#include "unity.h"
#include "fff.h"
#include "xy_log.h"

void xy_log_init(void);
void xy_log_str(char *str);
void xy_log_raw(char *data, size_t len);
void xy_log_set_dynamic_level(uint8_t level);
uint8_t xy_log_dynamic_level(void);

static char g_log_buffer[256];
static size_t g_log_len;

DEFINE_FFF_GLOBALS;

FAKE_VOID_FUNC(capture_printf, char *)

static void capture_printf_impl(char *str);

void xy_log_char(char ch)
{
    if (g_log_len + 1U < sizeof(g_log_buffer)) {
        g_log_buffer[g_log_len++] = ch;
        g_log_buffer[g_log_len] = '\0';
    }
}

static void capture_printf_impl(char *str)
{
    if (!str) {
        return;
    }
    while (*str != '\0') {
        xy_log_char(*str++);
    }
}

static void reset_capture(void)
{
    RESET_FAKE(capture_printf);
    FFF_RESET_HISTORY();
    capture_printf_fake.custom_fake = capture_printf_impl;

    g_log_len = 0;
    g_log_buffer[0] = '\0';
}

void setUp(void)
{
    reset_capture();
}

void tearDown(void)
{
}

static void test_log_raw_and_string_output(void)
{
    char mutable_msg[] = "abc";
    char raw[] = {'X', 'Y', 'Z'};

    xy_log_str(NULL);
    TEST_ASSERT_EQUAL_UINT(0U, g_log_len);
    xy_log_str(mutable_msg);
    TEST_ASSERT_EQUAL_STRING("abc", g_log_buffer);

    reset_capture();
    xy_log_raw(raw, sizeof(raw));
    TEST_ASSERT_EQUAL_STRING("XYZ", g_log_buffer);

    reset_capture();
    xy_log_raw(raw, 0U);
    TEST_ASSERT_EQUAL_UINT(0U, g_log_len);
}

static void test_dynamic_level_bounds(void)
{
    xy_log_set_dynamic_level(XY_LOG_LEVEL_ERROR);
    TEST_ASSERT_EQUAL_UINT8(XY_LOG_LEVEL_ERROR, xy_log_dynamic_level());
    xy_log_set_dynamic_level(XY_LOG_LEVEL_DEBUG);
    TEST_ASSERT_EQUAL_UINT8(XY_LOG_LEVEL_DEBUG, xy_log_dynamic_level());
    xy_log_set_dynamic_level((uint8_t)(XY_LOG_LEVEL_DEBUG + 1U));
    TEST_ASSERT_EQUAL_UINT8(XY_LOG_LEVEL_DEBUG, xy_log_dynamic_level());
}

static void test_log_init_and_public_macros(void)
{
    xy_stdio_printf_init(capture_printf);
    XY_LOG_E("err=%d", 7);
    TEST_ASSERT_EQUAL_UINT(1U, capture_printf_fake.call_count);
    TEST_ASSERT_EQUAL_STRING("[E] err=7", capture_printf_fake.arg0_val);
    TEST_ASSERT_EQUAL_STRING("[E] err=7", g_log_buffer);

    reset_capture();
    xy_log_i("info %s", "ok");
    TEST_ASSERT_EQUAL_UINT(1U, capture_printf_fake.call_count);
    TEST_ASSERT_EQUAL_STRING("[I] info ok", capture_printf_fake.arg0_val);
    TEST_ASSERT_EQUAL_STRING("[I] info ok", g_log_buffer);

    reset_capture();
    xy_log_init();
    XY_LOG_W("warn");
    TEST_ASSERT_EQUAL_STRING("[W] warn", g_log_buffer);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_log_raw_and_string_output);
    RUN_TEST(test_dynamic_level_bounds);
    RUN_TEST(test_log_init_and_public_macros);
    return UNITY_END();
}
