/**
 * @file test_xy_frame.c
 * @brief Byte-stream framing contracts
 *
 * Every one of these cases is a defect that actually shipped in the openCH probe before
 * the framer was extracted. The first two are the reason a delimiter cannot be used when
 * the payload is binary: 0x02 and 0x03 are ordinary counter bytes.
 */

#include "unity.h"
#include "xy_frame.h"

#include <string.h>

void setUp(void) {}
void tearDown(void) {}

static xy_frame_t frame;
static xy_frame_decoder_t decoder;

static void feed(const uint8_t *bytes, size_t count) {
    size_t i;
    for (i = 0; i < count; ++i)
        (void)xy_frame_decode_byte(&decoder, bytes[i], &frame);
}

/* Feed byte by byte and return the return value of the call that completed. */
static int feed_until_done(const uint8_t *bytes, size_t count) {
    size_t i;
    int result = XY_FRAME_NONE;
    for (i = 0; i < count; ++i) {
        result = xy_frame_decode_byte(&decoder, bytes[i], &frame);
        if (result != XY_FRAME_NONE)
            break;
    }
    return result;
}

static void test_a_frame_round_trips_through_the_encoder(void) {
    const uint8_t payload[] = {0x68, 0x75, 0x01};
    uint8_t wire[32];
    size_t length;

    xy_frame_decoder_reset(&decoder);
    length = xy_frame_encode('C', payload, sizeof(payload), wire, sizeof(wire));
    /* STX, length, type, payload, checksum. */
    TEST_ASSERT_EQUAL_INT(1 + 1 + 1 + sizeof(payload) + 1, (int)length);
    TEST_ASSERT_EQUAL_HEX8(XY_FRAME_STX, wire[0]);

    TEST_ASSERT_EQUAL_INT(XY_FRAME_COMPLETE, feed_until_done(wire, length));
    TEST_ASSERT_EQUAL_HEX8('C', frame.type);
    TEST_ASSERT_EQUAL_UINT8(sizeof(payload), frame.length);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(payload, frame.payload, sizeof(payload));
}

static void test_a_start_byte_inside_the_payload_does_not_split_the_frame(void) {
    /* 0x02 in the payload used to restart the frame, eating 285 of 2000 frames. */
    const uint8_t payload[] = {0x02, 0x00, 0x00, 0x00};
    uint8_t wire[32];
    size_t length;

    xy_frame_decoder_reset(&decoder);
    length = xy_frame_encode('I', payload, sizeof(payload), wire, sizeof(wire));
    TEST_ASSERT_EQUAL_INT(XY_FRAME_COMPLETE, feed_until_done(wire, length));
    TEST_ASSERT_EQUAL_HEX8_ARRAY(payload, frame.payload, sizeof(payload));
}

static void test_an_end_byte_inside_the_payload_does_not_end_the_frame(void) {
    /* 0x03 as a terminator truncated the frame that carried index 3. */
    const uint8_t payload[] = {0x03, 0x00, 0x00, 0x00};
    uint8_t wire[32];
    size_t length;

    xy_frame_decoder_reset(&decoder);
    length = xy_frame_encode('I', payload, sizeof(payload), wire, sizeof(wire));
    TEST_ASSERT_EQUAL_INT(XY_FRAME_COMPLETE, feed_until_done(wire, length));
    TEST_ASSERT_EQUAL_UINT8(sizeof(payload), frame.length);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(payload, frame.payload, sizeof(payload));
}

static void test_a_truncated_frame_is_not_reported_as_complete(void) {
    uint8_t wire[] = {XY_FRAME_STX, 3, 'C', 0x68, 0x75};

    xy_frame_decoder_reset(&decoder);
    TEST_ASSERT_EQUAL_INT(XY_FRAME_NONE, feed_until_done(wire, sizeof(wire)));
}

static void test_a_bad_checksum_is_rejected(void) {
    uint8_t wire[] = {XY_FRAME_STX, 2, 'C', 0x68, 0x00};

    xy_frame_decoder_reset(&decoder);
    TEST_ASSERT_EQUAL_INT(XY_FRAME_REJECTED, feed_until_done(wire, sizeof(wire)));
}

static void test_after_a_rejected_frame_the_next_one_still_decodes(void) {
    const uint8_t payload[] = {0x68, 0x75};
    uint8_t good[32];
    uint8_t corrupt[] = {XY_FRAME_STX, 2, 'C', 0x68, 0x00};
    size_t length;

    xy_frame_decoder_reset(&decoder);
    feed(corrupt, sizeof(corrupt));
    length = xy_frame_encode('C', payload, sizeof(payload), good, sizeof(good));
    TEST_ASSERT_EQUAL_INT(XY_FRAME_COMPLETE, feed_until_done(good, length));
    TEST_ASSERT_EQUAL_HEX8('C', frame.type);
}

