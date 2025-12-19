#include "xy_stdlib.h"
#include "xy_ctype.h"
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>

double xy_atof(const char *str)
{
    double result   = 0.0;
    double fraction = 1.0;
    int sign        = 1;
    int exponent    = 0;
    int has_digits  = 0;

    // Skip leading whitespace
    while (xy_isspace(*str)) {
        str++;
    }

    // Handle sign
    if (*str == '+') {
        str++;
    } else if (*str == '-') {
        sign = -1;
        str++;
    }

    // Integer part
    while (xy_isdigit(*str)) {
        has_digits = 1;
        result     = result * 10.0 + (*str - '0');
        str++;
    }

    // Fraction part
    if (*str == '.') {
        str++;
        while (xy_isdigit(*str)) {
            has_digits = 1;
            fraction *= 0.1;
            result += (*str - '0') * fraction;
            str++;
        }
    }

    // Exponent part
    if (*str == 'e' || *str == 'E') {
        str++;
        int exp_sign = 1;
        if (*str == '+') {
            str++;
        } else if (*str == '-') {
            exp_sign = -1;
            str++;
        }

        while (xy_isdigit(*str)) {
            exponent = exponent * 10 + (*str - '0');
            str++;
        }

        exponent *= exp_sign;
    }

    if (!has_digits) {
        return 0.0;
    }

    result *= sign;

    if (exponent != 0) {
        result *= pow(10.0, exponent);
    }

    return result;
}

int xy_atoi(const char *str)
{
    int result = 0;
    int sign   = 1;

    // Skip leading whitespace
    while (xy_isspace(*str)) {
        str++;
    }

    // Handle sign
    if (*str == '+') {
        str++;
    } else if (*str == '-') {
        sign = -1;
        str++;
    }

    // Convert digits
    while (xy_isdigit(*str)) {
        // Check for overflow
        if (result > INT_MAX / 10
            || (result == INT_MAX / 10 && (*str - '0') > INT_MAX % 10)) {
            return sign == 1 ? INT_MAX : INT_MIN;
        }

        result = result * 10 + (*str - '0');
        str++;
    }

    return sign * result;
}

long xy_atol(const char *str)
{
    int64_t result = 0;
    int sign       = 1;

    // Skip whitespace
    while (xy_isspace((unsigned char)*str))
        str++;

    // Handle sign
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }

    // Convert digits
    while (xy_isdigit((unsigned char)*str)) {
        int digit = *str - '0';

        // Check for overflow before multiplying
        if (result > (-(int64_t)LONG_MIN) / 10) {
            errno = ERANGE;
            return sign == 1 ? LONG_MAX : LONG_MIN;
        }
        result = result * 10;

        // Check for overflow before adding
        if (result > (-(int64_t)LONG_MIN) - digit) {
            errno = ERANGE;
            return sign == 1 ? LONG_MAX : LONG_MIN;
        }
        result += digit;
        str++;
    }

    // Handle final value
    if (sign > 0) {
        if (result > LONG_MAX) {
            errno = ERANGE;
            return LONG_MAX;
        }
    } else {
        if (result > (-(int64_t)LONG_MIN)) {
            errno = ERANGE;
            return LONG_MIN;
        }
    }

    return (long)(sign * result);
}

