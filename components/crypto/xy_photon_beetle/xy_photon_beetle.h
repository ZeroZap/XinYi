/**
 * @file xy_photon_beetle.h
 * @brief Photon Beetle Lightweight Authenticated Encryption
 * @version 1.0.0
 * @date 2026-04-07
 *
 * Photon Beetle is a finalist in the NIST Lightweight Cryptography competition.
 * It is based on the PHOTON hash function with a sponge-like construction
 * for authenticated encryption.
 *
 * Reference: https://csrc.nist.gov/Projects/lightweight-cryptography
 *
 * Features:
 * - PHOTON-BEETLE-AEAD-128 (128-bit key, 128-bit tag)
 * - PHOTON-BEETLE-Enc-128 (128-bit key, encryption only)
 * - PHOTON-Hash-256 (256-bit hash output)
 */

#ifndef XY_PHOTON_BEETLE_H
#define XY_PHOTON_BEETLE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Error Codes ==================== */

#define XY_PHOTON_BEETLE_SUCCESS        0
#define XY_PHOTON_BEETLE_ERROR         (-1)
#define XY_PHOTON_BEETLE_INVALID_PARAM (-2)
#define XY_PHOTON_BEETLE_AUTH_FAILED   (-4)

/* ==================== Constants ==================== */

/* Photon Beetle AEAD parameters */
#define XY_PHOTON_BEETLE_KEY_SIZE    16    /* 128 bits */
#define XY_PHOTON_BEETLE_NONCE_SIZE   16    /* 128 bits */
#define XY_PHOTON_BEETLE_TAG_SIZE     16    /* 128 bits */
#define XY_PHOTON_BEETLE_TAG_64_SIZE  8     /* 64 bits */

/* Photon Hash parameters */
#define XY_PHOTON_HASH_SIZE           32    /* 256 bits */

/* Internal constants */
#define XY_PHOTON_STATE_SIZE          32    /* 256 bits */
#define XY_PHOTON_RATE_SIZE           8     /* 64 bits (8 bytes) */

/* ==================== Photon Beetle AEAD ==================== */

/**
 * @brief Photon Beetle AEAD encryption
 *
 * @param key 128-bit key
 * @param nonce 128-bit nonce
 * @param ad Associated data (can be NULL)
 * @param ad_len Length of associated data
 * @param plaintext Input data
 * @param plaintext_len Length of plaintext
 * @param ciphertext Output buffer
 * @param tag Output tag (16 bytes)
 * @return XY_PHOTON_BEETLE_SUCCESS on success
 */
int xy_photon_beetle_encrypt(const uint8_t *key,
                              const uint8_t *nonce,
                              const uint8_t *ad, size_t ad_len,
                              const uint8_t *plaintext, size_t plaintext_len,
                              uint8_t *ciphertext,
                              uint8_t tag[XY_PHOTON_BEETLE_TAG_SIZE]);

/**
 * @brief Photon Beetle AEAD decryption
 *
 * @param key 128-bit key
 * @param nonce 128-bit nonce
 * @param ad Associated data
 * @param ad_len Length of associated data
 * @param ciphertext Input encrypted data
 * @param ciphertext_len Length of ciphertext
 * @param plaintext Output buffer
 * @param tag Authentication tag to verify
 * @return XY_PHOTON_BEETLE_SUCCESS if valid
 */
int xy_photon_beetle_decrypt(const uint8_t *key,
                              const uint8_t *nonce,
                              const uint8_t *ad, size_t ad_len,
                              const uint8_t *ciphertext, size_t ciphertext_len,
                              uint8_t *plaintext,
                              const uint8_t tag[XY_PHOTON_BEETLE_TAG_SIZE]);

/**
 * @brief Photon Beetle with 64-bit tag
 */
int xy_photon_beetle_encrypt_tag64(const uint8_t *key,
                                    const uint8_t *nonce,
                                    const uint8_t *ad, size_t ad_len,
                                    const uint8_t *plaintext, size_t plaintext_len,
                                    uint8_t *ciphertext,
                                    uint8_t tag[XY_PHOTON_BEETLE_TAG_64_SIZE]);

int xy_photon_beetle_decrypt_tag64(const uint8_t *key,
                                    const uint8_t *nonce,
                                    const uint8_t *ad, size_t ad_len,
                                    const uint8_t *ciphertext, size_t ciphertext_len,
                                    uint8_t *plaintext,
                                    const uint8_t tag[XY_PHOTON_BEETLE_TAG_64_SIZE]);

