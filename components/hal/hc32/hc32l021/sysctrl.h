/**
 * @file sysctrl.h
 * @brief Project shim required by HC32 SDK system_hc32l021.c.
 *
 * The vendor file `#include "sysctrl.h"` expects the host project to
 * supply one; the SDK actually ships `hc32l021_sysctrl.h`.  Forward to it.
 */
#ifndef __SYSCTRL_H_SHIM__
#define __SYSCTRL_H_SHIM__
#include "hc32l021_sysctrl.h"
#endif
