/**
 * @file xy_tinyjambu.c
 * @brief TinyJambu Lightweight AEAD Implementation
 * @version 1.0.0
 * @date 2026-04-07
 *
 * Implementation of TinyJambu-128, TinyJambu-192, and TinyJambu-256.
 * Based on the reference implementation from the TinyJambu team.
 */

#include <string.h>
#include "xy_tinyjambu.h"

/* ==================== Constants ==================== */

/* Number of rounds for different key sizes */
#define TINYJAMBU_ROUNDS_128   535
#define TINYJAMBU_ROUNDS_192   571
#define TINYJAMBU_ROUNDS_256   607

/* ==================== Helper Functions ==================== */

/**
 * @brief Rotate left on 64-bit value
 */
static inline uint64_t ROTL64(uint64_t x, int n)
{
    return (x << n) | (x >> (64 - n));
}

/**
 * @brief TinyJambu permutation function
 *
 * This is the core nonlinear function of TinyJambu.
 * It consists of XORs, ANDs, and rotations.
 */
static void tinyjambu_permutation(uint64_t R[3], int rounds)
{
    uint64_t t0, t1, t2, tmp;

    for (int i = 0; i < rounds; i++) {
        /* Save state */
        t0 = R[0];
        t1 = R[1];
        t2 = R[2];

        /* Nonlinear layer */
        R[0] ^= ROTL64(t1, 2) ^ (t2 & ROTL64(t1, 1));
        R[1] ^= ROTL64(t2, 2) ^ (t0 & ROTL64(t2, 1));
        R[2] ^= ROTL64(t0, 2) ^ (t1 & ROTL64(t0, 1));

        /* Word rotation */
        tmp = R[0];
        R[0] = R[1];
        R[1] = R[2];
        R[2] = tmp;
    }
}

/**
 * @brief Initialize state with key and nonce
 */
static void tinyjambu_init(uint64_t R[3], uint64_t K[2],
                           const uint8_t *key, const uint8_t *nonce,
                           int rounds)
{
    /* Load key (2 x 64-bit words) */
    K[0] = ((uint64_t)key[0] << 0) | ((uint64_t)key[1] << 8) |
           ((uint64_t)key[2] << 16) | ((uint64_t)key[3] << 24) |
           ((uint64_t)key[4] << 32) | ((uint64_t)key[5] << 40) |
           ((uint64_t)key[6] << 48) | ((uint64_t)key[7] << 56);
    K[1] = ((uint64_t)key[8] << 0) | ((uint64_t)key[9] << 8) |
           ((uint64_t)key[10] << 16) | ((uint64_t)key[11] << 24) |
           ((uint64_t)key[12] << 32) | ((uint64_t)key[13] << 40) |
           ((uint64_t)key[14] << 48) | ((uint64_t)key[15] << 56);

    /* Initialize rate with nonce */
    R[0] = ((uint64_t)nonce[0] << 0) | ((uint64_t)nonce[1] << 8) |
           ((uint64_t)nonce[2] << 16) | ((uint64_t)nonce[3] << 24) |
           ((uint64_t)nonce[4] << 32) | ((uint64_t)nonce[5] << 40) |
           ((uint64_t)nonce[6] << 48) | ((uint64_t)nonce[7] << 56);
    R[1] = ((uint64_t)nonce[8] << 0) | ((uint64_t)nonce[9] << 8) |
           ((uint64_t)nonce[10] << 16) | ((uint64_t)nonce[11] << 24) |
           ((uint64_t)nonce[12] << 32) | ((uint64_t)nonce[13] << 40) |
           ((uint64_t)nonce[14] << 48) | ((uint64_t)nonce[15] << 56);

    /* Capacity with key */
    R[2] = K[0] ^ K[1];

    /* Initial permutation */
    tinyjambu_permutation(R, rounds);
}

/**
 * @brief Absorb data into state
 */
static void tinyjambu_absorb(uint64_t R[3], const uint8_t *data,
                              uint64_t K[2], int rounds)
{
    uint64_t x0, x1;

    /* Load rate from plaintext */
    x0 = ((uint64_t)data[0] << 0) | ((uint64_t)data[1] << 8) |
         ((uint64_t)data[2] << 16) | ((uint64_t)data[3] << 24) |
         ((uint64_t)data[4] << 32) | ((uint64_t)data[5] << 40) |
         ((uint64_t)data[6] << 48) | ((uint64_t)data[7] << 56);
    x1 = ((uint64_t)data[8] << 0) | ((uint64_t)data[9] << 8) |
         ((uint64_t)data[10] << 16) | ((uint64_t)data[11] << 24) |
         ((uint64_t)data[12] << 32) | ((uint64_t)data[13] << 40) |
         ((uint64_t)data[14] << 48) | ((uint64_t)data[15] << 56);

    /* XOR rate and capacity with key */
    R[0] ^= x0;
    R[1] ^= x1;
    R[2] ^= K[0] ^ K[1];

    /* Permutation */
    tinyjambu_permutation(R, rounds);
}

/**
 * @brief Encrypt one block
 */
