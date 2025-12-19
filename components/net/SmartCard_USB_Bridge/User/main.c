/********************************** (C) COPYRIGHT
 ******************************** File Name          : main.c Author : WCH
 * Version            : V1.0.0
 * Date               : 2025/10/30
 * Description        : SmartCard-USB Bridge Main Program
 *                      MCU communicates with SIM card via UART SmartCard mode
 *                      and forwards APDU commands from PC via USB-CDC using TLV
 *protocol
 *******************************************************************************/

#include "debug.h"
#include "ch32x035_conf.h"
#include "ch32x035_usbfs_device.h"
#include "UART.h"
#include "smartcard.h"
#include "tlv_protocol.h"

/******************************************************************************/
/* Global Variables */
static uint8_t TLV_RxBuffer[TLV_MAX_PAYLOAD_SIZE + TLV_HEADER_SIZE];
static uint16_t TLV_RxIndex     = 0;
static uint8_t ATR_Received     = 0;
static uint8_t Current_UI_State = UI_LED_OFF;

/******************************************************************************/
/* LED Control Definitions */
#define LED_GREEN_PIN  GPIO_Pin_0 /* PA0 - Green LED */
#define LED_RED_PIN    GPIO_Pin_1 /* PA1 - Red LED */
#define LED_YELLOW_PIN GPIO_Pin_6 /* PA6 - Yellow LED */
#define LED_BLUE_PIN   GPIO_Pin_7 /* PA7 - Blue LED */
#define LED_GPIO_PORT  GPIOA

/******************************************************************************/
/* Function Prototypes */
void ProcessTLVCommand(TLV_Packet *tlv_in);
void SendTLVResponse(TLV_Packet *tlv_out);
void LED_Init(void);
void LED_SetState(uint8_t ui_state);

/**
 * @brief  Process TLV command from PC
 */
void ProcessTLVCommand(TLV_Packet *tlv_in)
{
    TLV_Packet tlv_out;
    SC_ATR_TypeDef atr;
    SC_Status_TypeDef status;
    uint8_t apdu_response[SC_MAX_APDU_SIZE];
    uint16_t response_len = 0;
    uint8_t result;

    switch (tlv_in->header.tag) {
    case TLV_TAG_RESET_SIM:
        /* Reset SIM card and get ATR */
        result = SC_ResetAndGetATR(&atr);
        if (result == 0) {
            /* Send ATR data to PC */
            uint8_t atr_data[ATR_MAX_SIZE + 1];
            atr_data[0] = atr.protocol; /* Protocol type */

            /* Build raw ATR */
            uint16_t atr_len    = 0;
            atr_data[++atr_len] = atr.TS;
            atr_data[++atr_len] = atr.T0;

            /* Add interface bytes and historical bytes */
            /* Simplified - in production, rebuild full ATR from structure */

            TLV_Build(&tlv_out, TLV_TAG_ATR_DATA, atr_data, atr_len + 1);
            SendTLVResponse(&tlv_out);
            ATR_Received = 1;
        } else {
            TLV_BuildErrorResponse(&tlv_out, TLV_ERR_ATR_PARSE_FAILED);
            SendTLVResponse(&tlv_out);
        }
        break;

    case TLV_TAG_APDU_REQUEST:
        /* Forward APDU to SIM card */
        if (!ATR_Received) {
            TLV_BuildErrorResponse(&tlv_out, TLV_ERR_NO_CARD);
            SendTLVResponse(&tlv_out);
            break;
        }

        result = SC_SendAPDU(
            tlv_in->value, tlv_in->header.length, apdu_response, &response_len);

        if (result == 0 && response_len > 0) {
            /* Send APDU response to PC */
            TLV_Build(
                &tlv_out, TLV_TAG_APDU_RESPONSE, apdu_response, response_len);
            SendTLVResponse(&tlv_out);
        } else {
            TLV_BuildErrorResponse(&tlv_out, TLV_ERR_APDU_FAILED);
            SendTLVResponse(&tlv_out);
        }
        break;

    case TLV_TAG_POWER_ON:
        SC_PowerOn();
        TLV_BuildAck(&tlv_out);
        SendTLVResponse(&tlv_out);
        break;

    case TLV_TAG_POWER_OFF:
        SC_PowerOff();
        ATR_Received = 0;
        TLV_BuildAck(&tlv_out);
        SendTLVResponse(&tlv_out);
        break;

    case TLV_TAG_STATUS_QUERY:
        SC_GetStatus(&status);
        {
            uint8_t status_data[4];
            status_data[0] =
                status.card_present ? TLV_STATUS_CARD_PRESENT : TLV_STATUS_IDLE;
            status_data[1] =
                status.card_active ? TLV_STATUS_CARD_ACTIVE : TLV_STATUS_IDLE;
            status_data[2] = status.atr_valid ? 1 : 0;
            status_data[3] = status.protocol;

            TLV_Build(&tlv_out, TLV_TAG_STATUS_RESPONSE, status_data, 4);
            SendTLVResponse(&tlv_out);
        }
        break;

    case TLV_TAG_GET_INFO:
        /* Get card information */
        SC_GetStatus(&status);
        if (status.atr_valid) {
            /* Send ATR info as response */
            uint8_t info_data[64];
            uint16_t info_len = 0;

            info_data[info_len++] = status.protocol;
            info_data[info_len++] = status.atr.Tin; /* Historical bytes count */

            TLV_Build(&tlv_out, TLV_TAG_INFO_RESPONSE, info_data, info_len);
            SendTLVResponse(&tlv_out);
        } else {
            TLV_BuildErrorResponse(&tlv_out, TLV_ERR_NO_CARD);
            SendTLVResponse(&tlv_out);
        }
        break;

    case TLV_TAG_SET_UI_INFO:
        /* Set UI indicator (LED control) */
        if (tlv_in->header.length > 0) {
            uint8_t ui_value = tlv_in->value[0];
            LED_SetState(ui_value);
            Current_UI_State = ui_value;

            /* Send ACK with current UI state */
            TLV_Build(&tlv_out, TLV_TAG_ACK, &ui_value, 1);
            SendTLVResponse(&tlv_out);

            printf("UI LED set to: 0x%02X\r\n", ui_value);
        } else {
            TLV_BuildErrorResponse(&tlv_out, TLV_ERR_INVALID_LENGTH);
            SendTLVResponse(&tlv_out);
        }
        break;

    default:
        /* Unknown command */
        TLV_BuildErrorResponse(&tlv_out, TLV_ERR_INVALID_TAG);
        SendTLVResponse(&tlv_out);
        break;
    }
}

