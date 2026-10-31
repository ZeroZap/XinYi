/**
 * @file xy_crc.h
 * @brief Compatibility forward to the canonical CRC header.
 *
 * This file used to be a second, independent `xy_crc.h`: three declarations (`xy_crc32`,
 * `xy_crc16`, `xy_crc8`) taking `uint32_t length`, none of which had an implementation anywhere in
 * the repository. The real CRC component lives in `components/crypto/xy_crc/xy_crc.h` and answers
 * to `xy_crc_calc()` plus the `<width>_<standard>` variants (`xy_crc16_ccitt_false()`,
 * `xy_crc16_modbus()`, ...).
 *
 * Keeping both was not mere redundancy, it was a compile error waiting for an include order: the
 * stale `xy_crc32(const uint8_t *, uint32_t)` conflicted with the
 * `xy_crc32(const uint8_t *, size_t)` that `xy_tiny_crypto.h` declares, so a caller that included
 * both failed to build on a 64-bit host while a 32-bit target (where `size_t` is `uint32_t`)
 * accepted it silently. Which header `#include "xy_crc.h"` resolved to was decided by include-path
 * order, so the same source could compile or not depending on the platform.
 *
 * Forwarding keeps `#include "xy_crc.h"` working from either directory and hands every reader the
 * API that actually has an implementation.
 */

#ifndef XY_CRC_COMPAT_H
#define XY_CRC_COMPAT_H

#include "../xy_crc/xy_crc.h"

#endif /* XY_CRC_COMPAT_H */