double xy_strtod(const char *str, char **endptr)
{
    double result     = 0.0;
    double fraction   = 1.0;
    int sign          = 1;
    int exponent      = 0;
    int has_digits    = 0;
    const char *start = str;

    // Skip leading whitespace
    while (xy_isspace(*str)) {
        str++;
    }

    // Handle sign
    if (*str == '+') {
        str++;
    } else if (*str == '-') {
        sign = -1;
        str++;
    }

    // Integer part
    while (xy_isdigit(*str)) {
        has_digits = 1;
        result     = result * 10.0 + (*str - '0');
        str++;
    }

    // Fraction part
    if (*str == '.') {
        str++;
        while (xy_isdigit(*str)) {
            has_digits = 1;
            fraction *= 0.1;
            result += (*str - '0') * fraction;
            str++;
        }
    }

    // Exponent part
    if (*str == 'e' || *str == 'E') {
        str++;
        int exp_sign = 1;
        if (*str == '+') {
            str++;
        } else if (*str == '-') {
            exp_sign = -1;
            str++;
        }

        while (xy_isdigit(*str)) {
            exponent = exponent * 10 + (*str - '0');
            str++;
        }

        exponent *= exp_sign;
    }

    if (endptr != NULL) {
        *endptr = (char *)(has_digits ? str : start);
    }

    if (!has_digits) {
        return 0.0;
    }

    result *= sign;

    if (exponent != 0) {
        result *= pow(10.0, exponent);
    }

    return result;
}

long xy_strtol_old(const char *str, char **endptr, int base_param)
{
    long result       = 0;
    int sign          = 1;
    int base          = base_param ? base_param : 10;
    const char *start = str;
    int has_digits    = 0;

    // Skip leading whitespace
    while (xy_isspace(*str)) {
        str++;
    }

    // Handle sign
    if (*str == '+') {
        str++;
    } else if (*str == '-') {
        sign = -1;
        str++;
    }

    // Detect base if not specified
    if (base == 0 && *str == '0') {
        has_digits = 1;
        str++;
        if (*str == 'x' || *str == 'X') {
            base = 16;
            str++;
        } else {
            base = 8;
        }
    } else if (base == 0) {
        base = 10;
    }

    // Convert digits
    while (1) {
        int digit;
        if (xy_isdigit(*str)) {
            digit = *str - '0';
        } else if (base == 16 && isxdigit(*str)) {
            digit = tolower(*str) - 'a' + 10;
        } else {
            break;
        }

        if (digit >= base) {
            break;
        }

        // Check for overflow
        if (result > LONG_MAX / base
            || (result == LONG_MAX / base && digit > LONG_MAX % base)) {
            if (endptr)
                *endptr = (char *)str;
            return sign == 1 ? LONG_MAX : LONG_MIN;
        }

        result     = result * base + digit;
        has_digits = 1;
        str++;
    }

    if (endptr != NULL) {
        *endptr = (char *)(has_digits ? str : start);
    }

    return sign * result;
}

void xy_qsort(void *base, size_t num, size_t size,
              int (*compar)(const void *, const void *))
{
    if (num <= 1)
        return;

    char *base_ptr = (char *)base;
    size_t pivot   = num / 2;
    size_t left    = 0;
    size_t right   = num - 1;

    // Swap pivot with last element
    for (size_t i = 0; i < size; i++) {
        char temp                  = base_ptr[pivot * size + i];
        base_ptr[pivot * size + i] = base_ptr[right * size + i];
        base_ptr[right * size + i] = temp;
    }

    // Partition
    for (size_t i = 0; i < num - 1; i++) {
        if (compar(base_ptr + i * size, base_ptr + right * size) < 0) {
            // Swap elements at i and left
            for (size_t j = 0; j < size; j++) {
                char temp                 = base_ptr[i * size + j];
                base_ptr[i * size + j]    = base_ptr[left * size + j];
                base_ptr[left * size + j] = temp;
            }
            left++;
        }
    }

    // Swap pivot back
    for (size_t i = 0; i < size; i++) {
        char temp                  = base_ptr[left * size + i];
        base_ptr[left * size + i]  = base_ptr[right * size + i];
        base_ptr[right * size + i] = temp;
    }

    // Recursively sort left and right parts
    if (left > 0) {
        xy_qsort(base, left, size, compar);
    }
    if (left + 1 < num) {
        xy_qsort(base_ptr + (left + 1) * size, num - left - 1, size, compar);
    }
}

