/**
 * @file xy_at.h
 * @brief Unified XY AT Command Framework Header
 * @version 1.0.0
 * @date 2025-10-27
 *
 * @note This is the main header file that includes both client and server
 */

#ifndef _XY_AT_H_
#define _XY_AT_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Include configuration */
#include "xy_at_cfg.h"

/* Include client and server headers */
#ifdef XY_AT_USING_CLIENT
#include "xy_at_client.h"
#endif

#ifdef XY_AT_USING_SERVER
#include "xy_at_server.h"
#endif

/* Version information */
#define XY_AT_VERSION       "1.0.0"
#define XY_AT_VERSION_MAJOR 1
#define XY_AT_VERSION_MINOR 0
#define XY_AT_VERSION_PATCH 0

/**
 * @brief Get XY AT framework version
 * @return Version string
 */
const char *xy_at_get_version(void);

/**
 * @brief Initialize XY AT framework
 * @return 0 on success, -1 on error
 */
int xy_at_init(void);

/**
 * @brief Deinitialize XY AT framework
 * @return 0 on success, -1 on error
 */
int xy_at_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* _XY_AT_H_ */
