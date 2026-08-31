/**
 * @file xy_tinyjambu.h
 * @brief TinyJambu Lightweight Authenticated Encryption
 * @version 1.0.0
 * @date 2026-04-07
 *
 * TinyJambu is a finalist in the NIST Lightweight Cryptography competition.
 * It is designed for extremely constrained environments with minimal
 * code size and memory requirements.
 *
 * Reference: https://csrc.nist.gov/Projects/lightweight-cryptography
 *
 * Features:
 * - TinyJambu-128: 128-bit key, 64-bit tag
 * - TinyJambu-192: 192-bit key, 64-bit tag
 * - TinyJambu-256: 256-bit key, 64-bit tag
 * - Optional 128-bit authentication tag
 */

#ifndef XY_TINYJAMBU_H
#define XY_TINYJAMBU_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Error Codes ==================== */

#define XY_TINYJAMBU_SUCCESS         0
#define XY_TINYJAMBU_ERROR          (-1)
#define XY_TINYJAMBU_INVALID_PARAM  (-2)
#define XY_TINYJAMBU_AUTH_FAILED    (-4)

/* ==================== Constants ==================== */

/* TinyJambu-128 parameters */
#define XY_TINYJAMBU_128_KEY_SIZE    16    /* 128 bits */
#define XY_TINYJAMBU_128_NONCE_SIZE  16    /* 128 bits */
#define XY_TINYJAMBU_128_TAG_SIZE    8     /* 64 bits (default) */
#define XY_TINYJAMBU_128_TAG_128     16    /* 128 bits (optional) */

/* TinyJambu-192 parameters */
#define XY_TINYJAMBU_192_KEY_SIZE    24    /* 192 bits */
#define XY_TINYJAMBU_192_NONCE_SIZE  16    /* 128 bits */
#define XY_TINYJAMBU_192_TAG_SIZE    8     /* 64 bits */

/* TinyJambu-256 parameters */
#define XY_TINYJAMBU_256_KEY_SIZE    32    /* 256 bits */
#define XY_TINYJAMBU_256_NONCE_SIZE  16    /* 128 bits */
#define XY_TINYJAMBU_256_TAG_SIZE    8     /* 64 bits */

/* Internal constants */
#define XY_TINYJAMBU_STATE_SIZE      24    /* 192 bits = 3 x 64-bit words */
#define XY_TINYJAMBU_RATE_SIZE       16    /* 128-bit rate */

/* ==================== TinyJambu-128 AEAD ==================== */

/**
 * @brief TinyJambu-128 authenticated encryption
 *
 * @param key 128-bit key
 * @param nonce 128-bit nonce (public)
 * @param ad Associated data (can be NULL)
 * @param ad_len Length of associated data
 * @param plaintext Input data
 * @param plaintext_len Length of plaintext
 * @param ciphertext Output buffer
 * @param tag Output tag (8 bytes for 64-bit tag)
 * @return XY_TINYJAMBU_SUCCESS on success
 */
int xy_tinyjambu_128_encrypt(const uint8_t *key,
                              const uint8_t *nonce,
                              const uint8_t *ad, size_t ad_len,
                              const uint8_t *plaintext, size_t plaintext_len,
                              uint8_t *ciphertext,
                              uint8_t tag[XY_TINYJAMBU_128_TAG_SIZE]);

/**
 * @brief TinyJambu-128 authenticated decryption
 *
 * @param key 128-bit key
 * @param nonce 128-bit nonce
 * @param ad Associated data
 * @param ad_len Length of associated data
 * @param ciphertext Input encrypted data
 * @param ciphertext_len Length of ciphertext
 * @param plaintext Output buffer
 * @param tag Authentication tag to verify
 * @return XY_TINYJAMBU_SUCCESS if valid
 */
int xy_tinyjambu_128_decrypt(const uint8_t *key,
                              const uint8_t *nonce,
                              const uint8_t *ad, size_t ad_len,
                              const uint8_t *ciphertext, size_t ciphertext_len,
                              uint8_t *plaintext,
                              const uint8_t tag[XY_TINYJAMBU_128_TAG_SIZE]);

