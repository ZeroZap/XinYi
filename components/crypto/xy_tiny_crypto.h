/**
 * @file xy_tiny_crypto.h
 * @brief Forwarding header - the crypto public API lives in `inc/xy_tiny_crypto.h`
 *
 * This file used to be a second, smaller copy of the API, under the same
 * `XY_TINY_CRYPTO_H` include guard as `inc/xy_tiny_crypto.h`. This copy declared 27 entry
 * points; `inc/` declares 70 - the whole SHA-1, HMAC-SHA1, CSPRNG, X25519/Ed25519, RSA and
 * bigint surface existed only in `inc/`, so a caller whose include path resolved this copy
 * first could not see it. Two headers, one guard: the second one read was silently dropped,
 * and which one that was depended on `-I` order (`-Icomponents/crypto` before
 * `-Icomponents/crypto/inc` produced `xy_sha1_hash: undeclared`, the other order compiled).
 *
 * Keep `#include "xy_tiny_crypto.h"` working (the root build and `example.c` use that
 * spelling, and `components/crypto` is the include root for the module trees below) by
 * forwarding to the canonical header. Do not declare anything here: a declaration added to
 * this file is invisible to every consumer that includes `inc/xy_tiny_crypto.h`. Guarded by
 * `include_guard_collisions` in `tests/unit/support/check_include_guard_collisions.py`.
 */

#ifndef XY_TINY_CRYPTO_ROOT_FORWARD_H
#define XY_TINY_CRYPTO_ROOT_FORWARD_H

#include "inc/xy_tiny_crypto.h"

/* The Chinese national-standard algorithms are separate module headers that only this
 * aggregate ever pulled in; `inc/xy_tiny_crypto.h` does not include them, so keeping them
 * here preserves the API a caller of "xy_tiny_crypto.h" already had. */
#include "xy_sm3/xy_sm3.h"
#include "xy_sm4/xy_sm4.h"
#include "xy_sm2/xy_sm2.h"

#endif /* XY_TINY_CRYPTO_ROOT_FORWARD_H */
