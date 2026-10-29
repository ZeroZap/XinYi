/**
 * @file xy_frame.h
 * @brief Length-prefixed frame codec for byte streams
 * @version 1.0
 * @date 2026-10-29
 *
 * A frame on the wire is: STX, length, type, payload..., checksum. The length is what
 * makes this safe to put binary data in - a terminator byte cannot be used, because
 * every value is a legal payload byte. Both mistakes were made in the openCH probe
 * before this was extracted: a 0x03 terminator truncated the frame carrying index 3,
 * and a 0x02 inside the payload was read as a new frame start, which silently ate 285
 * of 2000 frames.
 *
 * The decoder resynchronises on STX only while it is idle. Bytes before a start byte are
 * noise; a frame that fails its checksum is reported as rejected and the decoder returns
 * to idle, so the next start byte recovers the stream.
 */

#ifndef XY_FRAME_H
#define XY_FRAME_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XY_FRAME_STX 0x02U           /**< Start of frame */
#define XY_FRAME_CHECKSUM_SEED 0x5AU /**< Checksum seed, so an all-zero payload is not free */
#define XY_FRAME_PAYLOAD_MAX 16U     /**< Largest payload a frame may carry */

/** Decoder result codes. */
#define XY_FRAME_NONE 0     /**< Nothing complete yet */
#define XY_FRAME_COMPLETE 1 /**< A frame was decoded and is valid */
#define XY_FRAME_REJECTED (-1) /**< A frame ended but did not check out */

/**
 * @brief A decoded frame
 */
typedef struct {
    uint8_t type;                          /**< First payload byte, by convention the kind */
    uint8_t length;                        /**< Payload length following the type */
    uint8_t payload[XY_FRAME_PAYLOAD_MAX]; /**< Remaining payload bytes */
} xy_frame_t;

/**
 * @brief Decoder state; keep it zeroed, or initialise it with xy_frame_decoder_reset()
 */
typedef struct {
    uint8_t buffer[XY_FRAME_PAYLOAD_MAX + 1U];
    uint8_t count;
    uint8_t want;
} xy_frame_decoder_t;

/**
 * @brief Return a decoder to its idle state
 * @param decoder Decoder
 */
void xy_frame_decoder_reset(xy_frame_decoder_t *decoder);

/**
 * @brief Feed one byte to the decoder
 * @param decoder Decoder
 * @param byte Byte received
 * @param frame Receives the frame when XY_FRAME_COMPLETE is returned
 * @return XY_FRAME_NONE, XY_FRAME_COMPLETE or XY_FRAME_REJECTED
 */
int xy_frame_decode_byte(xy_frame_decoder_t *decoder, uint8_t byte, xy_frame_t *frame);

/**
 * @brief Encode a frame
 * @param type Frame type
 * @param payload Payload bytes
 * @param length Payload length, at most XY_FRAME_PAYLOAD_MAX
 * @param out Destination buffer
 * @param capacity Size of the destination
 * @return Number of bytes written, or 0 when the arguments do not fit
 */
size_t xy_frame_encode(uint8_t type, const uint8_t *payload, uint8_t length, uint8_t *out,
                       size_t capacity);

#ifdef __cplusplus
}
#endif

#endif /* XY_FRAME_H */
