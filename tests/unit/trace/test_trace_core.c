#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "xy_log.h"

void xy_log_init(void);
void xy_log_str(char *str);
void xy_log_raw(char *data, size_t len);
void xy_log_set_dynamic_level(uint8_t level);
uint8_t xy_log_dynamic_level(void);

static char g_log_buffer[256];
static size_t g_log_len;

void xy_log_char(char ch)
{
    if (g_log_len + 1U < sizeof(g_log_buffer)) {
        g_log_buffer[g_log_len++] = ch;
        g_log_buffer[g_log_len] = '\0';
    }
}

static void capture_printf(char *str)
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
    g_log_len = 0;
    g_log_buffer[0] = '\0';
}

static void test_log_raw_and_string_output(void)
{
    char mutable_msg[] = "abc";
    char raw[] = {'X', 'Y', 'Z'};

    reset_capture();
    xy_log_str(NULL);
    assert(g_log_len == 0U);
    xy_log_str(mutable_msg);
    assert(strcmp(g_log_buffer, "abc") == 0);

    reset_capture();
    xy_log_raw(raw, sizeof(raw));
    assert(strcmp(g_log_buffer, "XYZ") == 0);

    reset_capture();
    xy_log_raw(raw, 0U);
    assert(g_log_len == 0U);
}

static void test_dynamic_level_bounds(void)
{
    xy_log_set_dynamic_level(XY_LOG_LEVEL_ERROR);
    assert(xy_log_dynamic_level() == XY_LOG_LEVEL_ERROR);
    xy_log_set_dynamic_level(XY_LOG_LEVEL_DEBUG);
    assert(xy_log_dynamic_level() == XY_LOG_LEVEL_DEBUG);
    xy_log_set_dynamic_level((uint8_t)(XY_LOG_LEVEL_DEBUG + 1U));
    assert(xy_log_dynamic_level() == XY_LOG_LEVEL_DEBUG);
}

static void test_log_init_and_public_macros(void)
{
    reset_capture();
    xy_stdio_printf_init(capture_printf);
    XY_LOG_E("err=%d", 7);
    assert(strcmp(g_log_buffer, "[E] err=7") == 0);

    reset_capture();
    xy_log_i("info %s", "ok");
    assert(strcmp(g_log_buffer, "[I] info ok") == 0);

    reset_capture();
    xy_log_init();
    XY_LOG_W("warn");
    assert(strcmp(g_log_buffer, "[W] warn") == 0);
}

int main(void)
{
    test_log_raw_and_string_output();
    test_dynamic_level_bounds();
    test_log_init_and_public_macros();
    puts("Trace component tests passed");
    return 0;
}
