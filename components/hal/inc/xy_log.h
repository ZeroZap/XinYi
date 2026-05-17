/**
 * @file xy_log.h
 * @brief Logging Stub for HAL Compilation
 */

#ifndef XY_LOG_H
#define XY_LOG_H

/* Minimal logging macros (no-op stubs for HAL layer) */
#define XY_LOG_E(fmt, ...)    do { } while(0)
#define XY_LOG_W(fmt, ...)    do { } while(0)
#define XY_LOG_I(fmt, ...)    do { } while(0)
#define XY_LOG_D(fmt, ...)    do { } while(0)

/* Keep legacy names for compatibility */
#define XY_LOG_ERROR(fmt, ...)    XY_LOG_E(fmt, ##__VA_ARGS__)
#define XY_LOG_WARN(fmt, ...)     XY_LOG_W(fmt, ##__VA_ARGS__)
#define XY_LOG_INFO(fmt, ...)     XY_LOG_I(fmt, ##__VA_ARGS__)
#define XY_LOG_DEBUG(fmt, ...)    XY_LOG_D(fmt, ##__VA_ARGS__)

/* Lowercase aliases */
#define xy_log_e(fmt, ...)    XY_LOG_E(fmt, ##__VA_ARGS__)
#define xy_log_w(fmt, ...)    XY_LOG_W(fmt, ##__VA_ARGS__)
#define xy_log_i(fmt, ...)    XY_LOG_I(fmt, ##__VA_ARGS__)
#define xy_log_d(fmt, ...)    XY_LOG_D(fmt, ##__VA_ARGS__)

#endif /* XY_LOG_H */
