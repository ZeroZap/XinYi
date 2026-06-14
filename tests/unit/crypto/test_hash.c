/**
 * @file test_hash.c
 * @brief Unit tests for MD5 and SHA-256 hash helpers.
 */

#include "xy_tiny_crypto.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

static const uint8_t md5_empty[XY_MD5_DIGEST_SIZE] = {
    0xd4, 0x1d, 0x8c, 0xd9, 0x8f, 0x00, 0xb2, 0x04,
    0xe9, 0x80, 0x09, 0x98, 0xec, 0xf8, 0x42, 0x7e,
};

static const uint8_t md5_abc[XY_MD5_DIGEST_SIZE] = {
    0x90, 0x01, 0x50, 0x98, 0x3c, 0xd2, 0x4f, 0xb0,
    0xd6, 0x96, 0x3f, 0x7d, 0x28, 0xe1, 0x7f, 0x72,
};

static const uint8_t sha256_empty[XY_SHA256_DIGEST_SIZE] = {
    0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14,
    0x9a, 0xfb, 0xf4, 0xc8, 0x99, 0x6f, 0xb9, 0x24,
    0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b, 0x93, 0x4c,
    0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55,
};

static const uint8_t sha256_abc[XY_SHA256_DIGEST_SIZE] = {
    0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
    0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
    0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
    0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad,
};

static void test_md5_validation_and_vectors(void)
{
    uint8_t digest[XY_MD5_DIGEST_SIZE] = {0};
    xy_md5_ctx_t ctx;

    assert(xy_md5_init(NULL) == XY_CRYPTO_INVALID_PARAM);
    assert(xy_md5_update(NULL, (const uint8_t *)"abc", 3) ==
           XY_CRYPTO_INVALID_PARAM);
    assert(xy_md5_final(NULL, digest) == XY_CRYPTO_INVALID_PARAM);
    assert(xy_md5_final(&ctx, NULL) == XY_CRYPTO_INVALID_PARAM);

    assert(xy_md5_hash((const uint8_t *)"", 0, digest) == XY_CRYPTO_SUCCESS);
    assert(memcmp(digest, md5_empty, sizeof(md5_empty)) == 0);

    assert(xy_md5_hash((const uint8_t *)"abc", 3, digest) ==
           XY_CRYPTO_SUCCESS);
    assert(memcmp(digest, md5_abc, sizeof(md5_abc)) == 0);
}

static void test_md5_incremental_matches_oneshot(void)
{
    const uint8_t msg[] = "message digest";
    uint8_t one_shot[XY_MD5_DIGEST_SIZE] = {0};
    uint8_t incremental[XY_MD5_DIGEST_SIZE] = {0};
    xy_md5_ctx_t ctx;

    assert(xy_md5_hash(msg, strlen((const char *)msg), one_shot) ==
           XY_CRYPTO_SUCCESS);
    assert(xy_md5_init(&ctx) == XY_CRYPTO_SUCCESS);
    assert(xy_md5_update(&ctx, msg, 7) == XY_CRYPTO_SUCCESS);
    assert(xy_md5_update(&ctx, msg + 7, strlen((const char *)msg) - 7) ==
           XY_CRYPTO_SUCCESS);
    assert(xy_md5_final(&ctx, incremental) == XY_CRYPTO_SUCCESS);
    assert(memcmp(one_shot, incremental, sizeof(one_shot)) == 0);
}

static void test_sha256_validation_and_vectors(void)
{
    uint8_t digest[XY_SHA256_DIGEST_SIZE] = {0};
    xy_sha256_ctx_t ctx;

    assert(xy_sha256_init(NULL) == XY_CRYPTO_INVALID_PARAM);
    assert(xy_sha256_update(NULL, (const uint8_t *)"abc", 3) ==
           XY_CRYPTO_INVALID_PARAM);
    assert(xy_sha256_final(NULL, digest) == XY_CRYPTO_INVALID_PARAM);
    assert(xy_sha256_final(&ctx, NULL) == XY_CRYPTO_INVALID_PARAM);

    assert(xy_sha256_hash((const uint8_t *)"", 0, digest) ==
           XY_CRYPTO_SUCCESS);
    assert(memcmp(digest, sha256_empty, sizeof(sha256_empty)) == 0);

    assert(xy_sha256_hash((const uint8_t *)"abc", 3, digest) ==
           XY_CRYPTO_SUCCESS);
    assert(memcmp(digest, sha256_abc, sizeof(sha256_abc)) == 0);
}

static void test_sha256_incremental_matches_oneshot(void)
{
    const uint8_t msg[] = "message digest";
    uint8_t one_shot[XY_SHA256_DIGEST_SIZE] = {0};
    uint8_t incremental[XY_SHA256_DIGEST_SIZE] = {0};
    xy_sha256_ctx_t ctx;

    assert(xy_sha256_hash(msg, strlen((const char *)msg), one_shot) ==
           XY_CRYPTO_SUCCESS);
    assert(xy_sha256_init(&ctx) == XY_CRYPTO_SUCCESS);
    assert(xy_sha256_update(&ctx, msg, 7) == XY_CRYPTO_SUCCESS);
    assert(xy_sha256_update(&ctx, msg + 7, strlen((const char *)msg) - 7) ==
           XY_CRYPTO_SUCCESS);
    assert(xy_sha256_final(&ctx, incremental) == XY_CRYPTO_SUCCESS);
    assert(memcmp(one_shot, incremental, sizeof(one_shot)) == 0);
}

int main(void)
{
    test_md5_validation_and_vectors();
    test_md5_incremental_matches_oneshot();
    test_sha256_validation_and_vectors();
    test_sha256_incremental_matches_oneshot();
    return 0;
}