static void tinyjambu_encrypt_block(uint64_t R[3], const uint8_t *plaintext,
                                    uint8_t *ciphertext, uint64_t K[2], int rounds)
{
    uint64_t x0, x1, c0, c1;

    /* Load plaintext */
    x0 = ((uint64_t)plaintext[0] << 0) | ((uint64_t)plaintext[1] << 8) |
         ((uint64_t)plaintext[2] << 16) | ((uint64_t)plaintext[3] << 24) |
         ((uint64_t)plaintext[4] << 32) | ((uint64_t)plaintext[5] << 40) |
         ((uint64_t)plaintext[6] << 48) | ((uint64_t)plaintext[7] << 56);
    x1 = ((uint64_t)plaintext[8] << 0) | ((uint64_t)plaintext[9] << 8) |
         ((uint64_t)plaintext[10] << 16) | ((uint64_t)plaintext[11] << 24) |
         ((uint64_t)plaintext[12] << 32) | ((uint64_t)plaintext[13] << 40) |
         ((uint64_t)plaintext[14] << 48) | ((uint64_t)plaintext[15] << 56);

    /* XOR with rate */
    R[0] ^= x0;
    R[1] ^= x1;

    /* Permutation */
    tinyjambu_permutation(R, rounds);

    /* XOR with capacity and key */
    R[2] ^= K[0] ^ K[1];

    /* Extract ciphertext */
    c0 = R[0];
    c1 = R[1];

    ciphertext[0] = (uint8_t)(c0 >> 0);
    ciphertext[1] = (uint8_t)(c0 >> 8);
    ciphertext[2] = (uint8_t)(c0 >> 16);
    ciphertext[3] = (uint8_t)(c0 >> 24);
    ciphertext[4] = (uint8_t)(c0 >> 32);
    ciphertext[5] = (uint8_t)(c0 >> 40);
    ciphertext[6] = (uint8_t)(c0 >> 48);
    ciphertext[7] = (uint8_t)(c0 >> 56);
    ciphertext[8] = (uint8_t)(c1 >> 0);
    ciphertext[9] = (uint8_t)(c1 >> 8);
    ciphertext[10] = (uint8_t)(c1 >> 16);
    ciphertext[11] = (uint8_t)(c1 >> 24);
    ciphertext[12] = (uint8_t)(c1 >> 32);
    ciphertext[13] = (uint8_t)(c1 >> 40);
    ciphertext[14] = (uint8_t)(c1 >> 48);
    ciphertext[15] = (uint8_t)(c1 >> 56);
}

/**
 * @brief Decrypt one block
 */
static void tinyjambu_decrypt_block(uint64_t R[3], const uint8_t *ciphertext,
                                     uint8_t *plaintext, uint64_t K[2], int rounds)
{
    uint64_t x0, x1, c0, c1;

    /* Load ciphertext */
    c0 = ((uint64_t)ciphertext[0] << 0) | ((uint64_t)ciphertext[1] << 8) |
         ((uint64_t)ciphertext[2] << 16) | ((uint64_t)ciphertext[3] << 24) |
         ((uint64_t)ciphertext[4] << 32) | ((uint64_t)ciphertext[5] << 40) |
         ((uint64_t)ciphertext[6] << 48) | ((uint64_t)ciphertext[7] << 56);
    c1 = ((uint64_t)ciphertext[8] << 0) | ((uint64_t)ciphertext[9] << 8) |
         ((uint64_t)ciphertext[10] << 16) | ((uint64_t)ciphertext[11] << 24) |
         ((uint64_t)ciphertext[12] << 32) | ((uint64_t)ciphertext[13] << 40) |
         ((uint64_t)ciphertext[14] << 48) | ((uint64_t)ciphertext[15] << 56);

    /* XOR rate with ciphertext (same as encrypt) */
    R[0] ^= c0;
    R[1] ^= c1;

    /* Permutation */
    tinyjambu_permutation(R, rounds);

    /* XOR capacity with key (same as encrypt) */
    R[2] ^= K[0] ^ K[1];

    /* Extract plaintext */
    x0 = R[0];
    x1 = R[1];

    plaintext[0] = (uint8_t)(x0 >> 0);
    plaintext[1] = (uint8_t)(x0 >> 8);
    plaintext[2] = (uint8_t)(x0 >> 16);
    plaintext[3] = (uint8_t)(x0 >> 24);
    plaintext[4] = (uint8_t)(x0 >> 32);
    plaintext[5] = (uint8_t)(x0 >> 40);
    plaintext[6] = (uint8_t)(x0 >> 48);
    plaintext[7] = (uint8_t)(x0 >> 56);
    plaintext[8] = (uint8_t)(x1 >> 0);
    plaintext[9] = (uint8_t)(x1 >> 8);
    plaintext[10] = (uint8_t)(x1 >> 16);
    plaintext[11] = (uint8_t)(x1 >> 24);
    plaintext[12] = (uint8_t)(x1 >> 32);
    plaintext[13] = (uint8_t)(x1 >> 40);
    plaintext[14] = (uint8_t)(x1 >> 48);
    plaintext[15] = (uint8_t)(x1 >> 56);
}

/**
 * @brief Finalize and extract tag
 */
static void tinyjambu_finalize(uint64_t R[3], uint64_t K[2],
                                uint8_t *tag, int tag_size, int rounds)
{
    /* XOR with key */
    R[2] ^= K[0] ^ K[1];

    /* Final permutation */
    tinyjambu_permutation(R, rounds);

    /* XOR with key again */
    R[2] ^= K[0] ^ K[1];

    /* Extract tag */
    if (tag_size == 8) {
        tag[0] = (uint8_t)(R[2] >> 0);
        tag[1] = (uint8_t)(R[2] >> 8);
        tag[2] = (uint8_t)(R[2] >> 16);
        tag[3] = (uint8_t)(R[2] >> 24);
        tag[4] = (uint8_t)(R[2] >> 32);
        tag[5] = (uint8_t)(R[2] >> 40);
        tag[6] = (uint8_t)(R[2] >> 48);
        tag[7] = (uint8_t)(R[2] >> 56);
    } else if (tag_size == 16) {
        /* 128-bit tag: use R[1] and R[2] */
        tag[0] = (uint8_t)(R[1] >> 0);
        tag[1] = (uint8_t)(R[1] >> 8);
        tag[2] = (uint8_t)(R[1] >> 16);
        tag[3] = (uint8_t)(R[1] >> 24);
        tag[4] = (uint8_t)(R[1] >> 32);
        tag[5] = (uint8_t)(R[1] >> 40);
        tag[6] = (uint8_t)(R[1] >> 48);
        tag[7] = (uint8_t)(R[1] >> 56);
        tag[8] = (uint8_t)(R[2] >> 0);
        tag[9] = (uint8_t)(R[2] >> 8);
        tag[10] = (uint8_t)(R[2] >> 16);
        tag[11] = (uint8_t)(R[2] >> 24);
        tag[12] = (uint8_t)(R[2] >> 32);
        tag[13] = (uint8_t)(R[2] >> 40);
        tag[14] = (uint8_t)(R[2] >> 48);
        tag[15] = (uint8_t)(R[2] >> 56);
    }
}

