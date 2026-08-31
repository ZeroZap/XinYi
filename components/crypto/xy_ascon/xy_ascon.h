/**
 * @file xy_ascon.h
 * @brief Ascon Lightweight Authenticated Encryption
 * @version 1.0.0
 * @date 2026-04-07
 *
 * Ascon is the NIST Lightweight Cryptography standard finalist.
 * Based on the sponge construction with a 320-bit state.
 *
 * Reference: https://csrc.nist.gov/Projects/lightweight-cryptography
 *
 * Features:
 * - Ascon-128: 128-bit key, 128-bit tag, 64-bit rate
 * - Ascon-128a: 128-bit key, 128-bit tag, 128-bit rate
 * - Ascon-80pq: 160-bit key (with 64-bit penetration), 128-bit tag
 */

#ifndef XY_ASCON_H
#define XY_ASCON_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Error Codes ==================== */

#define XY_ASCON_SUCCESS          0
#define XY_ASCON_ERROR           (-1)
#define XY_ASCON_INVALID_PARAM   (-2)
#define XY_ASCON_BUFFER_TOO_SMALL (-3)
#define XY_ASCON_AUTH_FAILED     (-4)

/* ==================== Constants ==================== */

/* Ascon-128 parameters */
#define XY_ASCON_128_KEY_SIZE     16    /* 128 bits */
#define XY_ASCON_128_NONCE_SIZE   16    /* 128 bits */
#define XY_ASCON_128_TAG_SIZE     16    /* 128 bits */

/* Ascon-128a parameters (faster) */
#define XY_ASCON_128A_KEY_SIZE    16    /* 128 bits */
#define XY_ASCON_128A_NONCE_SIZE  16    /* 128 bits */
#define XY_ASCON_128A_TAG_SIZE    16    /* 128 bits */

/* Ascon-80pq parameters */
#define XY_ASCON_80PQ_KEY_SIZE    20    /* 160 bits (128 + 32 penetration) */
#define XY_ASCON_80PQ_NONCE_SIZE  16    /* 128 bits */
#define XY_ASCON_80PQ_TAG_SIZE    16    /* 128 bits */

/* Ascon-Hash parameters */
#define XY_ASCON_HASH_SIZE        32    /* 256 bits output */
#define XY_ASCON_HASH_RATE        8     /* 64-bit rate for hash */

/* Internal constants */
#define XY_ASCON_STATE_SIZE       40    /* 320 bits = 5 x 64-bit words */
#define XY_ASCON_RATE_SIZE        8     /* 64 bits */

/* ==================== Ascon-128 AEAD ==================== */

/**
 * @brief Ascon-128 authenticated encryption
 *
 * Encrypts plaintext and produces authentication tag.
 *
 * @param key 128-bit key
 * @param nonce 128-bit nonce (public)
 * @param ad Associated data (can be NULL if ad_len = 0)
 * @param ad_len Length of associated data
 * @param plaintext Input data to encrypt
 * @param plaintext_len Length of plaintext
 * @param ciphertext Output buffer (must have plaintext_len bytes)
 * @param tag Output authentication tag (16 bytes)
 * @return XY_ASCON_SUCCESS on success
 */
int xy_ascon_128_encrypt(const uint8_t *key,
                         const uint8_t *nonce,
                         const uint8_t *ad, size_t ad_len,
                         const uint8_t *plaintext, size_t plaintext_len,
                         uint8_t *ciphertext,
                         uint8_t tag[XY_ASCON_128_TAG_SIZE]);

/**
 * @brief Ascon-128 authenticated decryption
 *
 * Decrypts ciphertext and verifies authentication tag.
 *
 * @param key 128-bit key
 * @param nonce 128-bit nonce
 * @param ad Associated data (can be NULL if ad_len = 0)
 * @param ad_len Length of associated data
 * @param ciphertext Input encrypted data
 * @param ciphertext_len Length of ciphertext
 * @param plaintext Output buffer (must have ciphertext_len bytes)
 * @param tag Authentication tag to verify (16 bytes)
 * @return XY_ASCON_SUCCESS if tag valid, XY_ASCON_AUTH_FAILED otherwise
 */