/* ==================== Photon Hash ==================== */

/**
 * @brief Photon-256 hash
 *
 * @param message Input message
 * @param message_len Length of message
 * @param hash Output hash (32 bytes)
 * @return XY_PHOTON_BEETLE_SUCCESS on success
 */
int xy_photon_hash(const uint8_t *message, size_t message_len,
                   uint8_t hash[XY_PHOTON_HASH_SIZE]);

/* ==================== Incremental API ==================== */

/**
 * @brief Photon Beetle context for incremental operations
 */
typedef struct {
    uint8_t S[XY_PHOTON_STATE_SIZE];  /* 256-bit state */
    uint8_t K[XY_PHOTON_BEETLE_KEY_SIZE];  /* Key */
    uint8_t N[XY_PHOTON_BEETLE_NONCE_SIZE];  /* Nonce */
    size_t ad_len;            /* Total AD length */
    size_t plaintext_len;     /* Total plaintext length */
    int mode;                 /* 0=init, 1=AD, 2=data, 3=done */
} xy_photon_beetle_ctx_t;

/**
 * @brief Initialize Photon Beetle encryption
 *
 * @param ctx Context
 * @param key 128-bit key
 * @param nonce 128-bit nonce
 * @return XY_PHOTON_BEETLE_SUCCESS
 */
int xy_photon_beetle_encrypt_init(xy_photon_beetle_ctx_t *ctx,
                                   const uint8_t *key,
                                   const uint8_t *nonce);

/**
 * @brief Process associated data
 *
 * @param ctx Context
 * @param ad Associated data
 * @param ad_len Length of AD
 * @return XY_PHOTON_BEETLE_SUCCESS
 */
int xy_photon_beetle_encrypt_ad(xy_photon_beetle_ctx_t *ctx,
                                  const uint8_t *ad, size_t ad_len);

/**
 * @brief Encrypt plaintext
 *
 * @param ctx Context
 * @param plaintext Input
 * @param plaintext_len Length
 * @param ciphertext Output
 * @return XY_PHOTON_BEETLE_SUCCESS
 */
int xy_photon_beetle_encrypt_update(xy_photon_beetle_ctx_t *ctx,
                                     const uint8_t *plaintext, size_t plaintext_len,
                                     uint8_t *ciphertext);

/**
 * @brief Finalize and output tag
 *
 * @param ctx Context
 * @param tag Output tag
 * @return XY_PHOTON_BEETLE_SUCCESS
 */
int xy_photon_beetle_encrypt_final(xy_photon_beetle_ctx_t *ctx,
                                     uint8_t tag[XY_PHOTON_BEETLE_TAG_SIZE]);

/**
 * @brief Initialize Photon Beetle decryption
 *
 * @param ctx Context
 * @param key 128-bit key
 * @param nonce 128-bit nonce
 * @return XY_PHOTON_BEETLE_SUCCESS
 */
int xy_photon_beetle_decrypt_init(xy_photon_beetle_ctx_t *ctx,
                                   const uint8_t *key,
                                   const uint8_t *nonce);

/**
 * @brief Process associated data (decryption)
 *
 * @param ctx Context
 * @param ad Associated data
 * @param ad_len Length of AD
 * @return XY_PHOTON_BEETLE_SUCCESS
 */
int xy_photon_beetle_decrypt_ad(xy_photon_beetle_ctx_t *ctx,
                                  const uint8_t *ad, size_t ad_len);

/**
 * @brief Decrypt ciphertext
 *
 * @param ctx Context
 * @param ciphertext Input
 * @param ciphertext_len Length
 * @param plaintext Output
 * @return XY_PHOTON_BEETLE_SUCCESS
 */
int xy_photon_beetle_decrypt_update(xy_photon_beetle_ctx_t *ctx,
                                     const uint8_t *ciphertext, size_t ciphertext_len,
                                     uint8_t *plaintext);

/**
 * @brief Finalize decryption and verify tag
 *
 * @param ctx Context
 * @param tag Tag to verify
 * @return XY_PHOTON_BEETLE_SUCCESS if valid
 */
int xy_photon_beetle_decrypt_final(xy_photon_beetle_ctx_t *ctx,
                                     const uint8_t tag[XY_PHOTON_BEETLE_TAG_SIZE]);

#ifdef __cplusplus
}
#endif

#endif /* XY_PHOTON_BEETLE_H */
