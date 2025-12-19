#ifndef XY_CODE_STYLE_H
#define XY_CODE_STYLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

/**
 * \brief           Get minimal value between `x` and `y`
 * \param[in]       x: First value
 * \param[in]       y: Second value
 * \return          Minimal value between `x` and `y`
 * \hideinitializer
 */
#define XY_MIN(x, y) ((x) < (y) ? (x) : (y))

/**
 * \brief           Get maximal value between `x` and `y`
 * \param[in]       x: First value
 * \param[in]       y: Second value
 * \return          Maximal value between `x` and `y`
 * \hideinitializer
 */
#define XY_MAX(x, y) ((x) > (y) ? (x) : (y))

/**
 * \brief           Set point coordinates
 * \param[in]       p: Pointer to point_t
 * \param[in]       x: X coordinate
 * \param[in]       y: Y coordinate
 * \hideinitializer
 */
#define XY_SET_POINT(p, x, y) \
    do {                      \
        (p)->px = (x);        \
        (p)->py = (y);        \
    } while (0)

/**
 * \brief Demo structure following code style
 */
typedef struct {
    int32_t a; /**< member a */
    int32_t b; /**< member b */
} demo_struct_t;

/**
 * \brief Demo enum following code style
 */
typedef enum {
    demo_enum_none = 0,
    demo_enum_one,
    demo_enum_two,
} demo_enum_t;

/**
 * \brief Demo function prototype
 * \param[in] val Value to set
 * \return int32_t Result
 */
int32_t demo_func(int32_t val);

/**
 * \brief Demo function returning pointer
 * \return Pointer to demo_struct_t
 */
demo_struct_t *demo_get_struct(void);

#ifdef __cplusplus
}
#endif

#endif /* XY_CODE_STYLE_H */