/* ==================== TinyJambu-128 AEAD ==================== */

int xy_tinyjambu_128_encrypt(const uint8_t *key,
                              const uint8_t *nonce,
                              const uint8_t *ad, size_t ad_len,
                              const uint8_t *plaintext, size_t plaintext_len,
                              uint8_t *ciphertext,
                              uint8_t tag[XY_TINYJAMBU_128_TAG_SIZE])
{
    uint64_t R[3], K[2];
    uint8_t buffer[16];
    size_t i, blocks;

    if (!key || !nonce || !ciphertext || !tag) {
        return XY_TINYJAMBU_INVALID_PARAM;
    }

    /* Initialize */
    tinyjambu_init(R, K, key, nonce, TINYJAMBU_ROUNDS_128);

    /* Process associated data */
    if (ad && ad_len > 0) {
        i = 0;
        while (i < ad_len) {
            if (i + 16 <= ad_len) {
                tinyjambu_absorb(R, ad + i, K, TINYJAMBU_ROUNDS_128);
                i += 16;
            } else {
                memset(buffer, 0, 16);
                memcpy(buffer, ad + i, ad_len - i);
                tinyjambu_absorb(R, buffer, K, TINYJAMBU_ROUNDS_128);
                break;
            }
        }
    }

    /* Encrypt plaintext */
    blocks = plaintext_len / 16;
    for (i = 0; i < blocks; i++) {
        tinyjambu_encrypt_block(R, plaintext + i * 16,
                                ciphertext + i * 16, K, TINYJAMBU_ROUNDS_128);
    }

    /* Handle remaining bytes */
    if (plaintext_len % 16) {
        memset(buffer, 0, 16);
        memcpy(buffer, plaintext + blocks * 16, plaintext_len % 16);
        tinyjambu_encrypt_block(R, buffer, buffer, K, TINYJAMBU_ROUNDS_128);
        memcpy(ciphertext + blocks * 16, buffer, plaintext_len % 16);
    }

    /* Finalize and extract tag */
    tinyjambu_finalize(R, K, tag, 8, TINYJAMBU_ROUNDS_128);

    return XY_TINYJAMBU_SUCCESS;
}

int xy_tinyjambu_128_decrypt(const uint8_t *key,
                              const uint8_t *nonce,
                              const uint8_t *ad, size_t ad_len,
                              const uint8_t *ciphertext, size_t ciphertext_len,
                              uint8_t *plaintext,
                              const uint8_t tag[XY_TINYJAMBU_128_TAG_SIZE])
{
    uint64_t R[3], K[2];
    uint8_t buffer[16];
    uint8_t expected_tag[XY_TINYJAMBU_128_TAG_SIZE];
    size_t i, blocks;
    uint8_t diff;

    if (!key || !nonce || !ciphertext || !plaintext || !tag) {
        return XY_TINYJAMBU_INVALID_PARAM;
    }

    /* Initialize */
    tinyjambu_init(R, K, key, nonce, TINYJAMBU_ROUNDS_128);

    /* Process associated data */
    if (ad && ad_len > 0) {
        i = 0;
        while (i < ad_len) {
            if (i + 16 <= ad_len) {
                tinyjambu_absorb(R, ad + i, K, TINYJAMBU_ROUNDS_128);
                i += 16;
            } else {
                memset(buffer, 0, 16);
                memcpy(buffer, ad + i, ad_len - i);
                tinyjambu_absorb(R, buffer, K, TINYJAMBU_ROUNDS_128);
                break;
            }
        }
    }

    /* Decrypt ciphertext */
    blocks = ciphertext_len / 16;
    for (i = 0; i < blocks; i++) {
        tinyjambu_decrypt_block(R, ciphertext + i * 16,
                                plaintext + i * 16, K, TINYJAMBU_ROUNDS_128);
    }

    /* Handle remaining bytes */
    if (ciphertext_len % 16) {
        memset(buffer, 0, 16);
        memcpy(buffer, ciphertext + blocks * 16, ciphertext_len % 16);
        tinyjambu_decrypt_block(R, buffer, buffer, K, TINYJAMBU_ROUNDS_128);
        memcpy(plaintext + blocks * 16, buffer, ciphertext_len % 16);
    }

    /* Finalize and extract expected tag */
    tinyjambu_finalize(R, K, expected_tag, 8, TINYJAMBU_ROUNDS_128);

    /* Constant-time comparison */
    diff = 0;
    for (i = 0; i < XY_TINYJAMBU_128_TAG_SIZE; i++) {
        diff |= tag[i] ^ expected_tag[i];
    }

    if (diff) {
        memset(plaintext, 0, ciphertext_len);
        return XY_TINYJAMBU_AUTH_FAILED;
    }

    return XY_TINYJAMBU_SUCCESS;
}

int xy_tinyjambu_128_encrypt_tag128(const uint8_t *key,
                                     const uint8_t *nonce,
                                     const uint8_t *ad, size_t ad_len,
                                     const uint8_t *plaintext, size_t plaintext_len,
                                     uint8_t *ciphertext,
                                     uint8_t tag[XY_TINYJAMBU_128_TAG_128])
{
    uint64_t R[3], K[2];
    uint8_t buffer[16];
    size_t i, blocks;

    if (!key || !nonce || !ciphertext || !tag) {
        return XY_TINYJAMBU_INVALID_PARAM;
    }

    tinyjambu_init(R, K, key, nonce, TINYJAMBU_ROUNDS_128);

    if (ad && ad_len > 0) {
        i = 0;
        while (i < ad_len) {
            if (i + 16 <= ad_len) {
                tinyjambu_absorb(R, ad + i, K, TINYJAMBU_ROUNDS_128);
                i += 16;
            } else {
                memset(buffer, 0, 16);
                memcpy(buffer, ad + i, ad_len - i);
                tinyjambu_absorb(R, buffer, K, TINYJAMBU_ROUNDS_128);
                break;
            }
        }
    }

    blocks = plaintext_len / 16;
    for (i = 0; i < blocks; i++) {
        tinyjambu_encrypt_block(R, plaintext + i * 16,
                                ciphertext + i * 16, K, TINYJAMBU_ROUNDS_128);
    }

    if (plaintext_len % 16) {
        memset(buffer, 0, 16);
        memcpy(buffer, plaintext + blocks * 16, plaintext_len % 16);
        tinyjambu_encrypt_block(R, buffer, buffer, K, TINYJAMBU_ROUNDS_128);
        memcpy(ciphertext + blocks * 16, buffer, plaintext_len % 16);
    }

    tinyjambu_finalize(R, K, tag, 16, TINYJAMBU_ROUNDS_128);

    return XY_TINYJAMBU_SUCCESS;
}

