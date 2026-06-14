/**
 * @file test_dm.c
 * @brief DM (Data Management) Component Unit Tests using Unity Framework
 * @version 1.0.0
 * @date 2026-02-28
 */

#include <stdint.h>
#include <string.h>
#include "unity.h"

/* DM headers - TLV */
#include "xy_tlv.h"

/* ==================== Test Fixtures ==================== */

void setUp(void)
{
    /* Called before each test */
}

void tearDown(void)
{
    /* Called after each test */
}

/* ==================== Helper Functions ==================== */

static void print_tlv_buffer(const uint8_t *buffer, size_t len)
{
    (void)buffer;
    (void)len;
    /* Debug output if needed */
}

/* ==================== TLV Basic Tests ==================== */

void test_tlv_header_size(void)
{
    /* Test TLV header size constant */
    TEST_ASSERT_EQUAL(4, XY_TLV_HEADER_SIZE);
}

void test_tlv_predefined_types(void)
{
    /* Test basic type definitions */
    TEST_ASSERT_EQUAL(0x0001, XY_TLV_TYPE_UINT8);
    TEST_ASSERT_EQUAL(0x0002, XY_TLV_TYPE_UINT16);
    TEST_ASSERT_EQUAL(0x0003, XY_TLV_TYPE_UINT32);
    TEST_ASSERT_EQUAL(0x0004, XY_TLV_TYPE_UINT64);
    TEST_ASSERT_EQUAL(0x0005, XY_TLV_TYPE_INT8);
    TEST_ASSERT_EQUAL(0x0006, XY_TLV_TYPE_INT16);
    TEST_ASSERT_EQUAL(0x0007, XY_TLV_TYPE_INT32);
    TEST_ASSERT_EQUAL(0x0008, XY_TLV_TYPE_INT64);
}

void test_tlv_string_type(void)
{
    /* Test string type definition */
    TEST_ASSERT_EQUAL(0x0101, XY_TLV_TYPE_STRING);
    TEST_ASSERT_EQUAL(0x0102, XY_TLV_TYPE_BYTES);
    TEST_ASSERT_EQUAL(0x0103, XY_TLV_TYPE_BLOB);
}

void test_tlv_container_types(void)
{
    /* Test container type definitions */
    TEST_ASSERT_EQUAL(0x0201, XY_TLV_TYPE_CONTAINER);
    TEST_ASSERT_EQUAL(0x0202, XY_TLV_TYPE_ARRAY);
    TEST_ASSERT_EQUAL(0x0203, XY_TLV_TYPE_LIST);
}

void test_tlv_special_types(void)
{
    /* Test special type definitions */
    TEST_ASSERT_EQUAL(0x0301, XY_TLV_TYPE_TIMESTAMP);
    TEST_ASSERT_EQUAL(0x0302, XY_TLV_TYPE_UUID);
    TEST_ASSERT_EQUAL(0x0303, XY_TLV_TYPE_MAC_ADDR);
    TEST_ASSERT_EQUAL(0x0304, XY_TLV_TYPE_IPV4_ADDR);
    TEST_ASSERT_EQUAL(0x0305, XY_TLV_TYPE_IPV6_ADDR);
}

void test_tlv_error_codes(void)
{
    /* Test error code definitions */
    TEST_ASSERT_EQUAL(0, XY_TLV_OK);
    TEST_ASSERT_EQUAL(-1, XY_TLV_ERROR);
    TEST_ASSERT_EQUAL(-2, XY_TLV_INVALID_PARAM);
    TEST_ASSERT_EQUAL(-3, XY_TLV_BUFFER_OVERFLOW);
    TEST_ASSERT_EQUAL(-4, XY_TLV_BUFFER_UNDERFLOW);
    TEST_ASSERT_EQUAL(-5, XY_TLV_TYPE_MISMATCH);
    TEST_ASSERT_EQUAL(-6, XY_TLV_NOT_FOUND);
    TEST_ASSERT_EQUAL(-7, XY_TLV_INVALID_LENGTH);
    TEST_ASSERT_EQUAL(-8, XY_TLV_NESTING_OVERFLOW);
    TEST_ASSERT_EQUAL(-9, XY_TLV_CHECKSUM_ERROR);
}

