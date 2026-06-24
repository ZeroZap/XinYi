#include "xy_tlv.h"
#include "unity.h"

#include <stdint.h>
#include <string.h>

void setUp(void)
{
}

void tearDown(void)
{
}

#define DEVICE_ID_TYPE   0x1001U
#define DEVICE_NAME_TYPE 0x1002U
#define FLAG_TYPE        0x1003U
#define BYTES_TYPE       0x1004U

static void test_tlv_encode_decode_iterator_and_find(void)
{
    uint8_t buffer[128];
    xy_tlv_buffer_t tlv_buf;
    xy_tlv_iterator_t iter;
    xy_tlv_t tlv;
    uint32_t device_id = 0;
    char name[16];
    bool flag = false;
    uint8_t bytes_out[4] = { 0 };
    uint16_t bytes_len = sizeof(bytes_out);
    const uint8_t bytes_in[] = { 0xDE, 0xAD, 0xBE, 0xEF };

    xy_tlv_reset_stats();

    TEST_ASSERT_EQUAL(XY_TLV_OK, xy_tlv_buffer_init(&tlv_buf, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_UINT16(0U, xy_tlv_buffer_get_used(&tlv_buf));
    TEST_ASSERT_EQUAL_UINT16((uint16_t)sizeof(buffer), xy_tlv_buffer_get_free(&tlv_buf));

    TEST_ASSERT_EQUAL(XY_TLV_OK, xy_tlv_encode_uint32(&tlv_buf, DEVICE_ID_TYPE, 0x12345678U));
    TEST_ASSERT_EQUAL(XY_TLV_OK, xy_tlv_encode_string(&tlv_buf, DEVICE_NAME_TYPE, "XinYi"));
    TEST_ASSERT_EQUAL(XY_TLV_OK, xy_tlv_encode_bool(&tlv_buf, FLAG_TYPE, true));
    TEST_ASSERT_EQUAL(XY_TLV_OK,
                      xy_tlv_encode_bytes(&tlv_buf, BYTES_TYPE, bytes_in, sizeof(bytes_in)));

    const uint16_t used = xy_tlv_buffer_get_used(&tlv_buf);
    TEST_ASSERT_EQUAL_UINT16(30U, used);
    TEST_ASSERT_EQUAL(XY_TLV_OK, xy_tlv_validate(buffer, used));
    TEST_ASSERT_EQUAL_INT(4, xy_tlv_count(buffer, used));

    TEST_ASSERT_EQUAL(XY_TLV_OK, xy_tlv_iterator_init(&iter, buffer, used));
    TEST_ASSERT_TRUE(xy_tlv_iterator_has_next(&iter));

    TEST_ASSERT_EQUAL(XY_TLV_OK, xy_tlv_iterator_next(&iter, &tlv));
    TEST_ASSERT_EQUAL_UINT16(DEVICE_ID_TYPE, tlv.type);
    TEST_ASSERT_EQUAL_UINT16((uint16_t)sizeof(uint32_t), tlv.length);
    TEST_ASSERT_EQUAL(XY_TLV_OK, xy_tlv_decode_uint32(&tlv, &device_id));
    TEST_ASSERT_EQUAL_HEX32(0x12345678U, device_id);

    TEST_ASSERT_EQUAL(XY_TLV_OK, xy_tlv_iterator_next(&iter, &tlv));
    TEST_ASSERT_EQUAL_UINT16(DEVICE_NAME_TYPE, tlv.type);
    TEST_ASSERT_EQUAL(XY_TLV_OK, xy_tlv_decode_string(&tlv, name, sizeof(name)));
    TEST_ASSERT_EQUAL_STRING("XinYi", name);

    TEST_ASSERT_EQUAL(XY_TLV_OK, xy_tlv_iterator_next(&iter, &tlv));
    TEST_ASSERT_EQUAL_UINT16(FLAG_TYPE, tlv.type);
    TEST_ASSERT_EQUAL(XY_TLV_OK, xy_tlv_decode_bool(&tlv, &flag));
    TEST_ASSERT_TRUE(flag);

    TEST_ASSERT_EQUAL(XY_TLV_OK, xy_tlv_iterator_next(&iter, &tlv));
    TEST_ASSERT_EQUAL_UINT16(BYTES_TYPE, tlv.type);
    TEST_ASSERT_EQUAL(XY_TLV_OK, xy_tlv_decode_bytes(&tlv, bytes_out, &bytes_len));
    TEST_ASSERT_EQUAL_UINT16((uint16_t)sizeof(bytes_in), bytes_len);
    TEST_ASSERT_EQUAL_MEMORY(bytes_in, bytes_out, sizeof(bytes_in));
    TEST_ASSERT_FALSE(xy_tlv_iterator_has_next(&iter));
    TEST_ASSERT_EQUAL(XY_TLV_NOT_FOUND, xy_tlv_iterator_next(&iter, &tlv));

    TEST_ASSERT_EQUAL(XY_TLV_OK, xy_tlv_find(buffer, used, DEVICE_NAME_TYPE, &tlv));
    TEST_ASSERT_EQUAL_UINT16(DEVICE_NAME_TYPE, tlv.type);
    TEST_ASSERT_EQUAL(XY_TLV_NOT_FOUND, xy_tlv_find(buffer, used, 0x9999U, &tlv));

    xy_tlv_stats_t stats;
    TEST_ASSERT_EQUAL(XY_TLV_OK, xy_tlv_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(4U, stats.total_encoded);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(4U, stats.total_decoded);
    TEST_ASSERT_EQUAL_UINT32(0U, stats.encoding_errors);
}

static void test_tlv_find_all_and_reset(void)
{
    uint8_t buffer[64];
    xy_tlv_buffer_t tlv_buf;
    xy_tlv_t matches[2];
    uint16_t match_count = 1U;

    TEST_ASSERT_EQUAL(XY_TLV_OK, xy_tlv_buffer_init(&tlv_buf, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL(XY_TLV_OK, xy_tlv_encode_uint8(&tlv_buf, FLAG_TYPE, 1U));
    TEST_ASSERT_EQUAL(XY_TLV_OK, xy_tlv_encode_uint8(&tlv_buf, FLAG_TYPE, 2U));
    TEST_ASSERT_EQUAL(XY_TLV_OK, xy_tlv_encode_uint8(&tlv_buf, DEVICE_ID_TYPE, 3U));

    TEST_ASSERT_EQUAL_INT(2, xy_tlv_find_all(buffer, xy_tlv_buffer_get_used(&tlv_buf),
                                            FLAG_TYPE, matches, &match_count));
    TEST_ASSERT_EQUAL_UINT16(2U, match_count);
    TEST_ASSERT_EQUAL_UINT16(FLAG_TYPE, matches[0].type);

    TEST_ASSERT_EQUAL(XY_TLV_OK, xy_tlv_buffer_reset(&tlv_buf));
    TEST_ASSERT_EQUAL_UINT16(0U, xy_tlv_buffer_get_used(&tlv_buf));
}

static void test_tlv_rejects_invalid_inputs_and_lengths(void)
{
    uint8_t buffer[8];
    xy_tlv_buffer_t tlv_buf;
    xy_tlv_iterator_t iter;
    xy_tlv_t tlv;
    uint8_t tiny_out[1];
    uint16_t tiny_len = sizeof(tiny_out);

    TEST_ASSERT_EQUAL(XY_TLV_INVALID_PARAM, xy_tlv_buffer_init(NULL, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL(XY_TLV_INVALID_PARAM, xy_tlv_buffer_init(&tlv_buf, NULL, sizeof(buffer)));
    TEST_ASSERT_EQUAL(XY_TLV_OK, xy_tlv_buffer_init(&tlv_buf, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL(XY_TLV_INVALID_PARAM,
                      xy_tlv_encode_string(&tlv_buf, DEVICE_NAME_TYPE, NULL));
    TEST_ASSERT_EQUAL(XY_TLV_INVALID_PARAM,
                      xy_tlv_encode_bytes(&tlv_buf, BYTES_TYPE, NULL, 1U));
    TEST_ASSERT_EQUAL(XY_TLV_OK, xy_tlv_encode_uint32(&tlv_buf, DEVICE_ID_TYPE, 0xAABBCCDDU));
    TEST_ASSERT_EQUAL(XY_TLV_BUFFER_OVERFLOW, xy_tlv_encode_uint8(&tlv_buf, FLAG_TYPE, 1U));

    TEST_ASSERT_EQUAL(XY_TLV_OK,
                      xy_tlv_iterator_init(&iter, buffer, xy_tlv_buffer_get_used(&tlv_buf)));
    TEST_ASSERT_EQUAL(XY_TLV_OK, xy_tlv_iterator_next(&iter, &tlv));
    TEST_ASSERT_EQUAL(XY_TLV_INVALID_LENGTH, xy_tlv_decode_uint16(&tlv, &(uint16_t){ 0 }));
    TEST_ASSERT_EQUAL(XY_TLV_BUFFER_OVERFLOW, xy_tlv_decode_bytes(&tlv, tiny_out, &tiny_len));

    uint8_t malformed[] = { 0x10, 0x01, 0x00, 0x04, 0xAA };
    TEST_ASSERT_EQUAL(XY_TLV_INVALID_LENGTH, xy_tlv_validate(malformed, sizeof(malformed)));
    TEST_ASSERT_EQUAL_INT(0, xy_tlv_count(malformed, sizeof(malformed)));
    TEST_ASSERT_EQUAL(XY_TLV_INVALID_PARAM, xy_tlv_iterator_init(NULL, buffer, sizeof(buffer)));
}

static void test_tlv_utility_strings_and_checksum(void)
{
    const uint8_t vector[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };

    TEST_ASSERT_EQUAL_HEX16(0x4B37U, xy_tlv_checksum(vector, sizeof(vector)));
    TEST_ASSERT_EQUAL_STRING("UINT32", xy_tlv_get_type_name(XY_TLV_TYPE_UINT32));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN", xy_tlv_get_type_name(0xEEEEU));
    TEST_ASSERT_EQUAL_STRING("Buffer overflow", xy_tlv_get_error_string(XY_TLV_BUFFER_OVERFLOW));
    TEST_ASSERT_EQUAL_STRING("Unknown error", xy_tlv_get_error_string(1234));
    TEST_ASSERT_EQUAL(XY_TLV_INVALID_PARAM, xy_tlv_get_stats(NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_tlv_encode_decode_iterator_and_find);
    RUN_TEST(test_tlv_find_all_and_reset);
    RUN_TEST(test_tlv_rejects_invalid_inputs_and_lengths);
    RUN_TEST(test_tlv_utility_strings_and_checksum);
    return UNITY_END();
}