int xy_tinyjambu_128_decrypt_tag128(const uint8_t *key,
                                     const uint8_t *nonce,
                                     const uint8_t *ad, size_t ad_len,
                                     const uint8_t *ciphertext, size_t ciphertext_len,
                                     uint8_t *plaintext,
                                     const uint8_t tag[XY_TINYJAMBU_128_TAG_128])
{
    uint64_t R[3], K[2];
    uint8_t buffer[16];
    uint8_t expected_tag[XY_TINYJAMBU_128_TAG_128];
    size_t i, blocks;
    uint8_t diff;

    if (!key || !nonce || !ciphertext || !plaintext || !tag) {
        return XY_TINYJAMBU_INVALID_PARAM;
    }

    tinyjambu_init(R, K, key, nonce, TINYJAMBU_ROUNDS_128);

    if (ad && ad_len > 0) {
        i = 0;
        while (i < ad_len) {
            if (i + 16 <= ad_len) {
                tinyjambu_absorb(R, ad + i, K, TINYJAMBU_ROUNDS_128);
                i += 16;
            } else {
                memset(buffer, 0, 16);
                memcpy(buffer, ad + i, ad_len - i);
                tinyjambu_absorb(R, buffer, K, TINYJAMBU_ROUNDS_128);
                break;
            }
        }
    }

    blocks = ciphertext_len / 16;
    for (i = 0; i < blocks; i++) {
        tinyjambu_decrypt_block(R, ciphertext + i * 16,
                                plaintext + i * 16, K, TINYJAMBU_ROUNDS_128);
    }

    if (ciphertext_len % 16) {
        memset(buffer, 0, 16);
        memcpy(buffer, ciphertext + blocks * 16, ciphertext_len % 16);
        tinyjambu_decrypt_block(R, buffer, buffer, K, TINYJAMBU_ROUNDS_128);
        memcpy(plaintext + blocks * 16, buffer, ciphertext_len % 16);
    }

    tinyjambu_finalize(R, K, expected_tag, 16, TINYJAMBU_ROUNDS_128);

    diff = 0;
    for (i = 0; i < XY_TINYJAMBU_128_TAG_128; i++) {
        diff |= tag[i] ^ expected_tag[i];
    }

    if (diff) {
        memset(plaintext, 0, ciphertext_len);
        return XY_TINYJAMBU_AUTH_FAILED;
    }

    return XY_TINYJAMBU_SUCCESS;
}

/* ==================== TinyJambu-192 AEAD ==================== */

/**
 * @brief Initialize state for TinyJambu-192 (24-byte key)
 */
static void tinyjambu_init_192(uint64_t R[3], uint64_t K[3],
                                 const uint8_t *key, const uint8_t *nonce)
{
    /* Load 192-bit key (3 x 64-bit words, but only 24 bytes) */
    K[0] = ((uint64_t)key[0] << 0) | ((uint64_t)key[1] << 8) |
           ((uint64_t)key[2] << 16) | ((uint64_t)key[3] << 24) |
           ((uint64_t)key[4] << 32) | ((uint64_t)key[5] << 40) |
           ((uint64_t)key[6] << 48) | ((uint64_t)key[7] << 56);
    K[1] = ((uint64_t)key[8] << 0) | ((uint64_t)key[9] << 8) |
           ((uint64_t)key[10] << 16) | ((uint64_t)key[11] << 24) |
           ((uint64_t)key[12] << 32) | ((uint64_t)key[13] << 40) |
           ((uint64_t)key[14] << 48) | ((uint64_t)key[15] << 56);
    K[2] = ((uint64_t)key[16] << 0) | ((uint64_t)key[17] << 8) |
           ((uint64_t)key[18] << 16) | ((uint64_t)key[19] << 24) |
           ((uint64_t)key[20] << 32) | ((uint64_t)key[21] << 40) |
           ((uint64_t)key[22] << 48) | ((uint64_t)key[23] << 56);

    /* Initialize rate with nonce */
    R[0] = ((uint64_t)nonce[0] << 0) | ((uint64_t)nonce[1] << 8) |
           ((uint64_t)nonce[2] << 16) | ((uint64_t)nonce[3] << 24) |
           ((uint64_t)nonce[4] << 32) | ((uint64_t)nonce[5] << 40) |
           ((uint64_t)nonce[6] << 48) | ((uint64_t)nonce[7] << 56);
    R[1] = ((uint64_t)nonce[8] << 0) | ((uint64_t)nonce[9] << 8) |
           ((uint64_t)nonce[10] << 16) | ((uint64_t)nonce[11] << 24) |
           ((uint64_t)nonce[12] << 32) | ((uint64_t)nonce[13] << 40) |
           ((uint64_t)nonce[14] << 48) | ((uint64_t)nonce[15] << 56);

    /* Capacity with key */
    R[2] = K[0] ^ K[1] ^ K[2];

    tinyjambu_permutation(R, TINYJAMBU_ROUNDS_192);
}

