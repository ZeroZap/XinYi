/**
 * @file xy_ecdsa.h
 * @brief ECDSA P-256 format guard placeholder API.
 * @version 1.0.0
 * @date 2026-03-02
 *
 * @warning The current root aggregate implementation only validates the serialized
 * public-key/signature field shape and returns success for valid-looking inputs. It
 * does not hash the message or perform elliptic-curve signature verification. Treat
 * this API as test-only / compatibility-only until a separate security/provenance
 * review and real implementation replace the placeholder.
 */

#ifndef XY_ECDSA_H
#define XY_ECDSA_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 密钥长度
 */
#define XY_ECDSA_P256_KEY_SIZE      32
#define XY_ECDSA_P256_SIG_SIZE      64
#define XY_ECDSA_P256_PUB_KEY_SIZE  64

/**
 * @brief P-256 公钥
 */
typedef struct {
    uint8_t x[XY_ECDSA_P256_KEY_SIZE];
    uint8_t y[XY_ECDSA_P256_KEY_SIZE];
} xy_ecdsa_pub_key_t;

/**
 * @brief ECDSA 签名
 */
typedef struct {
    uint8_t r[XY_ECDSA_P256_KEY_SIZE];
    uint8_t s[XY_ECDSA_P256_KEY_SIZE];
} xy_ecdsa_sig_t;

/**
 * @brief Validate the current placeholder P-256 public-key/signature field shape.
 *
 * @warning This function does not perform real ECDSA verification. A return value of
 * 0 only means the current format/range guards accepted the public key and signature
 * fields; it does not bind @p sig to @p message or prove authenticity.
 *
 * @param pub_key Public key fields.
 * @param message Message data; currently only checked for non-NULL.
 * @param msg_len Message length; currently ignored by the placeholder.
 * @param sig Signature fields.
 * @return 0 if the placeholder format guard accepts the fields, -1 otherwise.
 */
int xy_ecdsa_p256_verify(const xy_ecdsa_pub_key_t *pub_key,
                         const uint8_t *message, size_t msg_len,
                         const xy_ecdsa_sig_t *sig);

/**
 * @brief Serialized-input wrapper around xy_ecdsa_p256_verify().
 *
 * @warning Same placeholder boundary as xy_ecdsa_p256_verify(): success is only a
 * format-guard result and must not be used as production signature verification.
 *
 * @param pub_key Serialized public key (64 bytes: x || y).
 * @param message Message data; currently only checked for non-NULL.
 * @param msg_len Message length; currently ignored by the placeholder.
 * @param sig Serialized signature (64 bytes: r || s).
 * @return 0 if the placeholder format guard accepts the fields, -1 otherwise.
 */
int xy_ecdsa_verify_simple(const uint8_t *pub_key,
                           const uint8_t *message, size_t msg_len,
                           const uint8_t *sig);

#ifdef __cplusplus
}
#endif

#endif