/* ==================== TLV Encoding Tests ==================== */

void test_tlv_encode_uint8(void)
{
    uint8_t buffer[16];
    int result;

    /* Encode a uint8 TLV */
    result = xy_tlv_encode_uint8(buffer, sizeof(buffer), 0x0001, 0x42);
    TEST_ASSERT_TRUE(result >= XY_TLV_HEADER_SIZE + 1);

    /* Verify header */
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[2]); /* Length */
    TEST_ASSERT_EQUAL_UINT8(0x42, buffer[3]); /* Value */
}

void test_tlv_encode_uint16(void)
{
    uint8_t buffer[16];
    int result;

    /* Encode a uint16 TLV */
    result = xy_tlv_encode_uint16(buffer, sizeof(buffer), 0x0002, 0x1234);
    TEST_ASSERT_TRUE(result >= XY_TLV_HEADER_SIZE + 2);

    /* Verify header */
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x02, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0x02, buffer[2]); /* Length */
}

void test_tlv_encode_uint32(void)
{
    uint8_t buffer[16];
    int result;

    /* Encode a uint32 TLV */
    result = xy_tlv_encode_uint32(buffer, sizeof(buffer), 0x0003, 0x12345678);
    TEST_ASSERT_TRUE(result >= XY_TLV_HEADER_SIZE + 4);
}

void test_tlv_encode_string(void)
{
    uint8_t buffer[64];
    const char *test_str = "Hello, TLV!";
    int result;

    /* Encode a string TLV */
    result = xy_tlv_encode_string(buffer, sizeof(buffer), 0x0101, test_str);
    TEST_ASSERT_TRUE(result >= XY_TLV_HEADER_SIZE + strlen(test_str));
}

void test_tlv_encode_bytes(void)
{
    uint8_t buffer[64];
    const uint8_t test_data[] = { 0xDE, 0xAD, 0xBE, 0xEF };
    int result;

    /* Encode a bytes TLV */
    result = xy_tlv_encode_bytes(buffer, sizeof(buffer), 0x0102, test_data, sizeof(test_data));
    TEST_ASSERT_TRUE(result >= XY_TLV_HEADER_SIZE + sizeof(test_data));

    /* Verify data */
    TEST_ASSERT_EQUAL_UINT8(0xDE, buffer[4]);
    TEST_ASSERT_EQUAL_UINT8(0xAD, buffer[5]);
    TEST_ASSERT_EQUAL_UINT8(0xBE, buffer[6]);
    TEST_ASSERT_EQUAL_UINT8(0xEF, buffer[7]);
}

/* ==================== TLV Decoding Tests ==================== */

void test_tlv_decode_uint8(void)
{
    uint8_t buffer[16] = { 0x00, 0x01, 0x01, 0x42 }; /* Type=1, Len=1, Value=0x42 */
    uint16_t type;
    uint8_t value;
    int result;

    /* Decode */
    result = xy_tlv_decode_uint8(buffer, sizeof(buffer), &type, &value);
    TEST_ASSERT_TRUE(result >= 0);
    TEST_ASSERT_EQUAL_UINT16(0x0001, type);
    TEST_ASSERT_EQUAL_UINT8(0x42, value);
}

void test_tlv_decode_uint16(void)
{
    uint8_t buffer[16] = { 0x00, 0x02, 0x02, 0x12, 0x34 };
    uint16_t type;
    uint16_t value;
    int result;

    /* Decode */
    result = xy_tlv_decode_uint16(buffer, sizeof(buffer), &type, &value);
    TEST_ASSERT_TRUE(result >= 0);
    TEST_ASSERT_EQUAL_UINT16(0x0002, type);
    TEST_ASSERT_EQUAL_UINT16(0x1234, value);
}