/**
 * @brief TinyJambu-128 with 128-bit tag
 */
int xy_tinyjambu_128_encrypt_tag128(const uint8_t *key,
                                     const uint8_t *nonce,
                                     const uint8_t *ad, size_t ad_len,
                                     const uint8_t *plaintext, size_t plaintext_len,
                                     uint8_t *ciphertext,
                                     uint8_t tag[XY_TINYJAMBU_128_TAG_128]);

int xy_tinyjambu_128_decrypt_tag128(const uint8_t *key,
                                     const uint8_t *nonce,
                                     const uint8_t *ad, size_t ad_len,
                                     const uint8_t *ciphertext, size_t ciphertext_len,
                                     uint8_t *plaintext,
                                     const uint8_t tag[XY_TINYJAMBU_128_TAG_128]);

/* ==================== TinyJambu-192 AEAD ==================== */

/**
 * @brief TinyJambu-192 authenticated encryption
 *
 * @param key 192-bit key
 * @param nonce 128-bit nonce
 * @param ad Associated data
 * @param ad_len Length of associated data
 * @param plaintext Input data
 * @param plaintext_len Length of plaintext
 * @param ciphertext Output buffer
 * @param tag Output tag (8 bytes)
 * @return XY_TINYJAMBU_SUCCESS on success
 */
int xy_tinyjambu_192_encrypt(const uint8_t *key,
                              const uint8_t *nonce,
                              const uint8_t *ad, size_t ad_len,
                              const uint8_t *plaintext, size_t plaintext_len,
                              uint8_t *ciphertext,
                              uint8_t tag[XY_TINYJAMBU_192_TAG_SIZE]);

/**
 * @brief TinyJambu-192 authenticated decryption
 *
 * @param key 192-bit key
 * @param nonce 128-bit nonce
 * @param ad Associated data
 * @param ad_len Length of associated data
 * @param ciphertext Input encrypted data
 * @param ciphertext_len Length of ciphertext
 * @param plaintext Output buffer
 * @param tag Tag to verify
 * @return XY_TINYJAMBU_SUCCESS if valid
 */
int xy_tinyjambu_192_decrypt(const uint8_t *key,
                              const uint8_t *nonce,
                              const uint8_t *ad, size_t ad_len,
                              const uint8_t *ciphertext, size_t ciphertext_len,
                              uint8_t *plaintext,
                              const uint8_t tag[XY_TINYJAMBU_192_TAG_SIZE]);

/* ==================== TinyJambu-256 AEAD ==================== */

/**
 * @brief TinyJambu-256 authenticated encryption
 *
 * @param key 256-bit key
 * @param nonce 128-bit nonce
 * @param ad Associated data
 * @param ad_len Length of associated data
 * @param plaintext Input data
 * @param plaintext_len Length of plaintext
 * @param ciphertext Output buffer
 * @param tag Output tag (8 bytes)
 * @return XY_TINYJAMBU_SUCCESS on success
 */
int xy_tinyjambu_256_encrypt(const uint8_t *key,
                              const uint8_t *nonce,
                              const uint8_t *ad, size_t ad_len,
                              const uint8_t *plaintext, size_t plaintext_len,
                              uint8_t *ciphertext,
                              uint8_t tag[XY_TINYJAMBU_256_TAG_SIZE]);

/**
 * @brief TinyJambu-256 authenticated decryption
 *
 * @param key 256-bit key
 * @param nonce 128-bit nonce
 * @param ad Associated data
 * @param ad_len Length of associated data
 * @param ciphertext Input encrypted data
 * @param ciphertext_len Length of ciphertext
 * @param plaintext Output buffer
 * @param tag Tag to verify
 * @return XY_TINYJAMBU_SUCCESS if valid
 */
int xy_tinyjambu_256_decrypt(const uint8_t *key,
                              const uint8_t *nonce,
                              const uint8_t *ad, size_t ad_len,
                              const uint8_t *ciphertext, size_t ciphertext_len,
                              uint8_t *plaintext,
                              const uint8_t tag[XY_TINYJAMBU_256_TAG_SIZE]);

