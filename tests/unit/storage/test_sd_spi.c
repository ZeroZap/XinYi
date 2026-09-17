#include "unity.h"
#include "xy_sd_spi.h"

#include <string.h>

static uint8_t g_cs;
static uint8_t g_queue[32];
static size_t g_queue_count;
static size_t g_queue_index;
static uint8_t g_last_command;
static uint32_t g_last_argument;
static uint32_t g_acmd41_count;
static uint32_t g_data_stage;
static uint8_t g_write_crc_count;
static xy_hal_error_t g_forced_error;
static uint8_t g_block[XY_SD_SPI_BLOCK_SIZE];

static void queue_bytes(const uint8_t *data, size_t length)
{
    memcpy(g_queue, data, length);
    g_queue_count = length;
    g_queue_index = 0U;
}

static xy_hal_error_t fake_cs(void *arg, uint8_t level)
{
    (void)arg;
    g_cs = level;
    return XY_HAL_OK;
}

static void fake_delay(uint32_t ms) { (void)ms; }

static xy_hal_error_t fake_transfer(void *spi, const uint8_t *tx, uint8_t *rx, size_t length,
                                    uint32_t timeout_ms)
{
    (void)spi;
    (void)timeout_ms;
    if (g_forced_error != XY_HAL_OK) {
        xy_hal_error_t result = g_forced_error;
        g_forced_error = XY_HAL_OK;
        return result;
    }
    memset(rx, 0xFF, length);
    if (length == 6U && (tx[0] & 0xC0U) == 0x40U) {
        uint8_t command = tx[0] & 0x3FU;
        uint8_t response[6] = {0};
        g_last_command = command;
        g_last_argument = ((uint32_t)tx[1] << 24) | ((uint32_t)tx[2] << 16) |
                          ((uint32_t)tx[3] << 8) | tx[4];
        if (command == 0U) {
            response[0] = 0x01U;
            queue_bytes(response, 1U);
        } else if (command == 8U) {
            const uint8_t r7[] = {0x01U, 0x00U, 0x00U, 0x01U, 0xAAU};
            queue_bytes(r7, sizeof(r7));
        } else if (command == 55U) {
            response[0] = g_acmd41_count < 2U ? 0x01U : 0x00U;
            queue_bytes(response, 1U);
        } else if (command == 41U) {
            response[0] = g_acmd41_count++ < 2U ? 0x01U : 0x00U;
            queue_bytes(response, 1U);
        } else if (command == 58U) {
            const uint8_t ocr[] = {0x00U, 0xC0U, 0xFFU, 0x80U, 0x00U};
            queue_bytes(ocr, sizeof(ocr));
        } else if (command == 9U) {
            const uint8_t csd_response[] = {0x00U, 0xFEU};
            queue_bytes(csd_response, sizeof(csd_response));
            g_data_stage = 1U;
        } else if (command == 17U) {
            const uint8_t read_response[] = {0x00U, 0xFEU};
            queue_bytes(read_response, sizeof(read_response));
            g_data_stage = 2U;
        } else if (command == 24U) {
            response[0] = 0x00U;
            queue_bytes(response, 1U);
            g_data_stage = 3U;
        }
        return XY_HAL_OK;
    }
    if (length == 1U && g_queue_index < g_queue_count) {
        rx[0] = g_queue[g_queue_index++];
        return XY_HAL_OK;
    }
    if (length == 16U && g_data_stage == 1U) {
        memset(rx, 0, length);
        rx[0] = 0x40U;
        rx[8] = 0xFFU;
        rx[9] = 0xFFU;
        g_data_stage = 0U;
        return XY_HAL_OK;
    }
    if (length == XY_SD_SPI_BLOCK_SIZE && g_data_stage == 2U) {
        memcpy(rx, g_block, length);
        g_data_stage = 0U;
        return XY_HAL_OK;
    }
    if (length == XY_SD_SPI_BLOCK_SIZE && g_data_stage == 3U) {
        TEST_ASSERT_EQUAL_MEMORY(g_block, tx, length);
        g_data_stage = 4U;
        return XY_HAL_OK;
    }
    if (length == 1U && g_data_stage == 4U && tx[0] == 0xFFU) {
        if (++g_write_crc_count == 2U) {
            g_data_stage = 5U;
            g_write_crc_count = 0U;
        }
        return XY_HAL_OK;
    }
    if (length == 1U && g_data_stage == 5U) {
        rx[0] = 0x05U;
        g_data_stage = 6U;
    } else if (length == 1U && g_data_stage == 6U) {
        rx[0] = 0xFFU;
        g_data_stage = 0U;
    }
    return XY_HAL_OK;
}

