/**
 * @file xy_ascon.c
 * @brief Ascon Lightweight Authenticated Encryption Implementation
 * @version 1.0.0
 * @date 2026-04-07
 *
 * Implementation of Ascon-128, Ascon-128a, Ascon-80pq, and Ascon-Hash.
 * Based on the reference implementation from the Ascon team.
 */

#include <string.h>
#include "xy_ascon.h"

/* ==================== Constants ==================== */

/* Number of rounds for initialization and finalization */
#define ASCON_ROUNDS_I 12
#define ASCON_ROUNDS_F 12
#define ASCON_ROUNDS_FA 8   /* Faster variant finalization */

/* Number of rounds for rate operations */
#define ASCON_ROUNDS_A 12

/* Initialization rate for different modes */
#define ASCON_RATE 8        /* 64-bit rate for Ascon-128 */
#define ASCON_RATE_128A 16  /* 128-bit rate for Ascon-128a */

/* IV constants for different modes */
static const uint64_t ASCON_128_IV[] = {
    0xee0f01000c0c0c0cULL,  /* Ascon-128 IV */
    0x80000c0000000000ULL,
    0, 0, 0
};

static const uint64_t ASCON_128A_IV[] = {
    0xee0f01000c0c0c0cULL,  /* Ascon-128a IV */
    0x80010c0000000000ULL,
    0, 0, 0
};

static const uint64_t ASCON_80PQ_IV[] = {
    0xee0f01000c0c0c0cULL,  /* Ascon-80pq IV */
    0x80090c0000000000ULL,
    0, 0, 0
};

static const uint64_t ASCON_HASH_IV[] = {
    0xee0f01000c0c0c0cULL,  /* Ascon-Hash IV */
    0x8000000000000000ULL,
    0, 0, 0
};

/* ==================== Helper Functions ==================== */

/**
 * @brief Rotate left on 64-bit value
 */
static inline uint64_t ROTR(uint64_t x, int n)
{
    return (x >> n) | (x << (64 - n));
}

/**
 * @brief Ascon round function (p)
 */
static void ascon_p(uint64_t S[5], int rounds)
{
    uint64_t t0, t1, t2, t3, t4;
    uint64_t x0, x1, x2, x3, x4;

    x0 = S[0];
    x1 = S[1];
    x2 = S[2];
    x3 = S[3];
    x4 = S[4];

    for (int i = 0; i < rounds; i++) {
        /* S-box layer */
        x0 ^= x4;
        x4 ^= x3;
        x2 ^= x1;
        t0 = x0;
        t1 = x1;
        t2 = x2;
        t3 = x3;
        t4 = x4;
        x1 = t0 & ~t1;
        x2 = t1 & ~t2;
        x3 = t2 & ~t3;
        x4 = t3 & ~t4;
        x0 = t4 & ~t0;
        x1 ^= t2;
        x2 ^= t3;
        x3 ^= t4;
        x4 ^= t0;
        x0 ^= t1;

        /* Linear layer */
        x0 ^= ROTR(x0, 19) ^ ROTR(x0, 36);
        x1 ^= ROTR(x1, 19) ^ ROTR(x1, 36);
        x2 ^= ROTR(x2, 19) ^ ROTR(x2, 36);
        x3 ^= ROTR(x3, 19) ^ ROTR(x3, 36);
        x4 ^= ROTR(x4, 19) ^ ROTR(x4, 36);
    }

    S[0] = x0;
    S[1] = x1;
    S[2] = x2;
    S[3] = x3;
    S[4] = x4;
}

/**
 * @brief Initialize state with IV and key
 */
static void ascon_init(uint64_t S[5],
                        const uint64_t IV[5],
                        const uint8_t *key,
                        const uint8_t *nonce)
{
    (void)0;  /* Reserved for future use */

    /* Load IV */
    S[0] = IV[0];
    S[1] = IV[1];
    S[2] = IV[2];
    S[3] = IV[3];
    S[4] = IV[4];

    /* XOR key into state (key is always 128-bit = 2 words) */
    S[3] ^= ((uint64_t)key[0] << 0) | ((uint64_t)key[1] << 8) |
            ((uint64_t)key[2] << 16) | ((uint64_t)key[3] << 24) |
            ((uint64_t)key[4] << 32) | ((uint64_t)key[5] << 40) |
            ((uint64_t)key[6] << 48) | ((uint64_t)key[7] << 56);
    S[4] ^= ((uint64_t)key[8] << 0) | ((uint64_t)key[9] << 8) |
            ((uint64_t)key[10] << 16) | ((uint64_t)key[11] << 24) |
            ((uint64_t)key[12] << 32) | ((uint64_t)key[13] << 40) |
            ((uint64_t)key[14] << 48) | ((uint64_t)key[15] << 56);

    /* XOR nonce into state */
    S[3] ^= ((uint64_t)nonce[0] << 0) | ((uint64_t)nonce[1] << 8) |
            ((uint64_t)nonce[2] << 16) | ((uint64_t)nonce[3] << 24) |
            ((uint64_t)nonce[4] << 32) | ((uint64_t)nonce[5] << 40) |
            ((uint64_t)nonce[6] << 48) | ((uint64_t)nonce[7] << 56);
    S[4] ^= ((uint64_t)nonce[8] << 0) | ((uint64_t)nonce[9] << 8) |
            ((uint64_t)nonce[10] << 16) | ((uint64_t)nonce[11] << 24) |
            ((uint64_t)nonce[12] << 32) | ((uint64_t)nonce[13] << 40) |
            ((uint64_t)nonce[14] << 48) | ((uint64_t)nonce[15] << 56);

    /* Initialization phase */
    ascon_p(S, ASCON_ROUNDS_I);

    /* XOR key again after initialization */
    S[3] ^= ((uint64_t)key[0] << 0) | ((uint64_t)key[1] << 8) |
            ((uint64_t)key[2] << 16) | ((uint64_t)key[3] << 24) |
            ((uint64_t)key[4] << 32) | ((uint64_t)key[5] << 40) |
            ((uint64_t)key[6] << 48) | ((uint64_t)key[7] << 56);
    S[4] ^= ((uint64_t)key[8] << 0) | ((uint64_t)key[9] << 8) |
            ((uint64_t)key[10] << 16) | ((uint64_t)key[11] << 24) |
            ((uint64_t)key[12] << 32) | ((uint64_t)key[13] << 40) |
            ((uint64_t)key[14] << 48) | ((uint64_t)key[15] << 56);
}

/**
 * @brief Initialize state for Ascon-80pq (160-bit key)
 */
