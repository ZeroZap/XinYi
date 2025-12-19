/**
 * @file xy_stdio.h
 *
 * @brief A little stdio
 */
#ifndef _XY_STDIO_H
#define _XY_STDIO_H

#include <stdarg.h>
#include "xy_common.h"

#ifndef XY_PRINTF_BUFSIZE
#define XY_PRINTF_BUFSIZE 1024 // Global printf buffer size

#endif

typedef void (*xy_print_char_t)(char *str);
typedef char *(*xy_get_input_t)(char *buf, uint32_t size);

void xy_stdio_printf_init(xy_print_char_t print_char);
void xy_stdio_scanf_init(xy_get_input_t get_input);

/**
 * @brief Converts strings to 32-bit unsigned integers.
 *        转换数字字符串转为具体的32bit整型数值
 *
 * @param str
 * @param endptr
 * @param base
 */
uint32_t xy_stdio_stroul(const char *str, char **endptr, int base);


int32_t xy_stdio_vsprintf(char *buf, const char *fmt, va_list args);

int32_t xy_stdio_snprintf(char *buf, uint32_t size, const char *fmt, ...);

int32_t xy_stdio_sprintf(char *buf, const char *fmt, ...);

/** 传入缓存了. */
int32_t xy_stdio_vsnprintf(char *buf, uint32_t n, const char *fmt,
                           va_list args);

int32_t xy_stdio_printf(const char *fmt, ...);

int32_t xy_stdio_vprintf(const char *fmt, va_list args);

int32_t xy_stdio_scanf(const char *fmt, ...);
int32_t xy_stdio_sscanf(const char *str, const char *fmt, ...);
int32_t xy_stdio_vscanf(const char *__restrict, va_list);
int32_t xy_stdio_vsscanf(const char *__restrict, const char *__restrict,
                         va_list);


#endif // _XY_STDIO_H