void *xy_bsearch(const void *key, const void *base, size_t num, size_t size,
                 int (*compar)(const void *, const void *))
{
    size_t low  = 0;
    size_t high = num;

    while (low < high) {
        size_t mid          = low + (high - low) / 2;
        const void *mid_ptr = (const char *)base + mid * size;
        int cmp             = compar(key, mid_ptr);

        if (cmp == 0) {
            return (void *)mid_ptr;
        } else if (cmp < 0) {
            high = mid;
        } else {
            low = mid + 1;
        }
    }

    return NULL;
}

int xy_abs(int x)
{
    return x < 0 ? -x : x;
}

/* ========================================================================
 * Additional stdlib functions
 * ======================================================================== */

/**
 * @brief Absolute value of long integer
 */
long xy_labs(long x)
{
    return x < 0 ? -x : x;
}

/**
 * @brief Absolute value of long long integer
 */
long long xy_llabs(long long x)
{
    return x < 0 ? -x : x;
}

/**
 * @brief Integer division
 */
xy_div_t xy_div(int numer, int denom)
{
    xy_div_t result;
    result.quot = numer / denom;
    result.rem = numer % denom;
    return result;
}

/**
 * @brief Long integer division
 */
xy_ldiv_t xy_ldiv(long numer, long denom)
{
    xy_ldiv_t result;
    result.quot = numer / denom;
    result.rem = numer % denom;
    return result;
}

/**
 * @brief Long long integer division
 */
xy_lldiv_t xy_lldiv(long long numer, long long denom)
{
    xy_lldiv_t result;
    result.quot = numer / denom;
    result.rem = numer % denom;
    return result;
}

/**
 * @brief Convert long to string
 */
long long xy_atoll(const char *str)
{
    long long result = 0;
    int sign = 1;

    while (xy_isspace(*str)) {
        str++;
    }

    if (*str == '+') {
        str++;
    } else if (*str == '-') {
        sign = -1;
        str++;
    }

    while (xy_isdigit(*str)) {
        result = result * 10 + (*str - '0');
        str++;
    }

    return sign * result;
}

/**
 * @brief Convert string to float
 */
float xy_strtof(const char *str, char **endptr)
{
    return (float)xy_strtod(str, endptr);
}

/**
 * @brief Convert string to unsigned long
 */
unsigned long xy_strtoul(const char *str, char **endptr, int base)
{
    unsigned long result = 0;
    const char *start = str;
    int digit;

    /* Skip whitespace */
    while (xy_isspace(*str)) {
        str++;
    }

    /* Handle base prefix */
    if (base == 0) {
        if (*str == '0') {
            if (*(str + 1) == 'x' || *(str + 1) == 'X') {
                base = 16;
                str += 2;
            } else {
                base = 8;
                str++;
            }
        } else {
            base = 10;
        }
    } else if (base == 16) {
        if (*str == '0' && (*(str + 1) == 'x' || *(str + 1) == 'X')) {
            str += 2;
        }
    }

    /* Convert digits */
    while (*str) {
        if (xy_isdigit(*str)) {
            digit = *str - '0';
        } else if (xy_isxdigit(*str)) {
            digit = xy_toupper(*str) - 'A' + 10;
        } else {
            break;
        }

        if (digit >= base) {
            break;
        }

        result = result * base + digit;
        str++;
    }

    if (endptr) {
        *endptr = (char *)(str > start ? str : start);
    }

    return result;
}

/**
 * @brief Convert string to long long
 */
long long xy_strtoll(const char *str, char **endptr, int base)
{
    long long result = 0;
    int sign = 1;
    const char *start = str;
    int digit;

    /* Skip whitespace */
    while (xy_isspace(*str)) {
        str++;
    }

    /* Handle sign */
    if (*str == '+') {
        str++;
    } else if (*str == '-') {
        sign = -1;
        str++;
    }

    /* Handle base prefix */
    if (base == 0) {
        if (*str == '0') {
            if (*(str + 1) == 'x' || *(str + 1) == 'X') {
                base = 16;
                str += 2;
            } else {
                base = 8;
                str++;
            }
        } else {
            base = 10;
        }
    } else if (base == 16) {
        if (*str == '0' && (*(str + 1) == 'x' || *(str + 1) == 'X')) {
            str += 2;
        }
    }

    /* Convert digits */
    while (*str) {
        if (xy_isdigit(*str)) {
            digit = *str - '0';
        } else if (xy_isxdigit(*str)) {
            digit = xy_toupper(*str) - 'A' + 10;
        } else {
            break;
        }

        if (digit >= base) {
            break;
        }

        result = result * base + digit;
        str++;
    }

    if (endptr) {
        *endptr = (char *)(str > start ? str : start);
    }

    return sign * result;
}