static void ascon_init_80pq(uint64_t S[5],
                             const uint64_t IV[5],
                             const uint8_t *key,
                             const uint8_t *nonce)
{
    /* Load IV */
    S[0] = IV[0];
    S[1] = IV[1];
    S[2] = IV[2];
    S[3] = IV[3];
    S[4] = IV[4];

    /* XOR 128-bit key into state */
    S[3] ^= ((uint64_t)key[0] << 0) | ((uint64_t)key[1] << 8) |
            ((uint64_t)key[2] << 16) | ((uint64_t)key[3] << 24) |
            ((uint64_t)key[4] << 32) | ((uint64_t)key[5] << 40) |
            ((uint64_t)key[6] << 48) | ((uint64_t)key[7] << 56);
    S[4] ^= ((uint64_t)key[8] << 0) | ((uint64_t)key[9] << 8) |
            ((uint64_t)key[10] << 16) | ((uint64_t)key[11] << 24) |
            ((uint64_t)key[12] << 32) | ((uint64_t)key[13] << 40) |
            ((uint64_t)key[14] << 48) | ((uint64_t)key[15] << 56);

    /* XOR 32-bit penetration key into MSB of S[0] */
    S[0] ^= ((uint64_t)key[16] << 0) | ((uint64_t)key[17] << 8) |
            ((uint64_t)key[18] << 16) | ((uint64_t)key[19] << 24);

    /* XOR nonce into state */
    S[3] ^= ((uint64_t)nonce[0] << 0) | ((uint64_t)nonce[1] << 8) |
            ((uint64_t)nonce[2] << 16) | ((uint64_t)nonce[3] << 24) |
            ((uint64_t)nonce[4] << 32) | ((uint64_t)nonce[5] << 40) |
            ((uint64_t)nonce[6] << 48) | ((uint64_t)nonce[7] << 56);
    S[4] ^= ((uint64_t)nonce[8] << 0) | ((uint64_t)nonce[9] << 8) |
            ((uint64_t)nonce[10] << 16) | ((uint64_t)nonce[11] << 24) |
            ((uint64_t)nonce[12] << 32) | ((uint64_t)nonce[13] << 40) |
            ((uint64_t)nonce[14] << 48) | ((uint64_t)nonce[15] << 56);

    /* Initialization phase */
    ascon_p(S, ASCON_ROUNDS_I);

    /* XOR key again */
    S[3] ^= ((uint64_t)key[0] << 0) | ((uint64_t)key[1] << 8) |
            ((uint64_t)key[2] << 16) | ((uint64_t)key[3] << 24) |
            ((uint64_t)key[4] << 32) | ((uint64_t)key[5] << 40) |
            ((uint64_t)key[6] << 48) | ((uint64_t)key[7] << 56);
    S[4] ^= ((uint64_t)key[8] << 0) | ((uint64_t)key[9] << 8) |
            ((uint64_t)key[10] << 16) | ((uint64_t)key[11] << 24) |
            ((uint64_t)key[12] << 32) | ((uint64_t)key[13] << 40) |
            ((uint64_t)key[14] << 48) | ((uint64_t)key[15] << 56);
    S[0] ^= ((uint64_t)key[16] << 0) | ((uint64_t)key[17] << 8) |
            ((uint64_t)key[18] << 16) | ((uint64_t)key[19] << 24);
}

/**
 * @brief Absorb data into rate portion of state
 */
static void ascon_absorb(uint64_t S[5], const uint8_t *data, int rate)
{
    if (rate == 8) {
        /* 64-bit rate */
        S[0] ^= ((uint64_t)data[0] << 0) | ((uint64_t)data[1] << 8) |
                ((uint64_t)data[2] << 16) | ((uint64_t)data[3] << 24) |
                ((uint64_t)data[4] << 32) | ((uint64_t)data[5] << 40) |
                ((uint64_t)data[6] << 48) | ((uint64_t)data[7] << 56);
    } else if (rate == 16) {
        /* 128-bit rate for Ascon-128a */
        S[0] ^= ((uint64_t)data[0] << 0) | ((uint64_t)data[1] << 8) |
                ((uint64_t)data[2] << 16) | ((uint64_t)data[3] << 24) |
                ((uint64_t)data[4] << 32) | ((uint64_t)data[5] << 40) |
                ((uint64_t)data[6] << 48) | ((uint64_t)data[7] << 56);
        S[1] ^= ((uint64_t)data[8] << 0) | ((uint64_t)data[9] << 8) |
                ((uint64_t)data[10] << 16) | ((uint64_t)data[11] << 24) |
                ((uint64_t)data[12] << 32) | ((uint64_t)data[13] << 40) |
                ((uint64_t)data[14] << 48) | ((uint64_t)data[15] << 56);
    }
}

/**
 * @brief Extract data from rate portion of state
 */
static void ascon_squeeze(uint8_t *data, uint64_t S[5], int rate)
{
    if (rate == 8) {
        /* 64-bit rate */
        data[0] = (uint8_t)(S[0] >> 0);
        data[1] = (uint8_t)(S[0] >> 8);
        data[2] = (uint8_t)(S[0] >> 16);
        data[3] = (uint8_t)(S[0] >> 24);
        data[4] = (uint8_t)(S[0] >> 32);
        data[5] = (uint8_t)(S[0] >> 40);
        data[6] = (uint8_t)(S[0] >> 48);
        data[7] = (uint8_t)(S[0] >> 56);
    } else if (rate == 16) {
        /* 128-bit rate for Ascon-128a */
        data[0] = (uint8_t)(S[0] >> 0);
        data[1] = (uint8_t)(S[0] >> 8);
        data[2] = (uint8_t)(S[0] >> 16);
        data[3] = (uint8_t)(S[0] >> 24);
        data[4] = (uint8_t)(S[0] >> 32);
        data[5] = (uint8_t)(S[0] >> 40);
        data[6] = (uint8_t)(S[0] >> 48);
        data[7] = (uint8_t)(S[0] >> 56);
        data[8] = (uint8_t)(S[1] >> 0);
        data[9] = (uint8_t)(S[1] >> 8);
        data[10] = (uint8_t)(S[1] >> 16);
        data[11] = (uint8_t)(S[1] >> 24);
        data[12] = (uint8_t)(S[1] >> 32);
        data[13] = (uint8_t)(S[1] >> 40);
        data[14] = (uint8_t)(S[1] >> 48);
        data[15] = (uint8_t)(S[1] >> 56);
    }
}

/**
 * @brief Pad data (Ascon padding rule: 10...0)
 */
static void ascon_pad(uint8_t *buffer, size_t len)
{
    buffer[len] = 0x80;
    memset(buffer + len + 1, 0, 7 - len);
}

/* ==================== Ascon-128 AEAD ==================== */

int xy_ascon_128_encrypt(const uint8_t *key,
                         const uint8_t *nonce,
                         const uint8_t *ad, size_t ad_len,
                         const uint8_t *plaintext, size_t plaintext_len,
                         uint8_t *ciphertext,
                         uint8_t tag[XY_ASCON_128_TAG_SIZE])
{
    uint64_t S[5];
    uint8_t buffer[16];
    size_t i;

    if (!key || !nonce || !ciphertext || !tag) {
        return XY_ASCON_INVALID_PARAM;
    }

    /* Initialize */
    ascon_init(S, ASCON_128_IV, key, nonce);

    /* Absorb associated data */
    if (ad && ad_len > 0) {
        i = 0;
        while (i < ad_len) {
            if (i + ASCON_RATE <= ad_len) {
                ascon_absorb(S, ad + i, ASCON_RATE);
                ascon_p(S, ASCON_ROUNDS_A);
                i += ASCON_RATE;
            } else {
                memset(buffer, 0, ASCON_RATE);
                memcpy(buffer, ad + i, ad_len - i);
                ascon_pad(buffer, ad_len - i);
                ascon_absorb(S, buffer, ASCON_RATE);
                ascon_p(S, ASCON_ROUNDS_A);
                break;
            }
        }
    } else {
        /* Domain separation: process empty AD */
        S[4] ^= 0x01;
        ascon_p(S, ASCON_ROUNDS_A);
    }

    /* Encrypt plaintext */
    i = 0;
    while (i < plaintext_len) {
        if (i + ASCON_RATE <= plaintext_len) {
            ascon_absorb(S, plaintext + i, ASCON_RATE);
            ascon_squeeze(ciphertext + i, S, ASCON_RATE);
            ciphertext[i + 0] ^= plaintext[i + 0];
            ciphertext[i + 1] ^= plaintext[i + 1];
            ciphertext[i + 2] ^= plaintext[i + 2];
            ciphertext[i + 3] ^= plaintext[i + 3];
            ciphertext[i + 4] ^= plaintext[i + 4];
            ciphertext[i + 5] ^= plaintext[i + 5];
            ciphertext[i + 6] ^= plaintext[i + 6];
            ciphertext[i + 7] ^= plaintext[i + 7];
            ascon_p(S, ASCON_ROUNDS_A);
            i += ASCON_RATE;
        } else {
            /* Last block */
            memset(buffer, 0, ASCON_RATE);
            memcpy(buffer, plaintext + i, plaintext_len - i);
            ascon_pad(buffer, plaintext_len - i);
            ascon_absorb(S, buffer, ASCON_RATE);
            ascon_squeeze(buffer, S, ASCON_RATE);
            memcpy(ciphertext + i, buffer, plaintext_len - i);
            ciphertext[i + 0] ^= plaintext[i + 0];
            ciphertext[i + 1] ^= plaintext[i + 1];
            ciphertext[i + 2] ^= plaintext[i + 2];
            ciphertext[i + 3] ^= plaintext[i + 3];
            ciphertext[i + 4] ^= plaintext[i + 4];
            ciphertext[i + 5] ^= plaintext[i + 5];
            ciphertext[i + 6] ^= plaintext[i + 6];
            ciphertext[i + 7] ^= plaintext[i + 7];
            break;
        }
    }

    /* Domain separation */
    S[4] ^= 0x01;

    /* Finalization */
    ascon_p(S, ASCON_ROUNDS_F);

    /* Extract tag */
    tag[0] = (uint8_t)(S[3] >> 0);
    tag[1] = (uint8_t)(S[3] >> 8);
    tag[2] = (uint8_t)(S[3] >> 16);
    tag[3] = (uint8_t)(S[3] >> 24);
    tag[4] = (uint8_t)(S[3] >> 32);
    tag[5] = (uint8_t)(S[3] >> 40);
    tag[6] = (uint8_t)(S[3] >> 48);
    tag[7] = (uint8_t)(S[3] >> 56);
    tag[8] = (uint8_t)(S[4] >> 0);
    tag[9] = (uint8_t)(S[4] >> 8);
    tag[10] = (uint8_t)(S[4] >> 16);
    tag[11] = (uint8_t)(S[4] >> 24);
    tag[12] = (uint8_t)(S[4] >> 32);
    tag[13] = (uint8_t)(S[4] >> 40);
    tag[14] = (uint8_t)(S[4] >> 48);
    tag[15] = (uint8_t)(S[4] >> 56);

    return XY_ASCON_SUCCESS;
}

