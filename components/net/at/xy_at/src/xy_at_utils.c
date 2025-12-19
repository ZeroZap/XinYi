/**
 * @file xy_at_utils.c
 * @brief XY AT Framework Utility Functions
 * @version 1.0.0
 */

#include "../xy_at.h"
#include <stdio.h>

/* ==================== Version Information ==================== */

const char *xy_at_get_version(void)
{
    return XY_AT_VERSION;
}

/* ==================== Framework Init/Deinit ==================== */

int xy_at_init(void)
{
    // Initialize framework (if needed)
    return 0;
}

int xy_at_deinit(void)
{
    // Cleanup framework (if needed)
    return 0;
}
