#include "xy_sd_spi.h"

#include <string.h>

#define SD_CMD0 0U
#define SD_CMD8 8U
#define SD_CMD9 9U
#define SD_CMD17 17U
#define SD_CMD24 24U
#define SD_CMD55 55U
#define SD_CMD58 58U
#define SD_ACMD41 41U

#define SD_R1_IDLE 0x01U
#define SD_TOKEN_START_BLOCK 0xFEU

static xy_hal_error_t transfer(xy_sd_spi_t *card, const uint8_t *tx, uint8_t *rx, size_t length)
{
    return card->config.transfer(card->config.spi, tx, rx, length, card->config.timeout_ms);
}

static xy_hal_error_t exchange_byte(xy_sd_spi_t *card, uint8_t tx, uint8_t *rx)
{
    uint8_t value;
    xy_hal_error_t result = transfer(card, &tx, &value, 1U);
    if (result == XY_HAL_OK && rx != NULL) {
        *rx = value;
    }
    return result;
}

static xy_hal_error_t select_card(xy_sd_spi_t *card)
{
    xy_hal_error_t result = card->config.set_cs(card->config.cs_arg, 0U);
    uint8_t ignored;
    if (result != XY_HAL_OK) {
        return result;
    }
    result = exchange_byte(card, 0xFFU, &ignored);
    if (result != XY_HAL_OK) {
        (void)card->config.set_cs(card->config.cs_arg, 1U);
    }
    return result;
}

static xy_hal_error_t deselect_card(xy_sd_spi_t *card)
{
    uint8_t ignored;
    xy_hal_error_t result = card->config.set_cs(card->config.cs_arg, 1U);
    xy_hal_error_t clock_result = exchange_byte(card, 0xFFU, &ignored);
    return result != XY_HAL_OK ? result : clock_result;
}

static xy_hal_error_t wait_byte(xy_sd_spi_t *card, uint8_t expected, uint32_t attempts)
{
    uint8_t value;
    while (attempts-- != 0U) {
        xy_hal_error_t result = exchange_byte(card, 0xFFU, &value);
        if (result != XY_HAL_OK) {
            return result;
        }
        if (value == expected) {
            return XY_HAL_OK;
        }
    }
    return XY_HAL_ERROR_TIMEOUT;
}

static xy_hal_error_t command_selected(xy_sd_spi_t *card, uint8_t command, uint32_t argument,
                                       uint8_t crc, uint8_t *response)
{
    uint8_t packet[6] = {(uint8_t)(0x40U | command), (uint8_t)(argument >> 24),
                         (uint8_t)(argument >> 16), (uint8_t)(argument >> 8),
                         (uint8_t)argument, crc};
    uint8_t ignored[sizeof(packet)];
    uint8_t value = 0xFFU;
    xy_hal_error_t result = transfer(card, packet, ignored, sizeof(packet));
    if (result != XY_HAL_OK) {
        return result;
    }
    for (uint32_t attempt = 0U; attempt < 10U; ++attempt) {
        result = exchange_byte(card, 0xFFU, &value);
        if (result != XY_HAL_OK) {
            return result;
        }
        if ((value & 0x80U) == 0U) {
            *response = value;
            return XY_HAL_OK;
        }
    }
    return XY_HAL_ERROR_TIMEOUT;
}

static xy_hal_error_t run_command(xy_sd_spi_t *card, uint8_t command, uint32_t argument,
                                  uint8_t crc, uint8_t *response)
{
    xy_hal_error_t result = select_card(card);
    if (result == XY_HAL_OK) {
        result = command_selected(card, command, argument, crc, response);
    }
    xy_hal_error_t release_result = deselect_card(card);
    return result != XY_HAL_OK ? result : release_result;
}

static uint32_t csd_block_count(const uint8_t csd[16])
{
    uint8_t version = (uint8_t)((csd[0] >> 6) & 0x03U);
    if (version == 1U) {
        uint32_t c_size = ((uint32_t)(csd[7] & 0x3FU) << 16) |
                          ((uint32_t)csd[8] << 8) | csd[9];
        return (c_size + 1U) * 1024U;
    }
    if (version == 0U) {
        uint32_t read_bl_len = csd[5] & 0x0FU;
        uint32_t c_size = ((uint32_t)(csd[6] & 0x03U) << 10) |
                          ((uint32_t)csd[7] << 2) | ((csd[8] >> 6) & 0x03U);
        uint32_t c_size_mult = ((uint32_t)(csd[9] & 0x03U) << 1) |
                               ((csd[10] >> 7) & 0x01U);
        uint64_t bytes = (uint64_t)(c_size + 1U) << (c_size_mult + read_bl_len + 2U);
        return (uint32_t)(bytes / XY_SD_SPI_BLOCK_SIZE);
    }
    return 0U;
}

