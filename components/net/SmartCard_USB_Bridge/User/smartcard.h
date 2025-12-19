/********************************** (C) COPYRIGHT
 ******************************** File Name          : smartcard.h Author : WCH
 * Version            : V1.0.0
 * Date               : 2025/10/30
 * Description        : SmartCard interface header for USB Bridge
 *******************************************************************************/

#ifndef __SMARTCARD_H__
#define __SMARTCARD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "debug.h"
#include <stdint.h>

/******************************************************************************/
/* SmartCard Definitions */

/* ATR Definitions */
#define ATR_MAX_SIZE       33
#define ATR_MAX_HISTORICAL 15
#define ATR_MAX_PROTOCOLS  7
#define ATR_MAX_IB         4

/* Interface Byte Indexes */
#define ATR_INTERFACE_BYTE_TA 0
#define ATR_INTERFACE_BYTE_TB 1
#define ATR_INTERFACE_BYTE_TC 2
#define ATR_INTERFACE_BYTE_TD 3

/* Pin Definitions */
#define SC_CK_PIN    GPIO_Pin_4
#define SC_IO_PIN    GPIO_Pin_2
#define SC_RST_PIN   GPIO_Pin_5
#define SC_GPIO_PORT GPIOA

/* USART Mode Definitions */
#define USART_Rx_Mode 0x0004
#define USART_Tx_Mode 0x0008
#define SC_USART_MODE (USART_Rx_Mode | USART_Tx_Mode)

/* Timeout */
#define SC_TIMEOUT_COUNT 500

/* APDU Command Limits */
#define SC_MAX_APDU_SIZE 512

/******************************************************************************/
/* SmartCard Structure Definitions */

/* ATR Structure */
typedef struct {
    uint32_t length;
    uint8_t TS; /* Initial character */
    uint8_t T0; /* Format character */

    struct {
        uint8_t value;
        uint8_t present;
    } ib[ATR_MAX_PROTOCOLS][ATR_MAX_IB], TCK;

    uint8_t pn;                     /* Protocol number */
    uint8_t Ti[ATR_MAX_HISTORICAL]; /* Historical bytes */
    uint8_t Tin;                    /* Historical bytes number */
    int8_t protocol;                /* Selected protocol */
} SC_ATR_TypeDef;

/* SmartCard Status */
typedef struct {
    uint8_t card_present; /* Card present flag */
    uint8_t card_active;  /* Card active flag */
    uint8_t atr_valid;    /* ATR valid flag */
    uint8_t protocol;     /* Current protocol (T=0, T=1) */
    SC_ATR_TypeDef atr;   /* ATR data */
} SC_Status_TypeDef;

/******************************************************************************/
/* Function Prototypes */

/**
 * @brief  Initialize SmartCard interface
 */
void SC_Init(void);

/**
 * @brief  Reset SmartCard and get ATR
 * @param  atr: Pointer to ATR structure
 * @return 0: Success, 1: Failed
 */
uint8_t SC_ResetAndGetATR(SC_ATR_TypeDef *atr);

/**
 * @brief  Parse ATR data
 * @param  atr: Pointer to ATR structure
 * @param  atr_buf: Raw ATR data buffer
 * @param  len: ATR data length
 * @return 0: Success, 1: Failed
 */
uint8_t SC_ParseATR(SC_ATR_TypeDef *atr, uint8_t *atr_buf, uint32_t len);

/**
 * @brief  Transmit data to SmartCard
 * @param  data: Data buffer to send
 * @param  len: Data length
 */
void SC_TransmitData(uint8_t *data, uint16_t len);

/**
 * @brief  Receive data from SmartCard
 * @param  data: Data buffer to receive
 * @param  len: Pointer to received data length
 */
void SC_ReceiveData(uint8_t *data, uint16_t *len);

/**
 * @brief  Send APDU command to SmartCard
 * @param  apdu: APDU command buffer
 * @param  apdu_len: APDU command length
 * @param  response: Response buffer
 * @param  response_len: Pointer to response length
 * @return 0: Success, 1: Failed
 */
uint8_t SC_SendAPDU(uint8_t *apdu, uint16_t apdu_len, uint8_t *response,
                    uint16_t *response_len);

/**
 * @brief  Power on SmartCard
 */
void SC_PowerOn(void);

/**
 * @brief  Power off SmartCard
 */
void SC_PowerOff(void);

/**
 * @brief  Get SmartCard status
 * @param  status: Pointer to status structure
 */
void SC_GetStatus(SC_Status_TypeDef *status);

#ifdef __cplusplus
}
#endif

#endif /* __SMARTCARD_H__ */
