/**
 * @file example_secure.c
 * @brief Secure FOTA with Signature Verification Example
 *
 * This example demonstrates secure FOTA functionality:
 * - ECDSA P-256 signature verification
 * - ChaCha20-Poly1305 encrypted firmware
 * - Secure boot chain verification
 * - Key management integration
 *
 * Reference: SECURE_FOTA.md
 */

#include "xy_fota.h"
#include <stdio.h>
#include <string.h>

/* ==================== Secure FOTA Configuration ==================== */

/* EC public key (256 bits) - in production, store in secure flash */
static const uint8_t g_ec_pub_key[64] = {
    /* This is a demo key - replace with actual server public key */
    0x04, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE,
    0xF0, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE,
    0xF0, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE,
    0xF0, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE,
    0xF0, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE,
    0xF0, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE,
    0xF0, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE,
    0xF0, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE,
};

/* Secure firmware package header */
#define SECURE_FW_MAGIC      0x58494E59  /* 'XINY' */
#define SECURE_FW_VERSION    0x00020000  /* v2.0.0 */

typedef struct {
    uint32_t magic;             /* Magic number */
    uint32_t version;           /* Header version */
    uint32_t fw_size;           /* Total firmware size */
    uint32_t timestamp;         /* Build timestamp */
    uint8_t  ecdsa_sig[64];     /* ECDSA P-256 signature */
    uint8_t  chacha_nonce[12];  /* ChaCha20 nonce */
    uint32_t crc32;             /* Header CRC */
    uint8_t  flags;             /* Flags */
    uint8_t  reserved[7];       /* Reserved */
} __attribute__((packed)) secure_fw_header_t;

/* Secure flags */
#define SECURE_FW_FLAG_ENCRYPTED   (1 << 0)
#define SECURE_FW_FLAG_SIGNED      (1 << 1)
#define SECURE_FW_FLAG_COMPRESSED  (1 << 2)

/* ==================== Mock Crypto Operations ==================== */

static int mock_ecdsa_verify(const uint8_t *pub_key,
                            const uint8_t *msg,
                            uint32_t msg_len,
                            const uint8_t *sig)
{
    printf("[Crypto] ECDSA P-256 signature verification\r\n");
    printf("         Public key: %p\r\n", (void*)pub_key);
    printf("         Message: %u bytes\r\n", msg_len);
    printf("         Signature: %p\r\n", (void*)sig);

    /* Simulate verification delay */
    /* In production: use hardware crypto engine */

    /* Demo: accept if signature is non-zero */
    for (int i = 0; i < 64; i++) {
        if (sig[i] != 0) {
            printf("[Crypto] Signature verified successfully\r\n");
            return 0;
        }
    }

    printf("[Crypto] Signature verification FAILED\r\n");
    return -1;
}

static int mock_chacha20_decrypt(const uint8_t *key,
                                 const uint8_t *nonce,
                                 const uint8_t *ciphertext,
                                 uint8_t *plaintext,
                                 uint32_t size)
{
    printf("[Crypto] ChaCha20-Poly1305 decryption\r\n");
    printf("         Key: %p (256-bit)\r\n", (void*)key);
    printf("         Nonce: %02X%02X%02X... (12 bytes)\r\n",
           nonce[0], nonce[1], nonce[2]);
    printf("         Data: %u bytes\r\n", size);

    /* Simulate decryption - in production use hardware */
    memcpy(plaintext, ciphertext, size);

    printf("[Crypto] Decryption completed\r\n");
    return 0;
}

static int mock_poly1305_verify(const uint8_t *key,
                                const uint8_t *msg,
                                uint32_t msg_len,
                                const uint8_t *tag)
{
    printf("[Crypto] Poly1305 authentication verification\r\n");

    /* Simulate tag check */
    if (tag[0] != 0 || tag[15] != 0) {
        printf("[Crypto] Authentication OK\r\n");
        return 0;
    }

    printf("[Crypto] Authentication FAILED\r\n");
    return -1;
}

/* ==================== Mock Flash Operations ==================== */

static uint8_t mock_flash[512 * 1024] = {0};

