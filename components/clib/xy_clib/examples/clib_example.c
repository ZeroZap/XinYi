/**
 * @file clib_example.c
 * @brief Tour of the xy_clib public API, compiled and run by CTest `clib_example`.
 *
 * This file is a build artifact, not prose. The clib library glob excludes
 * `*_example.c`, so before this target existed nothing had ever compiled this
 * sample, and it had drifted into a tour of an API that does not exist:
 * `struct rt_ringbuffer` (the pre-XinYi RT-Thread type), `xy_list_add_node`
 * (the macro is `xy_list_add_note`), `xy_container_of`, and a
 * `performance_example()` that used two undeclared locals. Every call below is
 * against a declaration the headers actually publish.
 *
 * Making it compile turned up three contracts that were only true on paper:
 *  - `xy_u32_div10()` was defined in xy_common.c and advertised in
 *    CLIB_DETAILED_DESCRIPTION.md, but declared in no header (now declared).
 *  - `XY_USE_SOFT_DIV` is a numeric macro tested with `#if(n)def`, so its value
 *    was ignored and the software path shipped unconditionally.
 *  - `xy_rotl32()` rotated in the operand's width, so a 64-bit host produced a
 *    64-bit result and only the 32-bit target could pass.
 * This example is also compiled a second time against `-DXY_USE_SOFT_DIV=1`
 * (CTest `clib_example_softdiv`), which is what proves the two division paths
 * agree instead of assuming it.
 *
 * Two measured include facts that the sample now states instead of hiding:
 *  - `xy_clib.h` does not include `xy_helper.h` (the `xy_strlcpy`/`xy_strlcat`
 *    inline helpers) or `xy_rbl.h` (the macro ring buffer), so both are
 *    included explicitly here rather than assumed to arrive with the umbrella.
 *  - `xy_rbl_put()` is a statement macro returning nothing; it reports "full"
 *    through `xy_rbl_full()`, so the sample must not test a return value.
 *
 * There is deliberately no `assert()`: the default build type is Release,
 * where `NDEBUG` turns `assert()` into nothing and the sample would silently
 * check nothing at all. `main` returns non-zero when a check fails.
 */

#include "xy_clib.h"

#include "xy_helper.h" /* xy_strlcpy / xy_strlcat: not re-exported by xy_clib.h */
#include "xy_rbl.h"    /* xy_rbl_t macro ring buffer: likewise */

#include <stdint.h>
#include <stdio.h>

static int g_checks;
static int g_failures;

static void check_int(const char *what, long expected, long actual)
{
    g_checks++;
    if (expected != actual) {
        g_failures++;
        printf("  FAIL %-42s expected %ld, got %ld\n", what, expected, actual);
        return;
    }
    printf("  ok   %-42s %ld\n", what, actual);
}

static void check_str(const char *what, const char *expected, const char *actual)
{
    g_checks++;
    if (xy_strcmp(expected, actual) != 0) {
        g_failures++;
        printf("  FAIL %-42s expected \"%s\", got \"%s\"\n", what, expected, actual);
        return;
    }
    printf("  ok   %-42s \"%s\"\n", what, actual);
}

/* --- 1. Strings and memory --- */

static void string_example(void)
{
    char buffer[64];
    char copied[8];
    char *found;

    printf("\n-- strings and memory --\n");

    check_int("xy_strlcpy() returns source length", 11,
              (long)xy_strlcpy(buffer, "Hello World", sizeof(buffer)));
    check_str("xy_strlcpy() copied", "Hello World", buffer);
    check_int("xy_strlen()", 11, (long)xy_strlen(buffer));
    check_int("xy_strcmp() equal", 0, xy_strcmp(buffer, "Hello World"));

    found = xy_strchr(buffer, 'W');
    check_int("xy_strchr('W') offset", 6, found ? (long)(found - buffer) : -1L);
    check_int("xy_strchr('z') absent", 1, xy_strchr(buffer, 'z') == NULL ? 1 : 0);

    xy_memset(buffer, 0, sizeof(buffer));
    check_int("xy_memset() first byte", 0, (long)(uint8_t)buffer[0]);
    check_int("xy_memset() last byte", 0, (long)(uint8_t)buffer[sizeof(buffer) - 1]);

    xy_memcpy(copied, "abc", 4);
    check_str("xy_memcpy()", "abc", copied);
    check_int("xy_array_size()", 8, (long)xy_array_size(copied));
}

/* --- 2. Number conversion and the software-division helpers --- */

