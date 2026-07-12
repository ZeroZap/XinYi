/**
 * @file xy_log.h
 * @brief Logging System
 */

#ifndef XY_LOG_H
#define XY_LOG_H

#include <xy_stdio.h>
#include <stdint.h>
#include <stddef.h>

/* Log levels */
#define XY_LOG_LEVEL_ERROR 0
#define XY_LOG_LEVEL_WARN  1
#define XY_LOG_LEVEL_INFO  2
#define XY_LOG_LEVEL_DEBUG 3

#ifndef XY_LOG_LEVEL
#define XY_LOG_LEVEL XY_LOG_LEVEL_INFO
#endif

/* Log macros */
#if XY_LOG_LEVEL >= XY_LOG_LEVEL_ERROR
#define XY_LOG_E(fmt, ...) xy_printf("[E] " fmt, ##__VA_ARGS__)
#else
#define XY_LOG_E(fmt, ...)
#endif

#if XY_LOG_LEVEL >= XY_LOG_LEVEL_INFO
#define XY_LOG_I(fmt, ...) xy_printf("[I] " fmt, ##__VA_ARGS__)
#else
#define XY_LOG_I(fmt, ...)
#endif

#if XY_LOG_LEVEL >= XY_LOG_LEVEL_WARN
#define XY_LOG_W(fmt, ...) xy_printf("[W] " fmt, ##__VA_ARGS__)
#else
#define XY_LOG_W(fmt, ...)
#endif

#if XY_LOG_LEVEL >= XY_LOG_LEVEL_DEBUG
#define XY_LOG_D(fmt, ...) xy_printf("[D] " fmt, ##__VA_ARGS__)
#else
#define XY_LOG_D(fmt, ...)
#endif

/* Lowercase aliases */
#define xy_log_e(fmt, ...) XY_LOG_E(fmt, ##__VA_ARGS__)
#define xy_log_w(fmt, ...) XY_LOG_W(fmt, ##__VA_ARGS__)
#define xy_log_i(fmt, ...) XY_LOG_I(fmt, ##__VA_ARGS__)
#define xy_log_d(fmt, ...) XY_LOG_D(fmt, ##__VA_ARGS__)

/* Uppercase legacy aliases used by older component drivers */
#define XY_LOG_ERROR(fmt, ...) XY_LOG_E(fmt, ##__VA_ARGS__)
#define XY_LOG_WARN(fmt, ...)  XY_LOG_W(fmt, ##__VA_ARGS__)
#define XY_LOG_INFO(fmt, ...)  XY_LOG_I(fmt, ##__VA_ARGS__)
#define XY_LOG_DEBUG(fmt, ...) XY_LOG_D(fmt, ##__VA_ARGS__)

void xy_log_init(void);
void xy_log_str(char *str);
void xy_log_raw(char *data, size_t len);
void xy_log_set_dynamic_level(uint8_t level);
uint8_t xy_log_dynamic_level(void);

#endif /* XY_LOG_H */
