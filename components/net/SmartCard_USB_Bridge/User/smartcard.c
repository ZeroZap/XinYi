/********************************** (C) COPYRIGHT
 ******************************** File Name          : smartcard.c Author : WCH
 * Version            : V1.0.0
 * Date               : 2025/10/30
 * Description        : SmartCard interface implementation for USB Bridge
 *******************************************************************************/

#include "smartcard.h"
#include "debug.h"
#include "ch32x035_conf.h"

/******************************************************************************/
/* Private Variables */
static SC_Status_TypeDef SC_Status = { 0 };

/******************************************************************************/
/* ATR Parse State Machine */
#define STATE_PARSE_TS         1
#define STATE_PARSE_T0         2
#define STATE_PARSE_TA         3
#define STATE_PARSE_TB         4
#define STATE_PARSE_TC         5
#define STATE_PARSE_TD         6
#define STATE_PARSE_HIST_BYTES 7
#define STATE_PARSE_TCK        8
#define STATE_PARSE_END        255

/**
 * @brief  Initialize SmartCard USART2 interface
 */
void SC_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure             = { 0 };
    USART_InitTypeDef USART_InitStructure           = { 0 };
    USART_ClockInitTypeDef USART_ClockInitStructure = { 0 };

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    /* USART2 CK --> PA.4, TX/RX --> PA.2 */
    GPIO_InitStructure.GPIO_Pin   = SC_IO_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(SC_GPIO_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin  = SC_CK_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(SC_GPIO_PORT, &GPIO_InitStructure);

    /* Configure USART2 Clock */
    USART_ClockInitStructure.USART_Clock = USART_Clock_Enable;
    USART_ClockInit(USART2, &USART_ClockInitStructure);

    /* Configure USART2 SmartCard Mode */
    USART_InitStructure.USART_BaudRate   = 9216;
    USART_InitStructure.USART_WordLength = USART_WordLength_9b;
    USART_InitStructure.USART_StopBits   = USART_StopBits_1_5;
    USART_InitStructure.USART_Parity     = USART_Parity_Even;
    USART_InitStructure.USART_HardwareFlowControl =
        USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx;
    USART_Init(USART2, &USART_InitStructure);

    USART_Cmd(USART2, ENABLE);

    /* Enable NACK transmission */
    USART_SmartCardNACKCmd(USART2, ENABLE);

    /* Enable SmartCard mode */
    USART_SmartCardCmd(USART2, ENABLE);

    /* Set prescaler: baud * ETU * Psc * 2 = SysClock */
    /* 9216 * 372 * 7 * 2 ≈ 48000000 */
    USART_SetPrescaler(USART2, 7);
    USART_SetGuardTime(USART2, 0);

    /* Initialize RST pin */
    GPIO_InitStructure.GPIO_Pin   = SC_RST_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(SC_GPIO_PORT, &GPIO_InitStructure);
    GPIO_ResetBits(SC_GPIO_PORT, SC_RST_PIN);

    /* Initialize status */
    memset(&SC_Status, 0, sizeof(SC_Status));
    SC_Status.card_present = 0;
    SC_Status.card_active  = 0;
    SC_Status.atr_valid    = 0;
}

/**
 * @brief  Transmit data to SmartCard
 */
void SC_TransmitData(uint8_t *data, uint16_t len)
{
    uint16_t i;

    USART2->CTLR1 &= ~SC_USART_MODE;
    USART2->CTLR1 |= USART_Tx_Mode;

    for (i = 0; i < len; i++) {
        USART_SendData(USART2, data[i]);
        while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET)
            ;
    }
}

/**
 * @brief  Receive data from SmartCard
 */
void SC_ReceiveData(uint8_t *data, uint16_t *len)
{
    uint32_t i = 0, timeout = 0;

    USART2->CTLR1 &= ~SC_USART_MODE;
    USART2->CTLR1 |= USART_Rx_Mode;

    while (1) {
        if (USART_GetFlagStatus(USART2, USART_FLAG_RXNE) == SET) {
            data[i] = USART_ReceiveData(USART2);
            i++;
            timeout = 0;
        } else if (timeout < SC_TIMEOUT_COUNT) {
            Delay_Us(500);
            timeout++;
        } else {
            break;
        }
    }
    *len = i;
}