int xy_ascon_128_decrypt(const uint8_t *key,
                         const uint8_t *nonce,
                         const uint8_t *ad, size_t ad_len,
                         const uint8_t *ciphertext, size_t ciphertext_len,
                         uint8_t *plaintext,
                         const uint8_t tag[XY_ASCON_128_TAG_SIZE]);

/* ==================== Ascon-128a AEAD (Faster Variant) ==================== */

/**
 * @brief Ascon-128a authenticated encryption (128-bit rate)
 *
 * Faster variant with 128-bit rate instead of 64-bit.
 *
 * @param key 128-bit key
 * @param nonce 128-bit nonce
 * @param ad Associated data
 * @param ad_len Length of associated data
 * @param plaintext Input data
 * @param plaintext_len Length of plaintext
 * @param ciphertext Output buffer
 * @param tag Output tag (16 bytes)
 * @return XY_ASCON_SUCCESS on success
 */
int xy_ascon_128a_encrypt(const uint8_t *key,
                          const uint8_t *nonce,
                          const uint8_t *ad, size_t ad_len,
                          const uint8_t *plaintext, size_t plaintext_len,
                          uint8_t *ciphertext,
                          uint8_t tag[XY_ASCON_128A_TAG_SIZE]);

/**
 * @brief Ascon-128a authenticated decryption
 *
 * @param key 128-bit key
 * @param nonce 128-bit nonce
 * @param ad Associated data
 * @param ad_len Length of associated data
 * @param ciphertext Input encrypted data
 * @param ciphertext_len Length of ciphertext
 * @param plaintext Output buffer
 * @param tag Tag to verify (16 bytes)
 * @return XY_ASCON_SUCCESS if tag valid
 */
int xy_ascon_128a_decrypt(const uint8_t *key,
                          const uint8_t *nonce,
                          const uint8_t *ad, size_t ad_len,
                          const uint8_t *ciphertext, size_t ciphertext_len,
                          uint8_t *plaintext,
                          const uint8_t tag[XY_ASCON_128A_TAG_SIZE]);

/* ==================== Ascon-80pq AEAD ==================== */

/**
 * @brief Ascon-80pq authenticated encryption (with penetration key)
 *
 * Uses 160-bit key: 128-bit key + 32-bit penetration key (NIST submission).
 *
 * @param key 160-bit key (128-bit key + 32-bit penetration)
 * @param nonce 128-bit nonce
 * @param ad Associated data
 * @param ad_len Length of associated data
 * @param plaintext Input data
 * @param plaintext_len Length of plaintext
 * @param ciphertext Output buffer
 * @param tag Output tag (16 bytes)
 * @return XY_ASCON_SUCCESS on success
 */
int xy_ascon_80pq_encrypt(const uint8_t *key,
                          const uint8_t *nonce,
                          const uint8_t *ad, size_t ad_len,
                          const uint8_t *plaintext, size_t plaintext_len,
                          uint8_t *ciphertext,
                          uint8_t tag[XY_ASCON_80PQ_TAG_SIZE]);

/**
 * @brief Ascon-80pq authenticated decryption
 *
 * @param key 160-bit key
 * @param nonce 128-bit nonce
 * @param ad Associated data
 * @param ad_len Length of associated data
 * @param ciphertext Input encrypted data
 * @param ciphertext_len Length of ciphertext
 * @param plaintext Output buffer
 * @param tag Tag to verify (16 bytes)
 * @return XY_ASCON_SUCCESS if tag valid
 */
int xy_ascon_80pq_decrypt(const uint8_t *key,
                          const uint8_t *nonce,
                          const uint8_t *ad, size_t ad_len,
                          const uint8_t *ciphertext, size_t ciphertext_len,
                          uint8_t *plaintext,
                          const uint8_t tag[XY_ASCON_80PQ_TAG_SIZE]);

/* ==================== Ascon-Hash ==================== */

/**
 * @brief Ascon-256 hash
 *
 * @param message Input message
 * @param message_len Length of message
 * @param hash Output hash (32 bytes)
 * @return XY_ASCON_SUCCESS on success
 */