void test_tlv_decode_string(void)
{
    uint8_t buffer[64] = { 0x01, 0x01, 0x0C, 'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd' };
    uint16_t type;
    char value[32];
    int result;

    /* Decode */
    result = xy_tlv_decode_string(buffer, sizeof(buffer), &type, value, sizeof(value));
    TEST_ASSERT_TRUE(result >= 0);
    TEST_ASSERT_EQUAL_UINT16(0x0101, type);
    TEST_ASSERT_EQUAL_STRING("Hello, World", value);
}

/* ==================== TLV Iterator Tests ==================== */

void test_tlv_iterator_init(void)
{
    uint8_t buffer[64] = { 0 };
    xy_tlv_iterator_t iter;

    /* Initialize iterator */
    xy_tlv_iterator_init(&iter, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_PTR(buffer, iter.buffer);
    TEST_ASSERT_EQUAL(0, iter.offset);
}

void test_tlv_iterator_next(void)
{
    uint8_t buffer[64];
    xy_tlv_iterator_t iter;
    xy_tlv_t tlv;
    int result;

    /* Build simple TLV stream */
    int pos = 0;
    pos += xy_tlv_encode_uint8(buffer + pos, sizeof(buffer) - pos, 0x0001, 10);
    pos += xy_tlv_encode_uint16(buffer + pos, sizeof(buffer) - pos, 0x0002, 200);

    /* Initialize iterator */
    xy_tlv_iterator_init(&iter, buffer, pos);

    /* Get first TLV */
    result = xy_tlv_iterator_next(&iter, &tlv);
    TEST_ASSERT_TRUE(result >= 0);
    TEST_ASSERT_EQUAL_UINT16(0x0001, tlv.type);

    /* Get second TLV */
    result = xy_tlv_iterator_next(&iter, &tlv);
    TEST_ASSERT_TRUE(result >= 0);
    TEST_ASSERT_EQUAL_UINT16(0x0002, tlv.type);
}

/* ==================== TLV Find Tests ==================== */

void test_tlv_find_by_type(void)
{
    uint8_t buffer[64];
    int pos = 0;
    int result;
    xy_tlv_t tlv;

    /* Build TLV stream with multiple types */
    pos += xy_tlv_encode_uint8(buffer + pos, sizeof(buffer) - pos, 0x0001, 10);
    pos += xy_tlv_encode_uint16(buffer + pos, sizeof(buffer) - pos, 0x0002, 200);
    pos += xy_tlv_encode_uint32(buffer + pos, sizeof(buffer) - pos, 0x0003, 30000);

    /* Find existing type */
    result = xy_tlv_find_by_type(buffer, pos, 0x0002, &tlv);
    TEST_ASSERT_TRUE(result >= 0);
    TEST_ASSERT_EQUAL_UINT16(0x0002, tlv.type);

    /* Find non-existing type */
    result = xy_tlv_find_by_type(buffer, pos, 0xFFFF, &tlv);
    TEST_ASSERT_EQUAL(XY_TLV_NOT_FOUND, result);
}

/* ==================== TLV Validation Tests ==================== */

void test_tlv_validate_valid(void)
{
    uint8_t buffer[16] = { 0x00, 0x01, 0x01, 0x42 };
    int result;

    /* Validate valid TLV */
    result = xy_tlv_validate(buffer, sizeof(buffer));
    TEST_ASSERT_TRUE(result >= 0);
}

void test_tlv_validate_invalid_length(void)
{
    uint8_t buffer[16] = { 0x00, 0x01, 0x10, 0x42 }; /* Length > available data */
    int result;

    /* Validate invalid TLV */
    result = xy_tlv_validate(buffer, sizeof(buffer));
    TEST_ASSERT_TRUE(result < 0);
}

/* ==================== TLV Buffer Tests ==================== */

void test_tlv_buffer_init(void)
{
    uint8_t storage[256];
    xy_tlv_buffer_t tlv_buf;

    xy_tlv_buffer_init(&tlv_buf, storage, sizeof(storage));
    TEST_ASSERT_EQUAL(0, tlv_buf.length);
    TEST_ASSERT_EQUAL_PTR(storage, tlv_buf.buffer);
    TEST_ASSERT_EQUAL(sizeof(storage), tlv_buf.capacity);
}

void test_tlv_buffer_append(void)
{
    uint8_t storage[256];
    xy_tlv_buffer_t tlv_buf;
    int result;

    xy_tlv_buffer_init(&tlv_buf, storage, sizeof(storage));

    /* Append uint8 */
    result = xy_tlv_buffer_append_uint8(&tlv_buf, 0x0001, 0x42);
    TEST_ASSERT_EQUAL(XY_TLV_OK, result);
    TEST_ASSERT_TRUE(tlv_buf.length > 0);

    /* Append uint16 */
    result = xy_tlv_buffer_append_uint16(&tlv_buf, 0x0002, 0x1234);
    TEST_ASSERT_EQUAL(XY_TLV_OK, result);
    TEST_ASSERT_TRUE(tlv_buf.length > 4);
}

void test_tlv_buffer_overflow(void)
{
    uint8_t storage[8];
    xy_tlv_buffer_t tlv_buf;
    int result;

    xy_tlv_buffer_init(&tlv_buf, storage, sizeof(storage));

    /* Try to append large data */
    result = xy_tlv_buffer_append_bytes(&tlv_buf, 0x0102, (const uint8_t *)"Hello, World!", 13);
    TEST_ASSERT_TRUE(result < 0); /* Should fail due to buffer overflow */
}

/* ==================== TLV Nesting Tests ==================== */

void test_tlv_container_start_end(void)
{
    uint8_t buffer[256];
    xy_tlv_buffer_t tlv_buf;
    int result;
    int container_start;

    xy_tlv_buffer_init(&tlv_buf, buffer, sizeof(buffer));

    /* Start container */
    result = xy_tlv_container_start(&tlv_buf, 0x0201, &container_start);
    TEST_ASSERT_EQUAL(XY_TLV_OK, result);

    /* Add content to container */
    result = xy_tlv_buffer_append_uint8(&tlv_buf, 0x0001, 0x42);
    TEST_ASSERT_EQUAL(XY_TLV_OK, result);

    /* End container */
    result = xy_tlv_container_end(&tlv_buf, container_start);
    TEST_ASSERT_EQUAL(XY_TLV_OK, result);
}

/* ==================== Main ==================== */

int main(void)
{
    UNITY_BEGIN();

    /* TLV Constants Tests */
    RUN_TEST(test_tlv_header_size);
    RUN_TEST(test_tlv_predefined_types);
    RUN_TEST(test_tlv_string_type);
    RUN_TEST(test_tlv_container_types);
    RUN_TEST(test_tlv_special_types);
    RUN_TEST(test_tlv_error_codes);

    /* TLV Encoding Tests */
    RUN_TEST(test_tlv_encode_uint8);
    RUN_TEST(test_tlv_encode_uint16);
    RUN_TEST(test_tlv_encode_uint32);
    RUN_TEST(test_tlv_encode_string);
    RUN_TEST(test_tlv_encode_bytes);

    /* TLV Decoding Tests */
    RUN_TEST(test_tlv_decode_uint8);
    RUN_TEST(test_tlv_decode_uint16);
    RUN_TEST(test_tlv_decode_string);

    /* TLV Iterator Tests */
    RUN_TEST(test_tlv_iterator_init);
    RUN_TEST(test_tlv_iterator_next);

    /* TLV Find Tests */
    RUN_TEST(test_tlv_find_by_type);

    /* TLV Validation Tests */
    RUN_TEST(test_tlv_validate_valid);
    RUN_TEST(test_tlv_validate_invalid_length);

    /* TLV Buffer Tests */
    RUN_TEST(test_tlv_buffer_init);
    RUN_TEST(test_tlv_buffer_append);
    RUN_TEST(test_tlv_buffer_overflow);

    /* TLV Nesting Tests */
    RUN_TEST(test_tlv_container_start_end);

    return UNITY_END();
}
