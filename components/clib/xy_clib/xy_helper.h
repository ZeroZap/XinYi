/*
 * 辅助宏: 获取成员偏移与通过成员指针反推结构指针。
 * 说明:
 * 1. 使用标准 C 写法，避免 GNU 扩展 typeof/语句表达式，增强可移植性。
 * 2. 偏移宏命名修正为 xy_offsetof；返回类型使用 size_t。
 */
#include <stddef.h>

#ifndef xy_offsetof
#define xy_offsetof(type, member) ((size_t)(&(((type *)0)->member)))
#endif

/*
 * xy_container_of: 根据成员指针获得包含该成员的结构体指针。
 * 用法: struct my { int a; char b; }; char *pb = &obj.b; struct my *pobj =
 * xy_container_of(pb, struct my, b);
 */
#ifndef xy_container_of
#define xy_container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - xy_offsetof(type, member)))
#endif
