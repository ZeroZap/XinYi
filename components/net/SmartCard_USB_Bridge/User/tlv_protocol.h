/********************************** (C) COPYRIGHT
 ******************************** File Name          : tlv_protocol.h Author :
 *WCH Version            : V1.0.0 Date               : 2025/10/30 Description :
 *TLV Protocol for SmartCard-USB Bridge
 *******************************************************************************/

#ifndef __TLV_PROTOCOL_H__
#define __TLV_PROTOCOL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>

/******************************************************************************/
/* TLV Protocol Definitions */

/* TLV Tag Definitions */
#define TLV_TAG_APDU_REQUEST    0x01 /* APDU request from PC to SIM */
#define TLV_TAG_APDU_RESPONSE   0x02 /* APDU response from SIM to PC */
#define TLV_TAG_ATR_DATA        0x03 /* ATR data from SIM to PC */
#define TLV_TAG_RESET_SIM       0x04 /* Reset SIM card command */
#define TLV_TAG_POWER_ON        0x05 /* Power on SIM card */
#define TLV_TAG_POWER_OFF       0x06 /* Power off SIM card */
#define TLV_TAG_STATUS_QUERY    0x07 /* Query status */
#define TLV_TAG_STATUS_RESPONSE 0x08 /* Status response */
#define TLV_TAG_ERROR           0x09 /* Error response */
#define TLV_TAG_ACK             0x0A /* Acknowledgement */
#define TLV_TAG_GET_INFO        0x0B /* Get card info */
#define TLV_TAG_INFO_RESPONSE   0x0C /* Card info response */
#define TLV_TAG_SET_UI_INFO     0x0D /* Set UI indicator (LED control) */

/* Error Codes */
#define TLV_ERR_NONE             0x00 /* No error */
#define TLV_ERR_INVALID_TAG      0x01 /* Invalid TLV tag */
#define TLV_ERR_INVALID_LENGTH   0x02 /* Invalid TLV length */
#define TLV_ERR_BUFFER_OVERFLOW  0x03 /* Buffer overflow */
#define TLV_ERR_NO_CARD          0x04 /* No card detected */
#define TLV_ERR_CARD_ERROR       0x05 /* Card communication error */
#define TLV_ERR_ATR_PARSE_FAILED 0x06 /* ATR parsing failed */
#define TLV_ERR_APDU_FAILED      0x07 /* APDU command failed */
#define TLV_ERR_TIMEOUT          0x08 /* Timeout error */

/* Status Codes */
#define TLV_STATUS_IDLE         0x00 /* Idle state */
#define TLV_STATUS_CARD_PRESENT 0x01 /* Card present */
#define TLV_STATUS_CARD_ACTIVE  0x02 /* Card active */
#define TLV_STATUS_BUSY         0x03 /* Busy processing */

/* UI Indicator Values for TLV_TAG_SET_UI_INFO */
#define UI_LED_OFF          0x00 /* All LEDs off */
#define UI_LED_GREEN        0x01 /* Green LED on (card ready) */
#define UI_LED_RED          0x02 /* Red LED on (error/busy) */
#define UI_LED_YELLOW       0x03 /* Yellow LED on (processing) */
#define UI_LED_BLUE         0x04 /* Blue LED on (waiting) */
#define UI_LED_BLINK_GREEN  0x11 /* Green LED blinking */
#define UI_LED_BLINK_RED    0x12 /* Red LED blinking */
#define UI_LED_BLINK_YELLOW 0x13 /* Yellow LED blinking */

/* Protocol Limits */
#define TLV_MAX_PAYLOAD_SIZE 512 /* Maximum TLV payload size */
#define TLV_HEADER_SIZE      3   /* Tag(1) + Length(2) */

/******************************************************************************/
/* TLV Structure Definitions */

/* TLV Header Structure */
typedef struct __attribute__((packed)) {
    uint8_t tag;     /* TLV tag */
    uint16_t length; /* TLV length (big-endian) */
} TLV_Header;

/* TLV Packet Structure */
typedef struct {
    TLV_Header header;
    uint8_t value[TLV_MAX_PAYLOAD_SIZE];
} TLV_Packet;

/******************************************************************************/
/* Function Prototypes */

/**
 * @brief  Build a TLV packet
 * @param  tlv: Pointer to TLV packet structure
 * @param  tag: TLV tag
 * @param  value: Pointer to value data
 * @param  length: Length of value data
 * @return 0: Success, other: Error code
 */
uint8_t TLV_Build(TLV_Packet *tlv, uint8_t tag, const uint8_t *value,
                  uint16_t length);

/**
 * @brief  Parse a TLV packet from buffer
 * @param  tlv: Pointer to TLV packet structure
 * @param  buffer: Pointer to input buffer
 * @param  buffer_len: Length of input buffer
 * @return Parsed TLV total length (header + value), 0 if error
 */
uint16_t TLV_Parse(TLV_Packet *tlv, const uint8_t *buffer, uint16_t buffer_len);

/**
 * @brief  Serialize TLV packet to buffer
 * @param  tlv: Pointer to TLV packet structure
 * @param  buffer: Pointer to output buffer
 * @param  buffer_len: Length of output buffer
 * @return Serialized length, 0 if error
 */
uint16_t TLV_Serialize(const TLV_Packet *tlv, uint8_t *buffer,
                       uint16_t buffer_len);

/**
 * @brief  Get total TLV packet size
 * @param  tlv: Pointer to TLV packet structure
 * @return Total size (header + value)
 */
uint16_t TLV_GetTotalSize(const TLV_Packet *tlv);

/**
 * @brief  Build error response TLV
 * @param  tlv: Pointer to TLV packet structure
 * @param  error_code: Error code
 * @return 0: Success
 */
uint8_t TLV_BuildErrorResponse(TLV_Packet *tlv, uint8_t error_code);

/**
 * @brief  Build ACK response TLV
 * @param  tlv: Pointer to TLV packet structure
 * @return 0: Success
 */
uint8_t TLV_BuildAck(TLV_Packet *tlv);

#ifdef __cplusplus
}
#endif

#endif /* __TLV_PROTOCOL_H__ */
