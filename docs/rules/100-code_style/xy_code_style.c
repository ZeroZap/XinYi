/**
 * \file            xy_code_style.c
 * \brief           Code style demonstration source (示例源码)
 */

#include "xy_code_style.h"
#include <stdlib.h>

/**
 * \brief Demo function implementation
 * \param[in] val Value to set
 * \return int32_t Result
 */
int32_t demo_func(int32_t val) {
    int32_t result = 0;

    if (val > 0) {
        result = val * 2;
    } else {
        result = -val;
    }
    return result;
}

/**
 * \brief Demo function returning pointer to struct
 * \return Pointer to demo_struct_t
 */
demo_struct_t* demo_get_struct(void) {
    demo_struct_t* s = malloc(sizeof(*s));
    if (s != NULL) {
        s->a = 1;
        s->b = 2;
    }
    return s;
}

void demo_point_init(xy_point_t* point, int32_t x, int32_t y) {
    if (point == NULL) {
        return;
    }
    XY_SET_POINT(point, x, y);
}

int32_t demo_get_min(int32_t lhs, int32_t rhs) {
    return XY_MIN(lhs, rhs);
}

int32_t demo_clamp_value(int32_t value, int32_t minimum, int32_t maximum) {
    return XY_CLAMP(value, minimum, maximum);
}

size_t demo_count_values(void) {
    static const int32_t values[] = {1, 3, 5, 7, 9};
    return XY_ARRAY_SIZE(values);
}

uint32_t demo_build_mask(uint32_t bit_position) {
    return XY_BIT(bit_position);
}

void demo_mark_unused(int32_t parameter) {
    XY_UNUSED(parameter);
}