int xy_ascon_128_decrypt(const uint8_t *key,
                         const uint8_t *nonce,
                         const uint8_t *ad, size_t ad_len,
                         const uint8_t *ciphertext, size_t ciphertext_len,
                         uint8_t *plaintext,
                         const uint8_t tag[XY_ASCON_128_TAG_SIZE])
{
    uint64_t S[5];
    uint8_t buffer[16];
    uint8_t expected_tag[XY_ASCON_128_TAG_SIZE];
    size_t i;

    if (!key || !nonce || !ciphertext || !plaintext || !tag) {
        return XY_ASCON_INVALID_PARAM;
    }

    /* Initialize */
    ascon_init(S, ASCON_128_IV, key, nonce);

    /* Absorb associated data */
    if (ad && ad_len > 0) {
        i = 0;
        while (i < ad_len) {
            if (i + ASCON_RATE <= ad_len) {
                ascon_absorb(S, ad + i, ASCON_RATE);
                ascon_p(S, ASCON_ROUNDS_A);
                i += ASCON_RATE;
            } else {
                memset(buffer, 0, ASCON_RATE);
                memcpy(buffer, ad + i, ad_len - i);
                ascon_pad(buffer, ad_len - i);
                ascon_absorb(S, buffer, ASCON_RATE);
                ascon_p(S, ASCON_ROUNDS_A);
                break;
            }
        }
    } else {
        S[4] ^= 0x01;
        ascon_p(S, ASCON_ROUNDS_A);
    }

    /* Decrypt ciphertext */
    i = 0;
    while (i < ciphertext_len) {
        if (i + ASCON_RATE <= ciphertext_len) {
            ascon_absorb(S, ciphertext + i, ASCON_RATE);
            ascon_squeeze(buffer, S, ASCON_RATE);
            plaintext[i + 0] = buffer[0] ^ ciphertext[i + 0];
            plaintext[i + 1] = buffer[1] ^ ciphertext[i + 1];
            plaintext[i + 2] = buffer[2] ^ ciphertext[i + 2];
            plaintext[i + 3] = buffer[3] ^ ciphertext[i + 3];
            plaintext[i + 4] = buffer[4] ^ ciphertext[i + 4];
            plaintext[i + 5] = buffer[5] ^ ciphertext[i + 5];
            plaintext[i + 6] = buffer[6] ^ ciphertext[i + 6];
            plaintext[i + 7] = buffer[7] ^ ciphertext[i + 7];
            ascon_p(S, ASCON_ROUNDS_A);
            i += ASCON_RATE;
        } else {
            memset(buffer, 0, ASCON_RATE);
            memcpy(buffer, ciphertext + i, ciphertext_len - i);
            ascon_pad(buffer, ciphertext_len - i);
            ascon_absorb(S, buffer, ASCON_RATE);
            ascon_squeeze(buffer, S, ASCON_RATE);
            plaintext[i + 0] = buffer[0] ^ ciphertext[i + 0];
            plaintext[i + 1] = buffer[1] ^ ciphertext[i + 1];
            plaintext[i + 2] = buffer[2] ^ ciphertext[i + 2];
            plaintext[i + 3] = buffer[3] ^ ciphertext[i + 3];
            plaintext[i + 4] = buffer[4] ^ ciphertext[i + 4];
            plaintext[i + 5] = buffer[5] ^ ciphertext[i + 5];
            plaintext[i + 6] = buffer[6] ^ ciphertext[i + 6];
            plaintext[i + 7] = buffer[7] ^ ciphertext[i + 7];
            break;
        }
    }

    /* Domain separation */
    S[4] ^= 0x01;

    /* Finalization */
    ascon_p(S, ASCON_ROUNDS_F);

    /* Extract tag and compare */
    expected_tag[0] = (uint8_t)(S[3] >> 0);
    expected_tag[1] = (uint8_t)(S[3] >> 8);
    expected_tag[2] = (uint8_t)(S[3] >> 16);
    expected_tag[3] = (uint8_t)(S[3] >> 24);
    expected_tag[4] = (uint8_t)(S[3] >> 32);
    expected_tag[5] = (uint8_t)(S[3] >> 40);
    expected_tag[6] = (uint8_t)(S[3] >> 48);
    expected_tag[7] = (uint8_t)(S[3] >> 56);
    expected_tag[8] = (uint8_t)(S[4] >> 0);
    expected_tag[9] = (uint8_t)(S[4] >> 8);
    expected_tag[10] = (uint8_t)(S[4] >> 16);
    expected_tag[11] = (uint8_t)(S[4] >> 24);
    expected_tag[12] = (uint8_t)(S[4] >> 32);
    expected_tag[13] = (uint8_t)(S[4] >> 40);
    expected_tag[14] = (uint8_t)(S[4] >> 48);
    expected_tag[15] = (uint8_t)(S[4] >> 56);

    /* Constant-time comparison */
    uint8_t diff = 0;
    for (i = 0; i < XY_ASCON_128_TAG_SIZE; i++) {
        diff |= tag[i] ^ expected_tag[i];
    }

    if (diff) {
        /* Authentication failed: wipe plaintext */
        memset(plaintext, 0, ciphertext_len);
        return XY_ASCON_AUTH_FAILED;
    }

    return XY_ASCON_SUCCESS;
}

/* ==================== Ascon-128a AEAD (Faster) ==================== */