int xy_tinyjambu_192_encrypt(const uint8_t *key,
                              const uint8_t *nonce,
                              const uint8_t *ad, size_t ad_len,
                              const uint8_t *plaintext, size_t plaintext_len,
                              uint8_t *ciphertext,
                              uint8_t tag[XY_TINYJAMBU_192_TAG_SIZE])
{
    uint64_t R[3], K[3];
    uint8_t buffer[16];
    size_t i, blocks;

    if (!key || !nonce || !ciphertext || !tag) {
        return XY_TINYJAMBU_INVALID_PARAM;
    }

    tinyjambu_init_192(R, K, key, nonce);

    if (ad && ad_len > 0) {
        i = 0;
        while (i < ad_len) {
            if (i + 16 <= ad_len) {
                tinyjambu_absorb(R, ad + i, (uint64_t *)K, TINYJAMBU_ROUNDS_192);
                i += 16;
            } else {
                memset(buffer, 0, 16);
                memcpy(buffer, ad + i, ad_len - i);
                tinyjambu_absorb(R, buffer, (uint64_t *)K, TINYJAMBU_ROUNDS_192);
                break;
            }
        }
    }

    blocks = plaintext_len / 16;
    for (i = 0; i < blocks; i++) {
        tinyjambu_encrypt_block(R, plaintext + i * 16,
                                ciphertext + i * 16, (uint64_t *)K, TINYJAMBU_ROUNDS_192);
    }

    if (plaintext_len % 16) {
        memset(buffer, 0, 16);
        memcpy(buffer, plaintext + blocks * 16, plaintext_len % 16);
        tinyjambu_encrypt_block(R, buffer, buffer, (uint64_t *)K, TINYJAMBU_ROUNDS_192);
        memcpy(ciphertext + blocks * 16, buffer, plaintext_len % 16);
    }

    R[2] ^= K[0] ^ K[1] ^ K[2];
    tinyjambu_permutation(R, TINYJAMBU_ROUNDS_192);
    R[2] ^= K[0] ^ K[1] ^ K[2];

    tag[0] = (uint8_t)(R[2] >> 0);
    tag[1] = (uint8_t)(R[2] >> 8);
    tag[2] = (uint8_t)(R[2] >> 16);
    tag[3] = (uint8_t)(R[2] >> 24);
    tag[4] = (uint8_t)(R[2] >> 32);
    tag[5] = (uint8_t)(R[2] >> 40);
    tag[6] = (uint8_t)(R[2] >> 48);
    tag[7] = (uint8_t)(R[2] >> 56);

    return XY_TINYJAMBU_SUCCESS;
}

int xy_tinyjambu_192_decrypt(const uint8_t *key,
                              const uint8_t *nonce,
                              const uint8_t *ad, size_t ad_len,
                              const uint8_t *ciphertext, size_t ciphertext_len,
                              uint8_t *plaintext,
                              const uint8_t tag[XY_TINYJAMBU_192_TAG_SIZE])
{
    uint64_t R[3], K[3];
    uint8_t buffer[16];
    uint8_t expected_tag[XY_TINYJAMBU_192_TAG_SIZE];
    size_t i, blocks;
    uint8_t diff;

    if (!key || !nonce || !ciphertext || !plaintext || !tag) {
        return XY_TINYJAMBU_INVALID_PARAM;
    }

    tinyjambu_init_192(R, K, key, nonce);

    if (ad && ad_len > 0) {
        i = 0;
        while (i < ad_len) {
            if (i + 16 <= ad_len) {
                tinyjambu_absorb(R, ad + i, (uint64_t *)K, TINYJAMBU_ROUNDS_192);
                i += 16;
            } else {
                memset(buffer, 0, 16);
                memcpy(buffer, ad + i, ad_len - i);
                tinyjambu_absorb(R, buffer, (uint64_t *)K, TINYJAMBU_ROUNDS_192);
                break;
            }
        }
    }

    blocks = ciphertext_len / 16;
    for (i = 0; i < blocks; i++) {
        tinyjambu_decrypt_block(R, ciphertext + i * 16,
                                plaintext + i * 16, (uint64_t *)K, TINYJAMBU_ROUNDS_192);
    }

    if (ciphertext_len % 16) {
        memset(buffer, 0, 16);
        memcpy(buffer, ciphertext + blocks * 16, ciphertext_len % 16);
        tinyjambu_decrypt_block(R, buffer, buffer, (uint64_t *)K, TINYJAMBU_ROUNDS_192);
        memcpy(plaintext + blocks * 16, buffer, ciphertext_len % 16);
    }

    R[2] ^= K[0] ^ K[1] ^ K[2];
    tinyjambu_permutation(R, TINYJAMBU_ROUNDS_192);
    R[2] ^= K[0] ^ K[1] ^ K[2];

    expected_tag[0] = (uint8_t)(R[2] >> 0);
    expected_tag[1] = (uint8_t)(R[2] >> 8);
    expected_tag[2] = (uint8_t)(R[2] >> 16);
    expected_tag[3] = (uint8_t)(R[2] >> 24);
    expected_tag[4] = (uint8_t)(R[2] >> 32);
    expected_tag[5] = (uint8_t)(R[2] >> 40);
    expected_tag[6] = (uint8_t)(R[2] >> 48);
    expected_tag[7] = (uint8_t)(R[2] >> 56);

    diff = 0;
    for (i = 0; i < XY_TINYJAMBU_192_TAG_SIZE; i++) {
        diff |= tag[i] ^ expected_tag[i];
    }

    if (diff) {
        memset(plaintext, 0, ciphertext_len);
        return XY_TINYJAMBU_AUTH_FAILED;
    }

    return XY_TINYJAMBU_SUCCESS;
}

/* ==================== TinyJambu-256 AEAD ==================== */

/**
 * @brief Initialize state for TinyJambu-256 (32-byte key)
 */
