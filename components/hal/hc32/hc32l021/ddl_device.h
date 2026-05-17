/**
 * @file ddl_device.h
 * @brief Per-project device selector required by HC32 SDK.
 *
 * The SDK's `hc32l021_ddl.h` `#include "ddl_device.h"` and uses these
 * macros to pick chip series / package.  Examples in the SDK provide
 * the same file per-app; we provide a single project-wide version.
 */
#ifndef __DDL_DEVICE_H__
#define __DDL_DEVICE_H__

#define DDL_MCU_SERIES  DDL_DEVICE_SERIES_HC32L021
#define DDL_MCU_PACKAGE DDL_DEVICE_PACKAGE

#endif /* __DDL_DEVICE_H__ */