static void number_example(void)
{
    char text[16];
    uint32_t v;
    int sweep_ok;

    printf("\n-- numbers --\n");

    check_int("xy_atoi(\"12345\")", 12345, xy_atoi("12345"));
    check_int("xy_atoi(\"-7\")", -7, xy_atoi("-7"));
    check_str("xy_itoa(12345, base 10)", "12345", xy_itoa(12345, text, 10));
    check_str("xy_itoa(-42, base 10)", "-42", xy_itoa(-42, text, 10));

    /* BCD helpers convert decimal digits to one nibble each, and back. */
    check_int("xy_dec2bcd(1234)", 0x1234, (long)xy_dec2bcd(1234U));
    check_int("xy_bcd2dec(0x1234)", 1234, (long)xy_bcd2dec(0x1234U));

    /* The div10/mod10 helpers are what the software-division path of
     * xy_common.c uses. Sweeping them against the arithmetic answer is what
     * makes the two builds (hardware path and -DXY_USE_SOFT_DIV=1) comparable
     * instead of merely compiling. */
    sweep_ok = 1;
    for (v = 0; v <= 1000U; v += 37U) {
        if (xy_u32_div10(v) != v / 10U || xy_u32_mod10(v) != v % 10U) {
            sweep_ok = 0;
            printf("  FAIL div10/mod10 mismatch at %u: div=%u mod=%u\n", v, xy_u32_div10(v),
                   xy_u32_mod10(v));
            break;
        }
    }
    check_int("div10/mod10 sweep 0..1000 step 37", 1, sweep_ok);
    check_int("xy_u32_div10(4294967295)", 429496729U, (long)xy_u32_div10(4294967295U));
    check_int("xy_u32_mod10(4294967295)", 5, (long)xy_u32_mod10(4294967295U));
    check_int("xy_u8_mod10(237)", 7, xy_u8_mod10(237U));
    check_int("xy_u16_mod10(65535)", 5, xy_u16_mod10(65535U));
    check_int("xy_u32_mod10(4000000000)", 0, (long)xy_u32_mod10(4000000000U));
    check_int("xy_u64_div10(1e9)", 100000000LL, (long)xy_u64_div10(1000000000ULL));
    check_int("xy_u64_div10(2^40)", 109951162777LL, (long)xy_u64_div10(1099511627776ULL));

    check_int("xy_abs(-42)", 42, xy_abs(-42));
    check_int("xy_min(3,9)", 3, xy_min(3, 9));
    check_int("xy_clamp(99,0,10)", 10, xy_clamp(99, 0, 10));
}

/* --- 3. Formatted output --- */

static void format_example(void)
{
    char buffer[64];
    char tiny[5];

    printf("\n-- formatted output --\n");

    /* Measured limit of this formatter: it handles the conversions `%c %s %d
     * %i %u %x %X %f %%` but no flags, field width or precision. `%04X` is
     * copied through as the literal text "%04X", so this sample only uses the
     * supported forms rather than teaching a printf compatibility that is not
     * there. */
    xy_snprintf(buffer, sizeof(buffer), "Value: %d, Hex: 0x%X", 123, 0xABCD);
    check_str("xy_snprintf() int + hex", "Value: 123, Hex: 0xABCD", buffer);

    xy_snprintf(buffer, sizeof(buffer), "%s/%u/%x/%c/%%", "ab", 4294967295U, 0xBEEFU, 'z');
    check_str("xy_snprintf() s/u/x/c/%", "ab/4294967295/beef/z/%", buffer);

    /* A too-small buffer must stay NUL-terminated and must not overrun. */
    xy_memset(tiny, 'X', sizeof(tiny));
    xy_snprintf(tiny, sizeof(tiny), "%s", "abcdefgh");
    check_int("xy_snprintf() truncates at size-1", 4, (long)xy_strlen(tiny));
    check_int("xy_snprintf() NUL-terminated", 0, (long)(uint8_t)tiny[sizeof(tiny) - 1]);

    xy_sprintf(buffer, "%u", 4294967295U);
    check_str("xy_sprintf() uint32", "4294967295", buffer);
}

/* --- 4. Bit fields --- */

