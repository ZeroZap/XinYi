/**
 * @file xy_photon_beetle.c
 * @brief Photon Beetle Lightweight AEAD Implementation
 * @version 1.0.0
 * @date 2026-04-07
 *
 * Implementation of Photon Beetle AEAD and PHOTON-256 hash.
 * Based on the reference implementation from the Photon team.
 *
 * Note: Photon Beetle uses the PHOTON permutation which is based on
 * a 4x4 state array with 8-bit cells (256 bits total).
 */

#include <string.h>
#include "xy_photon_beetle.h"

/* ==================== Constants ==================== */

/* PHOTON permutation parameters */
#define PHOTON_NROUND 12
#define PHOTON_STATE_SIZE 32  /* 4x4 cells of 8 bits */

/* Constants for PHOTON S-box and diffusion */
static const uint8_t PHOTON_SBOX[256] = {
    0x72, 0x31, 0xd3, 0xd9, 0x90, 0x6a, 0x16, 0xc9,
    0x55, 0xda, 0xa1, 0x92, 0x4e, 0x26, 0xb4, 0x3e,
    0x2b, 0x17, 0xd1, 0x60, 0xc3, 0x59, 0x44, 0x0e,
    0x9d, 0x89, 0xb1, 0xf2, 0x06, 0x95, 0x4d, 0x61,
    0xf1, 0x0a, 0xc8, 0xa5, 0x5a, 0x3b, 0x67, 0x18,
    0x1f, 0x3f, 0x24, 0x8f, 0x46, 0x7a, 0xab, 0x05,
    0x25, 0x77, 0x4c, 0x93, 0x66, 0xe8, 0x9f, 0x12,
    0x8a, 0x4f, 0x9c, 0x51, 0x7b, 0xf5, 0x54, 0x15,
    0x8e, 0x32, 0x68, 0x53, 0x33, 0xf4, 0x6d, 0x30,
    0x1b, 0x5b, 0x64, 0x1d, 0x48, 0xfd, 0x7c, 0xb7,
    0x57, 0x7d, 0x2e, 0x1c, 0x3c, 0x58, 0xac, 0xa6,
    0x42, 0x22, 0x9b, 0xe0, 0x2d, 0x6c, 0x8c, 0x07,
    0xa2, 0xa4, 0x78, 0x47, 0xb0, 0x9e, 0x10, 0xe2,
    0xb6, 0xa0, 0x96, 0x79, 0x2f, 0x73, 0x23, 0x88,
    0x84, 0xdf, 0x4a, 0x1a, 0x8d, 0x3a, 0x91, 0x6b,
    0xf3, 0x28, 0x34, 0x29, 0xd5, 0x9a, 0xba, 0xcf,
    0x69, 0x5c, 0x39, 0x52, 0x8b, 0x2a, 0x70, 0x1e,
    0x50, 0x13, 0x0f, 0x76, 0xe4, 0x35, 0x86, 0xcb,
    0x75, 0x6f, 0x7e, 0x5e, 0xbd, 0x02, 0x5d, 0x49,
    0x98, 0x01, 0x36, 0x03, 0x43, 0xb9, 0x04, 0xe9,
    0xc0, 0xbf, 0xb8, 0x0b, 0xd0, 0xea, 0x56, 0x0d,
    0x6e, 0x19, 0x27, 0xaa, 0xbe, 0x7f, 0x08, 0xe7,
    0x2c, 0xc5, 0x97, 0x9a, 0x11, 0x3d, 0xb2, 0x20,
    0xee, 0x40, 0xe1, 0xc7, 0x14, 0xf6, 0x5f, 0x82,
    0xde, 0xf7, 0x4b, 0xb5, 0x74, 0x99, 0x65, 0xe5,
    0x41, 0xaf, 0x62, 0x21, 0x6a, 0x00, 0xdc, 0x37,
    0xf0, 0xcc, 0x94, 0xc6, 0x38, 0xbc, 0xeb, 0xc1,
    0x85, 0xb3, 0xe6, 0x45, 0x71, 0xae, 0xa3, 0xc2,
    0xe3, 0x87, 0x63, 0x0c, 0xa8, 0xa9, 0xfa, 0xfb,
    0xdb, 0xbb, 0xfe, 0xce, 0xca, 0xa7, 0xf8, 0x83,
    0x80, 0xcd, 0x81, 0x22, 0xdd, 0xf9, 0xd2, 0x8a,
    0xd8, 0x45, 0x77, 0x44, 0x37, 0xb5, 0x8c, 0x22,
    0x31, 0x5b, 0x09, 0x6e, 0xa4, 0x79, 0x1a, 0x98
};

