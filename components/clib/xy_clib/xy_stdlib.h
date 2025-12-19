/**
 * @file xy_stdlib.h
 * @brief XinYi Standard Library - Unified header for embedded C library
 *
 * This header provides a unified interface to all xy_clib functions,
 * similar to standard C library headers. It includes:
 * - Memory management (malloc, free, etc.)
 * - String operations (string.h functions)
 * - Standard I/O (stdio.h functions)
 * - Type conversions (stdlib.h functions)
 * - Character classification (ctype.h functions)
 * - Math functions (math.h functions)
 * - Time structures (time.h types)
 * - Assertions (assert.h macros)
 *
 * Usage:
 *   #include <stdint.h>      // Standard types (allowed)
 *   #include "xy_stdlib.h"    // All xy_clib functions
 */

#ifndef XY_STDLIB_H
#define XY_STDLIB_H

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * Core Type Definitions (xy_typedef.h)
 * ======================================================================== */
#include "xy_typedef.h"

/* ========================================================================
 * Configuration
 * ======================================================================== */
#include "xy_config.h"

/* ========================================================================
 * Memory Management Functions (stdlib.h equivalent)
 * ======================================================================== */

/**
 * @defgroup memory_mgmt Memory Management
 * @brief Dynamic memory allocation functions
 * @{
 */

/**
 * @brief Allocate memory block
 * @param size Size of memory block in bytes
 * @return Pointer to allocated memory, or NULL on failure
 */
void *xy_malloc(size_t size);

/**
 * @brief Allocate and zero-initialize memory
 * @param nmemb Number of elements
 * @param size Size of each element
 * @return Pointer to allocated memory, or NULL on failure
 */
void *xy_calloc(size_t nmemb, size_t size);

/**
 * @brief Reallocate memory block
 * @param ptr Pointer to previously allocated memory
 * @param size New size in bytes
 * @return Pointer to reallocated memory, or NULL on failure
 */
void *xy_realloc(void *ptr, size_t size);

/**
 * @brief Free allocated memory
 * @param ptr Pointer to memory to free
 */
void xy_free(void *ptr);

/**
 * @brief Safely free allocated memory and set pointer to NULL
 * @param ptr Pointer to pointer to memory to free
 */
void xy_safe_free(void **ptr);

/** @} */ /* end of memory_mgmt */

/* Additional utility macros */
#define xy_safe_delete(ptr)            \
    do {                               \
        xy_safe_free((void **)&(ptr)); \
    } while (0)

/* ========================================================================
 * String Conversion Functions
 * ======================================================================== */

/**
 * @defgroup str_conversion String to Number Conversion
 * @{
 */

double xy_atof(const char *str);
int xy_atoi(const char *str);
long xy_atol(const char *str);
long long xy_atoll(const char *str);

double xy_strtod(const char *str, char **endptr);
float xy_strtof(const char *str, char **endptr);
long xy_strtol(const char *str, char **endptr, int base);
unsigned long xy_strtoul(const char *str, char **endptr, int base);
long long xy_strtoll(const char *str, char **endptr, int base);
unsigned long long xy_strtoull(const char *str, char **endptr, int base);

/** @} */ /* end of str_conversion */

/* ========================================================================
 * Number to String Conversion
 * ======================================================================== */

/**
 * @defgroup num_conversion Number to String Conversion
 * @{
 */

char *xy_itoa(int value, char *str, int base);
char *xy_ltoa(long value, char *str, int base);
char *xy_utoa(unsigned int value, char *str, int base);
char *xy_ultoa(unsigned long value, char *str, int base);

/** @} */ /* end of num_conversion */

/* ========================================================================
 * Sorting and Searching
 * ======================================================================== */

/**
 * @defgroup sort_search Sorting and Searching
 * @{
 */

void xy_qsort(void *base, size_t num, size_t size,
              int (*compar)(const void *, const void *));
void *xy_bsearch(const void *key, const void *base, size_t num, size_t size,
                 int (*compar)(const void *, const void *));

/** @} */ /* end of sort_search */

/* ========================================================================
 * Math Functions
 * ======================================================================== */

/**
 * @defgroup math_ops Math Operations
 * @{
 */

int xy_abs(int x);
long xy_labs(long x);
long long xy_llabs(long long x);

typedef struct {
    int quot;
    int rem;
} xy_div_t;
typedef struct {
    long quot;
    long rem;
} xy_ldiv_t;
typedef struct {
    long long quot;
    long long rem;
} xy_lldiv_t;

xy_div_t xy_div(int numer, int denom);
xy_ldiv_t xy_ldiv(long numer, long denom);
xy_lldiv_t xy_lldiv(long long numer, long long denom);

/** @} */ /* end of math_ops */

/* ========================================================================
 * Random Number Generation
 * ======================================================================== */

/**
 * @defgroup random Random Numbers
 * @{
 */

#define XY_RAND_MAX 32767

int xy_rand(void);
void xy_srand(unsigned int seed);

/** @} */ /* end of random */

/* ========================================================================
 * Include all xy_clib sub-headers
 * ======================================================================== */

/* String operations (string.h equivalent) */
#include "xy_string.h"

/* Standard I/O (stdio.h equivalent) */
#include "xy_stdio.h"

/* Character classification (ctype.h equivalent) */
#include "xy_ctype.h"

/* Time structures (time.h equivalent) */
#include "xy_time.h"

/* Math functions (math.h equivalent) */
#include "xy_math.h"

/* Assertions (assert.h equivalent) */
#include "xy_assert.h"

/* Common utilities */
#include "xy_common.h"

/* Error codes */
#include "xy_error.h"

/* Helper macros */
#include "xy_helper.h"

#ifdef __cplusplus
}
#endif

#endif /* XY_STDLIB_H */