/**
 * @brief  Send TLV response to PC via USB-CDC
 */
void SendTLVResponse(TLV_Packet *tlv_out)
{
    uint8_t tx_buffer[TLV_MAX_PAYLOAD_SIZE + TLV_HEADER_SIZE];
    uint16_t tx_len;
    uint16_t sent = 0;

    /* Serialize TLV packet */
    tx_len = TLV_Serialize(tlv_out, tx_buffer, sizeof(tx_buffer));

    if (tx_len == 0) {
        return; /* Serialization failed */
    }

    /* Send via USB-CDC (through UART Tx buffer) */
    /* Split into packets if necessary */
    while (sent < tx_len) {
        uint16_t chunk_size = (tx_len - sent) > DEF_USB_FS_PACK_LEN
                                  ? DEF_USB_FS_PACK_LEN
                                  : (tx_len - sent);

        /* Wait for buffer available */
        while (Uart.Tx_RemainNum >= (DEF_UARTx_TX_BUF_NUM_MAX - 1)) {
            UART2_DataTx_Deal();
            Delay_Us(100);
        }

        /* Copy to transmit buffer */
        NVIC_DisableIRQ(USBFS_IRQn);
        memcpy(&UART2_Tx_Buf[Uart.Tx_LoadNum * DEF_USB_FS_PACK_LEN],
               &tx_buffer[sent], chunk_size);
        Uart.Tx_PackLen[Uart.Tx_LoadNum] = chunk_size;
        Uart.Tx_LoadNum++;
        if (Uart.Tx_LoadNum >= DEF_UARTx_TX_BUF_NUM_MAX) {
            Uart.Tx_LoadNum = 0;
        }
        Uart.Tx_RemainNum++;
        NVIC_EnableIRQ(USBFS_IRQn);

        sent += chunk_size;
    }
}

/**
 * @brief  Process USB-CDC received data (TLV commands from PC)
 */
void ProcessUSBData(void)
{
    TLV_Packet tlv_cmd;
    uint16_t parsed_len;

    /* Check if we have data in USB receive buffer */
    if (Uart.Rx_RemainLen > 0) {
        NVIC_DisableIRQ(USBFS_IRQn);

        /* Copy available data to TLV buffer */
        uint16_t copy_len = Uart.Rx_RemainLen;
        if (copy_len > (sizeof(TLV_RxBuffer) - TLV_RxIndex)) {
            copy_len = sizeof(TLV_RxBuffer) - TLV_RxIndex;
        }

        uint16_t rx_ptr = Uart.Rx_DealPtr;
        for (uint16_t i = 0; i < copy_len; i++) {
            TLV_RxBuffer[TLV_RxIndex++] = UART2_Rx_Buf[rx_ptr++];
            if (rx_ptr >= DEF_UARTx_RX_BUF_LEN) {
                rx_ptr = 0;
            }
        }

        Uart.Rx_DealPtr = rx_ptr;
        Uart.Rx_RemainLen -= copy_len;

        NVIC_EnableIRQ(USBFS_IRQn);

        /* Try to parse complete TLV packet */
        parsed_len = TLV_Parse(&tlv_cmd, TLV_RxBuffer, TLV_RxIndex);

        if (parsed_len > 0) {
            /* Complete TLV packet received */
            ProcessTLVCommand(&tlv_cmd);

            /* Remove processed data from buffer */
            if (parsed_len < TLV_RxIndex) {
                memmove(TLV_RxBuffer, TLV_RxBuffer + parsed_len,
                        TLV_RxIndex - parsed_len);
                TLV_RxIndex -= parsed_len;
            } else {
                TLV_RxIndex = 0;
            }
        } else if (TLV_RxIndex >= sizeof(TLV_RxBuffer)) {
            /* Buffer overflow - reset */
            TLV_RxIndex = 0;
        }
    }
}

