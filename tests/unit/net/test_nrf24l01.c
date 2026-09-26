#include "unity.h"
#include "xy_nrf24l01.h"

#include <string.h>

#define MAX_FRAMES 24U

typedef struct {
    uint8_t tx[33];
    uint8_t rx[33];
    size_t length;
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
    frame->length = 2U;
    frame->result = result;
}

static xy_hal_error_t transfer(void *spi, const uint8_t *tx, uint8_t *rx,
                               size_t length, uint32_t timeout)
{
    frame_t *frame;
    TEST_ASSERT_NOT_NULL(spi);
    TEST_ASSERT_EQUAL_UINT32(10U, timeout);
    TEST_ASSERT_LESS_THAN(frame_count, frame_index);
    frame = &frames[frame_index++];
    TEST_ASSERT_EQUAL_UINT(frame->length, length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(frame->tx, tx, length);
    if (frame->result == XY_HAL_OK) memcpy(rx, frame->rx, length);
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

static uint32_t delay_total;

static void delay_us(uint32_t us) { delay_total += us; }

static void queue_buffer(const uint8_t *tx, size_t length, uint8_t status,
                         xy_hal_error_t result)
{
    frame_t *frame = &frames[frame_count++];
    memcpy(frame->tx, tx, length);
    frame->rx[0] = status;
    frame->length = length;
    frame->result = result;
}

static xy_nrf24l01_config_t config(void)
{
    static int spi;
    static int csn;
    static int ce;
    xy_nrf24l01_config_t result = {&spi, &csn, &ce, transfer, set_csn, set_ce, delay_us, 10U};
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
    delay_total = 0U;
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

static void test_send_reports_ack_and_retry_count(void)
{
    xy_nrf24l01_t radio;
    uint8_t retries = 0xA5U;
    static const uint8_t payload[3] = {'O', 'K', '\n'};
    const uint8_t payload_frame[4] = {0xA0U, 'O', 'K', '\n'};
    xy_nrf24l01_config_t cfg = config();

    memset(&radio, 0, sizeof(radio));
    radio.config = cfg;
    radio.initialized = 1U;
    queue_frame(0xE1U, 0xFFU, 0x0EU, 0U, XY_HAL_OK);
    queue_buffer(payload_frame, sizeof(payload_frame), 0x0EU, XY_HAL_OK);
    queue_frame(0xFFU, 0xFFU, 0x2EU, 0U, XY_HAL_OK);
    queue_frame(0x08U, 0xFFU, 0x2EU, 0x02U, XY_HAL_OK);
    queue_frame(0x27U, 0x20U, 0x2EU, 0U, XY_HAL_OK);

    TEST_ASSERT_EQUAL_INT(XY_HAL_OK,
                          xy_nrf24l01_send(&radio, payload, sizeof(payload), &retries));
    TEST_ASSERT_EQUAL_UINT8(2U, retries);
    TEST_ASSERT_EQUAL_UINT8(1U, ce_log[0]);
    TEST_ASSERT_EQUAL_UINT8(0U, ce_log[1]);
    TEST_ASSERT_EQUAL_UINT32(20U, delay_total);
    TEST_ASSERT_EQUAL_UINT(frame_count, frame_index);
}

static void test_send_max_retry_flushes_and_reports_failure(void)
{
    xy_nrf24l01_t radio;
    uint8_t retries = 0U;
    const uint8_t payload = 0x55U;
    const uint8_t payload_frame[2] = {0xA0U, 0x55U};
    xy_nrf24l01_config_t cfg = config();

    memset(&radio, 0, sizeof(radio));
    radio.config = cfg;
    radio.initialized = 1U;
    queue_frame(0xE1U, 0xFFU, 0x0EU, 0U, XY_HAL_OK);
    queue_buffer(payload_frame, sizeof(payload_frame), 0x0EU, XY_HAL_OK);
    queue_frame(0xFFU, 0xFFU, 0x1EU, 0U, XY_HAL_OK);
    queue_frame(0x08U, 0xFFU, 0x1EU, 0x0FU, XY_HAL_OK);
    queue_frame(0x27U, 0x10U, 0x1EU, 0U, XY_HAL_OK);
    queue_frame(0xE1U, 0xFFU, 0x0EU, 0U, XY_HAL_OK);

    TEST_ASSERT_EQUAL_INT(XY_HAL_ERROR_NOT_FOUND,
                          xy_nrf24l01_send(&radio, &payload, 1U, &retries));
    TEST_ASSERT_EQUAL_UINT8(15U, retries);
    TEST_ASSERT_EQUAL_UINT(frame_count, frame_index);
}

static void test_send_propagates_max_retry_flush_failure(void)
{
    xy_nrf24l01_t radio;
    uint8_t retries = 0U;
    const uint8_t payload = 0x55U;
    const uint8_t payload_frame[2] = {0xA0U, 0x55U};
    xy_nrf24l01_config_t cfg = config();

    memset(&radio, 0, sizeof(radio));
    radio.config = cfg;
    radio.initialized = 1U;
    queue_frame(0xE1U, 0xFFU, 0x0EU, 0U, XY_HAL_OK);
    queue_buffer(payload_frame, sizeof(payload_frame), 0x0EU, XY_HAL_OK);
    queue_frame(0xFFU, 0xFFU, 0x1EU, 0U, XY_HAL_OK);
    queue_frame(0x08U, 0xFFU, 0x1EU, 0x0FU, XY_HAL_OK);
    queue_frame(0x27U, 0x10U, 0x1EU, 0U, XY_HAL_OK);
    queue_frame(0xE1U, 0xFFU, 0x0EU, 0U, XY_HAL_ERROR_IO);

    TEST_ASSERT_EQUAL_INT(XY_HAL_ERROR_IO,
                          xy_nrf24l01_send(&radio, &payload, 1U, &retries));
    TEST_ASSERT_EQUAL_UINT8(15U, retries);
    TEST_ASSERT_EQUAL_UINT(frame_count, frame_index);
}

static void test_receive_fixed_payload_and_clears_irq(void)
{
    xy_nrf24l01_t radio;
    uint8_t payload[4] = {0U};
    size_t received = 99U;
    const uint8_t read_tx[5] = {0x61U, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
    xy_nrf24l01_config_t cfg = config();

    memset(&radio, 0, sizeof(radio));
    radio.config = cfg;
    radio.initialized = 1U;
    radio.rx_payload_width = 4U;
    queue_frame(0x17U, 0xFFU, 0x4EU, 0x00U, XY_HAL_OK);
    queue_buffer(read_tx, sizeof(read_tx), 0x4EU, XY_HAL_OK);
    frames[1].rx[1] = 'P'; frames[1].rx[2] = 'I';
    frames[1].rx[3] = 'N'; frames[1].rx[4] = 'G';
    queue_frame(0x27U, 0x40U, 0x0EU, 0U, XY_HAL_OK);

    TEST_ASSERT_EQUAL_INT(XY_HAL_OK,
                          xy_nrf24l01_receive(&radio, payload, sizeof(payload), &received));
    TEST_ASSERT_EQUAL_UINT(4U, received);
    TEST_ASSERT_EQUAL_UINT8_ARRAY("PING", payload, 4U);
    TEST_ASSERT_EQUAL_UINT(frame_count, frame_index);
}

static void test_receive_without_irq_preserves_payload(void)
{
    xy_nrf24l01_t radio;
    uint8_t payload[4] = {1U, 2U, 3U, 4U};
    const uint8_t old[4] = {1U, 2U, 3U, 4U};
    size_t received = 99U;
    xy_nrf24l01_config_t cfg = config();

    memset(&radio, 0, sizeof(radio));
    radio.config = cfg;
    radio.initialized = 1U;
    radio.rx_payload_width = 4U;
    queue_frame(0x17U, 0xFFU, 0x0EU, 0x01U, XY_HAL_OK);
    TEST_ASSERT_EQUAL_INT(XY_HAL_ERROR_NOT_FOUND,
                          xy_nrf24l01_receive(&radio, payload, sizeof(payload), &received));
    TEST_ASSERT_EQUAL_UINT(0U, received);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(old, payload, 4U);
    TEST_ASSERT_EQUAL_UINT(1U, frame_index);
}

static void test_receive_reads_fifo_even_when_rx_irq_is_clear(void)
{
    xy_nrf24l01_t radio;
    uint8_t payload[2] = {0U};
    size_t received = 0U;
    const uint8_t read_tx[3] = {0x61U, 0xFFU, 0xFFU};
    xy_nrf24l01_config_t cfg = config();

    memset(&radio, 0, sizeof(radio));
    radio.config = cfg;
    radio.initialized = 1U;
    radio.rx_payload_width = 2U;
    queue_frame(0x17U, 0xFFU, 0x0EU, 0x00U, XY_HAL_OK);
    queue_buffer(read_tx, sizeof(read_tx), 0x0EU, XY_HAL_OK);
    frames[1].rx[1] = 0x12U; frames[1].rx[2] = 0x34U;
    queue_frame(0x27U, 0x40U, 0x0EU, 0U, XY_HAL_OK);

    TEST_ASSERT_EQUAL_INT(XY_HAL_OK,
                          xy_nrf24l01_receive(&radio, payload, sizeof(payload), &received));
    TEST_ASSERT_EQUAL_UINT(2U, received);
    TEST_ASSERT_EQUAL_HEX8(0x12U, payload[0]);
    TEST_ASSERT_EQUAL_HEX8(0x34U, payload[1]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_probe_round_trip_restores_and_publishes);
    RUN_TEST(test_probe_mismatch_still_restores);
    RUN_TEST(test_probe_read_failure_after_write_restores);
    RUN_TEST(test_probe_rejects_floating_bus);
    RUN_TEST(test_send_reports_ack_and_retry_count);
    RUN_TEST(test_send_max_retry_flushes_and_reports_failure);
    RUN_TEST(test_send_propagates_max_retry_flush_failure);
    RUN_TEST(test_receive_fixed_payload_and_clears_irq);
    RUN_TEST(test_receive_without_irq_preserves_payload);
    RUN_TEST(test_receive_reads_fifo_even_when_rx_irq_is_clear);
    return UNITY_END();
}
