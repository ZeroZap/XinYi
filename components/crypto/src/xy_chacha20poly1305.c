/**
 * @file xy_chacha20poly1305.c
 * @brief Root ChaCha20-Poly1305 compatibility wrapper.
 *
 * Keep the compact historical root AEAD API in components/crypto/inc while
 * delegating arithmetic to the module-owned RFC 8439 implementation under
 * xy_chacha/. The module implementation is compiled in this translation unit
 * with private symbol names so its stronger API/ctx definitions do not collide
 * with the legacy root header names.
 */

#define xy_chacha20_ctx_t xy_module_chacha20_ctx_t
#define xy_chacha20_init xy_module_chacha20_init
#define xy_chacha20_crypt xy_module_chacha20_crypt
#define xy_poly1305_ctx_t xy_module_poly1305_ctx_t
#define xy_poly1305_init xy_module_poly1305_init
#define xy_poly1305_update xy_module_poly1305_update
#define xy_poly1305_finish xy_module_poly1305_finish
#define xy_chacha20_poly1305_encrypt xy_module_chacha20_poly1305_encrypt
#define xy_chacha20_poly1305_decrypt xy_module_chacha20_poly1305_decrypt
#include "../xy_chacha/xy_chacha20_poly1305.c"
#undef xy_chacha20_ctx_t
#undef xy_chacha20_init
#undef xy_chacha20_crypt
#undef xy_poly1305_ctx_t
#undef xy_poly1305_init
#undef xy_poly1305_update
#undef xy_poly1305_finish
#undef xy_chacha20_poly1305_encrypt
#undef xy_chacha20_poly1305_decrypt

#include "xy_chacha20poly1305.h"

#include <string.h>

#define XY_ROOT_CHACHA20POLY1305_TAG_SIZE 16U

int xy_chacha20poly1305_encrypt(const uint8_t *key, const uint8_t *nonce,
                                const uint8_t *aad, size_t aad_len,
                                const uint8_t *plaintext, size_t pt_len,
                                uint8_t *ciphertext, size_t *ct_len)
{
    uint8_t tag[XY_ROOT_CHACHA20POLY1305_TAG_SIZE];

    if (key == NULL || nonce == NULL || plaintext == NULL || ciphertext == NULL ||
        ct_len == NULL) {
        return -1;
    }
    if (aad_len > 0U && aad == NULL) {
        return -1;
    }
    if (*ct_len < pt_len + XY_ROOT_CHACHA20POLY1305_TAG_SIZE) {
        return -1;
    }
    if (xy_module_chacha20_poly1305_encrypt(key, nonce, aad, aad_len, plaintext, pt_len,
                                            ciphertext, tag) != 0) {
        return -1;
    }

    memcpy(&ciphertext[pt_len], tag, sizeof(tag));
    *ct_len = pt_len + XY_ROOT_CHACHA20POLY1305_TAG_SIZE;
    return 0;
}

int xy_chacha20poly1305_decrypt(const uint8_t *key, const uint8_t *nonce,
                                const uint8_t *aad, size_t aad_len,
                                const uint8_t *ciphertext, size_t ct_len,
                                uint8_t *plaintext, size_t *pt_len)
{
    size_t plaintext_len;

    if (key == NULL || nonce == NULL || ciphertext == NULL || plaintext == NULL ||
        pt_len == NULL) {
        return -1;
    }
    if (aad_len > 0U && aad == NULL) {
        return -1;
    }
    if (ct_len < XY_ROOT_CHACHA20POLY1305_TAG_SIZE) {
        return -1;
    }

    plaintext_len = ct_len - XY_ROOT_CHACHA20POLY1305_TAG_SIZE;
    if (xy_module_chacha20_poly1305_decrypt(key, nonce, aad, aad_len, ciphertext,
                                            plaintext_len, &ciphertext[plaintext_len],
                                            plaintext) != 0) {
        return -1;
    }

    *pt_len = plaintext_len;
    return 0;
}
