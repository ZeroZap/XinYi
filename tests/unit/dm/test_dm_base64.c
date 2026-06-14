#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "xy_base64.h"

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
        assert(len == strlen(vectors[i].encoded));
        assert(memcmp(output, vectors[i].encoded, len) == 0);
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
        assert(len == strlen(vectors[i].plain));
        assert(memcmp(output, vectors[i].plain, len) == 0);
    }
}

static void test_base64_roundtrip_binary(void)
{
    const uint8_t input[] = {0x00, 0x01, 0x02, 0x7f, 0x80, 0xfe, 0xff};
    uint8_t encoded[32] = {0};
    uint8_t decoded[sizeof(input)] = {0};

    uint32_t encoded_len = xy_base64_encode(input, sizeof(input), encoded);
    uint32_t decoded_len = xy_base64_decode(encoded, encoded_len, decoded);

    assert(decoded_len == sizeof(input));
    assert(memcmp(decoded, input, sizeof(input)) == 0);
}

int main(void)
{
    test_base64_encode_vectors();
    test_base64_decode_vectors();
    test_base64_roundtrip_binary();
    return 0;
}
