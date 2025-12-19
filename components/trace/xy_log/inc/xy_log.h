#include "xy_stdio.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef xy_log
#define xy_log(fmt, ...) xy_stdio_printf(fmt, ##__VA_ARGS__)
#endif

#define XY_DEBUG_ENABLE
#define XY_LOG_LEVEL_NEVER   0
#define XY_LOG_LEVEL_ERROR   1
#define XY_LOG_LEVEL_WARN    2
#define XY_LOG_LEVEL_INFO    3
#define XY_LOG_LEVEL_DEBUG   4
#define XY_LOG_LEVEL_VERBOSE 5

#ifndef LOCAL_LOG_LEVEL
#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG
#endif


#ifdef XY_DEBUG_ENABLE
#if 0
#define XY_MAKE_LOG_TAG(a, b, c, d)                               \
    ((unsigned)(a) | ((unsigned)(b) << 7) | ((unsigned)(c) << 14) \
     | ((unsigned)(d) << 21))
#else
#define XY_MAKE_LOG_TAG(a, b, c, d) (#a #b #c #d)
// #define XY_TAG              XY_MAKE_LOG_TAG('X', 'Y', ' ', ' ') // 'X', 'Y',
// ' ', ' '
#endif

// 如果调用者没有定义 XY_TAG，则使用默认值
#ifndef XY_TAG
#define XY_TAG "XY"
#endif

#ifndef LOG_TAG
#define LOG_TAG " "
#endif

#define xy_assert(EXPR)                                                 \
    if (!(EXPR)) {                                                      \
        xy_log("(%s) has assert failed at %s.\n", #EXPR, __FUNCTION__); \
        while (1)                                                       \
            ;                                                           \
    }


#if (LOCAL_LOG_LEVEL < XY_LOG_LEVEL_ERROR)
#define xy_log_e(...)
#else
#define xy_log_e(fmt, ...)                                              \
    xy_log("[%s:E]:F:%s() L:%d]: " fmt, XY_TAG, __FUNCTION__, __LINE__, \
           ##__VA_ARGS__)
#endif


#if (LOCAL_LOG_LEVEL < XY_LOG_LEVEL_WARN)
#define xy_log_w(...)
#else
#define xy_log_w(fmt, ...)                                              \
    xy_log("[%s:W]:F:%s() L:%d]: " fmt, XY_TAG, __FUNCTION__, __LINE__, \
           ##__VA_ARGS__)
#endif


#if (LOCAL_LOG_LEVEL < XY_LOG_LEVEL_INFO)
#define xy_log_i(...)
#else
#define xy_log_i(fmt, ...)                                              \
    xy_log("[%s:I]:F:%s() L:%d]: " fmt, XY_TAG, __FUNCTION__, __LINE__, \
           ##__VA_ARGS__)
#endif

#if (LOCAL_LOG_LEVEL < XY_LOG_LEVEL_DEBUG)
#define xy_log_d(...)
#else
#define xy_log_d(fmt, ...)                                              \
    xy_log("[%s:D]:F:%s() L:%d]: " fmt, XY_TAG, __FUNCTION__, __LINE__, \
           ##__VA_ARGS__)
#endif

#if (LOCAL_LOG_LEVEL < XY_LOG_LEVEL_VERBOSE)
#define xy_log_v(...)
#else
#define xy_log_v(fmt, ...)                                              \
    xy_log("[%s:V]:F:%s() L:%d]: " fmt, XY_TAG, __FUNCTION__, __LINE__, \
           ##__VA_ARGS__)
#endif

#else
#define xy_log_e(...)
#define xy_log_w(...)
#define xy_log_i(...)
#define xy_log_d(...)
#define xy_log_v(...)
#endif


void xy_log_str(char *str);
void xy_log_raw(char *data, size_t len);
void xy_log_init(void);
void xy_log_set_dynamic_level(uint8_t level);
uint8_t xy_log_dynamic_level(void);

#ifdef __cplusplus
}
#endif