static xy_hal_error_t read_csd(xy_sd_spi_t *card, uint8_t csd[16])
{
    uint8_t response;
    uint8_t ignored[16];
    uint8_t crc[2];
    xy_hal_error_t result = select_card(card);
    if (result == XY_HAL_OK) {
        result = command_selected(card, SD_CMD9, 0U, 0x01U, &response);
    }
    if (result == XY_HAL_OK && response != 0U) {
        result = XY_HAL_ERROR_IO;
    }
    if (result == XY_HAL_OK) {
        result = wait_byte(card, SD_TOKEN_START_BLOCK, 10000U);
    }
    if (result == XY_HAL_OK) {
        memset(ignored, 0xFF, sizeof(ignored));
        result = transfer(card, ignored, csd, 16U);
    }
    if (result == XY_HAL_OK) {
        result = exchange_byte(card, 0xFFU, &crc[0]);
    }
    if (result == XY_HAL_OK) {
        result = exchange_byte(card, 0xFFU, &crc[1]);
    }
    xy_hal_error_t release_result = deselect_card(card);
    return result != XY_HAL_OK ? result : release_result;
}

xy_hal_error_t xy_sd_spi_init(xy_sd_spi_t *card, const xy_sd_spi_config_t *config)
{
    uint8_t response;
    uint8_t r7[4];
    uint8_t ocr[4];
    uint8_t csd[16];
    uint8_t clocks_tx[10];
    uint8_t clocks_rx[10];
    xy_hal_error_t result;

    if (card == NULL || config == NULL || config->spi == NULL || config->transfer == NULL ||
        config->set_cs == NULL || config->timeout_ms == 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    memset(card, 0, sizeof(*card));
    card->config = *config;
    memset(clocks_tx, 0xFF, sizeof(clocks_tx));
    result = config->set_cs(config->cs_arg, 1U);
    if (result == XY_HAL_OK) {
        result = config->transfer(config->spi, clocks_tx, clocks_rx, sizeof(clocks_tx),
                                  config->timeout_ms);
    }
    if (result != XY_HAL_OK) {
        return result;
    }
    result = run_command(card, SD_CMD0, 0U, 0x95U, &response);
    if (result != XY_HAL_OK || response != SD_R1_IDLE) {
        return result != XY_HAL_OK ? result : XY_HAL_ERROR_NOT_FOUND;
    }

    result = select_card(card);
    if (result == XY_HAL_OK) {
        result = command_selected(card, SD_CMD8, 0x1AAU, 0x87U, &response);
    }
    for (size_t index = 0U; result == XY_HAL_OK && index < sizeof(r7); ++index) {
        result = exchange_byte(card, 0xFFU, &r7[index]);
    }
    xy_hal_error_t release_result = deselect_card(card);
    if (result == XY_HAL_OK) {
        result = release_result;
    }
    if (result != XY_HAL_OK || response != SD_R1_IDLE || r7[2] != 0x01U || r7[3] != 0xAAU) {
        return result != XY_HAL_OK ? result : XY_HAL_ERROR_NOT_SUPPORTED;
    }

    for (uint32_t attempt = 0U; attempt < 1000U; ++attempt) {
        result = run_command(card, SD_CMD55, 0U, 0x01U, &response);
        if (result != XY_HAL_OK) {
            return result;
        }
        result = run_command(card, SD_ACMD41, 0x40000000U, 0x01U, &response);
        if (result != XY_HAL_OK) {
            return result;
        }
        if (response == 0U) {
            break;
        }
        if (attempt == 999U) {
            return XY_HAL_ERROR_TIMEOUT;
        }
        if (config->delay_ms != NULL) {
            config->delay_ms(1U);
        }
    }

    result = select_card(card);
    if (result == XY_HAL_OK) {
        result = command_selected(card, SD_CMD58, 0U, 0x01U, &response);
    }
    for (size_t index = 0U; result == XY_HAL_OK && index < sizeof(ocr); ++index) {
        result = exchange_byte(card, 0xFFU, &ocr[index]);
    }
    release_result = deselect_card(card);
    if (result == XY_HAL_OK) {
        result = release_result;
    }
    if (result != XY_HAL_OK || response != 0U) {
        return result != XY_HAL_OK ? result : XY_HAL_ERROR_IO;
    }
    card->type = (ocr[0] & 0x40U) != 0U ? XY_SD_SPI_CARD_SDHC : XY_SD_SPI_CARD_SDSC;

    result = read_csd(card, csd);
    if (result != XY_HAL_OK) {
        return result;
    }
    card->block_count = csd_block_count(csd);
    if (card->block_count == 0U) {
        return XY_HAL_ERROR_IO;
    }
    card->capacity_bytes = (uint64_t)card->block_count * XY_SD_SPI_BLOCK_SIZE;
    card->initialized = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_sd_spi_read_block(xy_sd_spi_t *card, uint32_t block, uint8_t *data)
{
    uint8_t response;
    uint8_t tx[XY_SD_SPI_BLOCK_SIZE];
    uint8_t crc;
    xy_hal_error_t result;
    if (card == NULL || data == NULL || card->initialized == 0U || block >= card->block_count) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    result = select_card(card);
    if (result == XY_HAL_OK) {
        uint32_t address = card->type == XY_SD_SPI_CARD_SDHC ? block : block * XY_SD_SPI_BLOCK_SIZE;
        result = command_selected(card, SD_CMD17, address, 0x01U, &response);
    }
    if (result == XY_HAL_OK && response != 0U) {
        result = XY_HAL_ERROR_IO;
    }
    if (result == XY_HAL_OK) {
        result = wait_byte(card, SD_TOKEN_START_BLOCK, 10000U);
    }
    if (result == XY_HAL_OK) {
        memset(tx, 0xFF, sizeof(tx));
        result = transfer(card, tx, data, sizeof(tx));
    }
    if (result == XY_HAL_OK) result = exchange_byte(card, 0xFFU, &crc);
    if (result == XY_HAL_OK) result = exchange_byte(card, 0xFFU, &crc);
    xy_hal_error_t release_result = deselect_card(card);
    return result != XY_HAL_OK ? result : release_result;
}

xy_hal_error_t xy_sd_spi_write_block(xy_sd_spi_t *card, uint32_t block, const uint8_t *data)
{
    uint8_t response;
    uint8_t ignored[XY_SD_SPI_BLOCK_SIZE];
    uint8_t token = SD_TOKEN_START_BLOCK;
    uint8_t crc[2] = {0xFFU, 0xFFU};
    uint8_t data_response;
    xy_hal_error_t result;
    if (card == NULL || data == NULL || card->initialized == 0U || block >= card->block_count) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    result = select_card(card);
    if (result == XY_HAL_OK) {
        uint32_t address = card->type == XY_SD_SPI_CARD_SDHC ? block : block * XY_SD_SPI_BLOCK_SIZE;
        result = command_selected(card, SD_CMD24, address, 0x01U, &response);
    }
    if (result == XY_HAL_OK && response != 0U) {
        result = XY_HAL_ERROR_IO;
    }
    if (result == XY_HAL_OK) {
        result = exchange_byte(card, token, NULL);
    }
    if (result == XY_HAL_OK) {
        result = transfer(card, data, ignored, XY_SD_SPI_BLOCK_SIZE);
    }
    if (result == XY_HAL_OK) {
        result = exchange_byte(card, crc[0], NULL);
    }
    if (result == XY_HAL_OK) {
        result = exchange_byte(card, crc[1], NULL);
    }
    if (result == XY_HAL_OK) {
        result = exchange_byte(card, 0xFFU, &data_response);
    }
    if (result == XY_HAL_OK && (data_response & 0x1FU) != 0x05U) {
        result = XY_HAL_ERROR_IO;
    }
    if (result == XY_HAL_OK) {
        result = wait_byte(card, 0xFFU, 100000U);
    }
    xy_hal_error_t release_result = deselect_card(card);
    return result != XY_HAL_OK ? result : release_result;
}