/* Diffusion matrix for PHOTON (column-wise) */
static const uint8_t PHOTON_MATRIX[16] = {
    0x01, 0x01, 0x03, 0x07,
    0x0f, 0x0e, 0x0d, 0x0b,
    0x05, 0x0b, 0x07, 0x03,
    0x06, 0x0c, 0x08, 0x01
};

/* Round constants for PHOTON */
static const uint8_t PHOTON_RC[12] = {
    0x00, 0x01, 0x02, 0x04,
    0x08, 0x10, 0x20, 0x11,
    0x22, 0x05, 0x0a, 0x14
};

/* ==================== Helper Functions ==================== */

/**
 * @brief Apply PHOTON S-box to all cells
 */
static inline uint8_t photon_sbox(uint8_t x)
{
    return PHOTON_SBOX[x];
}

/**
 * @brief PHOTON permutation
 *
 * Operates on a 4x4 state array of 8-bit cells.
 */
static void photon_permutation(uint8_t state[PHOTON_STATE_SIZE])
{
    uint8_t temp[PHOTON_STATE_SIZE];
    uint8_t new_state[PHOTON_STATE_SIZE];
    int r, c, i;

    /* Copy state */
    for (i = 0; i < 16; i++) {
        temp[i] = state[i];
    }

    for (r = 0; r < PHOTON_NROUND; r++) {
        /* Step 1: SubCells (S-box) */
        for (i = 0; i < 16; i++) {
            new_state[i] = photon_sbox(temp[i]);
        }

        /* Step 2: AddConstants */
        new_state[0] ^= PHOTON_RC[r];

        /* Step 3: ShiftRows (rotate left each row) */
        temp[0] = new_state[0];
        temp[1] = new_state[5];
        temp[2] = new_state[10];
        temp[3] = new_state[15];
        temp[4] = new_state[4];
        temp[5] = new_state[9];
        temp[6] = new_state[14];
        temp[7] = new_state[3];
        temp[8] = new_state[8];
        temp[9] = new_state[13];
        temp[10] = new_state[2];
        temp[11] = new_state[7];
        temp[12] = new_state[12];
        temp[13] = new_state[1];
        temp[14] = new_state[6];
        temp[15] = new_state[11];

        /* Step 4: MixColumns (FCS multiplication) */
        for (c = 0; c < 4; c++) {
            uint8_t col[4];
            col[0] = temp[c];
            col[1] = temp[c + 4];
            col[2] = temp[c + 8];
            col[3] = temp[c + 12];

            new_state[c] = PHOTON_MATRIX[0] * col[0] ^
                           PHOTON_MATRIX[1] * col[1] ^
                           PHOTON_MATRIX[2] * col[2] ^
                           PHOTON_MATRIX[3] * col[3];
            new_state[c + 4] = PHOTON_MATRIX[4] * col[0] ^
                               PHOTON_MATRIX[5] * col[1] ^
                               PHOTON_MATRIX[6] * col[2] ^
                               PHOTON_MATRIX[7] * col[3];
            new_state[c + 8] = PHOTON_MATRIX[8] * col[0] ^
                               PHOTON_MATRIX[9] * col[1] ^
                               PHOTON_MATRIX[10] * col[2] ^
                               PHOTON_MATRIX[11] * col[3];
            new_state[c + 12] = PHOTON_MATRIX[12] * col[0] ^
                                PHOTON_MATRIX[13] * col[1] ^
                                PHOTON_MATRIX[14] * col[2] ^
                                PHOTON_MATRIX[15] * col[3];
        }

        /* Copy back */
        for (i = 0; i < 16; i++) {
            temp[i] = new_state[i];
        }
    }

    /* Copy final state */
    for (i = 0; i < 16; i++) {
        state[i] = temp[i];
    }
}

/* ==================== Photon Beetle Core Functions ==================== */

/**
 * @brief Initialize state with key and nonce
 */
