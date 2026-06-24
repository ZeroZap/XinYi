#include "xy_tlv.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

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

    assert(xy_tlv_buffer_init(&tlv_buf, buffer, sizeof(buffer)) == XY_TLV_OK);
    assert(xy_tlv_buffer_get_used(&tlv_buf) == 0U);
    assert(xy_tlv_buffer_get_free(&tlv_buf) == sizeof(buffer));

    assert(xy_tlv_encode_uint32(&tlv_buf, DEVICE_ID_TYPE, 0x12345678U) == XY_TLV_OK);
    assert(xy_tlv_encode_string(&tlv_buf, DEVICE_NAME_TYPE, "XinYi") == XY_TLV_OK);
    assert(xy_tlv_encode_bool(&tlv_buf, FLAG_TYPE, true) == XY_TLV_OK);
    assert(xy_tlv_encode_bytes(&tlv_buf, BYTES_TYPE, bytes_in, sizeof(bytes_in)) == XY_TLV_OK);

    const uint16_t used = xy_tlv_buffer_get_used(&tlv_buf);
    assert(used == 30U);
    assert(xy_tlv_validate(buffer, used) == XY_TLV_OK);
    assert(xy_tlv_count(buffer, used) == 4);

    assert(xy_tlv_iterator_init(&iter, buffer, used) == XY_TLV_OK);
    assert(xy_tlv_iterator_has_next(&iter));

    assert(xy_tlv_iterator_next(&iter, &tlv) == XY_TLV_OK);
    assert(tlv.type == DEVICE_ID_TYPE);
    assert(tlv.length == sizeof(uint32_t));
    assert(xy_tlv_decode_uint32(&tlv, &device_id) == XY_TLV_OK);
    assert(device_id == 0x12345678U);

    assert(xy_tlv_iterator_next(&iter, &tlv) == XY_TLV_OK);
    assert(tlv.type == DEVICE_NAME_TYPE);
    assert(xy_tlv_decode_string(&tlv, name, sizeof(name)) == XY_TLV_OK);
    assert(strcmp(name, "XinYi") == 0);

    assert(xy_tlv_iterator_next(&iter, &tlv) == XY_TLV_OK);
    assert(tlv.type == FLAG_TYPE);
    assert(xy_tlv_decode_bool(&tlv, &flag) == XY_TLV_OK);
    assert(flag);

    assert(xy_tlv_iterator_next(&iter, &tlv) == XY_TLV_OK);
    assert(tlv.type == BYTES_TYPE);
    assert(xy_tlv_decode_bytes(&tlv, bytes_out, &bytes_len) == XY_TLV_OK);
    assert(bytes_len == sizeof(bytes_in));
    assert(memcmp(bytes_out, bytes_in, sizeof(bytes_in)) == 0);
    assert(!xy_tlv_iterator_has_next(&iter));
    assert(xy_tlv_iterator_next(&iter, &tlv) == XY_TLV_NOT_FOUND);

    assert(xy_tlv_find(buffer, used, DEVICE_NAME_TYPE, &tlv) == XY_TLV_OK);
    assert(tlv.type == DEVICE_NAME_TYPE);
    assert(xy_tlv_find(buffer, used, 0x9999U, &tlv) == XY_TLV_NOT_FOUND);

    xy_tlv_stats_t stats;
    assert(xy_tlv_get_stats(&stats) == XY_TLV_OK);
    assert(stats.total_encoded == 4U);
    assert(stats.total_decoded >= 4U);
    assert(stats.encoding_errors == 0U);
}

