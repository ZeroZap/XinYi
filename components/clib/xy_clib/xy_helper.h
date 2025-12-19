#ifndef XY_HELPER_H
#define XY_HELPER_H

#include <stddef.h>

/*
 * 辅助宏: 获取成员偏移与通过成员指针反推结构指针。
 * 说明:
 * 1. 使用标准 C 写法，避免 GNU 扩展 typeof/语句表达式，增强可移植性。
 * 2. 偏移宏命名修正为 xy_offsetof；返回类型使用 size_t。
 */
#define xy_offsetof(type, member) ((size_t)(&(((type *)0)->member)))

/*
 * xy_container_of: 根据成员指针获得包含该成员的结构体指针。
 * 用法: struct my { int a; char b; }; char *pb = &obj.b; struct my *pobj =
 * xy_container_of(pb, struct my, b);
 */
#if !defined(__GNUC__)
#define xy_container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - xy_offsetof(type, member)))
#else
#define xy_container_of(ptr, type, member)                    \
    ({                                                        \
        const typeof(((type *)0)->member) *__mptr = (ptr);    \
        (type *)((char *)__mptr - xy_offsetof(type, member)); \
    })
#endif

/** 连接2个字符 */
#define xy_cat(a, b) a##b

/** 将字符转换为字符串 */
#define xy_stringify(a) #a

/**
 * @def xy_make_log_tag(...)
 * @brief 生成日志标签，支持2-4个参数自动拼接
 * @param ... 2-4个标识符参数
 * @return 拼接后的字符串字面量
 *
 * 示例:
 *   xy_make_log_tag(App, Module)        -> "AppModule"
 *   xy_make_log_tag(App, Module, Debug)  -> "AppModuleDebug"
 *   xy_make_log_tag(A, B, C, D)          -> "ABCD"
 */
#define xy_make_log_tag(...) \
    xy_make_log_tag_selector(xy_count_args(__VA_ARGS__), __VA_ARGS__)

#define xy_make_log_tag_selector(N, ...) \
    xy_cat(xy_make_log_tag_, N)(__VA_ARGS__)

// command 差不多
#endif /* XY_HELPER_H */