static void photon_beetle_init(uint8_t state[PHOTON_STATE_SIZE],
                               const uint8_t *key,
                               const uint8_t *nonce)
{
    int i;

    /* Clear state */
    memset(state, 0, PHOTON_STATE_SIZE);

    /* Load key and nonce into state */
    /* State format for Photon-Beetle:
     * Bytes 0-15: rate portion (for nonce/key injection)
     * Bytes 16-31: capacity/key portion
     */

    /* Inject key into capacity */
    for (i = 0; i < 16; i++) {
        state[16 + i] = key[i];
    }

    /* Inject nonce into rate */
    for (i = 0; i < 16; i++) {
        state[i] = nonce[i];
    }

    /* XOR key into rate */
    for (i = 0; i < 16; i++) {
        state[i] ^= key[i];
    }

    /* Permutation */
    photon_permutation(state);

    /* XOR key into capacity again */
    for (i = 0; i < 16; i++) {
        state[16 + i] ^= key[i];
    }
}

/**
 * @brief Absorb rate bytes into state
 */
static void photon_beetle_absorb(uint8_t state[PHOTON_STATE_SIZE],
                                  const uint8_t *data, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        state[i] ^= data[i];
    }
    photon_permutation(state);
}

/**
 * @brief Squeeze rate bytes from state
 */
static void photon_beetle_squeeze(uint8_t state[PHOTON_STATE_SIZE],
                                   uint8_t *data, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        data[i] = state[i];
    }
}

/**
 * @brief Pad data block
 */
static void photon_beetle_pad(uint8_t block[16], size_t offset)
{
    memset(block, 0, 16);
    block[offset] = 0x80;
}

/* ==================== Photon Beetle AEAD ==================== */

int xy_photon_beetle_encrypt(const uint8_t *key,
                              const uint8_t *nonce,
                              const uint8_t *ad, size_t ad_len,
                              const uint8_t *plaintext, size_t plaintext_len,
                              uint8_t *ciphertext,
                              uint8_t tag[XY_PHOTON_BEETLE_TAG_SIZE])
{
    uint8_t state[PHOTON_STATE_SIZE];
    uint8_t buffer[16];
    size_t i;
    size_t blocks;

    if (!key || !nonce || !ciphertext || !tag) {
        return XY_PHOTON_BEETLE_INVALID_PARAM;
    }

    /* Initialize */
    photon_beetle_init(state, key, nonce);

    /* Absorb associated data */
    if (ad && ad_len > 0) {
        i = 0;
        while (i < ad_len) {
            if (i + 16 <= ad_len) {
                photon_beetle_absorb(state, ad + i, 16);
                i += 16;
            } else {
                memset(buffer, 0, 16);
                memcpy(buffer, ad + i, ad_len - i);
                buffer[ad_len - i] = 0x80;
                photon_beetle_absorb(state, buffer, 16);
                break;
            }
        }
    } else {
        /* Empty AD: absorb padding */
        memset(buffer, 0, 16);
        buffer[0] = 0x80;
        photon_beetle_absorb(state, buffer, 16);
    }

    /* Encrypt plaintext */
    blocks = plaintext_len / 16;
    for (i = 0; i < blocks; i++) {
        /* XOR plaintext into rate */
        int j;
        for (j = 0; j < 16; j++) {
            state[j] ^= plaintext[i * 16 + j];
        }
        /* Copy to ciphertext */
        memcpy(ciphertext + i * 16, state, 16);
        /* Permutation */
        photon_permutation(state);
    }

    /* Handle remaining bytes */
    if (plaintext_len % 16) {
        size_t rem = plaintext_len % 16;
        memset(buffer, 0, 16);
        memcpy(buffer, plaintext + blocks * 16, rem);
        /* XOR plaintext into state */
        for (i = 0; i < rem; i++) {
            state[i] ^= buffer[i];
        }
        memcpy(ciphertext + blocks * 16, state, rem);
        /* Pad and permute */
        buffer[rem] = 0x80;
        photon_beetle_absorb(state, buffer, 16);
    }

    /* Domain separation */
    state[15] ^= 0x01;
    photon_permutation(state);

    /* Extract tag */
    memcpy(tag, state + 16, 16);

    return XY_PHOTON_BEETLE_SUCCESS;
}

