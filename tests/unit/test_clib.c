#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "xy_common.h"
#include "xy_rb.h"
#include "xy_stdio.h"
#include "xy_string.h"

static void test_clib_common_helpers(void)
{
    uint32_t value = 0;

    assert(xy_u64_div10(1234567890123ULL) == 123456789012ULL);
    assert(xy_u8_mod10(27U) == 7U);
    assert(xy_u16_mod10(1234U) == 4U);
    assert(xy_u32_mod10(98765U) == 5U);
    assert(xy_hex2bcd(0x1234U) == 0x1234U);
    assert(xy_bcd2hex(0x1234U) == 1234U);
    assert(xy_dec2bcd(9876U) == 0x9876U);
    assert(xy_bcd2dec(0x9876U) == 9876U);

    xy_set_bit(value, 3);
    assert(value == 8U);
    xy_toggle_bit(value, 1);
    assert(value == 10U);
    xy_clear_bit(value, 3);
    assert(value == 2U);
    assert(xy_max(3, 7) == 7);
    assert(xy_min(3, 7) == 3);
    assert(xy_clamp(12, 0, 10) == 10);
}

static void test_clib_string_helpers(void)
{
    char buffer[32];
    char overlap[] = "abcdef";

    assert(xy_strlen("hello") == 5U);
    assert(xy_strlen(NULL) == 0U);
    assert(xy_strcmp("abc", "abc") == 0);
    assert(xy_strcmp("abc", "abd") < 0);
    assert(xy_stricmp("AbC", "aBc") == 0);

    assert(xy_memset(buffer, 'x', 3U) == buffer);
    buffer[3] = '\0';
    assert(strcmp(buffer, "xxx") == 0);
    assert(xy_memcpy(buffer, "abc", 4U) == buffer);
    assert(strcmp(buffer, "abc") == 0);
    assert(xy_memcmp("abc", "abc", 3U) == 0);

    assert(xy_strcpy(buffer, "xy") == buffer);
    assert(strcmp(buffer, "xy") == 0);
    assert(xy_strcat(buffer, "z") == buffer);
    assert(strcmp(buffer, "xyz") == 0);
    assert(xy_strchr(buffer, 'y') == &buffer[1]);
    assert(xy_strrchr("abca", 'a') == &"abca"[3]);
    assert(xy_strstr("hello world", "world") != NULL);
    assert(xy_strnlen("abcdef", 3U) == 3U);

    assert(xy_memmove(overlap + 2, overlap, 4U) == overlap + 2);
    assert(memcmp(overlap, "ababcd", 6U) == 0);
}

static void test_clib_stdio_helpers(void)
{
    char buffer[64];
    char *end = NULL;

    assert(xy_sprintf(buffer, "n=%d hex=%X", 42, 0x2AU) > 0);
    assert(strcmp(buffer, "n=42 hex=2A") == 0);
    assert(xy_snprintf(buffer, sizeof(buffer), "%s-%u", "id", 7U) > 0);
    assert(strcmp(buffer, "id-7") == 0);
    assert(xy_atoi("-123") == -123);
    assert(xy_atol("456") == 456L);
    assert(xy_strtoul("0x10", &end, 0) == 16UL);
    assert(end && *end == '\0');
    assert(xy_strtol("-20", &end, 10) == -20L);
    assert(end && *end == '\0');
}

static void test_clib_ring_buffer_helpers(void)
{
    xy_rb_t rb;
    uint8_t pool[5];
    uint8_t out[5] = {0};
    uint8_t *peek = NULL;

    xy_rb_init(&rb, pool, (int32_t)sizeof(pool));
    assert(xy_rb_data_len(&rb) == 0U);
    assert(xy_rb_space_len(&rb) == sizeof(pool));
    assert(xy_rb_put(&rb, (const uint8_t *)"abcd", 4U) == 4U);
    assert(xy_rb_data_len(&rb) == 4U);
    assert(xy_rb_peek(&rb, &peek) == 4U);
    assert(peek == pool);
    assert(xy_rb_get(&rb, out, 2U) == 2U);
    assert(out[0] == 'a' && out[1] == 'b');
    assert(xy_rb_putchar(&rb, 'e') == 1U);
    assert(xy_rb_putchar_force(&rb, 'f') == 1U);
    assert(xy_rb_get(&rb, out, sizeof(out)) > 0U);

    xy_rb_t *dyn = xy_rb_create(4U);
    assert(dyn != NULL);
    assert(xy_rb_put(dyn, (const uint8_t *)"xy", 2U) == 2U);
    assert(xy_rb_getchar(dyn, out) == 1U);
    assert(out[0] == 'x');
    xy_rb_destroy(dyn);
}

int main(void)
{
    test_clib_common_helpers();
    test_clib_string_helpers();
    test_clib_stdio_helpers();
    test_clib_ring_buffer_helpers();
    puts("CLIB component tests passed");
    return 0;
}
