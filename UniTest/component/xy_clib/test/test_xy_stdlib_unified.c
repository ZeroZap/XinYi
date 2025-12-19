/**
 * @file test_xy_stdlib_unified.c
 * @brief Test file demonstrating unified xy_stdlib.h usage
 *
 * This file shows how to use xy_stdlib.h as a unified header
 * that replaces all standard C library includes.
 */

#include <stdint.h>      /* Standard types (allowed) */
#include "xy_stdlib.h"   /* All xy_clib functions */

/**
 * @brief Test string operations
 */
void test_string_operations(void)
{
    char buffer[100];
    char dest[50];
    const char *src = "Hello, XinYi!";

    /* String copy */
    xy_strcpy(buffer, src);

    /* String length */
    uint32_t len = xy_strlen(buffer);

    /* String compare (case insensitive) */
    if (xy_strcasecmp(buffer, "HELLO, XINYI!") == 0) {
        /* Match */
    }

    /* Memory operations */
    xy_memset(dest, 0, sizeof(dest));
    xy_memcpy(dest, buffer, len + 1);
    xy_memmove(buffer + 2, buffer, 10);  /* Handle overlap */

    /* Memory search */
    void *found = xy_memchr(buffer, 'X', len);
    (void)found;
}

/**
 * @brief Test formatted I/O
 */
void test_formatted_io(void)
{
    char buffer[100];
    int value = 42;

    /* Formatted output */
    xy_stdio_sprintf(buffer, "Value: %d", value);
    xy_stdio_snprintf(buffer, sizeof(buffer), "Limited: %d", value);

    /* Formatted input */
    int parsed;
    xy_stdio_sscanf("123", "%d", &parsed);
}

/**
 * @brief Test number conversions
 */
void test_number_conversions(void)
{
    char buffer[32];

    /* String to number */
    int i = xy_atoi("42");
    long l = xy_atol("1234567890");
    double d = xy_atof("3.14159");

    /* Number to string */
    xy_itoa(42, buffer, 10);      /* Decimal */
    xy_itoa(255, buffer, 16);     /* Hexadecimal */
    xy_ltoa(1000000L, buffer, 10);

    /* With base specification */
    char *end;
    unsigned long ul = xy_strtoul("0x1234", &end, 16);
    long long ll = xy_strtoll("-9876543210", &end, 10);

    (void)i; (void)l; (void)d; (void)ul; (void)ll;
}

/**
 * @brief Test math operations
 */
void test_math_operations(void)
{
    int abs_val = xy_abs(-42);
    long labs_val = xy_labs(-1000000L);

    xy_div_t div_result = xy_div(17, 5);
    xy_ldiv_t ldiv_result = xy_ldiv(1000L, 3L);

    (void)abs_val; (void)labs_val;
    (void)div_result; (void)ldiv_result;
}

/**
 * @brief Test sorting and searching
 */
static int compare_ints(const void *a, const void *b)
{
    return *(const int *)a - *(const int *)b;
}

void test_sort_search(void)
{
    int array[] = {5, 2, 8, 1, 9, 3, 7};
    int n = sizeof(array) / sizeof(array[0]);

    /* Sort array */
    xy_qsort(array, n, sizeof(int), compare_ints);

    /* Binary search */
    int key = 7;
    void *found = xy_bsearch(&key, array, n, sizeof(int), compare_ints);
    (void)found;
}

/**
 * @brief Test random number generation
 */
void test_random(void)
{
    xy_srand(12345);

    for (int i = 0; i < 10; i++) {
        int rand_val = xy_rand();
        (void)rand_val;
    }
}

/**
 * @brief Test character classification
 */
void test_character_classification(void)
{
    char c = 'A';

    if (xy_isupper(c)) {
        c = xy_tolower(c);  /* Convert to lowercase */
    }

    if (xy_isdigit('5')) {
        /* Is a digit */
    }

    if (xy_isxdigit('F')) {
        /* Is a hexadecimal digit */
    }

    (void)c;
}

/**
 * @brief Test memory management (when implemented)
 */
void test_memory_management(void)
{
    /* Note: These require platform-specific implementation */
    #if 0  /* Disabled until memory allocator is implemented */
    void *ptr = xy_malloc(100);
    if (ptr) {
        xy_memset(ptr, 0, 100);
        ptr = xy_realloc(ptr, 200);
        xy_free(ptr);
    }

    void *arr = xy_calloc(10, sizeof(int));
    if (arr) {
        xy_free(arr);
    }
    #endif
}

/**
 * @brief Main test function
 */
int main(void)
{
    test_string_operations();
    test_formatted_io();
    test_number_conversions();
    test_math_operations();
    test_sort_search();
    test_random();
    test_character_classification();
    test_memory_management();

    return 0;
}