static int mock_flash_init(void)
{
    printf("[Flash] Secure flash initialized\r\n");
    return XY_FOTA_OK;
}

static int mock_flash_write(uint32_t addr, const uint8_t *data, uint32_t size)
{
    if (addr + size > sizeof(mock_flash)) {
        return XY_FOTA_ERROR;
    }
    memcpy(&mock_flash[addr], data, size);
    printf("[Flash] Wrote %u bytes at 0x%08X\r\n", size, addr);
    return XY_FOTA_OK;
}

static int mock_flash_read(uint32_t addr, uint8_t *data, uint32_t size)
{
    if (addr + size > sizeof(mock_flash)) {
        return XY_FOTA_ERROR;
    }
    memcpy(data, &mock_flash[addr], size);
    return XY_FOTA_OK;
}

static int mock_flash_erase(uint32_t addr, uint32_t size)
{
    if (addr + size > sizeof(mock_flash)) {
        return XY_FOTA_ERROR;
    }
    memset(&mock_flash[addr], 0xFF, size);
    printf("[Flash] Erased %u bytes at 0x%08X\r\n", size, addr);
    return XY_FOTA_OK;
}

static int mock_flash_deinit(void)
{
    printf("[Flash] Secure flash deinitialized\r\n");
    return XY_FOTA_OK;
}

static const xy_fota_flash_ops_t g_secure_flash_ops = {
    .init   = mock_flash_init,
    .write  = mock_flash_write,
    .read   = mock_flash_read,
    .erase  = mock_flash_erase,
    .deinit = mock_flash_deinit,
};

/* ==================== Secure FOTA Functions ==================== */

/**
 * @brief Verify secure firmware package signature
 */
static int secure_verify_signature(const secure_fw_header_t *header,
                                  const uint8_t *fw_data,
                                  uint32_t fw_size)
{
    /* Verify ECDSA signature over firmware */
    return mock_ecdsa_verify(g_ec_pub_key, fw_data, fw_size, header->ecdsa_sig);
}

/**
 * @brief Decrypt secure firmware
 */
static int secure_decrypt_firmware(const secure_fw_header_t *header,
                                  uint8_t *fw_data,
                                  uint32_t fw_size)
{
    /* In production: derive key using ECDH */
    uint8_t derived_key[32] = {0};  /* Would be derived from ECDH */

    /* Decrypt with ChaCha20 */
    return mock_chacha20_decrypt(derived_key,
                                 header->chacha_nonce,
                                 fw_data,
                                 fw_data,
                                 fw_size);
}

/**
 * @brief Verify firmware authenticity
 */
static int secure_verify_authenticity(const secure_fw_header_t *header,
                                      const uint8_t *fw_data,
                                      uint32_t fw_size)
{
    /* Verify Poly1305 tag */
    uint8_t expected_tag[16] = {0};

    return mock_poly1305_verify(expected_tag, fw_data, fw_size, header->ecdsa_sig + 48);
}

/**
 * @brief Parse and validate secure firmware header
 */
static int secure_parse_header(const uint8_t *pkg, uint32_t pkg_size,
                              secure_fw_header_t *header,
                              uint8_t **fw_data,
                              uint32_t *fw_size)
{
    if (!pkg || !header || pkg_size < sizeof(secure_fw_header_t)) {
        return XY_FOTA_INVALID_PARAM;
    }

    memcpy(header, pkg, sizeof(secure_fw_header_t));

    /* Validate magic */
    if (header->magic != SECURE_FW_MAGIC) {
        printf("[Error] Invalid secure firmware magic: 0x%08X\r\n", header->magic);
        return XY_FOTA_ERROR;
    }

    /* Validate size */
    if (header->fw_size > XY_FOTA_MAX_IMAGE_SIZE) {
        printf("[Error] Firmware size too large: %u bytes\r\n", header->fw_size);
        return XY_FOTA_ERROR;
    }

    *fw_data = (uint8_t*)(pkg + sizeof(secure_fw_header_t));
    *fw_size = pkg_size - sizeof(secure_fw_header_t);

    printf("[Info] Secure firmware header parsed:\r\n");
    printf("       Version: 0x%08X\r\n", header->version);
    printf("       Size: %u bytes\r\n", header->fw_size);
    printf("       Flags: 0x%02X\r\n", header->flags);

    return XY_FOTA_OK;
}

