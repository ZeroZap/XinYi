#include "unity.h"
#include "xy_fota_secure.h"

#include <stdint.h>
#include <string.h>

#define SLOT_SIZE (32u * 1024u)
#define PAYLOAD_SIZE 4u
#define PACKAGE_SIZE (sizeof(xy_fota_secure_header_t) + PAYLOAD_SIZE + XY_FOTA_POLY1305_TAG_SIZE)

static uint32_t g_seen_key_id;
static const uint8_t *g_seen_message;
static uint32_t g_seen_message_size;
static const uint8_t *g_seen_signature;
static uint32_t g_seen_signature_size;
static int g_provider_result;
static unsigned int g_provider_calls;
static unsigned int g_flash_init_calls;

void setUp(void)
{
    g_seen_key_id = 0;
    g_seen_message = NULL;
    g_seen_message_size = 0;
    g_seen_signature = NULL;
    g_seen_signature_size = 0;
    g_provider_result = XY_FOTA_OK;
    g_provider_calls = 0;
    g_flash_init_calls = 0;
}

void tearDown(void)
{
}

static int flash_init(void)
{
    g_flash_init_calls++;
    return XY_FOTA_OK;
}

static int verify_signature(void *context,
                            uint32_t key_id,
                            const uint8_t *message,
                            uint32_t message_size,
                            const uint8_t *signature,
                            uint32_t signature_size)
{
    TEST_ASSERT_EQUAL_PTR((void *)0x1234, context);
    g_provider_calls++;
    g_seen_key_id = key_id;
    g_seen_message = message;
    g_seen_message_size = message_size;
    g_seen_signature = signature;
    g_seen_signature_size = signature_size;
    return g_provider_result;
}

static const xy_fota_flash_ops_t g_flash_ops = {
    .init = flash_init,
};

static const xy_fota_signature_provider_t g_provider = {
    .verify = verify_signature,
    .context = (void *)0x1234,
};

static xy_fota_secure_config_t default_config(void)
{
    xy_fota_secure_config_t config = {0};
    config.signature_provider = &g_provider;
    config.key_id = 0x42u;
    config.min_version = 7u;
    config.slot0_addr = 0x08010000u;
    config.slot1_addr = 0x08020000u;
    config.slot_size = SLOT_SIZE;
    config.dual_bank = true;
    return config;
}

static void build_package(uint8_t package[PACKAGE_SIZE], uint32_t version)
{
    xy_fota_secure_header_t header = {0};
    header.magic = XY_FOTA_SECURE_MAGIC;
    header.version = version;
    header.fw_size = PAYLOAD_SIZE;
    memset(header.ecdsa_sig, 0x5a, sizeof(header.ecdsa_sig));
    memcpy(package, &header, sizeof(header));
    memset(package + sizeof(header), 0xa5, PAYLOAD_SIZE + XY_FOTA_POLY1305_TAG_SIZE);
}

static void test_init_requires_signature_provider(void)
{
    xy_fota_secure_t fota;
    xy_fota_secure_config_t config = default_config();

    config.signature_provider = NULL;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_AUTH_ERROR, xy_fota_secure_init(&fota, &config, (xy_fota_flash_ops_t *)&g_flash_ops));
    TEST_ASSERT_FALSE(fota.initialized);
    TEST_ASSERT_EQUAL_UINT(0, g_flash_init_calls);

    config = default_config();
    xy_fota_signature_provider_t missing_verify = {0};
    config.signature_provider = &missing_verify;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_AUTH_ERROR, xy_fota_secure_init(&fota, &config, (xy_fota_flash_ops_t *)&g_flash_ops));
    TEST_ASSERT_EQUAL_UINT(0, g_flash_init_calls);
}

static void test_provider_receives_key_message_and_signature(void)
{
    xy_fota_secure_t fota;
    xy_fota_secure_config_t config = default_config();
    uint8_t package[PACKAGE_SIZE];

    build_package(package, config.min_version);
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_secure_init(&fota, &config, (xy_fota_flash_ops_t *)&g_flash_ops));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_secure_verify(&fota, package, sizeof(package)));
    TEST_ASSERT_EQUAL_UINT(1, g_provider_calls);
    TEST_ASSERT_EQUAL_HEX32(config.key_id, g_seen_key_id);
    TEST_ASSERT_EQUAL_PTR(package + sizeof(xy_fota_secure_header_t), g_seen_message);
    TEST_ASSERT_EQUAL_UINT32(PAYLOAD_SIZE + XY_FOTA_POLY1305_TAG_SIZE, g_seen_message_size);
    TEST_ASSERT_EQUAL_PTR(fota.header.ecdsa_sig, g_seen_signature);
    TEST_ASSERT_EQUAL_UINT32(XY_FOTA_ECDSA_P256_SIG_SIZE, g_seen_signature_size);
    TEST_ASSERT_TRUE(fota.verified);
}

static void test_provider_rejection_fails_closed(void)
{
    xy_fota_secure_t fota;
    xy_fota_secure_config_t config = default_config();
    uint8_t package[PACKAGE_SIZE];

    build_package(package, config.min_version);
    g_provider_result = XY_FOTA_AUTH_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_secure_init(&fota, &config, (xy_fota_flash_ops_t *)&g_flash_ops));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_AUTH_ERROR, xy_fota_secure_verify(&fota, package, sizeof(package)));
    TEST_ASSERT_FALSE(fota.verified);
}

static void test_wrong_key_id_and_rollback_version_fail_closed(void)
{
    xy_fota_secure_t fota;
    xy_fota_secure_config_t config = default_config();
    uint8_t package[PACKAGE_SIZE];

    build_package(package, config.min_version - 1u);
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_secure_init(&fota, &config, (xy_fota_flash_ops_t *)&g_flash_ops));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_VERSION_ERROR, xy_fota_secure_verify(&fota, package, sizeof(package)));
    TEST_ASSERT_EQUAL_UINT(0, g_provider_calls);

    build_package(package, config.min_version);
    g_provider_result = XY_FOTA_AUTH_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_AUTH_ERROR, xy_fota_secure_verify(&fota, package, sizeof(package)));
    TEST_ASSERT_EQUAL_HEX32(config.key_id, g_seen_key_id);
    TEST_ASSERT_FALSE(fota.verified);
}

static void test_truncated_package_does_not_call_provider(void)
{
    xy_fota_secure_t fota;
    xy_fota_secure_config_t config = default_config();
    uint8_t package[PACKAGE_SIZE];

    build_package(package, config.min_version);
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_secure_init(&fota, &config, (xy_fota_flash_ops_t *)&g_flash_ops));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_ERROR, xy_fota_secure_verify(&fota, package, sizeof(package) - 1u));
    TEST_ASSERT_EQUAL_UINT(0, g_provider_calls);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_requires_signature_provider);
    RUN_TEST(test_provider_receives_key_message_and_signature);
    RUN_TEST(test_provider_rejection_fails_closed);
    RUN_TEST(test_wrong_key_id_and_rollback_version_fail_closed);
    RUN_TEST(test_truncated_package_does_not_call_provider);
    return UNITY_END();
}