/**
 * @brief  Parse ATR data
 */
uint8_t SC_ParseATR(SC_ATR_TypeDef *atr, uint8_t *atr_buf, uint32_t len)
{
    uint8_t data;
    uint8_t TCK = 0;
    uint8_t K   = 0;
    uint8_t Yi  = 0;
    int32_t k, state, index, length, protocol;
    uint8_t *ptr;

    length   = len;
    ptr      = atr_buf;
    state    = STATE_PARSE_TS;
    index    = 0;
    k        = 0;
    protocol = 0;

    memset(atr, 0, sizeof(SC_ATR_TypeDef));

    while (ptr < (atr_buf + length)) {
        data = *ptr++;
        if (state != STATE_PARSE_TS) {
            TCK ^= data;
        }

        switch (state) {
        case STATE_PARSE_TS:
            atr->TS = data;
            if (data == 0x3b) {
                state = STATE_PARSE_T0;
            } else if (data == 0x3f) {
                return 1; /* Inverse convention not supported */
            } else {
                return 1; /* Invalid TS */
            }
            break;

        case STATE_PARSE_T0:
            atr->T0  = data;
            atr->Tin = data & 0x0f;
            K        = data & 0x0F;
            Yi       = data;
            if (data & 0x10) {
                state = STATE_PARSE_TA;
            } else if (data & 0x20) {
                state = STATE_PARSE_TB;
            } else if (data & 0x40) {
                state = STATE_PARSE_TC;
            } else if (data & 0x80) {
                state = STATE_PARSE_TD;
            } else {
                state = STATE_PARSE_HIST_BYTES;
            }
            break;

        case STATE_PARSE_TA:
            atr->ib[index][ATR_INTERFACE_BYTE_TA].present = 1;
            atr->ib[index][ATR_INTERFACE_BYTE_TA].value   = data;
            if (Yi & 0x20) {
                state = STATE_PARSE_TB;
            } else if (Yi & 0x40) {
                state = STATE_PARSE_TC;
            } else if (Yi & 0x80) {
                state = STATE_PARSE_TD;
            } else {
                state = STATE_PARSE_HIST_BYTES;
            }
            break;

        case STATE_PARSE_TB:
            atr->ib[index][ATR_INTERFACE_BYTE_TB].present = 1;
            atr->ib[index][ATR_INTERFACE_BYTE_TB].value   = data;
            if (Yi & 0x40) {
                state = STATE_PARSE_TC;
            } else if (Yi & 0x80) {
                state = STATE_PARSE_TD;
            } else {
                state = STATE_PARSE_HIST_BYTES;
            }
            break;

        case STATE_PARSE_TC:
            atr->ib[index][ATR_INTERFACE_BYTE_TC].present = 1;
            atr->ib[index][ATR_INTERFACE_BYTE_TC].value   = data;
            if (Yi & 0x80) {
                state = STATE_PARSE_TD;
            } else {
                state = STATE_PARSE_HIST_BYTES;
            }
            break;

        case STATE_PARSE_TD:
            Yi                                            = data;
            atr->ib[index][ATR_INTERFACE_BYTE_TD].present = 1;
            atr->ib[index][ATR_INTERFACE_BYTE_TD].value   = data;
            if (index == 0) {
                protocol = Yi & 0x0F;
            }
            index++;
            atr->pn = index;

            if (Yi & 0xF0) {
                if (Yi & 0x10) {
                    state = STATE_PARSE_TA;
                } else if (Yi & 0x20) {
                    state = STATE_PARSE_TB;
                } else if (Yi & 0x40) {
                    state = STATE_PARSE_TC;
                } else if (Yi & 0x80) {
                    state = STATE_PARSE_TD;
                }
            } else {
                state = STATE_PARSE_HIST_BYTES;
            }
            break;

        case STATE_PARSE_HIST_BYTES:
            if (K) {
                if (k < K) {
                    atr->Ti[k] = data;
                    k++;
                    if (k == K) {
                        if (protocol > 0) {
                            state = STATE_PARSE_TCK;
                        }
                    }
                }
            }
            break;

        case STATE_PARSE_TCK:
            atr->TCK.present = 1;
            atr->TCK.value   = data;
            if (TCK != 0) {
                return 1; /* TCK error */
            }
            break;
        }

        if (state == STATE_PARSE_HIST_BYTES && K == 0 && protocol == 0)
            break;
    }

    atr->protocol = protocol;
    atr->length   = len;
    return 0;
}