/* ==================== Progress Callback ==================== */

static void secure_progress_callback(uint32_t current, uint32_t total, void *user_data)
{
    uint8_t progress = (uint8_t)((current * 100) / total);
    printf("[Secure FOTA] Progress: %u%% (%u/%u bytes)\r\n", progress, current, total);
    (void)user_data;
}

/* ==================== Main Example ==================== */

int main(void)
{
    int ret;
    xy_fota_t fota;
    xy_fota_config_t config;
    secure_fw_header_t sec_header;
    uint8_t *fw_data = NULL;
    uint32_t fw_size = 0;

    printf("\r\n");
    printf("========================================\r\n");
    printf("   Secure FOTA Example\r\n");
    printf("   (ECDSA + ChaCha20-Poly1305)\r\n");
    printf("========================================\r\n");
    printf("\r\n");

    /* 1. FOTA initialization with security enabled */
    printf("[Step 1] Initializing Secure FOTA...\r\n");
    memset(&config, 0, sizeof(config));
    config.mode = XY_FOTA_MODE_DUAL_BANK;
    config.flash_base_addr = 0x08000000;
    config.slot_size = 192 * 1024;
    config.slot_count = 2;
    config.enable_secure_boot = true;
    config.enable_rollback = true;
    config.min_version = 1;

    ret = xy_fota_init(&fota, &config);
    if (ret != XY_FOTA_OK) {
        printf("[Error] FOTA init failed: %d\r\n", ret);
        return -1;
    }

    ret = xy_fota_set_flash_ops(&fota, &g_secure_flash_ops);
    if (ret != XY_FOTA_OK) {
        printf("[Error] Set flash ops failed: %d\r\n", ret);
        return -1;
    }

    ret = xy_fota_set_progress_callback(&fota, secure_progress_callback, NULL);
    if (ret != XY_FOTA_OK) {
        printf("[Error] Set progress callback failed: %d\r\n", ret);
        return -1;
    }

    printf("[OK] Secure FOTA initialized\r\n");
    printf("\r\n");

    /* 2. Simulate receiving secure firmware package */
    printf("[Step 2] Receiving secure firmware package...\r\n");

    /* Build secure firmware package */
    uint8_t secure_pkg[sizeof(secure_fw_header_t) + 2048];
    memset(&sec_header, 0, sizeof(sec_header));

    sec_header.magic = SECURE_FW_MAGIC;
    sec_header.version = SECURE_FW_VERSION;
    sec_header.fw_size = 2048;
    sec_header.timestamp = 0x12345678;
    sec_header.flags = SECURE_FW_FLAG_ENCRYPTED | SECURE_FW_FLAG_SIGNED;

    /* Demo signature (in production: ECDSA sign) */
    memset(sec_header.ecdsa_sig, 0xAB, sizeof(sec_header.ecdsa_sig));

    /* Demo nonce */
    memset(sec_header.chacha_nonce, 0x12, sizeof(sec_header.chacha_nonce));

    /* Demo firmware data */
    uint8_t *demo_fw = secure_pkg + sizeof(sec_header);
    for (int i = 0; i < 2048; i++) {
        demo_fw[i] = (uint8_t)(i & 0xFF);
    }

    memcpy(secure_pkg, &sec_header, sizeof(sec_header));

    printf("[OK] Secure package created: %u bytes\r\n",
           (uint32_t)(sizeof(sec_header) + 2048));
    printf("\r\n");

    /* 3. Parse secure firmware header */
    printf("[Step 3] Parsing secure firmware header...\r\n");
    ret = secure_parse_header(secure_pkg, sizeof(secure_pkg),
                              &sec_header, &fw_data, &fw_size);
    if (ret != XY_FOTA_OK) {
        printf("[Error] Parse header failed: %d\r\n", ret);
        return -1;
    }
    printf("[OK] Header parsed successfully\r\n");
    printf("\r\n");

    /* 4. Verify signature */
    printf("[Step 4] Verifying ECDSA signature...\r\n");
    ret = secure_verify_signature(&sec_header, fw_data, fw_size);
    if (ret != XY_FOTA_OK) {
        printf("[Error] Signature verification failed: %d\r\n", ret);
        printf("[Error] Firmware may be tampered or from unknown source!\r\n");
        return -1;
    }
    printf("[OK] Signature verified - firmware is authentic\r\n");
    printf("\r\n");

    /* 5. Decrypt firmware (if encrypted) */
    if (sec_header.flags & SECURE_FW_FLAG_ENCRYPTED) {
        printf("[Step 5] Decrypting firmware...\r\n");
        ret = secure_decrypt_firmware(&sec_header, fw_data, fw_size);
        if (ret != XY_FOTA_OK) {
            printf("[Error] Decryption failed: %d\r\n", ret);
            return -1;
        }
        printf("[OK] Firmware decrypted successfully\r\n");
        printf("\r\n");
    }

    /* 6. Authenticate (Poly1305) */
    printf("[Step 6] Authenticating firmware...\r\n");
    ret = secure_verify_authenticity(&sec_header, fw_data, fw_size);
    if (ret != XY_FOTA_OK) {
        printf("[Error] Authentication failed: %d\r\n", ret);
        return -1;
    }
    printf("[OK] Firmware authenticated\r\n");
    printf("\r\n");

    /* 7. Start FOTA download process */
    printf("[Step 7] Starting secure FOTA download...\r\n");
    ret = xy_fota_start_download(&fota, 2, fw_size, false);
    if (ret != XY_FOTA_OK) {
        printf("[Error] Start download failed: %d\r\n", ret);
        return -1;
    }

    /* Download firmware in chunks */
    uint32_t chunk_size = 256;
    uint32_t offset = 0;
    while (offset < fw_size) {
        uint32_t remaining = fw_size - offset;
        uint32_t to_send = (remaining < chunk_size) ? remaining : chunk_size;

        ret = xy_fota_download_chunk(&fota, &fw_data[offset], to_send);
        if (ret != XY_FOTA_OK) {
            printf("[Error] Download chunk failed at offset %u: %d\r\n", offset, ret);
            return -1;
        }
        offset += to_send;
    }

    ret = xy_fota_finish_download(&fota);
    if (ret != XY_FOTA_OK) {
        printf("[Error] Finish download failed: %d\r\n", ret);
        return -1;
    }
    printf("[OK] Secure firmware downloaded successfully\r\n");
    printf("\r\n");

    /* 8. Apply update */
    printf("[Step 8] Applying secure update...\r\n");
    ret = xy_fota_start_update(&fota);
    if (ret != XY_FOTA_OK) {
        printf("[Error] Apply update failed: %d\r\n", ret);
        return -1;
    }
    printf("[OK] Secure update applied\r\n");
    printf("     System will reboot with secure boot chain\r\n");
    printf("\r\n");

    /* 9. Verify rollback status */
    printf("[Step 9] Checking rollback status...\r\n");
    if (xy_fota_needs_rollback(&fota)) {
        printf("[Info] Rollback needed, restoring secure backup...\r\n");
        ret = xy_fota_rollback(&fota);
        if (ret != XY_FOTA_OK) {
            printf("[Error] Rollback failed: %d\r\n", ret);
            return -1;
        }
        printf("[OK] Secure rollback completed\r\n");
    } else {
        printf("[Info] No rollback needed, secure firmware is healthy\r\n");
    }
    printf("\r\n");

    /* Cleanup */
    printf("[Step 10] Cleanup...\r\n");
    xy_fota_deinit(&fota);
    printf("[OK] Secure FOTA deinitialized\r\n");
    printf("\r\n");

    printf("========================================\r\n");
    printf("   Secure FOTA Example Completed\r\n");
    printf("   All security checks passed!\r\n");
    printf("========================================\r\n");

    return 0;
}