static void test_tlv_find_all_and_reset(void)
{
    uint8_t buffer[64];
    xy_tlv_buffer_t tlv_buf;
    xy_tlv_t matches[2];
    uint16_t match_count = 1U;

    assert(xy_tlv_buffer_init(&tlv_buf, buffer, sizeof(buffer)) == XY_TLV_OK);
    assert(xy_tlv_encode_uint8(&tlv_buf, FLAG_TYPE, 1U) == XY_TLV_OK);
    assert(xy_tlv_encode_uint8(&tlv_buf, FLAG_TYPE, 2U) == XY_TLV_OK);
    assert(xy_tlv_encode_uint8(&tlv_buf, DEVICE_ID_TYPE, 3U) == XY_TLV_OK);

    assert(xy_tlv_find_all(buffer, xy_tlv_buffer_get_used(&tlv_buf), FLAG_TYPE, matches,
                           &match_count) == 2);
    assert(match_count == 2U);
    assert(matches[0].type == FLAG_TYPE);

    assert(xy_tlv_buffer_reset(&tlv_buf) == XY_TLV_OK);
    assert(xy_tlv_buffer_get_used(&tlv_buf) == 0U);
}

static void test_tlv_rejects_invalid_inputs_and_lengths(void)
{
    uint8_t buffer[8];
    xy_tlv_buffer_t tlv_buf;
    xy_tlv_iterator_t iter;
    xy_tlv_t tlv;
    uint8_t tiny_out[1];
    uint16_t tiny_len = sizeof(tiny_out);

    assert(xy_tlv_buffer_init(NULL, buffer, sizeof(buffer)) == XY_TLV_INVALID_PARAM);
    assert(xy_tlv_buffer_init(&tlv_buf, NULL, sizeof(buffer)) == XY_TLV_INVALID_PARAM);
    assert(xy_tlv_buffer_init(&tlv_buf, buffer, sizeof(buffer)) == XY_TLV_OK);
    assert(xy_tlv_encode_string(&tlv_buf, DEVICE_NAME_TYPE, NULL) == XY_TLV_INVALID_PARAM);
    assert(xy_tlv_encode_bytes(&tlv_buf, BYTES_TYPE, NULL, 1U) == XY_TLV_INVALID_PARAM);
    assert(xy_tlv_encode_uint32(&tlv_buf, DEVICE_ID_TYPE, 0xAABBCCDDU) == XY_TLV_OK);
    assert(xy_tlv_encode_uint8(&tlv_buf, FLAG_TYPE, 1U) == XY_TLV_BUFFER_OVERFLOW);

    assert(xy_tlv_iterator_init(&iter, buffer, xy_tlv_buffer_get_used(&tlv_buf)) == XY_TLV_OK);
    assert(xy_tlv_iterator_next(&iter, &tlv) == XY_TLV_OK);
    assert(xy_tlv_decode_uint16(&tlv, &(uint16_t){ 0 }) == XY_TLV_INVALID_LENGTH);
    assert(xy_tlv_decode_bytes(&tlv, tiny_out, &tiny_len) == XY_TLV_BUFFER_OVERFLOW);

    uint8_t malformed[] = { 0x10, 0x01, 0x00, 0x04, 0xAA };
    assert(xy_tlv_validate(malformed, sizeof(malformed)) == XY_TLV_INVALID_LENGTH);
    assert(xy_tlv_count(malformed, sizeof(malformed)) == 0);
    assert(xy_tlv_iterator_init(NULL, buffer, sizeof(buffer)) == XY_TLV_INVALID_PARAM);
}

static void test_tlv_utility_strings_and_checksum(void)
{
    const uint8_t vector[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };

    assert(xy_tlv_checksum(vector, sizeof(vector)) == 0x4B37U);
    assert(strcmp(xy_tlv_get_type_name(XY_TLV_TYPE_UINT32), "UINT32") == 0);
    assert(strcmp(xy_tlv_get_type_name(0xEEEEU), "UNKNOWN") == 0);
    assert(strcmp(xy_tlv_get_error_string(XY_TLV_BUFFER_OVERFLOW), "Buffer overflow") == 0);
    assert(strcmp(xy_tlv_get_error_string(1234), "Unknown error") == 0);
    assert(xy_tlv_get_stats(NULL) == XY_TLV_INVALID_PARAM);
}

int main(void)
{
    test_tlv_encode_decode_iterator_and_find();
    test_tlv_find_all_and_reset();
    test_tlv_rejects_invalid_inputs_and_lengths();
    test_tlv_utility_strings_and_checksum();
    return 0;
}
