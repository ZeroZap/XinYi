#include <stdint.h>
#include <string.h>

#include "unity.h"
#include "xy_common.h"
#include "xy_rb.h"
#include "xy_ctype.h"
#include "xy_stdio.h"
#include "xy_string.h"

static void test_clib_common_helpers(void)
{
    uint32_t value = 0;

    TEST_ASSERT_EQUAL_UINT64(123456789012ULL, xy_u64_div10(1234567890123ULL));
    TEST_ASSERT_EQUAL_UINT8(7U, xy_u8_mod10(27U));
    TEST_ASSERT_EQUAL_UINT16(4U, xy_u16_mod10(1234U));
    TEST_ASSERT_EQUAL_UINT32(5U, xy_u32_mod10(98765U));
    TEST_ASSERT_EQUAL_UINT32(0x1234U, xy_hex2bcd(0x1234U));
    TEST_ASSERT_EQUAL_UINT32(1234U, xy_bcd2hex(0x1234U));
    TEST_ASSERT_EQUAL_UINT32(0x9876U, xy_dec2bcd(9876U));
    TEST_ASSERT_EQUAL_UINT32(9876U, xy_bcd2dec(0x9876U));

    xy_set_bit(value, 3);
    TEST_ASSERT_EQUAL_UINT32(8U, value);
    xy_toggle_bit(value, 1);
    TEST_ASSERT_EQUAL_UINT32(10U, value);
    xy_clear_bit(value, 3);
    TEST_ASSERT_EQUAL_UINT32(2U, value);
    TEST_ASSERT_EQUAL_INT(7, xy_max(3, 7));
    TEST_ASSERT_EQUAL_INT(3, xy_min(3, 7));
    TEST_ASSERT_EQUAL_INT(10, xy_clamp(12, 0, 10));
}

static void test_clib_string_helpers(void)
{
    char buffer[32];
    char overlap[] = "abcdef";

    TEST_ASSERT_EQUAL_UINT32(5U, xy_strlen("hello"));
    TEST_ASSERT_EQUAL_UINT32(0U, xy_strlen(NULL));
    TEST_ASSERT_EQUAL_INT(0, xy_strcmp("abc", "abc"));
    TEST_ASSERT_LESS_THAN_INT(0, xy_strcmp("abc", "abd"));
    TEST_ASSERT_EQUAL_INT(0, xy_stricmp("AbC", "aBc"));

    TEST_ASSERT_EQUAL_PTR(buffer, xy_memset(buffer, 'x', 3U));
    buffer[3] = '\0';
    TEST_ASSERT_EQUAL_STRING("xxx", buffer);
    TEST_ASSERT_EQUAL_PTR(buffer, xy_memcpy(buffer, "abc", 4U));
    TEST_ASSERT_EQUAL_STRING("abc", buffer);
    TEST_ASSERT_EQUAL_INT(0, xy_memcmp("abc", "abc", 3U));

    TEST_ASSERT_EQUAL_PTR(buffer, xy_strcpy(buffer, "xy"));
    TEST_ASSERT_EQUAL_STRING("xy", buffer);
    TEST_ASSERT_EQUAL_PTR(buffer, xy_strcat(buffer, "z"));
    TEST_ASSERT_EQUAL_STRING("xyz", buffer);
    TEST_ASSERT_EQUAL_PTR(&buffer[1], xy_strchr(buffer, 'y'));
    TEST_ASSERT_EQUAL_PTR(&"abca"[3], xy_strrchr("abca", 'a'));
    TEST_ASSERT_NOT_NULL(xy_strstr("hello world", "world"));
    TEST_ASSERT_EQUAL_UINT32(3U, xy_strnlen("abcdef", 3U));

    TEST_ASSERT_EQUAL_PTR(overlap + 2, xy_memmove(overlap + 2, overlap, 4U));
    TEST_ASSERT_EQUAL_MEMORY("ababcd", overlap, 6U);
}

