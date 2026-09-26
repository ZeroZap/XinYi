#include "unity.h"
#include "xy_nrf24l01.h"

#include <string.h>

#define MAX_FRAMES 24U

typedef struct {
    uint8_t tx[2];
    uint8_t rx[2];
    xy_hal_error_t result;
} frame_t;

static frame_t frames[MAX_FRAMES];
static size_t frame_count;
static size_t frame_index;
static uint8_t csn_log[64];
static size_t csn_count;
static uint8_t ce_log[8];
static size_t ce_count;

static void queue_frame(uint8_t command, uint8_t value, uint8_t status,
                        uint8_t response, xy_hal_error_t result)
{
    frame_t *frame = &frames[frame_count++];
    frame->tx[0] = command;
    frame->tx[1] = value;
    frame->rx[0] = status;
    frame->rx[1] = response;
    frame->result = result;
}

static xy_hal_error_t transfer(void *spi, const uint8_t *tx, uint8_t *rx,
                               size_t length, uint32_t timeout)
{
    frame_t *frame;
    TEST_ASSERT_NOT_NULL(spi);
    TEST_ASSERT_EQUAL_UINT32(10U, timeout);
    TEST_ASSERT_EQUAL_UINT(2U, length);
    TEST_ASSERT_LESS_THAN(frame_count, frame_index);
    frame = &frames[frame_index++];
    TEST_ASSERT_EQUAL_UINT8_ARRAY(frame->tx, tx, 2U);
    if (frame->result == XY_HAL_OK) memcpy(rx, frame->rx, 2U);
    return frame->result;
}

static xy_hal_error_t set_csn(void *arg, uint8_t level)
{
    TEST_ASSERT_NOT_NULL(arg);
    csn_log[csn_count++] = level;
    return XY_HAL_OK;
}

static xy_hal_error_t set_ce(void *arg, uint8_t level)
{
    TEST_ASSERT_NOT_NULL(arg);
    ce_log[ce_count++] = level;
    return XY_HAL_OK;
}

static xy_nrf24l01_config_t config(void)
{
    static int spi;
    static int csn;
    static int ce;
    xy_nrf24l01_config_t result = {&spi, &csn, &ce, transfer, set_csn, set_ce, 10U};
    return result;
}

static void queue_success(void)
{
    queue_frame(0x05U, 0xFFU, 0x0EU, 40U, XY_HAL_OK);
    queue_frame(0x25U, 2U, 0x0EU, 0U, XY_HAL_OK);
    queue_frame(0x05U, 0xFFU, 0x0EU, 2U, XY_HAL_OK);
    queue_frame(0x25U, 40U, 0x0EU, 0U, XY_HAL_OK);
    queue_frame(0x05U, 0xFFU, 0x0EU, 40U, XY_HAL_OK);
    queue_frame(0x00U, 0xFFU, 0x0EU, 0x08U, XY_HAL_OK);
    queue_frame(0x01U, 0xFFU, 0x0EU, 0x3FU, XY_HAL_OK);
    queue_frame(0x03U, 0xFFU, 0x0EU, 0x03U, XY_HAL_OK);
    queue_frame(0x06U, 0xFFU, 0x0EU, 0x0EU, XY_HAL_OK);
    queue_frame(0x17U, 0xFFU, 0x0EU, 0x11U, XY_HAL_OK);
}

void setUp(void)
{
    memset(frames, 0, sizeof(frames));
    memset(csn_log, 0, sizeof(csn_log));
    memset(ce_log, 0, sizeof(ce_log));
    frame_count = frame_index = csn_count = ce_count = 0U;
}
void tearDown(void) {}

static void test_probe_round_trip_restores_and_publishes(void)
{
    xy_nrf24l01_t radio;
    xy_nrf24l01_config_t cfg = config();
    queue_success();
    memset(&radio, 0xA5, sizeof(radio));
    TEST_ASSERT_EQUAL_INT(XY_HAL_OK, xy_nrf24l01_probe(&radio, &cfg));
    TEST_ASSERT_TRUE(radio.initialized);
    TEST_ASSERT_EQUAL_HEX8(40U, radio.rf_ch);
    TEST_ASSERT_EQUAL_HEX8(0x03U, radio.setup_aw);
    TEST_ASSERT_EQUAL_UINT(frame_count, frame_index);
    TEST_ASSERT_EQUAL_UINT8(0U, ce_log[0]);
    TEST_ASSERT_EQUAL_UINT8(1U, csn_log[0]);
    for (size_t i = 1U; i < csn_count; ++i) TEST_ASSERT_EQUAL_UINT8(i & 1U ? 0U : 1U, csn_log[i]);
}

static void test_probe_mismatch_still_restores(void)
{
    xy_nrf24l01_t radio;
    xy_nrf24l01_t old;
    xy_nrf24l01_config_t cfg = config();
    queue_frame(0x05U, 0xFFU, 0x0EU, 40U, XY_HAL_OK);
    queue_frame(0x25U, 2U, 0x0EU, 0U, XY_HAL_OK);
    queue_frame(0x05U, 0xFFU, 0x0EU, 3U, XY_HAL_OK);
    queue_frame(0x25U, 40U, 0x0EU, 0U, XY_HAL_OK);
    queue_frame(0x05U, 0xFFU, 0x0EU, 40U, XY_HAL_OK);
    memset(&radio, 0xA5, sizeof(radio));
    old = radio;
    TEST_ASSERT_EQUAL_INT(XY_HAL_ERROR_NOT_FOUND, xy_nrf24l01_probe(&radio, &cfg));
    TEST_ASSERT_EQUAL_MEMORY(&old, &radio, sizeof(radio));
    TEST_ASSERT_EQUAL_UINT(5U, frame_index);
}

static void test_probe_read_failure_after_write_restores(void)
{
    xy_nrf24l01_t radio;
    xy_nrf24l01_config_t cfg = config();
    queue_frame(0x05U, 0xFFU, 0x0EU, 40U, XY_HAL_OK);
    queue_frame(0x25U, 2U, 0x0EU, 0U, XY_HAL_OK);
    queue_frame(0x05U, 0xFFU, 0U, 0U, XY_HAL_ERROR_TIMEOUT);
    queue_frame(0x25U, 40U, 0x0EU, 0U, XY_HAL_OK);
    queue_frame(0x05U, 0xFFU, 0x0EU, 40U, XY_HAL_OK);
    TEST_ASSERT_EQUAL_INT(XY_HAL_ERROR_TIMEOUT, xy_nrf24l01_probe(&radio, &cfg));
    TEST_ASSERT_EQUAL_UINT(5U, frame_index);
}

static void test_probe_rejects_floating_bus(void)
{
    xy_nrf24l01_t radio;
    xy_nrf24l01_config_t cfg = config();
    queue_frame(0x05U, 0xFFU, 0xFFU, 0xFFU, XY_HAL_OK);
    TEST_ASSERT_EQUAL_INT(XY_HAL_ERROR_NOT_FOUND, xy_nrf24l01_probe(&radio, &cfg));
    TEST_ASSERT_EQUAL_UINT(1U, frame_index);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_probe_round_trip_restores_and_publishes);
    RUN_TEST(test_probe_mismatch_still_restores);
    RUN_TEST(test_probe_read_failure_after_write_restores);
    RUN_TEST(test_probe_rejects_floating_bus);
    return UNITY_END();
}