static void tinyjambu_init_256(uint64_t R[3], uint64_t K[4],
                                 const uint8_t *key, const uint8_t *nonce)
{
    /* Load 256-bit key (4 x 64-bit words) */
    K[0] = ((uint64_t)key[0] << 0) | ((uint64_t)key[1] << 8) |
           ((uint64_t)key[2] << 16) | ((uint64_t)key[3] << 24) |
           ((uint64_t)key[4] << 32) | ((uint64_t)key[5] << 40) |
           ((uint64_t)key[6] << 48) | ((uint64_t)key[7] << 56);
    K[1] = ((uint64_t)key[8] << 0) | ((uint64_t)key[9] << 8) |
           ((uint64_t)key[10] << 16) | ((uint64_t)key[11] << 24) |
           ((uint64_t)key[12] << 32) | ((uint64_t)key[13] << 40) |
           ((uint64_t)key[14] << 48) | ((uint64_t)key[15] << 56);
    K[2] = ((uint64_t)key[16] << 0) | ((uint64_t)key[17] << 8) |
           ((uint64_t)key[18] << 16) | ((uint64_t)key[19] << 24) |
           ((uint64_t)key[20] << 32) | ((uint64_t)key[21] << 40) |
           ((uint64_t)key[22] << 48) | ((uint64_t)key[23] << 56);
    K[3] = ((uint64_t)key[24] << 0) | ((uint64_t)key[25] << 8) |
           ((uint64_t)key[26] << 16) | ((uint64_t)key[27] << 24) |
           ((uint64_t)key[28] << 32) | ((uint64_t)key[29] << 40) |
           ((uint64_t)key[30] << 48) | ((uint64_t)key[31] << 56);

    /* Initialize rate with nonce */
    R[0] = ((uint64_t)nonce[0] << 0) | ((uint64_t)nonce[1] << 8) |
           ((uint64_t)nonce[2] << 16) | ((uint64_t)nonce[3] << 24) |
           ((uint64_t)nonce[4] << 32) | ((uint64_t)nonce[5] << 40) |
           ((uint64_t)nonce[6] << 48) | ((uint64_t)nonce[7] << 56);
    R[1] = ((uint64_t)nonce[8] << 0) | ((uint64_t)nonce[9] << 8) |
           ((uint64_t)nonce[10] << 16) | ((uint64_t)nonce[11] << 24) |
           ((uint64_t)nonce[12] << 32) | ((uint64_t)nonce[13] << 40) |
           ((uint64_t)nonce[14] << 48) | ((uint64_t)nonce[15] << 56);

    /* Capacity with key */
    R[2] = K[0] ^ K[1] ^ K[2] ^ K[3];

    tinyjambu_permutation(R, TINYJAMBU_ROUNDS_256);
}

int xy_tinyjambu_256_encrypt(const uint8_t *key,
                              const uint8_t *nonce,
                              const uint8_t *ad, size_t ad_len,
                              const uint8_t *plaintext, size_t plaintext_len,
                              uint8_t *ciphertext,
                              uint8_t tag[XY_TINYJAMBU_256_TAG_SIZE])
{
    uint64_t R[3], K[4];
    uint8_t buffer[16];
    size_t i, blocks;

    if (!key || !nonce || !ciphertext || !tag) {
        return XY_TINYJAMBU_INVALID_PARAM;
    }

    tinyjambu_init_256(R, K, key, nonce);

    if (ad && ad_len > 0) {
        i = 0;
        while (i < ad_len) {
            if (i + 16 <= ad_len) {
                tinyjambu_absorb(R, ad + i, (uint64_t *)K, TINYJAMBU_ROUNDS_256);
                i += 16;
            } else {
                memset(buffer, 0, 16);
                memcpy(buffer, ad + i, ad_len - i);
                tinyjambu_absorb(R, buffer, (uint64_t *)K, TINYJAMBU_ROUNDS_256);
                break;
            }
        }
    }

    blocks = plaintext_len / 16;
    for (i = 0; i < blocks; i++) {
        tinyjambu_encrypt_block(R, plaintext + i * 16,
                                ciphertext + i * 16, (uint64_t *)K, TINYJAMBU_ROUNDS_256);
    }

    if (plaintext_len % 16) {
        memset(buffer, 0, 16);
        memcpy(buffer, plaintext + blocks * 16, plaintext_len % 16);
        tinyjambu_encrypt_block(R, buffer, buffer, (uint64_t *)K, TINYJAMBU_ROUNDS_256);
        memcpy(ciphertext + blocks * 16, buffer, plaintext_len % 16);
    }

    R[2] ^= K[0] ^ K[1] ^ K[2] ^ K[3];
    tinyjambu_permutation(R, TINYJAMBU_ROUNDS_256);
    R[2] ^= K[0] ^ K[1] ^ K[2] ^ K[3];

    tag[0] = (uint8_t)(R[2] >> 0);
    tag[1] = (uint8_t)(R[2] >> 8);
    tag[2] = (uint8_t)(R[2] >> 16);
    tag[3] = (uint8_t)(R[2] >> 24);
    tag[4] = (uint8_t)(R[2] >> 32);
    tag[5] = (uint8_t)(R[2] >> 40);
    tag[6] = (uint8_t)(R[2] >> 48);
    tag[7] = (uint8_t)(R[2] >> 56);

    return XY_TINYJAMBU_SUCCESS;
}