/**
 * @brief Convert string to unsigned long long
 */
unsigned long long xy_strtoull(const char *str, char **endptr, int base)
{
    unsigned long long result = 0;
    const char *start = str;
    int digit;

    /* Skip whitespace */
    while (xy_isspace(*str)) {
        str++;
    }

    /* Handle base prefix */
    if (base == 0) {
        if (*str == '0') {
            if (*(str + 1) == 'x' || *(str + 1) == 'X') {
                base = 16;
                str += 2;
            } else {
                base = 8;
                str++;
            }
        } else {
            base = 10;
        }
    } else if (base == 16) {
        if (*str == '0' && (*(str + 1) == 'x' || *(str + 1) == 'X')) {
            str += 2;
        }
    }

    /* Convert digits */
    while (*str) {
        if (xy_isdigit(*str)) {
            digit = *str - '0';
        } else if (xy_isxdigit(*str)) {
            digit = xy_toupper(*str) - 'A' + 10;
        } else {
            break;
        }

        if (digit >= base) {
            break;
        }

        result = result * base + digit;
        str++;
    }

    if (endptr) {
        *endptr = (char *)(str > start ? str : start);
    }

    return result;
}

/**
 * @brief Convert integer to string
 */
char *xy_itoa(int value, char *str, int base)
{
    char *ptr = str;
    char *ptr1 = str;
    char tmp_char;
    int tmp_value;

    if (base < 2 || base > 36) {
        *str = '\0';
        return str;
    }

    /* Handle negative numbers for base 10 */
    if (value < 0 && base == 10) {
        *ptr++ = '-';
        ptr1++;
        value = -value;
    }

    /* Convert to string (reversed) */
    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "0123456789abcdefghijklmnopqrstuvwxyz"[tmp_value - value * base];
    } while (value);

    *ptr-- = '\0';

    /* Reverse the string */
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }

    return str;
}

/**
 * @brief Convert long to string
 */
char *xy_ltoa(long value, char *str, int base)
{
    char *ptr = str;
    char *ptr1 = str;
    char tmp_char;
    long tmp_value;

    if (base < 2 || base > 36) {
        *str = '\0';
        return str;
    }

    /* Handle negative numbers for base 10 */
    if (value < 0 && base == 10) {
        *ptr++ = '-';
        ptr1++;
        value = -value;
    }

    /* Convert to string (reversed) */
    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "0123456789abcdefghijklmnopqrstuvwxyz"[tmp_value - value * base];
    } while (value);

    *ptr-- = '\0';

    /* Reverse the string */
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }

    return str;
}

/**
 * @brief Convert unsigned int to string
 */
char *xy_utoa(unsigned int value, char *str, int base)
{
    char *ptr = str;
    char *ptr1 = str;
    char tmp_char;
    unsigned int tmp_value;

    if (base < 2 || base > 36) {
        *str = '\0';
        return str;
    }

    /* Convert to string (reversed) */
    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "0123456789abcdefghijklmnopqrstuvwxyz"[tmp_value - value * base];
    } while (value);

    *ptr-- = '\0';

    /* Reverse the string */
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }

    return str;
}

/**
 * @brief Convert unsigned long to string
 */
char *xy_ultoa(unsigned long value, char *str, int base)
{
    char *ptr = str;
    char *ptr1 = str;
    char tmp_char;
    unsigned long tmp_value;

    if (base < 2 || base > 36) {
        *str = '\0';
        return str;
    }

    /* Convert to string (reversed) */
    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "0123456789abcdefghijklmnopqrstuvwxyz"[tmp_value - value * base];
    } while (value);

    *ptr-- = '\0';

    /* Reverse the string */
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }

    return str;
}