/**
 * @brief  Reset SmartCard and get ATR
 */
uint8_t SC_ResetAndGetATR(SC_ATR_TypeDef *atr)
{
    uint8_t atr_buf[ATR_MAX_SIZE] = { 0 };
    uint16_t atr_len              = 0;

    /* Reset card */
    GPIO_ResetBits(SC_GPIO_PORT, SC_RST_PIN);
    Delay_Ms(1);
    GPIO_SetBits(SC_GPIO_PORT, SC_RST_PIN);

    /* Receive ATR */
    SC_ReceiveData(atr_buf, &atr_len);

    if (atr_len == 0) {
        SC_Status.card_present = 0;
        SC_Status.atr_valid    = 0;
        return 1; /* No ATR received */
    }

    /* Parse ATR */
    if (SC_ParseATR(atr, atr_buf, atr_len) != 0) {
        SC_Status.atr_valid = 0;
        return 1; /* ATR parse failed */
    }

    /* Try to switch to T=0 if not already */
    if (atr->protocol != 0) {
        uint8_t pps[] = { 0xff, 0x00, 0xff };
        uint8_t pps_resp[10];
        uint16_t pps_len = 0;

        SC_TransmitData(pps, sizeof(pps));
        SC_ReceiveData(pps_resp, &pps_len);

        if (pps_len != 3 || memcmp(pps, pps_resp, 3) != 0) {
            /* PPS failed, but continue */
        }
    }

    SC_Status.card_present = 1;
    SC_Status.card_active  = 1;
    SC_Status.atr_valid    = 1;
    SC_Status.protocol     = atr->protocol;
    memcpy(&SC_Status.atr, atr, sizeof(SC_ATR_TypeDef));

    return 0;
}

/**
 * @brief  Send APDU command to SmartCard
 */
uint8_t SC_SendAPDU(uint8_t *apdu, uint16_t apdu_len, uint8_t *response,
                    uint16_t *response_len)
{
    if (!SC_Status.card_active || !SC_Status.atr_valid) {
        *response_len = 0;
        return 1; /* Card not active */
    }

    if (apdu_len == 0 || apdu_len > SC_MAX_APDU_SIZE) {
        *response_len = 0;
        return 1; /* Invalid APDU length */
    }

    /* Transmit APDU */
    SC_TransmitData(apdu, apdu_len);

    /* Receive response */
    SC_ReceiveData(response, response_len);

    if (*response_len == 0) {
        return 1; /* No response */
    }

    return 0;
}

/**
 * @brief  Power on SmartCard
 */
void SC_PowerOn(void)
{
    GPIO_SetBits(SC_GPIO_PORT, SC_RST_PIN);
    SC_Status.card_present = 1;
}

/**
 * @brief  Power off SmartCard
 */
void SC_PowerOff(void)
{
    GPIO_ResetBits(SC_GPIO_PORT, SC_RST_PIN);
    SC_Status.card_present = 0;
    SC_Status.card_active  = 0;
    SC_Status.atr_valid    = 0;
}

/**
 * @brief  Get SmartCard status
 */
void SC_GetStatus(SC_Status_TypeDef *status)
{
    memcpy(status, &SC_Status, sizeof(SC_Status_TypeDef));
}