int xy_tinyjambu_256_decrypt(const uint8_t *key,
                              const uint8_t *nonce,
                              const uint8_t *ad, size_t ad_len,
                              const uint8_t *ciphertext, size_t ciphertext_len,
                              uint8_t *plaintext,
                              const uint8_t tag[XY_TINYJAMBU_256_TAG_SIZE])
{
    uint64_t R[3], K[4];
    uint8_t buffer[16];
    uint8_t expected_tag[XY_TINYJAMBU_256_TAG_SIZE];
    size_t i, blocks;
    uint8_t diff;

    if (!key || !nonce || !ciphertext || !plaintext || !tag) {
        return XY_TINYJAMBU_INVALID_PARAM;
    }

    tinyjambu_init_256(R, K, key, nonce);

    if (ad && ad_len > 0) {
        i = 0;
        while (i < ad_len) {
            if (i + 16 <= ad_len) {
                tinyjambu_absorb(R, ad + i, (uint64_t *)K, TINYJAMBU_ROUNDS_256);
                i += 16;
            } else {
                memset(buffer, 0, 16);
                memcpy(buffer, ad + i, ad_len - i);
                tinyjambu_absorb(R, buffer, (uint64_t *)K, TINYJAMBU_ROUNDS_256);
                break;
            }
        }
    }

    blocks = ciphertext_len / 16;
    for (i = 0; i < blocks; i++) {
        tinyjambu_decrypt_block(R, ciphertext + i * 16,
                                plaintext + i * 16, (uint64_t *)K, TINYJAMBU_ROUNDS_256);
    }

    if (ciphertext_len % 16) {
        memset(buffer, 0, 16);
        memcpy(buffer, ciphertext + blocks * 16, ciphertext_len % 16);
        tinyjambu_decrypt_block(R, buffer, buffer, (uint64_t *)K, TINYJAMBU_ROUNDS_256);
        memcpy(plaintext + blocks * 16, buffer, ciphertext_len % 16);
    }

    R[2] ^= K[0] ^ K[1] ^ K[2] ^ K[3];
    tinyjambu_permutation(R, TINYJAMBU_ROUNDS_256);
    R[2] ^= K[0] ^ K[1] ^ K[2] ^ K[3];

    expected_tag[0] = (uint8_t)(R[2] >> 0);
    expected_tag[1] = (uint8_t)(R[2] >> 8);
    expected_tag[2] = (uint8_t)(R[2] >> 16);
    expected_tag[3] = (uint8_t)(R[2] >> 24);
    expected_tag[4] = (uint8_t)(R[2] >> 32);
    expected_tag[5] = (uint8_t)(R[2] >> 40);
    expected_tag[6] = (uint8_t)(R[2] >> 48);
    expected_tag[7] = (uint8_t)(R[2] >> 56);

    diff = 0;
    for (i = 0; i < XY_TINYJAMBU_256_TAG_SIZE; i++) {
        diff |= tag[i] ^ expected_tag[i];
    }

    if (diff) {
        memset(plaintext, 0, ciphertext_len);
        return XY_TINYJAMBU_AUTH_FAILED;
    }

    return XY_TINYJAMBU_SUCCESS;
}

/* ==================== Incremental API ==================== */

int xy_tinyjambu_128_encrypt_init(xy_tinyjambu_128_ctx_t *ctx,
                                   const uint8_t *key,
                                   const uint8_t *nonce)
{
    if (!ctx || !key || !nonce) {
        return XY_TINYJAMBU_INVALID_PARAM;
    }

    memset(ctx, 0, sizeof(xy_tinyjambu_128_ctx_t));
    ctx->rounds = TINYJAMBU_ROUNDS_128;

    /* Initialize state */
    ctx->K[0] = ((uint64_t)key[0] << 0) | ((uint64_t)key[1] << 8) |
                ((uint64_t)key[2] << 16) | ((uint64_t)key[3] << 24) |
                ((uint64_t)key[4] << 32) | ((uint64_t)key[5] << 40) |
                ((uint64_t)key[6] << 48) | ((uint64_t)key[7] << 56);
    ctx->K[1] = ((uint64_t)key[8] << 0) | ((uint64_t)key[9] << 8) |
                ((uint64_t)key[10] << 16) | ((uint64_t)key[11] << 24) |
                ((uint64_t)key[12] << 32) | ((uint64_t)key[13] << 40) |
                ((uint64_t)key[14] << 48) | ((uint64_t)key[15] << 56);

    ctx->R[0] = ((uint64_t)nonce[0] << 0) | ((uint64_t)nonce[1] << 8) |
                ((uint64_t)nonce[2] << 16) | ((uint64_t)nonce[3] << 24) |
                ((uint64_t)nonce[4] << 32) | ((uint64_t)nonce[5] << 40) |
                ((uint64_t)nonce[6] << 48) | ((uint64_t)nonce[7] << 56);
    ctx->R[1] = ((uint64_t)nonce[8] << 0) | ((uint64_t)nonce[9] << 8) |
                ((uint64_t)nonce[10] << 16) | ((uint64_t)nonce[11] << 24) |
                ((uint64_t)nonce[12] << 32) | ((uint64_t)nonce[13] << 40) |
                ((uint64_t)nonce[14] << 48) | ((uint64_t)nonce[15] << 56);
    ctx->R[2] = ctx->K[0] ^ ctx->K[1];

    tinyjambu_permutation(ctx->R, ctx->rounds);

    ctx->mode = 0;

    return XY_TINYJAMBU_SUCCESS;
}

int xy_tinyjambu_128_encrypt_ad(xy_tinyjambu_128_ctx_t *ctx,
                                  const uint8_t *ad, size_t ad_len)
{
    uint8_t buffer[16];
    size_t i;

    if (!ctx || !ad) {
        return XY_TINYJAMBU_INVALID_PARAM;
    }

    ctx->ad_len = ad_len;
    i = 0;

    while (i < ad_len) {
        if (i + 16 <= ad_len) {
            tinyjambu_absorb(ctx->R, ad + i, ctx->K, ctx->rounds);
            i += 16;
        } else {
            memset(buffer, 0, 16);
            memcpy(buffer, ad + i, ad_len - i);
            tinyjambu_absorb(ctx->R, buffer, ctx->K, ctx->rounds);
            break;
        }
    }

    ctx->mode = 1;

    return XY_TINYJAMBU_SUCCESS;
}

int xy_tinyjambu_128_encrypt_update(xy_tinyjambu_128_ctx_t *ctx,
                                     const uint8_t *plaintext, size_t plaintext_len,
                                     uint8_t *ciphertext)
{
    uint8_t buffer[16];
    size_t i, blocks;

    if (!ctx || !plaintext || !ciphertext) {
        return XY_TINYJAMBU_INVALID_PARAM;
    }

    ctx->plaintext_len = plaintext_len;
    blocks = plaintext_len / 16;

    for (i = 0; i < blocks; i++) {
        tinyjambu_encrypt_block(ctx->R, plaintext + i * 16,
                                ciphertext + i * 16, ctx->K, ctx->rounds);
    }

    if (plaintext_len % 16) {
        memset(buffer, 0, 16);
        memcpy(buffer, plaintext + blocks * 16, plaintext_len % 16);
        tinyjambu_encrypt_block(ctx->R, buffer, buffer, ctx->K, ctx->rounds);
        memcpy(ciphertext + blocks * 16, buffer, plaintext_len % 16);
    }

    ctx->mode = 2;

    return XY_TINYJAMBU_SUCCESS;
}

