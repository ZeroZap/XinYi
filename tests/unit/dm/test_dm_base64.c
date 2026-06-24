#include <stdint.h>
#include <string.h>

#include "unity.h"
#include "xy_base64.h"

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_base64_encode_vectors(void)
{
    struct vector {
        const char *plain;
        const char *encoded;
    } vectors[] = {
        {"", ""},
        {"f", "Zg=="},
        {"fo", "Zm8="},
        {"foo", "Zm9v"},
        {"foob", "Zm9vYg=="},
        {"fooba", "Zm9vYmE="},
        {"foobar", "Zm9vYmFy"},
        {"Hello, World!", "SGVsbG8sIFdvcmxkIQ=="},
    };

    for (uint32_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); i++) {
        uint8_t output[128] = {0};
        uint32_t len = xy_base64_encode((const uint8_t *)vectors[i].plain,
                                        (uint32_t)strlen(vectors[i].plain), output);
        TEST_ASSERT_EQUAL_UINT32((uint32_t)strlen(vectors[i].encoded), len);
        TEST_ASSERT_EQUAL_STRING(vectors[i].encoded, (const char *)output);
    }
}

static void test_base64_decode_vectors(void)
{
    struct vector {
        const char *encoded;
        const char *plain;
    } vectors[] = {
        {"", ""},
        {"Zg==", "f"},
        {"Zm8=", "fo"},
        {"Zm9v", "foo"},
        {"Zm9vYg==", "foob"},
        {"Zm9vYmE=", "fooba"},
        {"Zm9vYmFy", "foobar"},
        {"SGVsbG8sIFdvcmxkIQ==", "Hello, World!"},
    };

    for (uint32_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); i++) {
        uint8_t output[128] = {0};
        uint32_t len = xy_base64_decode((const uint8_t *)vectors[i].encoded,
                                        (uint32_t)strlen(vectors[i].encoded), output);
        TEST_ASSERT_EQUAL_UINT32((uint32_t)strlen(vectors[i].plain), len);
        TEST_ASSERT_EQUAL_STRING(vectors[i].plain, (const char *)output);
    }
}

static void test_base64_roundtrip_binary(void)
{
    const uint8_t input[] = {0x00, 0x01, 0x02, 0x7f, 0x80, 0xfe, 0xff};
    uint8_t encoded[32] = {0};
    uint8_t decoded[sizeof(input)] = {0};

    uint32_t encoded_len = xy_base64_encode(input, sizeof(input), encoded);
    uint32_t decoded_len = xy_base64_decode(encoded, encoded_len, decoded);

    TEST_ASSERT_EQUAL_UINT32((uint32_t)sizeof(input), decoded_len);
    TEST_ASSERT_EQUAL_MEMORY(input, decoded, sizeof(input));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_base64_encode_vectors);
    RUN_TEST(test_base64_decode_vectors);
    RUN_TEST(test_base64_roundtrip_binary);
    return UNITY_END();
}