/* ==================== Incremental API ==================== */

/**
 * @brief TinyJambu-128 context for incremental operations
 */
typedef struct {
    uint64_t R[3];            /* 192-bit state */
    uint64_t K[2];            /* 128-bit key schedule */
    uint32_t rounds;          /* Number of rounds (535 for 128-bit key) */
    size_t ad_len;            /* Total AD length */
    size_t plaintext_len;    /* Total plaintext length */
    int mode;                /* 0=init, 1=AD, 2=data, 3=done */
} xy_tinyjambu_128_ctx_t;

/**
 * @brief Initialize TinyJambu-128 encryption
 *
 * @param ctx Context
 * @param key 128-bit key
 * @param nonce 128-bit nonce
 * @return XY_TINYJAMBU_SUCCESS
 */
int xy_tinyjambu_128_encrypt_init(xy_tinyjambu_128_ctx_t *ctx,
                                   const uint8_t *key,
                                   const uint8_t *nonce);

/**
 * @brief Process associated data
 *
 * @param ctx Context
 * @param ad Associated data
 * @param ad_len Length of AD
 * @return XY_TINYJAMBU_SUCCESS
 */
int xy_tinyjambu_128_encrypt_ad(xy_tinyjambu_128_ctx_t *ctx,
                                  const uint8_t *ad, size_t ad_len);

/**
 * @brief Encrypt plaintext (can be called multiple times)
 *
 * @param ctx Context
 * @param plaintext Input
 * @param plaintext_len Length
 * @param ciphertext Output
 * @return XY_TINYJAMBU_SUCCESS
 */
int xy_tinyjambu_128_encrypt_update(xy_tinyjambu_128_ctx_t *ctx,
                                     const uint8_t *plaintext, size_t plaintext_len,
                                     uint8_t *ciphertext);

/**
 * @brief Finalize and output tag
 *
 * @param ctx Context
 * @param tag Output tag
 * @return XY_TINYJAMBU_SUCCESS
 */
int xy_tinyjambu_128_encrypt_final(xy_tinyjambu_128_ctx_t *ctx,
                                     uint8_t tag[XY_TINYJAMBU_128_TAG_SIZE]);

/**
 * @brief Initialize TinyJambu-128 decryption
 *
 * @param ctx Context
 * @param key 128-bit key
 * @param nonce 128-bit nonce
 * @return XY_TINYJAMBU_SUCCESS
 */
int xy_tinyjambu_128_decrypt_init(xy_tinyjambu_128_ctx_t *ctx,
                                   const uint8_t *key,
                                   const uint8_t *nonce);

/**
 * @brief Process associated data (decryption)
 *
 * @param ctx Context
 * @param ad Associated data
 * @param ad_len Length of AD
 * @return XY_TINYJAMBU_SUCCESS
 */
int xy_tinyjambu_128_decrypt_ad(xy_tinyjambu_128_ctx_t *ctx,
                                  const uint8_t *ad, size_t ad_len);

/**
 * @brief Decrypt ciphertext (can be called multiple times)
 *
 * @param ctx Context
 * @param ciphertext Input
 * @param ciphertext_len Length
 * @param plaintext Output
 * @return XY_TINYJAMBU_SUCCESS
 */
int xy_tinyjambu_128_decrypt_update(xy_tinyjambu_128_ctx_t *ctx,
                                     const uint8_t *ciphertext, size_t ciphertext_len,
                                     uint8_t *plaintext);

/**
 * @brief Finalize decryption and verify tag
 *
 * @param ctx Context
 * @param tag Tag to verify
 * @return XY_TINYJAMBU_SUCCESS if valid
 */
int xy_tinyjambu_128_decrypt_final(xy_tinyjambu_128_ctx_t *ctx,
                                     const uint8_t tag[XY_TINYJAMBU_128_TAG_SIZE]);

#ifdef __cplusplus
}
#endif

#endif /* XY_TINYJAMBU_H */