/**
 * @brief  Initialize LED GPIO pins
 */
void LED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = { 0 };

    /* Enable GPIOA clock (already enabled for SmartCard, but ensure it's on) */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /* Configure LED pins as output push-pull */
    GPIO_InitStructure.GPIO_Pin =
        LED_GREEN_PIN | LED_RED_PIN | LED_YELLOW_PIN | LED_BLUE_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(LED_GPIO_PORT, &GPIO_InitStructure);

    /* Turn off all LEDs initially */
    GPIO_ResetBits(LED_GPIO_PORT,
                   LED_GREEN_PIN | LED_RED_PIN | LED_YELLOW_PIN | LED_BLUE_PIN);

    printf("LED GPIO initialized\r\n");
}

/**
 * @brief  Set LED state based on UI indicator value
 * @param  ui_state: UI indicator value (UI_LED_xxx)
 */
void LED_SetState(uint8_t ui_state)
{
    /* Turn off all LEDs first */
    GPIO_ResetBits(LED_GPIO_PORT,
                   LED_GREEN_PIN | LED_RED_PIN | LED_YELLOW_PIN | LED_BLUE_PIN);

    switch (ui_state) {
    case UI_LED_OFF:
        /* All LEDs off - already done */
        break;

    case UI_LED_GREEN:
        GPIO_SetBits(LED_GPIO_PORT, LED_GREEN_PIN);
        break;

    case UI_LED_RED:
        GPIO_SetBits(LED_GPIO_PORT, LED_RED_PIN);
        break;

    case UI_LED_YELLOW:
        GPIO_SetBits(LED_GPIO_PORT, LED_YELLOW_PIN);
        break;

    case UI_LED_BLUE:
        GPIO_SetBits(LED_GPIO_PORT, LED_BLUE_PIN);
        break;

    case UI_LED_BLINK_GREEN:
    case UI_LED_BLINK_RED:
    case UI_LED_BLINK_YELLOW:
        /* Blinking modes - for now just turn on solid
         * In production, use a timer to toggle the LED */
        if (ui_state == UI_LED_BLINK_GREEN) {
            GPIO_SetBits(LED_GPIO_PORT, LED_GREEN_PIN);
        } else if (ui_state == UI_LED_BLINK_RED) {
            GPIO_SetBits(LED_GPIO_PORT, LED_RED_PIN);
        } else if (ui_state == UI_LED_BLINK_YELLOW) {
            GPIO_SetBits(LED_GPIO_PORT, LED_YELLOW_PIN);
        }
        /* TODO: Implement timer-based blinking if needed */
        break;

    default:
        /* Unknown state - turn off all LEDs */
        break;
    }
}

/**
 * @brief  Main program
 */
int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init(115200);

    printf("SystemClk:%d\r\n", SystemCoreClock);
    printf("SmartCard-USB Bridge System\r\n");
    printf("ChipID:%08x\r\n", DBGMCU_GetCHIPID());

    /* Initialize RCC */
    RCC_Configuration();

    /* Initialize Timer */
    TIM3_Init();

    /* Initialize SmartCard interface */
    SC_Init();
    printf("SmartCard initialized\r\n");

    /* Initialize LED indicators */
    LED_Init();
    LED_SetState(UI_LED_BLUE); /* Blue LED - system starting */

    /* Initialize USB-CDC (using USART1 for debug, USART2 simulated as CDC) */
    UART2_Init(1, DEF_UARTx_BAUDRATE, DEF_UARTx_STOPBIT, DEF_UARTx_PARITY);
    printf("USB-CDC initialized\r\n");

    /* Initialize USB */
    USBFS_RCC_Init();
    USBFS_Device_Init(ENABLE, PWR_VDD_SupplyVoltage());
    printf("USB Device initialized\r\n");

    printf("System ready. Waiting for PC commands...\r\n");

    /* Set LED to green - ready state */
    LED_SetState(UI_LED_GREEN);

    while (1) {
        /* Process USB-CDC data transmission */
        UART2_DataTx_Deal();
        UART2_DataRx_Deal();

        /* Process received TLV commands */
        ProcessUSBData();
    }
}