int xy_ascon_128a_encrypt(const uint8_t *key,
                          const uint8_t *nonce,
                          const uint8_t *ad, size_t ad_len,
                          const uint8_t *plaintext, size_t plaintext_len,
                          uint8_t *ciphertext,
                          uint8_t tag[XY_ASCON_128A_TAG_SIZE])
{
    uint64_t S[5];
    uint8_t buffer[16];
    size_t i;

    if (!key || !nonce || !ciphertext || !tag) {
        return XY_ASCON_INVALID_PARAM;
    }

    /* Initialize with 128a IV */
    ascon_init(S, ASCON_128A_IV, key, nonce);

    /* Absorb associated data (64-bit rate for AD) */
    if (ad && ad_len > 0) {
        i = 0;
        while (i < ad_len) {
            if (i + ASCON_RATE <= ad_len) {
                ascon_absorb(S, ad + i, ASCON_RATE);
                ascon_p(S, ASCON_ROUNDS_A);
                i += ASCON_RATE;
            } else {
                memset(buffer, 0, ASCON_RATE);
                memcpy(buffer, ad + i, ad_len - i);
                ascon_pad(buffer, ad_len - i);
                ascon_absorb(S, buffer, ASCON_RATE);
                ascon_p(S, ASCON_ROUNDS_A);
                break;
            }
        }
    } else {
        S[4] ^= 0x01;
        ascon_p(S, ASCON_ROUNDS_A);
    }

    /* Encrypt plaintext (128-bit rate) */
    i = 0;
    while (i < plaintext_len) {
        if (i + ASCON_RATE_128A <= plaintext_len) {
            ascon_absorb(S, plaintext + i, ASCON_RATE_128A);
            ascon_squeeze(ciphertext + i, S, ASCON_RATE_128A);
            ciphertext[i + 0] ^= plaintext[i + 0];
            ciphertext[i + 1] ^= plaintext[i + 1];
            ciphertext[i + 2] ^= plaintext[i + 2];
            ciphertext[i + 3] ^= plaintext[i + 3];
            ciphertext[i + 4] ^= plaintext[i + 4];
            ciphertext[i + 5] ^= plaintext[i + 5];
            ciphertext[i + 6] ^= plaintext[i + 6];
            ciphertext[i + 7] ^= plaintext[i + 7];
            ciphertext[i + 8] ^= plaintext[i + 8];
            ciphertext[i + 9] ^= plaintext[i + 9];
            ciphertext[i + 10] ^= plaintext[i + 10];
            ciphertext[i + 11] ^= plaintext[i + 11];
            ciphertext[i + 12] ^= plaintext[i + 12];
            ciphertext[i + 13] ^= plaintext[i + 13];
            ciphertext[i + 14] ^= plaintext[i + 14];
            ciphertext[i + 15] ^= plaintext[i + 15];
            ascon_p(S, ASCON_ROUNDS_A);
            i += ASCON_RATE_128A;
        } else {
            memset(buffer, 0, ASCON_RATE_128A);
            memcpy(buffer, plaintext + i, plaintext_len - i);
            ascon_pad(buffer, plaintext_len - i);
            ascon_absorb(S, buffer, ASCON_RATE_128A);
            ascon_squeeze(buffer, S, ASCON_RATE_128A);
            memcpy(ciphertext + i, buffer, plaintext_len - i);
            ciphertext[i + 0] ^= plaintext[i + 0];
            ciphertext[i + 1] ^= plaintext[i + 1];
            ciphertext[i + 2] ^= plaintext[i + 2];
            ciphertext[i + 3] ^= plaintext[i + 3];
            ciphertext[i + 4] ^= plaintext[i + 4];
            ciphertext[i + 5] ^= plaintext[i + 5];
            ciphertext[i + 6] ^= plaintext[i + 6];
            ciphertext[i + 7] ^= plaintext[i + 7];
            ciphertext[i + 8] ^= plaintext[i + 8];
            ciphertext[i + 9] ^= plaintext[i + 9];
            ciphertext[i + 10] ^= plaintext[i + 10];
            ciphertext[i + 11] ^= plaintext[i + 11];
            ciphertext[i + 12] ^= plaintext[i + 12];
            ciphertext[i + 13] ^= plaintext[i + 13];
            ciphertext[i + 14] ^= plaintext[i + 14];
            ciphertext[i + 15] ^= plaintext[i + 15];
            break;
        }
    }

    /* Domain separation */
    S[4] ^= 0x01;

    /* Finalization (fewer rounds) */
    ascon_p(S, ASCON_ROUNDS_FA);

    /* Extract tag */
    tag[0] = (uint8_t)(S[3] >> 0);
    tag[1] = (uint8_t)(S[3] >> 8);
    tag[2] = (uint8_t)(S[3] >> 16);
    tag[3] = (uint8_t)(S[3] >> 24);
    tag[4] = (uint8_t)(S[3] >> 32);
    tag[5] = (uint8_t)(S[3] >> 40);
    tag[6] = (uint8_t)(S[3] >> 48);
    tag[7] = (uint8_t)(S[3] >> 56);
    tag[8] = (uint8_t)(S[4] >> 0);
    tag[9] = (uint8_t)(S[4] >> 8);
    tag[10] = (uint8_t)(S[4] >> 16);
    tag[11] = (uint8_t)(S[4] >> 24);
    tag[12] = (uint8_t)(S[4] >> 32);
    tag[13] = (uint8_t)(S[4] >> 40);
    tag[14] = (uint8_t)(S[4] >> 48);
    tag[15] = (uint8_t)(S[4] >> 56);

    return XY_ASCON_SUCCESS;
}

