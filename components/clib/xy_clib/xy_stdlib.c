/**
 * @file xy_stdlib.c
 * @brief XinYi Standard Library Implementation
 * @version 2.0
 * @date 2026-02-28
 */

#include "xy_stdlib.h"
#include <string.h>
#include <math.h>

#ifndef XY_UNUSED
#define XY_UNUSED(x) (void)(x)
#endif

/* Memory Management */

void *xy_malloc(size_t size)
{
    /* In embedded systems, this might use a memory pool */
    /* For now, delegate to system malloc if available */
    XY_UNUSED(size);
    return NULL; /* Placeholder - implement with memory pool in real system */
}

void *xy_calloc(size_t nmemb, size_t size)
{
    size_t total_size = nmemb * size;
    void *ptr = xy_malloc(total_size);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

void *xy_realloc(void *ptr, size_t size)
{
    XY_UNUSED(ptr);
    XY_UNUSED(size);
    /* Placeholder - implement with memory pool in real system */
    return NULL;
}

void xy_free(void *ptr)
{
    XY_UNUSED(ptr);
    /* Placeholder - implement with memory pool in real system */
}

void xy_safe_free(void **ptr)
{
    if (ptr && *ptr) {
        xy_free(*ptr);
        *ptr = NULL;
    }
}

/* Number to String Conversion */

char *xy_itoa(int value, char *str, int base)
{
    if (!str || base < 2 || base > 36) return NULL;
    
    if (value == 0) {
        str[0] = '0';
        str[1] = '\0';
        return str;
    }
    
    int negative = 0;
    if (value < 0 && base == 10) {
        negative = 1;
        value = -value;
    }
    
    char *start = str;
    char *end = str;
    
    /* Generate digits in reverse order */
    while (value > 0) {
        int digit = value % base;
        *end++ = (digit < 10) ? '0' + digit : 'A' + digit - 10;
        value /= base;
    }
    
    if (negative) {
        *end++ = '-';
    }
    
    /* Reverse the string */
    *end = '\0';
    end--; /* Move back to last digit */
    
    while (start < end) {
        char temp = *start;
        *start = *end;
        *end = temp;
        start++;
        end--;
    }
    
    return str;
}

char *xy_ltoa(long value, char *str, int base)
{
    if (!str || base < 2 || base > 36) return NULL;
    
    if (value == 0) {
        str[0] = '0';
        str[1] = '\0';
        return str;
    }
    
    int negative = 0;
    if (value < 0 && base == 10) {
        negative = 1;
        value = -value;
    }
    
    char *start = str;
    char *end = str;
    
    /* Generate digits in reverse order */
    while (value > 0) {
        long digit = value % base;
        *end++ = (digit < 10) ? '0' + digit : 'A' + digit - 10;
        value /= base;
    }
    
    if (negative) {
        *end++ = '-';
    }
    
    /* Reverse the string */
    *end = '\0';
    end--; /* Move back to last digit */
    
    while (start < end) {
        char temp = *start;
        *start = *end;
        *end = temp;
        start++;
        end--;
    }
    
    return str;
}

char *xy_ultoa(unsigned long value, char *str, int base)
{
    if (!str || base < 2 || base > 36) return NULL;
    
    if (value == 0) {
        str[0] = '0';
        str[1] = '\0';
        return str;
    }
    
    char *start = str;
    char *end = str;
    
    /* Generate digits in reverse order */
    while (value > 0) {
        unsigned long digit = value % base;
        *end++ = (digit < 10) ? '0' + digit : 'A' + digit - 10;
        value /= base;
    }
    
    /* Reverse the string */
    *end = '\0';
    end--; /* Move back to last digit */
    
    while (start < end) {
        char temp = *start;
        *start = *end;
        *end = temp;
        start++;
        end--;
    }
    
    return str;
}

char *xy_utoa(unsigned int value, char *str, int base)
{
    if (!str || base < 2 || base > 36) return NULL;
    
    if (value == 0) {
        str[0] = '0';
        str[1] = '\0';
        return str;
    }
    
    char *start = str;
    char *end = str;
    
    /* Generate digits in reverse order */
    while (value > 0) {
        unsigned int digit = value % base;
        *end++ = (digit < 10) ? '0' + digit : 'A' + digit - 10;
        value /= base;
    }
    
    /* Reverse the string */
    *end = '\0';
    end--; /* Move back to last digit */
    
    while (start < end) {
        char temp = *start;
        *start = *end;
        *end = temp;
        start++;
        end--;
    }
    
    return str;
}

/* Sorting and Searching */

void xy_qsort(void *base, size_t num, size_t size,
              int (*compar)(const void *, const void *))
{
    if (!base || !compar || num <= 1) return;
    
    char *arr = (char *)base;
    for (size_t i = 0; i < num - 1; i++) {
        for (size_t j = 0; j < num - i - 1; j++) {
            char *a = arr + j * size;
            char *b = arr + (j + 1) * size;
            
            if (compar(a, b) > 0) {
                /* Swap elements */
                for (size_t k = 0; k < size; k++) {
                    char temp = a[k];
                    a[k] = b[k];
                    b[k] = temp;
                }
            }
        }
    }
}

void *xy_bsearch(const void *key, const void *base, size_t num, size_t size,
                 int (*compar)(const void *, const void *))
{
    if (!key || !base || !compar || num == 0) return NULL;
    
    const char *arr = (const char *)base;
    size_t left = 0, right = num - 1;
    
    while (left <= right) {
        size_t mid = left + (right - left) / 2;
        const char *mid_elem = arr + mid * size;
        int cmp = compar(key, mid_elem);
        
        if (cmp == 0) {
            return (void *)mid_elem;
        } else if (cmp < 0) {
            if (mid == 0) break; /* Prevent underflow */
            right = mid - 1;
        } else {
            left = mid + 1;
        }
    }
    
    return NULL;
}

/* Random Number Generation */

static uint32_t g_rand_state = 1;

void xy_srand(unsigned int seed)
{
    g_rand_state = seed ? seed : 1;
}

/* Environment Functions */

void xy_abort(void)
{
    while (1) {
        // Halt execution
    }
}

void xy_exit(int status)
{
    XY_UNUSED(status);
    xy_abort();
}

char *xy_getenv(const char *name)
{
    XY_UNUSED(name);
    // In embedded systems, environment variables may not be available
    return NULL;
}

int xy_system(const char *command)
{
    XY_UNUSED(command);
    // In embedded systems, system commands may not be available
    return -1;
}

/* String Conversion */

char *xy_itoa_append(int value, char *str)
{
    if (!str) return NULL;
    
    // Find end of string
    char *end = str;
    while (*end != '\0') end++;
    
    // Convert value to string
    char temp[32];
    xy_itoa(value, temp, 10);
    
    // Append to existing string
    int i = 0;
    while (temp[i] != '\0') {
        *end++ = temp[i++];
    }
    *end = '\0';
    
    return str;
}