int xy_photon_beetle_decrypt(const uint8_t *key,
                              const uint8_t *nonce,
                              const uint8_t *ad, size_t ad_len,
                              const uint8_t *ciphertext, size_t ciphertext_len,
                              uint8_t *plaintext,
                              const uint8_t tag[XY_PHOTON_BEETLE_TAG_SIZE])
{
    uint8_t state[PHOTON_STATE_SIZE];
    uint8_t buffer[16];
    uint8_t expected_tag[16];
    size_t i;
    size_t blocks;
    uint8_t diff;

    if (!key || !nonce || !ciphertext || !plaintext || !tag) {
        return XY_PHOTON_BEETLE_INVALID_PARAM;
    }

    /* Initialize */
    photon_beetle_init(state, key, nonce);

    /* Absorb associated data */
    if (ad && ad_len > 0) {
        i = 0;
        while (i < ad_len) {
            if (i + 16 <= ad_len) {
                photon_beetle_absorb(state, ad + i, 16);
                i += 16;
            } else {
                memset(buffer, 0, 16);
                memcpy(buffer, ad + i, ad_len - i);
                buffer[ad_len - i] = 0x80;
                photon_beetle_absorb(state, buffer, 16);
                break;
            }
        }
    } else {
        memset(buffer, 0, 16);
        buffer[0] = 0x80;
        photon_beetle_absorb(state, buffer, 16);
    }

    /* Decrypt ciphertext */
    blocks = ciphertext_len / 16;
    for (i = 0; i < blocks; i++) {
        int j;
        /* Copy ciphertext to state, then XOR with rate to decrypt */
        memcpy(buffer, ciphertext + i * 16, 16);
        for (j = 0; j < 16; j++) {
            plaintext[i * 16 + j] = state[j] ^ buffer[j];
            state[j] = buffer[j];
        }
        photon_permutation(state);
    }

    /* Handle remaining bytes */
    if (ciphertext_len % 16) {
        size_t rem = ciphertext_len % 16;
        memcpy(buffer, ciphertext + blocks * 16, rem);
        for (i = 0; i < rem; i++) {
            plaintext[blocks * 16 + i] = state[i] ^ buffer[i];
            state[i] = buffer[i];
        }
        buffer[rem] = 0x80;
        photon_beetle_absorb(state, buffer, 16);
    }

    /* Domain separation */
    state[15] ^= 0x01;
    photon_permutation(state);

    /* Extract expected tag */
    memcpy(expected_tag, state + 16, 16);

    /* Constant-time comparison */
    diff = 0;
    for (i = 0; i < 16; i++) {
        diff |= tag[i] ^ expected_tag[i];
    }

    if (diff) {
        memset(plaintext, 0, ciphertext_len);
        return XY_PHOTON_BEETLE_AUTH_FAILED;
    }

    return XY_PHOTON_BEETLE_SUCCESS;
}

int xy_photon_beetle_encrypt_tag64(const uint8_t *key,
                                    const uint8_t *nonce,
                                    const uint8_t *ad, size_t ad_len,
                                    const uint8_t *plaintext, size_t plaintext_len,
                                    uint8_t *ciphertext,
                                    uint8_t tag[XY_PHOTON_BEETLE_TAG_64_SIZE])
{
    uint8_t full_tag[16];
    int ret;

    ret = xy_photon_beetle_encrypt(key, nonce, ad, ad_len,
                                     plaintext, plaintext_len, ciphertext, full_tag);
    if (ret == XY_PHOTON_BEETLE_SUCCESS) {
        memcpy(tag, full_tag, 8);
    }

    return ret;
}

int xy_photon_beetle_decrypt_tag64(const uint8_t *key,
                                    const uint8_t *nonce,
                                    const uint8_t *ad, size_t ad_len,
                                    const uint8_t *ciphertext, size_t ciphertext_len,
                                    uint8_t *plaintext,
                                    const uint8_t tag[XY_PHOTON_BEETLE_TAG_64_SIZE])
{
    uint8_t full_tag[16];

    memcpy(full_tag, tag, 8);
    memset(full_tag + 8, 0, 8);

    return xy_photon_beetle_decrypt(key, nonce, ad, ad_len,
                                     ciphertext, ciphertext_len, plaintext, full_tag);
}

/* ==================== PHOTON Hash ==================== */

/**
 * @brief Initialize hash state
 */
static void photon_hash_init(uint8_t state[PHOTON_STATE_SIZE])
{
    memset(state, 0, PHOTON_STATE_SIZE);
    state[0] = 0x01;  /* Hash domain separation */
}

/**
 * @brief Absorb data for hashing
 */
static void photon_hash_absorb(uint8_t state[PHOTON_STATE_SIZE],
                                const uint8_t *data, size_t len)
{
    size_t i = 0;

    while (i < len) {
        if (i + 16 <= len) {
            photon_beetle_absorb(state, data + i, 16);
            i += 16;
        } else {
            uint8_t buffer[16];
            memset(buffer, 0, 16);
            memcpy(buffer, data + i, len - i);
            buffer[len - i] = 0x80;
            photon_beetle_absorb(state, buffer, 16);
            break;
        }
    }
}