/* ========================================================================
 * Simple pseudo-random number generator
 * ======================================================================== */

static unsigned long xy_rand_seed = 1;

/**
 * @brief Generate pseudo-random number
 */
int xy_rand(void)
{
    xy_rand_seed = xy_rand_seed * 1103515245 + 12345;
    return (int)((xy_rand_seed / 65536) % (XY_RAND_MAX + 1));
}

/**
 * @brief Seed the random number generator
 */
void xy_srand(unsigned int seed)
{
    xy_rand_seed = seed;
}

/* ========================================================================
 * Memory Management Placeholder
 * Note: These functions require a memory allocator implementation
 * ======================================================================== */

/**
 * @brief Allocate memory (placeholder)
 * @note This is a placeholder. Actual implementation depends on the platform
 */
void *xy_malloc(size_t size)
{
    /* TODO: Implement memory allocator or map to RTOS heap */
    #if defined(USE_FREERTOS)
        /* extern void *pvPortMalloc(size_t size); */
        /* return pvPortMalloc(size); */
    #elif defined(USE_RT_THREAD)
        /* extern void *rt_malloc(size_t size); */
        /* return rt_malloc(size); */
    #else
        /* Fallback to standard malloc for now */
        extern void *malloc(size_t size);
        return malloc(size);
    #endif
    (void)size;
    return NULL;
}

/**
 * @brief Allocate and zero memory (placeholder)
 */
void *xy_calloc(size_t nmemb, size_t size)
{
    #if defined(USE_FREERTOS) || defined(USE_RT_THREAD)
        /* Implement using xy_malloc + memset */
        void *ptr = xy_malloc(nmemb * size);
        if (ptr) {
            extern void xy_memset(void *dst, uint8_t val, uint32_t len);
            xy_memset(ptr, 0, nmemb * size);
        }
        return ptr;
    #else
        /* Fallback to standard calloc */
        extern void *calloc(size_t nmemb, size_t size);
        return calloc(nmemb, size);
    #endif
    (void)nmemb;
    (void)size;
    return NULL;
}

/**
 * @brief Reallocate memory (placeholder)
 */
void *xy_realloc(void *ptr, size_t size)
{
    /* TODO: Implement or map to RTOS */
    #if defined(USE_FREERTOS) || defined(USE_RT_THREAD)
        /* Simple implementation: alloc new + copy + free old */
        if (size == 0) {
            xy_free(ptr);
            return NULL;
        }
        if (ptr == NULL) {
            return xy_malloc(size);
        }
        void *new_ptr = xy_malloc(size);
        if (new_ptr) {
            /* Note: We don't know the old size, so this is unsafe */
            /* A real implementation needs to track allocation sizes */
            extern void *xy_memcpy(void *dst, const void *src, uint32_t n);
            xy_memcpy(new_ptr, ptr, size);
            xy_free(ptr);
        }
        return new_ptr;
    #else
        extern void *realloc(void *ptr, size_t size);
        return realloc(ptr, size);
    #endif
    (void)ptr;
    (void)size;
    return NULL;
}

/**
 * @brief Free memory (placeholder)
 */
void xy_free(void *ptr)
{
    /* TODO: Implement or map to RTOS heap */
    #if defined(USE_FREERTOS)
        /* extern void vPortFree(void *ptr); */
        /* vPortFree(ptr); */
    #elif defined(USE_RT_THREAD)
        /* extern void rt_free(void *ptr); */
        /* rt_free(ptr); */
    #else
        extern void free(void *ptr);
        free(ptr);
    #endif
    (void)ptr;
}

/**
 * @brief Safely free allocated memory and set pointer to NULL
 */
void xy_safe_free(void **ptr)
{
    if (ptr && *ptr) {
        xy_free(*ptr);
        *ptr = NULL;
    }
}