static void bit_example(void)
{
    uint32_t reg = 0;

    printf("\n-- bit fields --\n");

    xy_set_bit(reg, 3);
    check_int("xy_set_bit(3)", 0x08, (long)reg);

    xy_set_bits(reg, 8, 0xFU);
    check_int("xy_set_bits(8,0xF)", 0x0F08, (long)reg);

    check_int("xy_get_bit(3)", 1, xy_get_bit(reg, 3) ? 1 : 0);
    check_int("xy_get_bit(4)", 0, xy_get_bit(reg, 4) ? 1 : 0);
    check_int("xy_get_bits(8,0xF)", 0xF, (long)(xy_get_bits(reg, 8, 0xFU) >> 8));

    xy_toggle_bit(reg, 4);
    check_int("xy_toggle_bit(4) sets", 0x0F18, (long)reg);
    xy_toggle_bit(reg, 4);
    check_int("xy_toggle_bit(4) clears", 0x0F08, (long)reg);

    xy_clear_bit(reg, 3);
    check_int("xy_clear_bit(3)", 0x0F00, (long)reg);
    xy_clear_bits(reg, 8, 0xFU);
    check_int("xy_clear_bits(8,0xF)", 0x0000, (long)reg);

    check_int("xy_htons(0x1234)", 0x3412, xy_htons(0x1234));
    check_int("xy_htonl(0x12345678)", 0x78563412, (long)xy_htonl(0x12345678UL));
    check_int("xy_rotl32(0x80000001,1)", 0x00000003, (long)xy_rotl32(0x80000001UL, 1));
}

/* --- 5. Ring buffers: the full-width one and the macro one --- */

static void ringbuffer_example(void)
{
    uint8_t pool[32];
    uint8_t readback[32];
    uint8_t macro_pool[16];
    xy_rb_t rb;
    xy_rbl_t macro_rb = {macro_pool, 0, 0, 0, sizeof(macro_pool)};
    const char *data = "Hello Ring Buffer";
    size_t written;
    size_t read;
    int i;

    printf("\n-- ring buffers --\n");

    xy_rb_init(&rb, pool, (int32_t)sizeof(pool));
    check_int("xy_rb_data_len() empty", 0, (long)xy_rb_data_len(&rb));

    written = xy_rb_put(&rb, (const uint8_t *)data, (uint32_t)xy_strlen(data));
    check_int("xy_rb_put() accepted every byte", (long)xy_strlen(data), (long)written);
    check_int("xy_rb_data_len() after put", (long)xy_strlen(data), (long)xy_rb_data_len(&rb));

    read = xy_rb_get(&rb, readback, (uint32_t)sizeof(readback));
    check_int("xy_rb_get() returned the bytes", (long)xy_strlen(data), (long)read);
    readback[read] = '\0';
    check_str("xy_rb round trip", "Hello Ring Buffer", (const char *)readback);
    check_int("xy_rb_data_len() drained", 0, (long)xy_rb_data_len(&rb));

    /* The macro ring buffer requires a power-of-two size: it masks the index
     * with `size - 1` instead of comparing against the end. */
    for (i = 0; i < 10; i++) {
        xy_rbl_put(&macro_rb, (uint8_t)('A' + i));
    }
    check_int("xy_rbl count after 10 puts", 10, (long)macro_rb.count);
    check_int("xy_rbl_full() false at 10/16", 0, xy_rbl_full(&macro_rb) ? 1 : 0);
    check_int("xy_rbl_get() first byte", 'A', (long)xy_rbl_get(&macro_rb));
    check_int("xy_rbl_get() second byte", 'B', (long)xy_rbl_get(&macro_rb));
    check_int("xy_rbl count after 2 gets", 8, (long)macro_rb.count);

    for (i = 0; i < 16; i++) {
        xy_rbl_put(&macro_rb, 0x5A);
    }
    check_int("xy_rbl_full() after overfill", 1, xy_rbl_full(&macro_rb) ? 1 : 0);
    check_int("xy_rbl count saturates at size", 16, (long)macro_rb.count);
}

/* --- 6. Intrusive singly linked list --- */

static void list_example(void)
{
    /* Static nodes, no heap: xy_list_node carries the payload itself
     * (`int value`), so an embedded target needs no allocator here. */
    static xy_list_node nodes[4];
    xy_list_node *head = NULL;
    xy_list_node *node;
    xy_list_node *tmp;
    int seen = 0;
    int i;

    printf("\n-- linked list --\n");

    for (i = 0; i < 4; i++) {
        nodes[i].value = i;
        nodes[i].next = NULL;
        xy_list_add_note(head, &nodes[i]);
    }
    check_int("head is the last node added", 3, head->value);

    xy_list_for_node(head, node)
    {
        seen++;
    }
    check_int("xy_list_for_node() visits every node", 4, seen);

    xy_list_del_node(head, &nodes[1], tmp);
    seen = 0;
    xy_list_for_node(head, node) { seen++; }
    check_int("xy_list_del_node() removes one", 3, seen);

    seen = 0;
    xy_list_for_node_safe(head, node, tmp) { seen++; }
    check_int("xy_list_for_node_safe() walks after deletion", 3, seen);
}

