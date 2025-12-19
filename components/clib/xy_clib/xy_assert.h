#ifndef _XY_ASSERT_H_
#define _XY_ASSERT_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Simple assertion macro for embedded systems
 *
 * When assertion fails, this will trigger an infinite loop or halt.
 * Users can customize this behavior by defining XY_ASSERT_HANDLER.
 */
#ifndef XY_ASSERT_HANDLER
#define XY_ASSERT_HANDLER() \
    do {                    \
        while (1)           \
            ;               \
    } while (0)
#endif

#ifndef NDEBUG
#define xy_assert(expr)          \
    do {                         \
        if (!(expr)) {           \
            XY_ASSERT_HANDLER(); \
        }                        \
    } while (0)
#else
#define xy_assert(expr) ((void)0)
#endif

/* Legacy support */
#define assert(expr) xy_assert(expr)

#ifdef __cplusplus
}
#endif

#endif /* _XY_ASSERT_H_ */;