/**
 * @brief Squeeze hash output
 */
static void photon_hash_squeeze(uint8_t state[PHOTON_STATE_SIZE],
                                 uint8_t *hash, size_t hash_len)
{
    size_t i = 0;

    while (i < hash_len) {
        if (i + 16 <= hash_len) {
            photon_beetle_squeeze(state, hash + i, 16);
            i += 16;
            if (i < hash_len) {
                photon_permutation(state);
            }
        } else {
            uint8_t buffer[16];
            photon_beetle_squeeze(state, buffer, 16);
            memcpy(hash + i, buffer, hash_len - i);
            break;
        }
    }
}

int xy_photon_hash(const uint8_t *message, size_t message_len,
                   uint8_t hash[XY_PHOTON_HASH_SIZE])
{
    uint8_t state[PHOTON_STATE_SIZE];

    if (!message || !hash) {
        return XY_PHOTON_BEETLE_INVALID_PARAM;
    }

    /* Initialize */
    photon_hash_init(state);

    /* Absorb message */
    photon_hash_absorb(state, message, message_len);

    /* Squeeze hash output (256 bits = 32 bytes) */
    photon_hash_squeeze(state, hash, XY_PHOTON_HASH_SIZE);

    return XY_PHOTON_BEETLE_SUCCESS;
}

/* ==================== Incremental API ==================== */

int xy_photon_beetle_encrypt_init(xy_photon_beetle_ctx_t *ctx,
                                   const uint8_t *key,
                                   const uint8_t *nonce)
{
    if (!ctx || !key || !nonce) {
        return XY_PHOTON_BEETLE_INVALID_PARAM;
    }

    memset(ctx, 0, sizeof(xy_photon_beetle_ctx_t));
    memcpy(ctx->K, key, XY_PHOTON_BEETLE_KEY_SIZE);
    memcpy(ctx->N, nonce, XY_PHOTON_BEETLE_NONCE_SIZE);

    photon_beetle_init(ctx->S, key, nonce);

    ctx->mode = 0;

    return XY_PHOTON_BEETLE_SUCCESS;
}

int xy_photon_beetle_encrypt_ad(xy_photon_beetle_ctx_t *ctx,
                                  const uint8_t *ad, size_t ad_len)
{
    uint8_t buffer[16];
    size_t i;

    if (!ctx) {
        return XY_PHOTON_BEETLE_INVALID_PARAM;
    }

    ctx->ad_len = ad_len;
    i = 0;

    while (i < ad_len) {
        if (i + 16 <= ad_len) {
            photon_beetle_absorb(ctx->S, ad + i, 16);
            i += 16;
        } else {
            memset(buffer, 0, 16);
            memcpy(buffer, ad + i, ad_len - i);
            buffer[ad_len - i] = 0x80;
            photon_beetle_absorb(ctx->S, buffer, 16);
            break;
        }
    }

    ctx->mode = 1;

    return XY_PHOTON_BEETLE_SUCCESS;
}

int xy_photon_beetle_encrypt_update(xy_photon_beetle_ctx_t *ctx,
                                     const uint8_t *plaintext, size_t plaintext_len,
                                     uint8_t *ciphertext)
{
    uint8_t buffer[16];
    size_t i, blocks;

    if (!ctx || !plaintext || !ciphertext) {
        return XY_PHOTON_BEETLE_INVALID_PARAM;
    }

    ctx->plaintext_len = plaintext_len;
    blocks = plaintext_len / 16;

    for (i = 0; i < blocks; i++) {
        int j;
        for (j = 0; j < 16; j++) {
            ctx->S[j] ^= plaintext[i * 16 + j];
        }
        memcpy(ciphertext + i * 16, ctx->S, 16);
        photon_permutation(ctx->S);
    }

    if (plaintext_len % 16) {
        size_t rem = plaintext_len % 16;
        memset(buffer, 0, 16);
        memcpy(buffer, plaintext + blocks * 16, rem);
        for (i = 0; i < rem; i++) {
            ctx->S[i] ^= buffer[i];
        }
        memcpy(ciphertext + blocks * 16, ctx->S, rem);
        buffer[rem] = 0x80;
        photon_beetle_absorb(ctx->S, buffer, 16);
    }

    ctx->mode = 2;

    return XY_PHOTON_BEETLE_SUCCESS;
}