/* --- 7. Safe string handling --- */

static void safety_example(void)
{
    char buffer[8];
    const char *long_str = "This is far longer than the buffer";

    printf("\n-- safe strings --\n");

    /* strlcpy returns the length it would have needed, not how much fit:
     * that is how a caller detects truncation. */
    check_int("xy_strlcpy() reports full length", (long)xy_strlen(long_str),
              (long)xy_strlcpy(buffer, long_str, sizeof(buffer)));
    check_int("destination fits size-1 chars", 7, (long)xy_strlen(buffer));
    check_str("destination is truncated", "This is", buffer);

    xy_strlcpy(buffer, "abc", sizeof(buffer));
    /* BSD semantics: the return value is the length it tried to build
     * (strlen(dest) + strlen(src) = 5), not the number of bytes appended. */
    check_int("xy_strlcat() returns the size it built", 5,
              (long)xy_strlcat(buffer, "de", sizeof(buffer)));
    check_str("xy_strlcat() appended", "abcde", buffer);

    /* Refused inputs, not crashes. */
    check_int("xy_strlcpy(NULL src)", 0, (long)xy_strlcpy(buffer, NULL, sizeof(buffer)));
    check_int("xy_strlcpy(size 0)", 0, (long)xy_strlcpy(buffer, "x", 0));
}

/* --- 8. Error codes --- */

static xy_error_t fail_if_two(int value)
{
    /* The macro returns early from the enclosing function on failure. */
    XY_RETURN_ON_ERROR(value == 2 ? XY_ERROR_INVALID_PARAM : XY_OK);
    return XY_OK;
}

static void error_example(void)
{
    xy_error_t ret = fail_if_two(2);

    printf("\n-- error codes --\n");

    check_int("XY_OK is 0", 0, (long)XY_OK);
    check_int("XY_FAILED(XY_OK)", 0, XY_FAILED(XY_OK) ? 1 : 0);
    check_int("XY_FAILED(XY_ERROR_INVALID_PARAM)", 1, XY_FAILED(XY_ERROR_INVALID_PARAM) ? 1 : 0);
    check_int("XY_RETURN_ON_ERROR propagates", (long)XY_ERROR_INVALID_PARAM, (long)ret);
    check_int("XY_RETURN_ON_ERROR passes success", 0, (long)fail_if_two(1));
    check_str("xy_error_to_string(XY_OK)", "XY_OK", xy_error_to_string(XY_OK));
    check_str("xy_error_to_string(INVALID_PARAM)", "XY_ERROR_INVALID_PARAM",
              xy_error_to_string(XY_ERROR_INVALID_PARAM));
}

/* --- 9. Build configuration this TU was compiled with --- */

static void config_example(void)
{
    printf("\n-- configuration --\n");
    check_int("XY_PRINTF_BUFSIZE is positive", 1, XY_PRINTF_BUFSIZE > 0 ? 1 : 0);
    printf("  info XY_PRINTF_BUFSIZE                %d\n", (int)XY_PRINTF_BUFSIZE);

    /* Report the *value*, not just that the macro exists: `#ifdef` is true for
     * `#define XY_USE_SOFT_DIV 0`, which is exactly how the flag used to be
     * read as if it were enabled. */
    printf("  info XY_USE_SOFT_DIV                  %d\n", (int)XY_USE_SOFT_DIV);
    check_int("division path this binary was compiled with", 1,
              (XY_USE_SOFT_DIV == 0 || XY_USE_SOFT_DIV == 1) ? 1 : 0);
#if XY_USE_SOFT_DIV
    printf("  info path: software divide/multiply (no hardware divider assumed)\n");
#else
    printf("  info path: hardware divide (`/` and `%%`)\n");
#endif

#if defined(XY_PRINTF_FLOAT_ENABLE) && (XY_PRINTF_FLOAT_ENABLE)
    printf("  info float formatting enabled\n");
#else
    printf("  info float formatting disabled\n");
#endif
}

int main(void)
{
    printf("XY CLIB example: samples of the public xy_clib API\n");

    string_example();
    number_example();
    format_example();
    bit_example();
    ringbuffer_example();
    list_example();
    safety_example();
    error_example();
    config_example();

    printf("\n%d checks, %d failure(s)\n", g_checks, g_failures);
    return (g_failures == 0) ? 0 : 1;
}