int xy_ascon_128a_decrypt(const uint8_t *key,
                          const uint8_t *nonce,
                          const uint8_t *ad, size_t ad_len,
                          const uint8_t *ciphertext, size_t ciphertext_len,
                          uint8_t *plaintext,
                          const uint8_t tag[XY_ASCON_128A_TAG_SIZE])
{
    uint64_t S[5];
    uint8_t buffer[16];
    uint8_t expected_tag[XY_ASCON_128A_TAG_SIZE];
    size_t i;

    if (!key || !nonce || !ciphertext || !plaintext || !tag) {
        return XY_ASCON_INVALID_PARAM;
    }

    /* Initialize */
    ascon_init(S, ASCON_128A_IV, key, nonce);

    /* Absorb associated data */
    if (ad && ad_len > 0) {
        i = 0;
        while (i < ad_len) {
            if (i + ASCON_RATE <= ad_len) {
                ascon_absorb(S, ad + i, ASCON_RATE);
                ascon_p(S, ASCON_ROUNDS_A);
                i += ASCON_RATE;
            } else {
                memset(buffer, 0, ASCON_RATE);
                memcpy(buffer, ad + i, ad_len - i);
                ascon_pad(buffer, ad_len - i);
                ascon_absorb(S, buffer, ASCON_RATE);
                ascon_p(S, ASCON_ROUNDS_A);
                break;
            }
        }
    } else {
        S[4] ^= 0x01;
        ascon_p(S, ASCON_ROUNDS_A);
    }

    /* Decrypt ciphertext */
    i = 0;
    while (i < ciphertext_len) {
        if (i + ASCON_RATE_128A <= ciphertext_len) {
            ascon_absorb(S, ciphertext + i, ASCON_RATE_128A);
            ascon_squeeze(buffer, S, ASCON_RATE_128A);
            plaintext[i + 0] = buffer[0] ^ ciphertext[i + 0];
            plaintext[i + 1] = buffer[1] ^ ciphertext[i + 1];
            plaintext[i + 2] = buffer[2] ^ ciphertext[i + 2];
            plaintext[i + 3] = buffer[3] ^ ciphertext[i + 3];
            plaintext[i + 4] = buffer[4] ^ ciphertext[i + 4];
            plaintext[i + 5] = buffer[5] ^ ciphertext[i + 5];
            plaintext[i + 6] = buffer[6] ^ ciphertext[i + 6];
            plaintext[i + 7] = buffer[7] ^ ciphertext[i + 7];
            plaintext[i + 8] = buffer[8] ^ ciphertext[i + 8];
            plaintext[i + 9] = buffer[9] ^ ciphertext[i + 9];
            plaintext[i + 10] = buffer[10] ^ ciphertext[i + 10];
            plaintext[i + 11] = buffer[11] ^ ciphertext[i + 11];
            plaintext[i + 12] = buffer[12] ^ ciphertext[i + 12];
            plaintext[i + 13] = buffer[13] ^ ciphertext[i + 13];
            plaintext[i + 14] = buffer[14] ^ ciphertext[i + 14];
            plaintext[i + 15] = buffer[15] ^ ciphertext[i + 15];
            ascon_p(S, ASCON_ROUNDS_A);
            i += ASCON_RATE_128A;
        } else {
            memset(buffer, 0, ASCON_RATE_128A);
            memcpy(buffer, ciphertext + i, ciphertext_len - i);
            ascon_pad(buffer, ciphertext_len - i);
            ascon_absorb(S, buffer, ASCON_RATE_128A);
            ascon_squeeze(buffer, S, ASCON_RATE_128A);
            plaintext[i + 0] = buffer[0] ^ ciphertext[i + 0];
            plaintext[i + 1] = buffer[1] ^ ciphertext[i + 1];
            plaintext[i + 2] = buffer[2] ^ ciphertext[i + 2];
            plaintext[i + 3] = buffer[3] ^ ciphertext[i + 3];
            plaintext[i + 4] = buffer[4] ^ ciphertext[i + 4];
            plaintext[i + 5] = buffer[5] ^ ciphertext[i + 5];
            plaintext[i + 6] = buffer[6] ^ ciphertext[i + 6];
            plaintext[i + 7] = buffer[7] ^ ciphertext[i + 7];
            plaintext[i + 8] = buffer[8] ^ ciphertext[i + 8];
            plaintext[i + 9] = buffer[9] ^ ciphertext[i + 9];
            plaintext[i + 10] = buffer[10] ^ ciphertext[i + 10];
            plaintext[i + 11] = buffer[11] ^ ciphertext[i + 11];
            plaintext[i + 12] = buffer[12] ^ ciphertext[i + 12];
            plaintext[i + 13] = buffer[13] ^ ciphertext[i + 13];
            plaintext[i + 14] = buffer[14] ^ ciphertext[i + 14];
            plaintext[i + 15] = buffer[15] ^ ciphertext[i + 15];
            break;
        }
    }

    /* Domain separation */
    S[4] ^= 0x01;

    /* Finalization */
    ascon_p(S, ASCON_ROUNDS_FA);

    /* Extract and compare tag */
    expected_tag[0] = (uint8_t)(S[3] >> 0);
    expected_tag[1] = (uint8_t)(S[3] >> 8);
    expected_tag[2] = (uint8_t)(S[3] >> 16);
    expected_tag[3] = (uint8_t)(S[3] >> 24);
    expected_tag[4] = (uint8_t)(S[3] >> 32);
    expected_tag[5] = (uint8_t)(S[3] >> 40);
    expected_tag[6] = (uint8_t)(S[3] >> 48);
    expected_tag[7] = (uint8_t)(S[3] >> 56);
    expected_tag[8] = (uint8_t)(S[4] >> 0);
    expected_tag[9] = (uint8_t)(S[4] >> 8);
    expected_tag[10] = (uint8_t)(S[4] >> 16);
    expected_tag[11] = (uint8_t)(S[4] >> 24);
    expected_tag[12] = (uint8_t)(S[4] >> 32);
    expected_tag[13] = (uint8_t)(S[4] >> 40);
    expected_tag[14] = (uint8_t)(S[4] >> 48);
    expected_tag[15] = (uint8_t)(S[4] >> 56);

    uint8_t diff = 0;
    for (i = 0; i < XY_ASCON_128A_TAG_SIZE; i++) {
        diff |= tag[i] ^ expected_tag[i];
    }

    if (diff) {
        memset(plaintext, 0, ciphertext_len);
        return XY_ASCON_AUTH_FAILED;
    }

    return XY_ASCON_SUCCESS;
}

/* ==================== Ascon-80pq AEAD ==================== */

int xy_ascon_80pq_encrypt(const uint8_t *key,
                          const uint8_t *nonce,
                          const uint8_t *ad, size_t ad_len,
                          const uint8_t *plaintext, size_t plaintext_len,
                          uint8_t *ciphertext,
                          uint8_t tag[XY_ASCON_80PQ_TAG_SIZE])
{
    uint64_t S[5];
    uint8_t buffer[16];
    size_t i;

    if (!key || !nonce || !ciphertext || !tag) {
        return XY_ASCON_INVALID_PARAM;
    }

    /* Initialize with 80pq IV */
    ascon_init_80pq(S, ASCON_80PQ_IV, key, nonce);

    /* Absorb associated data */
    if (ad && ad_len > 0) {
        i = 0;
        while (i < ad_len) {
            if (i + ASCON_RATE <= ad_len) {
                ascon_absorb(S, ad + i, ASCON_RATE);
                ascon_p(S, ASCON_ROUNDS_A);
                i += ASCON_RATE;
            } else {
                memset(buffer, 0, ASCON_RATE);
                memcpy(buffer, ad + i, ad_len - i);
                ascon_pad(buffer, ad_len - i);
                ascon_absorb(S, buffer, ASCON_RATE);
                ascon_p(S, ASCON_ROUNDS_A);
                break;
            }
        }
    } else {
        S[4] ^= 0x01;
        ascon_p(S, ASCON_ROUNDS_A);
    }

    /* Encrypt plaintext */
    i = 0;
    while (i < plaintext_len) {
        if (i + ASCON_RATE <= plaintext_len) {
            ascon_absorb(S, plaintext + i, ASCON_RATE);
            ascon_squeeze(ciphertext + i, S, ASCON_RATE);
            ciphertext[i + 0] ^= plaintext[i + 0];
            ciphertext[i + 1] ^= plaintext[i + 1];
            ciphertext[i + 2] ^= plaintext[i + 2];
            ciphertext[i + 3] ^= plaintext[i + 3];
            ciphertext[i + 4] ^= plaintext[i + 4];
            ciphertext[i + 5] ^= plaintext[i + 5];
            ciphertext[i + 6] ^= plaintext[i + 6];
            ciphertext[i + 7] ^= plaintext[i + 7];
            ascon_p(S, ASCON_ROUNDS_A);
            i += ASCON_RATE;
        } else {
            memset(buffer, 0, ASCON_RATE);
            memcpy(buffer, plaintext + i, plaintext_len - i);
            ascon_pad(buffer, plaintext_len - i);
            ascon_absorb(S, buffer, ASCON_RATE);
            ascon_squeeze(buffer, S, ASCON_RATE);
            memcpy(ciphertext + i, buffer, plaintext_len - i);
            ciphertext[i + 0] ^= plaintext[i + 0];
            ciphertext[i + 1] ^= plaintext[i + 1];
            ciphertext[i + 2] ^= plaintext[i + 2];
            ciphertext[i + 3] ^= plaintext[i + 3];
            ciphertext[i + 4] ^= plaintext[i + 4];
            ciphertext[i + 5] ^= plaintext[i + 5];
            ciphertext[i + 6] ^= plaintext[i + 6];
            ciphertext[i + 7] ^= plaintext[i + 7];
            break;
        }
    }

    S[4] ^= 0x01;
    ascon_p(S, ASCON_ROUNDS_F);

    /* Extract tag */
    tag[0] = (uint8_t)(S[3] >> 0);
    tag[1] = (uint8_t)(S[3] >> 8);
    tag[2] = (uint8_t)(S[3] >> 16);
    tag[3] = (uint8_t)(S[3] >> 24);
    tag[4] = (uint8_t)(S[3] >> 32);
    tag[5] = (uint8_t)(S[3] >> 40);
    tag[6] = (uint8_t)(S[3] >> 48);
    tag[7] = (uint8_t)(S[3] >> 56);
    tag[8] = (uint8_t)(S[4] >> 0);
    tag[9] = (uint8_t)(S[4] >> 8);
    tag[10] = (uint8_t)(S[4] >> 16);
    tag[11] = (uint8_t)(S[4] >> 24);
    tag[12] = (uint8_t)(S[4] >> 32);
    tag[13] = (uint8_t)(S[4] >> 40);
    tag[14] = (uint8_t)(S[4] >> 48);
    tag[15] = (uint8_t)(S[4] >> 56);

    return XY_ASCON_SUCCESS;
}