static void test_clib_ctype_helpers(void)
{
    TEST_ASSERT_TRUE(xy_isalpha('a'));
    TEST_ASSERT_TRUE(xy_isalpha('Z'));
    TEST_ASSERT_FALSE(xy_isalpha('0'));
    TEST_ASSERT_TRUE(xy_isalnum('z'));
    TEST_ASSERT_TRUE(xy_isalnum('9'));
    TEST_ASSERT_FALSE(xy_isalnum('@'));
    TEST_ASSERT_TRUE(xy_isgraph('!'));
    TEST_ASSERT_TRUE(xy_isgraph('~'));
    TEST_ASSERT_FALSE(xy_isgraph(127));
    TEST_ASSERT_EQUAL_INT('a', xy_tolower('A'));
    TEST_ASSERT_EQUAL_INT('Z', xy_toupper('z'));
}

static void test_clib_stdio_helpers(void)
{
    char buffer[64];
    char *end = NULL;

    TEST_ASSERT_GREATER_THAN_INT(0, xy_sprintf(buffer, "n=%d hex=%X", 42, 0x2AU));
    TEST_ASSERT_EQUAL_STRING("n=42 hex=2A", buffer);
    TEST_ASSERT_GREATER_THAN_INT(0, xy_snprintf(buffer, sizeof(buffer), "%s-%u", "id", 7U));
    TEST_ASSERT_EQUAL_STRING("id-7", buffer);
    TEST_ASSERT_EQUAL_INT(4, xy_snprintf(buffer, 5U, "%s", "abcdef"));
    TEST_ASSERT_EQUAL_STRING("abcd", buffer);
    TEST_ASSERT_EQUAL_INT(-123, xy_atoi("-123"));
    TEST_ASSERT_EQUAL_INT64(456L, xy_atol("456"));
    TEST_ASSERT_EQUAL_UINT64(16UL, xy_strtoul("0x10", &end, 0));
    TEST_ASSERT_NOT_NULL(end);
    TEST_ASSERT_EQUAL_CHAR('\0', *end);
    TEST_ASSERT_EQUAL_INT64(-20L, xy_strtol("-20", &end, 10));
    TEST_ASSERT_NOT_NULL(end);
    TEST_ASSERT_EQUAL_CHAR('\0', *end);
}

static void test_clib_ring_buffer_helpers(void)
{
    xy_rb_t rb;
    uint8_t pool[5];
    uint8_t out[5] = {0};
    uint8_t *peek = NULL;

    xy_rb_init(&rb, pool, (int32_t)sizeof(pool));
    TEST_ASSERT_EQUAL_UINT32(0U, xy_rb_data_len(&rb));
    TEST_ASSERT_EQUAL_UINT32(sizeof(pool), xy_rb_space_len(&rb));
    TEST_ASSERT_EQUAL_UINT32(4U, xy_rb_put(&rb, (const uint8_t *)"abcd", 4U));
    TEST_ASSERT_EQUAL_UINT32(4U, xy_rb_data_len(&rb));
    TEST_ASSERT_EQUAL_UINT32(4U, xy_rb_peek(&rb, &peek));
    TEST_ASSERT_EQUAL_PTR(pool, peek);
    TEST_ASSERT_EQUAL_UINT32(2U, xy_rb_get(&rb, out, 2U));
    TEST_ASSERT_EQUAL_CHAR('a', out[0]);
    TEST_ASSERT_EQUAL_CHAR('b', out[1]);
    TEST_ASSERT_EQUAL_UINT32(1U, xy_rb_putchar(&rb, 'e'));
    TEST_ASSERT_EQUAL_UINT32(1U, xy_rb_putchar_force(&rb, 'f'));
    TEST_ASSERT_GREATER_THAN_UINT32(0U, xy_rb_get(&rb, out, sizeof(out)));

    xy_rb_t *dyn = xy_rb_create(4U);
    TEST_ASSERT_NOT_NULL(dyn);
    TEST_ASSERT_EQUAL_UINT32(2U, xy_rb_put(dyn, (const uint8_t *)"xy", 2U));
    TEST_ASSERT_EQUAL_UINT32(1U, xy_rb_getchar(dyn, out));
    TEST_ASSERT_EQUAL_CHAR('x', out[0]);
    xy_rb_destroy(dyn);
}

void setUp(void)
{
}

void tearDown(void)
{
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_clib_common_helpers);
    RUN_TEST(test_clib_string_helpers);
    RUN_TEST(test_clib_ctype_helpers);
    RUN_TEST(test_clib_stdio_helpers);
    RUN_TEST(test_clib_ring_buffer_helpers);
    return UNITY_END();
}