void setUp(void)
{
    g_cs = 1U;
    g_queue_count = 0U;
    g_queue_index = 0U;
    g_last_command = 0U;
    g_last_argument = 0U;
    g_acmd41_count = 0U;
    g_data_stage = 0U;
    g_write_crc_count = 0U;
    g_forced_error = XY_HAL_OK;
    for (size_t i = 0U; i < sizeof(g_block); ++i) g_block[i] = (uint8_t)i;
}
void tearDown(void) {}

static xy_sd_spi_t make_card(void)
{
    static int spi;
    xy_sd_spi_t card;
    const xy_sd_spi_config_t config = {
        .spi = &spi,
        .transfer = fake_transfer,
        .set_cs = fake_cs,
        .delay_ms = fake_delay,
        .timeout_ms = 100U,
    };
    TEST_ASSERT_EQUAL_INT(XY_HAL_OK, xy_sd_spi_init(&card, &config));
    return card;
}

static void test_init_identifies_32_gib_sdhc(void)
{
    xy_sd_spi_t card = make_card();
    TEST_ASSERT_EQUAL_INT(XY_SD_SPI_CARD_SDHC, card.type);
    TEST_ASSERT_EQUAL_UINT32(67108864U, card.block_count);
    TEST_ASSERT_EQUAL_UINT64(34359738368ULL, card.capacity_bytes);
    TEST_ASSERT_EQUAL_UINT8(1U, card.initialized);
    TEST_ASSERT_EQUAL_UINT8(1U, g_cs);
}

static void test_read_and_write_use_block_addressing(void)
{
    uint8_t data[XY_SD_SPI_BLOCK_SIZE];
    xy_sd_spi_t card = make_card();
    TEST_ASSERT_EQUAL_INT(XY_HAL_OK, xy_sd_spi_read_block(&card, 7U, data));
    TEST_ASSERT_EQUAL_UINT8(17U, g_last_command);
    TEST_ASSERT_EQUAL_UINT32(7U, g_last_argument);
    TEST_ASSERT_EQUAL_MEMORY(g_block, data, sizeof(data));
    TEST_ASSERT_EQUAL_INT(XY_HAL_OK, xy_sd_spi_write_block(&card, 9U, g_block));
    TEST_ASSERT_EQUAL_UINT8(24U, g_last_command);
    TEST_ASSERT_EQUAL_UINT32(9U, g_last_argument);
}

static void test_transport_error_is_preserved_and_state_is_not_committed(void)
{
    int spi;
    xy_sd_spi_t card;
    const xy_sd_spi_config_t config = {
        .spi = &spi, .transfer = fake_transfer, .set_cs = fake_cs,
        .delay_ms = fake_delay, .timeout_ms = 100U,
    };
    g_forced_error = XY_HAL_ERROR_TIMEOUT;
    TEST_ASSERT_EQUAL_INT(XY_HAL_ERROR_TIMEOUT, xy_sd_spi_init(&card, &config));
    TEST_ASSERT_EQUAL_UINT8(0U, card.initialized);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_identifies_32_gib_sdhc);
    RUN_TEST(test_read_and_write_use_block_addressing);
    RUN_TEST(test_transport_error_is_preserved_and_state_is_not_committed);
    return UNITY_END();
}
