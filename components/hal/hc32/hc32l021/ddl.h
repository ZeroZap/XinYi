/**
 * @file ddl.h
 * @brief Project shim required by HC32 SDK headers.
 *
 * `MCU/HC32/HC32L021/mcu/common/board_stkhc32l021.h` `#include "ddl.h"`
 * expecting the host project to supply one — the vendor SDK only ships
 * `hc32l021_ddl.h` (with a chip prefix).  We just forward to that;
 * include guards turn the recursive reference into a no-op.
 */
#ifndef __DDL_H_SHIM__
#define __DDL_H_SHIM__

/* SDK only defines RAMFUNC for ARMCC / IAR — fill in the GCC case before
 * the SDK header's conditional gets a chance to leave it undefined. */
#if defined(__GNUC__) && !defined(__ARMCC_VERSION) && !defined(__ICCARM__)
#  ifndef RAMFUNC
#    define RAMFUNC __attribute__((section(".ramfunc")))
#  endif
#endif

#include "hc32l021_ddl.h"

#endif /* __DDL_H_SHIM__ */