int xy_ascon_80pq_decrypt(const uint8_t *key,
                          const uint8_t *nonce,
                          const uint8_t *ad, size_t ad_len,
                          const uint8_t *ciphertext, size_t ciphertext_len,
                          uint8_t *plaintext,
                          const uint8_t tag[XY_ASCON_80PQ_TAG_SIZE])
{
    uint64_t S[5];
    uint8_t buffer[16];
    uint8_t expected_tag[XY_ASCON_80PQ_TAG_SIZE];
    size_t i;

    if (!key || !nonce || !ciphertext || !plaintext || !tag) {
        return XY_ASCON_INVALID_PARAM;
    }

    /* Initialize */
    ascon_init_80pq(S, ASCON_80PQ_IV, key, nonce);

    /* Absorb associated data */
    if (ad && ad_len > 0) {
        i = 0;
        while (i < ad_len) {
            if (i + ASCON_RATE <= ad_len) {
                ascon_absorb(S, ad + i, ASCON_RATE);
                ascon_p(S, ASCON_ROUNDS_A);
                i += ASCON_RATE;
            } else {
                memset(buffer, 0, ASCON_RATE);
                memcpy(buffer, ad + i, ad_len - i);
                ascon_pad(buffer, ad_len - i);
                ascon_absorb(S, buffer, ASCON_RATE);
                ascon_p(S, ASCON_ROUNDS_A);
                break;
            }
        }
    } else {
        S[4] ^= 0x01;
        ascon_p(S, ASCON_ROUNDS_A);
    }

    /* Decrypt ciphertext */
    i = 0;
    while (i < ciphertext_len) {
        if (i + ASCON_RATE <= ciphertext_len) {
            ascon_absorb(S, ciphertext + i, ASCON_RATE);
            ascon_squeeze(buffer, S, ASCON_RATE);
            plaintext[i + 0] = buffer[0] ^ ciphertext[i + 0];
            plaintext[i + 1] = buffer[1] ^ ciphertext[i + 1];
            plaintext[i + 2] = buffer[2] ^ ciphertext[i + 2];
            plaintext[i + 3] = buffer[3] ^ ciphertext[i + 3];
            plaintext[i + 4] = buffer[4] ^ ciphertext[i + 4];
            plaintext[i + 5] = buffer[5] ^ ciphertext[i + 5];
            plaintext[i + 6] = buffer[6] ^ ciphertext[i + 6];
            plaintext[i + 7] = buffer[7] ^ ciphertext[i + 7];
            ascon_p(S, ASCON_ROUNDS_A);
            i += ASCON_RATE;
        } else {
            memset(buffer, 0, ASCON_RATE);
            memcpy(buffer, ciphertext + i, ciphertext_len - i);
            ascon_pad(buffer, ciphertext_len - i);
            ascon_absorb(S, buffer, ASCON_RATE);
            ascon_squeeze(buffer, S, ASCON_RATE);
            plaintext[i + 0] = buffer[0] ^ ciphertext[i + 0];
            plaintext[i + 1] = buffer[1] ^ ciphertext[i + 1];
            plaintext[i + 2] = buffer[2] ^ ciphertext[i + 2];
            plaintext[i + 3] = buffer[3] ^ ciphertext[i + 3];
            plaintext[i + 4] = buffer[4] ^ ciphertext[i + 4];
            plaintext[i + 5] = buffer[5] ^ ciphertext[i + 5];
            plaintext[i + 6] = buffer[6] ^ ciphertext[i + 6];
            plaintext[i + 7] = buffer[7] ^ ciphertext[i + 7];
            break;
        }
    }

    S[4] ^= 0x01;
    ascon_p(S, ASCON_ROUNDS_F);

    /* Extract and compare tag */
    expected_tag[0] = (uint8_t)(S[3] >> 0);
    expected_tag[1] = (uint8_t)(S[3] >> 8);
    expected_tag[2] = (uint8_t)(S[3] >> 16);
    expected_tag[3] = (uint8_t)(S[3] >> 24);
    expected_tag[4] = (uint8_t)(S[3] >> 32);
    expected_tag[5] = (uint8_t)(S[3] >> 40);
    expected_tag[6] = (uint8_t)(S[3] >> 48);
    expected_tag[7] = (uint8_t)(S[3] >> 56);
    expected_tag[8] = (uint8_t)(S[4] >> 0);
    expected_tag[9] = (uint8_t)(S[4] >> 8);
    expected_tag[10] = (uint8_t)(S[4] >> 16);
    expected_tag[11] = (uint8_t)(S[4] >> 24);
    expected_tag[12] = (uint8_t)(S[4] >> 32);
    expected_tag[13] = (uint8_t)(S[4] >> 40);
    expected_tag[14] = (uint8_t)(S[4] >> 48);
    expected_tag[15] = (uint8_t)(S[4] >> 56);

    uint8_t diff = 0;
    for (i = 0; i < XY_ASCON_80PQ_TAG_SIZE; i++) {
        diff |= tag[i] ^ expected_tag[i];
    }

    if (diff) {
        memset(plaintext, 0, ciphertext_len);
        return XY_ASCON_AUTH_FAILED;
    }

    return XY_ASCON_SUCCESS;
}

/* ==================== Ascon-Hash ==================== */

/**
 * @brief Initialize hash state
 */
static void ascon_hash_init(uint64_t S[5])
{
    S[0] = ASCON_HASH_IV[0];
    S[1] = ASCON_HASH_IV[1];
    S[2] = ASCON_HASH_IV[2];
    S[3] = ASCON_HASH_IV[3];
    S[4] = ASCON_HASH_IV[4];
    ascon_p(S, 12);
}

/**
 * @brief Absorb rate bytes into state
 */
static void ascon_hash_absorb(uint64_t S[5], const uint8_t *data, int rate)
{
    if (rate == 8) {
        S[0] ^= ((uint64_t)data[0] << 0) | ((uint64_t)data[1] << 8) |
                ((uint64_t)data[2] << 16) | ((uint64_t)data[3] << 24) |
                ((uint64_t)data[4] << 32) | ((uint64_t)data[5] << 40) |
                ((uint64_t)data[6] << 48) | ((uint64_t)data[7] << 56);
    }
}

/**
 * @brief Squeeze and extract hash output
 */
static void ascon_hash_squeeze(uint8_t *data, uint64_t S[5], int rate)
{
    if (rate == 8) {
        data[0] = (uint8_t)(S[0] >> 0);
        data[1] = (uint8_t)(S[0] >> 8);
        data[2] = (uint8_t)(S[0] >> 16);
        data[3] = (uint8_t)(S[0] >> 24);
        data[4] = (uint8_t)(S[0] >> 32);
        data[5] = (uint8_t)(S[0] >> 40);
        data[6] = (uint8_t)(S[0] >> 48);
        data[7] = (uint8_t)(S[0] >> 56);
    }
}

