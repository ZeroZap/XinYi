/**
 * @file xy_version.c
 * @brief XinYi Framework Version Implementation
 * @version 1.0.0
 * @date 2026-03-05
 */

#include "xy_version.h"

const char* xy_version_get_string(void)
{
    return XY_VERSION_STRING;
}

int xy_version_get_major(void)
{
    return XY_VERSION_MAJOR;
}

int xy_version_get_minor(void)
{
    return XY_VERSION_MINOR;
}

int xy_version_get_patch(void)
{
    return XY_VERSION_PATCH;
}

int xy_version_check_compatibility(int major, int minor, int patch)
{
    /*
     * 参数是要请求的版本，XY_VERSION_* 是这份二进制实际提供的版本。
     * 主版本不同一定不兼容；请求的次版本/补丁号高于本机提供的版本同样不兼容——
     * 那个方向才是危险的：回答“兼容”等于承诺一个并不存在的 API。
     * 负数不是“更旧的版本”，一律拒绝。
     */
    if (major < 0 || minor < 0 || patch < 0) {
        return 0;
    }

    /* 主版本号必须匹配 */
    if (major != XY_VERSION_MAJOR) {
        return 0;
    }

    /* 请求的次版本号高于本机提供的次版本号则无法满足 */
    if (minor > XY_VERSION_MINOR) {
        return 0;
    }

    /* 同次版本下，请求的补丁号高于本机补丁号同样无法满足 */
    if (minor == XY_VERSION_MINOR && patch > XY_VERSION_PATCH) {
        return 0;
    }

    return 1;
}