int xy_ascon_hash(const uint8_t *message, size_t message_len,
                  uint8_t hash[XY_ASCON_HASH_SIZE]);

/* ==================== Incremental API ==================== */

/**
 * @brief Ascon-128a context for incremental operations
 */
typedef struct {
    uint64_t S[5];           /* 320-bit state */
    uint64_t hash[5];        /* State snapshot for hash */
    size_t ad_len;           /* Total AD length */
    size_t plaintext_len;    /* Total plaintext length */
    size_t ad_pos;           /* Position in current AD block */
    size_t data_pos;         /* Position in current data block */
    int mode;                /* 0=AD, 1=plaintext, 2=done */
} xy_ascon_128a_ctx_t;

/**
 * @brief Initialize Ascon-128a encryption context
 *
 * @param ctx Context structure
 * @param key 128-bit key
 * @param nonce 128-bit nonce
 * @return XY_ASCON_SUCCESS on success
 */
int xy_ascon_128a_encrypt_init(xy_ascon_128a_ctx_t *ctx,
                                const uint8_t *key,
                                const uint8_t *nonce);

/**
 * @brief Process associated data
 *
 * @param ctx Context
 * @param ad Associated data
 * @param ad_len Length of AD
 * @return XY_ASCON_SUCCESS on success
 */
int xy_ascon_128a_encrypt_ad(xy_ascon_128a_ctx_t *ctx,
                              const uint8_t *ad, size_t ad_len);

/**
 * @brief Encrypt plaintext (can be called multiple times)
 *
 * @param ctx Context
 * @param plaintext Input data
 * @param plaintext_len Length of plaintext
 * @param ciphertext Output buffer (must have plaintext_len bytes)
 * @return XY_ASCON_SUCCESS on success
 */
int xy_ascon_128a_encrypt_update(xy_ascon_128a_ctx_t *ctx,
                                  const uint8_t *plaintext, size_t plaintext_len,
                                  uint8_t *ciphertext);

/**
 * @brief Finalize encryption and output tag
 *
 * @param ctx Context
 * @param tag Output tag (16 bytes)
 * @return XY_ASCON_SUCCESS on success
 */
int xy_ascon_128a_encrypt_final(xy_ascon_128a_ctx_t *ctx,
                                 uint8_t tag[XY_ASCON_128A_TAG_SIZE]);

/**
 * @brief Initialize Ascon-128a decryption context
 *
 * @param ctx Context
 * @param key 128-bit key
 * @param nonce 128-bit nonce
 * @return XY_ASCON_SUCCESS on success
 */
int xy_ascon_128a_decrypt_init(xy_ascon_128a_ctx_t *ctx,
                                const uint8_t *key,
                                const uint8_t *nonce);

/**
 * @brief Process associated data (decryption)
 *
 * @param ctx Context
 * @param ad Associated data
 * @param ad_len Length of AD
 * @return XY_ASCON_SUCCESS on success
 */
int xy_ascon_128a_decrypt_ad(xy_ascon_128a_ctx_t *ctx,
                              const uint8_t *ad, size_t ad_len);

/**
 * @brief Decrypt ciphertext (can be called multiple times)
 *
 * @param ctx Context
 * @param ciphertext Input encrypted data
 * @param ciphertext_len Length of ciphertext
 * @param plaintext Output buffer
 * @return XY_ASCON_SUCCESS on success
 */
int xy_ascon_128a_decrypt_update(xy_ascon_128a_ctx_t *ctx,
                                  const uint8_t *ciphertext, size_t ciphertext_len,
                                  uint8_t *plaintext);

/**
 * @brief Finalize decryption and verify tag
 *
 * @param ctx Context
 * @param tag Tag to verify (16 bytes)
 * @return XY_ASCON_SUCCESS if tag valid
 */
int xy_ascon_128a_decrypt_final(xy_ascon_128a_ctx_t *ctx,
                                 const uint8_t tag[XY_ASCON_128A_TAG_SIZE]);

#ifdef __cplusplus
}
#endif

#endif /* XY_ASCON_H */