int xy_tinyjambu_128_encrypt_final(xy_tinyjambu_128_ctx_t *ctx,
                                     uint8_t tag[XY_TINYJAMBU_128_TAG_SIZE])
{
    if (!ctx || !tag) {
        return XY_TINYJAMBU_INVALID_PARAM;
    }

    tinyjambu_finalize(ctx->R, ctx->K, tag, 8, ctx->rounds);

    ctx->mode = 3;

    return XY_TINYJAMBU_SUCCESS;
}

int xy_tinyjambu_128_decrypt_init(xy_tinyjambu_128_ctx_t *ctx,
                                   const uint8_t *key,
                                   const uint8_t *nonce)
{
    if (!ctx || !key || !nonce) {
        return XY_TINYJAMBU_INVALID_PARAM;
    }

    memset(ctx, 0, sizeof(xy_tinyjambu_128_ctx_t));
    ctx->rounds = TINYJAMBU_ROUNDS_128;

    ctx->K[0] = ((uint64_t)key[0] << 0) | ((uint64_t)key[1] << 8) |
                ((uint64_t)key[2] << 16) | ((uint64_t)key[3] << 24) |
                ((uint64_t)key[4] << 32) | ((uint64_t)key[5] << 40) |
                ((uint64_t)key[6] << 48) | ((uint64_t)key[7] << 56);
    ctx->K[1] = ((uint64_t)key[8] << 0) | ((uint64_t)key[9] << 8) |
                ((uint64_t)key[10] << 16) | ((uint64_t)key[11] << 24) |
                ((uint64_t)key[12] << 32) | ((uint64_t)key[13] << 40) |
                ((uint64_t)key[14] << 48) | ((uint64_t)key[15] << 56);

    ctx->R[0] = ((uint64_t)nonce[0] << 0) | ((uint64_t)nonce[1] << 8) |
                ((uint64_t)nonce[2] << 16) | ((uint64_t)nonce[3] << 24) |
                ((uint64_t)nonce[4] << 32) | ((uint64_t)nonce[5] << 40) |
                ((uint64_t)nonce[6] << 48) | ((uint64_t)nonce[7] << 56);
    ctx->R[1] = ((uint64_t)nonce[8] << 0) | ((uint64_t)nonce[9] << 8) |
                ((uint64_t)nonce[10] << 16) | ((uint64_t)nonce[11] << 24) |
                ((uint64_t)nonce[12] << 32) | ((uint64_t)nonce[13] << 40) |
                ((uint64_t)nonce[14] << 48) | ((uint64_t)nonce[15] << 56);
    ctx->R[2] = ctx->K[0] ^ ctx->K[1];

    tinyjambu_permutation(ctx->R, ctx->rounds);

    ctx->mode = 0;

    return XY_TINYJAMBU_SUCCESS;
}

int xy_tinyjambu_128_decrypt_ad(xy_tinyjambu_128_ctx_t *ctx,
                                  const uint8_t *ad, size_t ad_len)
{
    uint8_t buffer[16];
    size_t i;

    if (!ctx || !ad) {
        return XY_TINYJAMBU_INVALID_PARAM;
    }

    ctx->ad_len = ad_len;
    i = 0;

    while (i < ad_len) {
        if (i + 16 <= ad_len) {
            tinyjambu_absorb(ctx->R, ad + i, ctx->K, ctx->rounds);
            i += 16;
        } else {
            memset(buffer, 0, 16);
            memcpy(buffer, ad + i, ad_len - i);
            tinyjambu_absorb(ctx->R, buffer, ctx->K, ctx->rounds);
            break;
        }
    }

    ctx->mode = 1;

    return XY_TINYJAMBU_SUCCESS;
}

int xy_tinyjambu_128_decrypt_update(xy_tinyjambu_128_ctx_t *ctx,
                                     const uint8_t *ciphertext, size_t ciphertext_len,
                                     uint8_t *plaintext)
{
    uint8_t buffer[16];
    size_t i, blocks;

    if (!ctx || !ciphertext || !plaintext) {
        return XY_TINYJAMBU_INVALID_PARAM;
    }

    ctx->plaintext_len = ciphertext_len;
    blocks = ciphertext_len / 16;

    for (i = 0; i < blocks; i++) {
        tinyjambu_decrypt_block(ctx->R, ciphertext + i * 16,
                                plaintext + i * 16, ctx->K, ctx->rounds);
    }

    if (ciphertext_len % 16) {
        memset(buffer, 0, 16);
        memcpy(buffer, ciphertext + blocks * 16, ciphertext_len % 16);
        tinyjambu_decrypt_block(ctx->R, buffer, buffer, ctx->K, ctx->rounds);
        memcpy(plaintext + blocks * 16, buffer, ciphertext_len % 16);
    }

    ctx->mode = 2;

    return XY_TINYJAMBU_SUCCESS;
}

int xy_tinyjambu_128_decrypt_final(xy_tinyjambu_128_ctx_t *ctx,
                                     const uint8_t tag[XY_TINYJAMBU_128_TAG_SIZE])
{
    uint8_t expected_tag[XY_TINYJAMBU_128_TAG_SIZE];
    size_t i;
    uint8_t diff;

    if (!ctx || !tag) {
        return XY_TINYJAMBU_INVALID_PARAM;
    }

    tinyjambu_finalize(ctx->R, ctx->K, expected_tag, 8, ctx->rounds);

    diff = 0;
    for (i = 0; i < XY_TINYJAMBU_128_TAG_SIZE; i++) {
        diff |= tag[i] ^ expected_tag[i];
    }

    ctx->mode = 3;

    if (diff) {
        return XY_TINYJAMBU_AUTH_FAILED;
    }

    return XY_TINYJAMBU_SUCCESS;
}
