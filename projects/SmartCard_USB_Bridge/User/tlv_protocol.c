/********************************** (C) COPYRIGHT
 ******************************** File Name          : tlv_protocol.c Author :
 *WCH Version            : V1.0.0 Date               : 2025/10/30 Description :
 *TLV Protocol Implementation for SmartCard-USB Bridge
 *******************************************************************************/

#include "tlv_protocol.h"

/**
 * @brief  Build a TLV packet
 */
uint8_t TLV_Build(TLV_Packet *tlv, uint8_t tag, const uint8_t *value,
                  uint16_t length)
{
    if (tlv == NULL) {
        return TLV_ERR_INVALID_TAG;
    }

    if (length > TLV_MAX_PAYLOAD_SIZE) {
        return TLV_ERR_INVALID_LENGTH;
    }

    tlv->header.tag    = tag;
    tlv->header.length = length; /* Store in host byte order internally */

    if (length > 0 && value != NULL) {
        memcpy(tlv->value, value, length);
    }

    return TLV_ERR_NONE;
}

/**
 * @brief  Parse a TLV packet from buffer
 */
uint16_t TLV_Parse(TLV_Packet *tlv, const uint8_t *buffer, uint16_t buffer_len)
{
    uint16_t length;

    if (tlv == NULL || buffer == NULL || buffer_len < TLV_HEADER_SIZE) {
        return 0;
    }

    /* Parse header */
    tlv->header.tag = buffer[0];
    /* Big-endian to host */
    tlv->header.length = ((uint16_t)buffer[1] << 8) | buffer[2];
    length             = tlv->header.length;

    /* Validate length */
    if (length > TLV_MAX_PAYLOAD_SIZE) {
        return 0;
    }

    /* Check if buffer has enough data */
    if (buffer_len < (TLV_HEADER_SIZE + length)) {
        return 0;
    }

    /* Copy value */
    if (length > 0) {
        memcpy(tlv->value, buffer + TLV_HEADER_SIZE, length);
    }

    return TLV_HEADER_SIZE + length;
}

/**
 * @brief  Serialize TLV packet to buffer
 */
uint16_t TLV_Serialize(const TLV_Packet *tlv, uint8_t *buffer,
                       uint16_t buffer_len)
{
    uint16_t total_len;

    if (tlv == NULL || buffer == NULL) {
        return 0;
    }

    total_len = TLV_HEADER_SIZE + tlv->header.length;

    if (buffer_len < total_len) {
        return 0;
    }

    /* Serialize header */
    buffer[0] = tlv->header.tag;
    /* Host to big-endian */
    buffer[1] = (uint8_t)(tlv->header.length >> 8);
    buffer[2] = (uint8_t)(tlv->header.length & 0xFF);

    /* Copy value */
    if (tlv->header.length > 0) {
        memcpy(buffer + TLV_HEADER_SIZE, tlv->value, tlv->header.length);
    }

    return total_len;
}

/**
 * @brief  Get total TLV packet size
 */
uint16_t TLV_GetTotalSize(const TLV_Packet *tlv)
{
    if (tlv == NULL) {
        return 0;
    }

    return TLV_HEADER_SIZE + tlv->header.length;
}

/**
 * @brief  Build error response TLV
 */
uint8_t TLV_BuildErrorResponse(TLV_Packet *tlv, uint8_t error_code)
{
    return TLV_Build(tlv, TLV_TAG_ERROR, &error_code, 1);
}

/**
 * @brief  Build ACK response TLV
 */
uint8_t TLV_BuildAck(TLV_Packet *tlv)
{
    return TLV_Build(tlv, TLV_TAG_ACK, NULL, 0);
}
