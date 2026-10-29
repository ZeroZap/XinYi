/**
 * @file xy_frame.c
 * @brief Length-prefixed frame codec for byte streams
 * @version 1.0
 * @date 2026-10-29
 */

#include "xy_frame.h"

/*
 * CRC-8/ATM: polynomial 0x07, initial value 0x00, no reflection, no final xor. The check
 * value for "123456789" is 0xF4, which the host test pins.
 *
 * This used to be a plain xor. A xor checksum cannot see two errors that cancel, and it
 * cannot see a transposition of two bytes at all, so a frame carrying a misordered
 * counter looked perfect. CRC-8 catches every single-bit error, every double-bit error,
 * and every odd-length burst up to eight bits, which is what a byte stream actually does
 * when it goes wrong.
 */
uint8_t xy_frame_crc8(const uint8_t *bytes, size_t count)
{
    uint8_t crc = XY_FRAME_CHECKSUM_SEED;
    size_t i;

    if (bytes == NULL) {
        return crc;
    }
    for (i = 0; i < count; ++i) {
        uint8_t bit;
        crc ^= bytes[i];
        for (bit = 0U; bit < 8U; ++bit) {
            if ((crc & 0x80U) != 0U) {
                crc = (uint8_t)((uint8_t)(crc << 1) ^ 0x07U);
            } else {
                crc = (uint8_t)(crc << 1);
            }
        }
    }
    return crc;
}

static uint8_t frame_checksum(const uint8_t *bytes, uint8_t count)
{
    return xy_frame_crc8(bytes, count);
}

void xy_frame_decoder_reset(xy_frame_decoder_t *decoder)
{
    if (decoder == NULL) {
        return;
    }
    decoder->count = 0U;
    decoder->want = 0U;
}

int xy_frame_decode_byte(xy_frame_decoder_t *decoder, uint8_t byte, xy_frame_t *frame)
{
    if (decoder == NULL || frame == NULL) {
        return XY_FRAME_REJECTED;
    }

    /* Idle: only a start byte means anything. Inside a frame, 0x02 is an ordinary
       payload value and must not be mistaken for a new start. */
    if (decoder->want == 0U) {
        if (byte == XY_FRAME_STX) {
            decoder->want = 1U;
            decoder->count = 0U;
        }
        return XY_FRAME_NONE;
    }

    if (decoder->want == 1U) {
        /* The length counts the payload plus its checksum byte. A length this decoder
           cannot hold is refused now rather than allowed to wrap the buffer. */
        if (byte < 2U || byte > XY_FRAME_PAYLOAD_MAX + 1U) {
            xy_frame_decoder_reset(decoder);
            return XY_FRAME_REJECTED;
        }
        decoder->want = (uint8_t)(byte + 1U); /* what still has to arrive */
        return XY_FRAME_NONE;
    }

    decoder->buffer[decoder->count++] = byte;
    if (decoder->count < decoder->want) {
        return XY_FRAME_NONE;
    }

    /* The last byte is the checksum over everything before it. */
    {
        uint8_t body = (uint8_t)(decoder->want - 1U);
        uint8_t expected = frame_checksum(decoder->buffer, body);
        uint8_t actual = decoder->buffer[body];

        xy_frame_decoder_reset(decoder);
        if (expected != actual) {
            return XY_FRAME_REJECTED;
        }
        if (body < 1U) {
            return XY_FRAME_REJECTED;
        }
        frame->type = decoder->buffer[0];
        frame->length = (uint8_t)(body - 1U);
        if (frame->length > 0U) {
            uint8_t i;
            for (i = 0U; i < frame->length; ++i) {
                frame->payload[i] = decoder->buffer[i + 1U];
            }
        }
        return XY_FRAME_COMPLETE;
    }
}

size_t xy_frame_encode(uint8_t type, const uint8_t *payload, uint8_t length, uint8_t *out,
                       size_t capacity)
{
    size_t total;
    uint8_t i;
    uint8_t checksum;

    if (out == NULL || length > XY_FRAME_PAYLOAD_MAX) {
        return 0U;
    }
    if (length > 0U && payload == NULL) {
        return 0U;
    }
    total = (size_t)length + 4U; /* STX, length, type, payload..., checksum */
    if (capacity < total) {
        return 0U;
    }

    out[0] = XY_FRAME_STX;
    out[1] = (uint8_t)(length + 1U);
    out[2] = type;
    for (i = 0U; i < length; ++i) {
        out[3U + i] = payload[i];
    }
    checksum = frame_checksum(&out[2], (uint8_t)(length + 1U));
    out[3U + length] = checksum;
    return total;
}