int xy_ascon_hash(const uint8_t *message, size_t message_len,
                  uint8_t hash[XY_ASCON_HASH_SIZE])
{
    uint64_t S[5];
    uint8_t buffer[8];
    size_t i;
    size_t hash_len = XY_ASCON_HASH_SIZE;

    if (!message || !hash) {
        return XY_ASCON_INVALID_PARAM;
    }

    /* Initialize */
    ascon_hash_init(S);

    /* Absorb message */
    i = 0;
    while (i < message_len) {
        if (i + 8 <= message_len) {
            ascon_hash_absorb(S, message + i, 8);
            ascon_p(S, 12);
            i += 8;
        } else {
            memset(buffer, 0, 8);
            memcpy(buffer, message + i, message_len - i);
            buffer[message_len - i] = 0x80;
            ascon_hash_absorb(S, buffer, 8);
            ascon_p(S, 12);
            break;
        }
    }

    /* Squeeze hash output */
    i = 0;
    while (i < hash_len) {
        ascon_hash_squeeze(hash + i, S, 8);
        if (i + 8 < hash_len) {
            ascon_p(S, 12);
        }
        i += 8;
    }

    return XY_ASCON_SUCCESS;
}

/* ==================== Incremental API ==================== */

int xy_ascon_128a_encrypt_init(xy_ascon_128a_ctx_t *ctx,
                                const uint8_t *key,
                                const uint8_t *nonce)
{
    if (!ctx || !key || !nonce) {
        return XY_ASCON_INVALID_PARAM;
    }

    memset(ctx, 0, sizeof(xy_ascon_128a_ctx_t));
    ascon_init(ctx->S, ASCON_128A_IV, key, nonce);

    return XY_ASCON_SUCCESS;
}

int xy_ascon_128a_encrypt_ad(xy_ascon_128a_ctx_t *ctx,
                              const uint8_t *ad, size_t ad_len)
{
    uint8_t buffer[8];
    size_t i;

    if (!ctx) {
        return XY_ASCON_INVALID_PARAM;
    }

    ctx->ad_len = ad_len;
    i = 0;

    while (i < ad_len) {
        if (i + ASCON_RATE <= ad_len) {
            ascon_absorb(ctx->S, ad + i, ASCON_RATE);
            ascon_p(ctx->S, ASCON_ROUNDS_A);
            i += ASCON_RATE;
        } else {
            memset(buffer, 0, ASCON_RATE);
            memcpy(buffer, ad + i, ad_len - i);
            ascon_pad(buffer, ad_len - i);
            ascon_absorb(ctx->S, buffer, ASCON_RATE);
            ascon_p(ctx->S, ASCON_ROUNDS_A);
            break;
        }
    }

    ctx->ad_pos = i;
    ctx->mode = 0;

    return XY_ASCON_SUCCESS;
}

int xy_ascon_128a_encrypt_update(xy_ascon_128a_ctx_t *ctx,
                                  const uint8_t *plaintext, size_t plaintext_len,
                                  uint8_t *ciphertext)
{
    uint8_t buffer[16];
    size_t i;

    if (!ctx || !plaintext || !ciphertext) {
        return XY_ASCON_INVALID_PARAM;
    }

    if (ctx->mode == 0) {
        /* Switching from AD to plaintext */
        ctx->S[4] ^= 0x01;
        ascon_p(ctx->S, ASCON_ROUNDS_A);
        ctx->mode = 1;
    }

    ctx->plaintext_len = plaintext_len;
    i = 0;

    while (i < plaintext_len) {
        if (i + ASCON_RATE_128A <= plaintext_len) {
            ascon_absorb(ctx->S, plaintext + i, ASCON_RATE_128A);
            ascon_squeeze(ciphertext + i, ctx->S, ASCON_RATE_128A);
            ciphertext[i + 0] ^= plaintext[i + 0];
            ciphertext[i + 1] ^= plaintext[i + 1];
            ciphertext[i + 2] ^= plaintext[i + 2];
            ciphertext[i + 3] ^= plaintext[i + 3];
            ciphertext[i + 4] ^= plaintext[i + 4];
            ciphertext[i + 5] ^= plaintext[i + 5];
            ciphertext[i + 6] ^= plaintext[i + 6];
            ciphertext[i + 7] ^= plaintext[i + 7];
            ciphertext[i + 8] ^= plaintext[i + 8];
            ciphertext[i + 9] ^= plaintext[i + 9];
            ciphertext[i + 10] ^= plaintext[i + 10];
            ciphertext[i + 11] ^= plaintext[i + 11];
            ciphertext[i + 12] ^= plaintext[i + 12];
            ciphertext[i + 13] ^= plaintext[i + 13];
            ciphertext[i + 14] ^= plaintext[i + 14];
            ciphertext[i + 15] ^= plaintext[i + 15];
            ascon_p(ctx->S, ASCON_ROUNDS_A);
            i += ASCON_RATE_128A;
        } else {
            memset(buffer, 0, ASCON_RATE_128A);
            memcpy(buffer, plaintext + i, plaintext_len - i);
            ascon_pad(buffer, plaintext_len - i);
            ascon_absorb(ctx->S, buffer, ASCON_RATE_128A);
            ascon_squeeze(buffer, ctx->S, ASCON_RATE_128A);
            memcpy(ciphertext + i, buffer, plaintext_len - i);
            ciphertext[i + 0] ^= plaintext[i + 0];
            ciphertext[i + 1] ^= plaintext[i + 1];
            ciphertext[i + 2] ^= plaintext[i + 2];
            ciphertext[i + 3] ^= plaintext[i + 3];
            ciphertext[i + 4] ^= plaintext[i + 4];
            ciphertext[i + 5] ^= plaintext[i + 5];
            ciphertext[i + 6] ^= plaintext[i + 6];
            ciphertext[i + 7] ^= plaintext[i + 7];
            ciphertext[i + 8] ^= plaintext[i + 8];
            ciphertext[i + 9] ^= plaintext[i + 9];
            ciphertext[i + 10] ^= plaintext[i + 10];
            ciphertext[i + 11] ^= plaintext[i + 11];
            ciphertext[i + 12] ^= plaintext[i + 12];
            ciphertext[i + 13] ^= plaintext[i + 13];
            ciphertext[i + 14] ^= plaintext[i + 14];
            ciphertext[i + 15] ^= plaintext[i + 15];
            break;
        }
    }

    ctx->data_pos = i;
    ctx->mode = 1;

    return XY_ASCON_SUCCESS;
}

int xy_ascon_128a_encrypt_final(xy_ascon_128a_ctx_t *ctx,
                                 uint8_t tag[XY_ASCON_128A_TAG_SIZE])
{
    if (!ctx || !tag) {
        return XY_ASCON_INVALID_PARAM;
    }

    /* Domain separation */
    ctx->S[4] ^= 0x01;

    /* Finalization */
    ascon_p(ctx->S, ASCON_ROUNDS_FA);

    /* Extract tag */
    tag[0] = (uint8_t)(ctx->S[3] >> 0);
    tag[1] = (uint8_t)(ctx->S[3] >> 8);
    tag[2] = (uint8_t)(ctx->S[3] >> 16);
    tag[3] = (uint8_t)(ctx->S[3] >> 24);
    tag[4] = (uint8_t)(ctx->S[3] >> 32);
    tag[5] = (uint8_t)(ctx->S[3] >> 40);
    tag[6] = (uint8_t)(ctx->S[3] >> 48);
    tag[7] = (uint8_t)(ctx->S[3] >> 56);
    tag[8] = (uint8_t)(ctx->S[4] >> 0);
    tag[9] = (uint8_t)(ctx->S[4] >> 8);
    tag[10] = (uint8_t)(ctx->S[4] >> 16);
    tag[11] = (uint8_t)(ctx->S[4] >> 24);
    tag[12] = (uint8_t)(ctx->S[4] >> 32);
    tag[13] = (uint8_t)(ctx->S[4] >> 40);
    tag[14] = (uint8_t)(ctx->S[4] >> 48);
    tag[15] = (uint8_t)(ctx->S[4] >> 56);

    ctx->mode = 2;

    return XY_ASCON_SUCCESS;
}

