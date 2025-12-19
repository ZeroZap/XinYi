#include "sd.h"
#include "sd_spi.h"


int32_t sd_init(void)
{

    uint8_t dummy = 0xff;

    // Send 80 clock cycles to initialize the card and cs stay high
    for (int i = 0; i < 10; i++) {
        sd_spi_write_bytes(&dummy, 1);
    }

    // Send CMD0
    sd_send_cmd(SD_CMD0_GO_IDLE_STATE, 0);

    // Wait for response
    sd_match_response(SD_RESP_IDLE, 10);

    // Send CMD8 (After SDv2) for checking voltage range
    sd_send_cmd(SD_CMD8_SEND_IF_COND, 0x1aa);

    // Wait for response
    sd_match_response(SD_RESP_IDLE, 10);

    // need send cmd55 before send acmd41
    sd_send_cmd(SD_CMD55_APP_CMD, 0);
    // Send ACMD41  to initialize the card
    sd_send_cmd(SD_ACMD41_SD_SEND_OP_COND, 0);

    // Wait for response , if response is , then the card is initialized
    sd_match_response(SD_RESP_IDLE, 10);

    // Send CMD58 Read OCR, 32 Bits
    sd_send_cmd(SD_CMD58_READ_OCR, 0);

    // Wait for response
    sd_match_response(SD_RESP_IDLE, 10);

    // Send CMD16
    sd_send_cmd(SD_CMD16_SET_BLOCKLEN, 512);

    // Wait for response
    sd_match_response(SD_RESP_IDLE, 10);

    return 0;
}


int32_t sd_read_block(uint32_t block_addr, uint8_t *data)
{
}


int32_t sd_write_block(uint32_t block_addr, uint8_t *data)
{
}

int32_t sd_earase_block(uint32_t start_block, uint32_t end_block)
{
}


int32_t sd_wait_ready(uint32_t timeout)
{
    while (timeout--) {
        if (sd_spi_rw_byte(0xFF) > 0) {
            if (resp == 0xff) {
                break;
            }
        }
    }
    return 0;
}

int32_t sd_go_idle(void)
{
    // Send CMD0
    sd_send_cmd(SD_CMD0_GO_IDLE_STATE, 0);

    // Wait for response
    sd_match_response(SD_RESP_IDLE, 10);

    return 0;
}

int32_t sd_send_cmd(uint8_t cmd, uint32_t arg)
{
    // Send command
    uint8_t crc = 0xff;

    uint8_t cmd_pack[6];

    cmd_pack[0] = cmd;

    cmd_pack[1] = (uint8_t)(arg >> 24);

    cmd_pack[2] = (uint8_t)(arg >> 16);

    cmd_pack[3] = (uint8_t)(arg >> 8);

    cmd_pack[4] = (uint8_t)(arg);

    // only cmd0, cmd8 need crc for spi sd.
    switch (cmd) {
    case SD_CMD0_GO_IDLE_STATE:
        cmd_pack[5] = SD_CRC_CMD0_GO_IDLE_STATE;
        break;
    case SD_CMD8_SEND_IF_COND:
        cmd_pack[5] = SD_CRC_CMD8_SEND_IF_COND;
        break;
    default:
        cmd_pack[5] = 0xff;
        break;
    }

    sd_spi_write_bytes(cmd_pack, 6);

    return 0;
}

int32_t sd_match_response(uint8_t desired_resp, uint8_t retry)
{
    uint8_t resp;
    while (retry--) {
        if (sd_spi_read_byte(&resp, 1) > 0) {
            if (resp == desired_resp) {
                break;
            }
        }
    }
}

int32_t sd_get_response(uint8_t *resp, uint8_t len)
{
}