static void test_noise_before_a_frame_is_ignored(void) {
    const uint8_t payload[] = {0x01};
    uint8_t garbage[] = {0x55, 0xAA, 0x00, 0xFF};
    uint8_t wire[32];
    size_t length;

    xy_frame_decoder_reset(&decoder);
    feed(garbage, sizeof(garbage));
    length = xy_frame_encode('C', payload, sizeof(payload), wire, sizeof(wire));
    TEST_ASSERT_EQUAL_INT(XY_FRAME_COMPLETE, feed_until_done(wire, length));
}

static void test_an_impossible_length_is_rejected(void) {
    /* A length the buffer cannot hold must be refused outright, not wrapped. */
    uint8_t wire[] = {XY_FRAME_STX, XY_FRAME_PAYLOAD_MAX + 8, 'C'};

    xy_frame_decoder_reset(&decoder);
    TEST_ASSERT_EQUAL_INT(XY_FRAME_REJECTED, feed_until_done(wire, sizeof(wire)));
}

static void test_encoding_refuses_a_buffer_that_cannot_hold_the_frame(void) {
    const uint8_t payload[] = {1, 2, 3};
    uint8_t wire[4];

    TEST_ASSERT_EQUAL_size_t(0, xy_frame_encode('C', payload, sizeof(payload), wire,
                                                sizeof(wire)));
}

static void test_a_frame_longer_than_the_payload_limit_is_refused(void) {
    uint8_t payload[XY_FRAME_PAYLOAD_MAX + 1];
    uint8_t wire[64];

    memset(payload, 0x11, sizeof(payload));
    TEST_ASSERT_EQUAL_size_t(0, xy_frame_encode('C', payload, sizeof(payload), wire,
                                                sizeof(wire)));
}

static void test_the_crc_matches_the_published_check_value(void) {
    /* CRC-8/ATM("123456789") is 0xF4. A check value is what makes this the standard
       polynomial rather than eight bits of arbitrary feedback. */
    const uint8_t vector[] = "123456789";
    TEST_ASSERT_EQUAL_HEX8(0xF4, xy_frame_crc8(vector, sizeof(vector) - 1));
}

static void test_the_crc_sees_a_transposition_a_xor_sum_cannot(void) {
    /* {0x12,0x34} and {0x34,0x12} have identical xor sums, so the previous checksum
       accepted a frame whose counter bytes had swapped. That is the reason the check
       changed, so it is worth pinning. */
    const uint8_t one[] = {0x12, 0x34};
    const uint8_t other[] = {0x34, 0x12};

    TEST_ASSERT_EQUAL_HEX8(one[0] ^ one[1], other[0] ^ other[1]);
    TEST_ASSERT_NOT_EQUAL(xy_frame_crc8(one, sizeof(one)), xy_frame_crc8(other, sizeof(other)));
}

static void test_the_crc_covers_length_mismatches(void) {
    const uint8_t short_one[] = {0x01, 0x02, 0x03};
    const uint8_t long_one[] = {0x01, 0x02, 0x03, 0x00};

    TEST_ASSERT_NOT_EQUAL(xy_frame_crc8(short_one, sizeof(short_one)),
                          xy_frame_crc8(long_one, sizeof(long_one)));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_a_frame_round_trips_through_the_encoder);
    RUN_TEST(test_a_start_byte_inside_the_payload_does_not_split_the_frame);
    RUN_TEST(test_an_end_byte_inside_the_payload_does_not_end_the_frame);
    RUN_TEST(test_a_truncated_frame_is_not_reported_as_complete);
    RUN_TEST(test_a_bad_checksum_is_rejected);
    RUN_TEST(test_after_a_rejected_frame_the_next_one_still_decodes);
    RUN_TEST(test_noise_before_a_frame_is_ignored);
    RUN_TEST(test_an_impossible_length_is_rejected);
    RUN_TEST(test_encoding_refuses_a_buffer_that_cannot_hold_the_frame);
    RUN_TEST(test_a_frame_longer_than_the_payload_limit_is_refused);
    RUN_TEST(test_the_crc_matches_the_published_check_value);
    RUN_TEST(test_the_crc_sees_a_transposition_a_xor_sum_cannot);
    RUN_TEST(test_the_crc_covers_length_mismatches);
    return UNITY_END();
}