int xy_ascon_128a_decrypt_init(xy_ascon_128a_ctx_t *ctx,
                                const uint8_t *key,
                                const uint8_t *nonce)
{
    if (!ctx || !key || !nonce) {
        return XY_ASCON_INVALID_PARAM;
    }

    memset(ctx, 0, sizeof(xy_ascon_128a_ctx_t));
    ascon_init(ctx->S, ASCON_128A_IV, key, nonce);

    return XY_ASCON_SUCCESS;
}

int xy_ascon_128a_decrypt_ad(xy_ascon_128a_ctx_t *ctx,
                              const uint8_t *ad, size_t ad_len)
{
    uint8_t buffer[8];
    size_t i;

    if (!ctx) {
        return XY_ASCON_INVALID_PARAM;
    }

    ctx->ad_len = ad_len;
    i = 0;

    while (i < ad_len) {
        if (i + ASCON_RATE <= ad_len) {
            ascon_absorb(ctx->S, ad + i, ASCON_RATE);
            ascon_p(ctx->S, ASCON_ROUNDS_A);
            i += ASCON_RATE;
        } else {
            memset(buffer, 0, ASCON_RATE);
            memcpy(buffer, ad + i, ad_len - i);
            ascon_pad(buffer, ad_len - i);
            ascon_absorb(ctx->S, buffer, ASCON_RATE);
            ascon_p(ctx->S, ASCON_ROUNDS_A);
            break;
        }
    }

    ctx->ad_pos = i;
    ctx->mode = 0;

    return XY_ASCON_SUCCESS;
}

int xy_ascon_128a_decrypt_update(xy_ascon_128a_ctx_t *ctx,
                                  const uint8_t *ciphertext, size_t ciphertext_len,
                                  uint8_t *plaintext)
{
    uint8_t buffer[16];
    size_t i;

    if (!ctx || !ciphertext || !plaintext) {
        return XY_ASCON_INVALID_PARAM;
    }

    if (ctx->mode == 0) {
        ctx->S[4] ^= 0x01;
        ascon_p(ctx->S, ASCON_ROUNDS_A);
        ctx->mode = 1;
    }

    ctx->plaintext_len = ciphertext_len;
    i = 0;

    while (i < ciphertext_len) {
        if (i + ASCON_RATE_128A <= ciphertext_len) {
            ascon_absorb(ctx->S, ciphertext + i, ASCON_RATE_128A);
            ascon_squeeze(buffer, ctx->S, ASCON_RATE_128A);
            plaintext[i + 0] = buffer[0] ^ ciphertext[i + 0];
            plaintext[i + 1] = buffer[1] ^ ciphertext[i + 1];
            plaintext[i + 2] = buffer[2] ^ ciphertext[i + 2];
            plaintext[i + 3] = buffer[3] ^ ciphertext[i + 3];
            plaintext[i + 4] = buffer[4] ^ ciphertext[i + 4];
            plaintext[i + 5] = buffer[5] ^ ciphertext[i + 5];
            plaintext[i + 6] = buffer[6] ^ ciphertext[i + 6];
            plaintext[i + 7] = buffer[7] ^ ciphertext[i + 7];
            plaintext[i + 8] = buffer[8] ^ ciphertext[i + 8];
            plaintext[i + 9] = buffer[9] ^ ciphertext[i + 9];
            plaintext[i + 10] = buffer[10] ^ ciphertext[i + 10];
            plaintext[i + 11] = buffer[11] ^ ciphertext[i + 11];
            plaintext[i + 12] = buffer[12] ^ ciphertext[i + 12];
            plaintext[i + 13] = buffer[13] ^ ciphertext[i + 13];
            plaintext[i + 14] = buffer[14] ^ ciphertext[i + 14];
            plaintext[i + 15] = buffer[15] ^ ciphertext[i + 15];
            ascon_p(ctx->S, ASCON_ROUNDS_A);
            i += ASCON_RATE_128A;
        } else {
            memset(buffer, 0, ASCON_RATE_128A);
            memcpy(buffer, ciphertext + i, ciphertext_len - i);
            ascon_pad(buffer, ciphertext_len - i);
            ascon_absorb(ctx->S, buffer, ASCON_RATE_128A);
            ascon_squeeze(buffer, ctx->S, ASCON_RATE_128A);
            plaintext[i + 0] = buffer[0] ^ ciphertext[i + 0];
            plaintext[i + 1] = buffer[1] ^ ciphertext[i + 1];
            plaintext[i + 2] = buffer[2] ^ ciphertext[i + 2];
            plaintext[i + 3] = buffer[3] ^ ciphertext[i + 3];
            plaintext[i + 4] = buffer[4] ^ ciphertext[i + 4];
            plaintext[i + 5] = buffer[5] ^ ciphertext[i + 5];
            plaintext[i + 6] = buffer[6] ^ ciphertext[i + 6];
            plaintext[i + 7] = buffer[7] ^ ciphertext[i + 7];
            plaintext[i + 8] = buffer[8] ^ ciphertext[i + 8];
            plaintext[i + 9] = buffer[9] ^ ciphertext[i + 9];
            plaintext[i + 10] = buffer[10] ^ ciphertext[i + 10];
            plaintext[i + 11] = buffer[11] ^ ciphertext[i + 11];
            plaintext[i + 12] = buffer[12] ^ ciphertext[i + 12];
            plaintext[i + 13] = buffer[13] ^ ciphertext[i + 13];
            plaintext[i + 14] = buffer[14] ^ ciphertext[i + 14];
            plaintext[i + 15] = buffer[15] ^ ciphertext[i + 15];
            break;
        }
    }

    ctx->data_pos = i;
    ctx->mode = 1;

    return XY_ASCON_SUCCESS;
}

int xy_ascon_128a_decrypt_final(xy_ascon_128a_ctx_t *ctx,
                                 const uint8_t tag[XY_ASCON_128A_TAG_SIZE])
{
    uint8_t expected_tag[XY_ASCON_128A_TAG_SIZE];
    size_t i;

    if (!ctx || !tag) {
        return XY_ASCON_INVALID_PARAM;
    }

    /* Domain separation */
    ctx->S[4] ^= 0x01;

    /* Finalization */
    ascon_p(ctx->S, ASCON_ROUNDS_FA);

    /* Extract expected tag */
    expected_tag[0] = (uint8_t)(ctx->S[3] >> 0);
    expected_tag[1] = (uint8_t)(ctx->S[3] >> 8);
    expected_tag[2] = (uint8_t)(ctx->S[3] >> 16);
    expected_tag[3] = (uint8_t)(ctx->S[3] >> 24);
    expected_tag[4] = (uint8_t)(ctx->S[3] >> 32);
    expected_tag[5] = (uint8_t)(ctx->S[3] >> 40);
    expected_tag[6] = (uint8_t)(ctx->S[3] >> 48);
    expected_tag[7] = (uint8_t)(ctx->S[3] >> 56);
    expected_tag[8] = (uint8_t)(ctx->S[4] >> 0);
    expected_tag[9] = (uint8_t)(ctx->S[4] >> 8);
    expected_tag[10] = (uint8_t)(ctx->S[4] >> 16);
    expected_tag[11] = (uint8_t)(ctx->S[4] >> 24);
    expected_tag[12] = (uint8_t)(ctx->S[4] >> 32);
    expected_tag[13] = (uint8_t)(ctx->S[4] >> 40);
    expected_tag[14] = (uint8_t)(ctx->S[4] >> 48);
    expected_tag[15] = (uint8_t)(ctx->S[4] >> 56);

    /* Constant-time comparison */
    uint8_t diff = 0;
    for (i = 0; i < XY_ASCON_128A_TAG_SIZE; i++) {
        diff |= tag[i] ^ expected_tag[i];
    }

    ctx->mode = 2;

    if (diff) {
        return XY_ASCON_AUTH_FAILED;
    }

    return XY_ASCON_SUCCESS;
}