int xy_photon_beetle_encrypt_final(xy_photon_beetle_ctx_t *ctx,
                                     uint8_t tag[XY_PHOTON_BEETLE_TAG_SIZE])
{
    if (!ctx || !tag) {
        return XY_PHOTON_BEETLE_INVALID_PARAM;
    }

    ctx->S[15] ^= 0x01;
    photon_permutation(ctx->S);

    memcpy(tag, ctx->S + 16, 16);

    ctx->mode = 3;

    return XY_PHOTON_BEETLE_SUCCESS;
}

int xy_photon_beetle_decrypt_init(xy_photon_beetle_ctx_t *ctx,
                                   const uint8_t *key,
                                   const uint8_t *nonce)
{
    if (!ctx || !key || !nonce) {
        return XY_PHOTON_BEETLE_INVALID_PARAM;
    }

    memset(ctx, 0, sizeof(xy_photon_beetle_ctx_t));
    memcpy(ctx->K, key, XY_PHOTON_BEETLE_KEY_SIZE);
    memcpy(ctx->N, nonce, XY_PHOTON_BEETLE_NONCE_SIZE);

    photon_beetle_init(ctx->S, key, nonce);

    ctx->mode = 0;

    return XY_PHOTON_BEETLE_SUCCESS;
}

int xy_photon_beetle_decrypt_ad(xy_photon_beetle_ctx_t *ctx,
                                  const uint8_t *ad, size_t ad_len)
{
    uint8_t buffer[16];
    size_t i;

    if (!ctx) {
        return XY_PHOTON_BEETLE_INVALID_PARAM;
    }

    ctx->ad_len = ad_len;
    i = 0;

    while (i < ad_len) {
        if (i + 16 <= ad_len) {
            photon_beetle_absorb(ctx->S, ad + i, 16);
            i += 16;
        } else {
            memset(buffer, 0, 16);
            memcpy(buffer, ad + i, ad_len - i);
            buffer[ad_len - i] = 0x80;
            photon_beetle_absorb(ctx->S, buffer, 16);
            break;
        }
    }

    ctx->mode = 1;

    return XY_PHOTON_BEETLE_SUCCESS;
}

int xy_photon_beetle_decrypt_update(xy_photon_beetle_ctx_t *ctx,
                                     const uint8_t *ciphertext, size_t ciphertext_len,
                                     uint8_t *plaintext)
{
    uint8_t buffer[16];
    size_t i, blocks;

    if (!ctx || !ciphertext || !plaintext) {
        return XY_PHOTON_BEETLE_INVALID_PARAM;
    }

    ctx->plaintext_len = ciphertext_len;
    blocks = ciphertext_len / 16;

    for (i = 0; i < blocks; i++) {
        int j;
        memcpy(buffer, ciphertext + i * 16, 16);
        for (j = 0; j < 16; j++) {
            plaintext[i * 16 + j] = ctx->S[j] ^ buffer[j];
            ctx->S[j] = buffer[j];
        }
        photon_permutation(ctx->S);
    }

    if (ciphertext_len % 16) {
        size_t rem = ciphertext_len % 16;
        memcpy(buffer, ciphertext + blocks * 16, rem);
        for (i = 0; i < rem; i++) {
            plaintext[blocks * 16 + i] = ctx->S[i] ^ buffer[i];
            ctx->S[i] = buffer[i];
        }
        buffer[rem] = 0x80;
        photon_beetle_absorb(ctx->S, buffer, 16);
    }

    ctx->mode = 2;

    return XY_PHOTON_BEETLE_SUCCESS;
}

int xy_photon_beetle_decrypt_final(xy_photon_beetle_ctx_t *ctx,
                                     const uint8_t tag[XY_PHOTON_BEETLE_TAG_SIZE])
{
    uint8_t expected_tag[16];
    size_t i;
    uint8_t diff;

    if (!ctx || !tag) {
        return XY_PHOTON_BEETLE_INVALID_PARAM;
    }

    ctx->S[15] ^= 0x01;
    photon_permutation(ctx->S);

    memcpy(expected_tag, ctx->S + 16, 16);

    diff = 0;
    for (i = 0; i < 16; i++) {
        diff |= tag[i] ^ expected_tag[i];
    }

    ctx->mode = 3;

    if (diff) {
        return XY_PHOTON_BEETLE_AUTH_FAILED;
    }

    return XY_PHOTON_BEETLE_SUCCESS;
}
