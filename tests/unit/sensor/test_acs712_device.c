#include "unity.h"
#include "xy_acs712.h"

#include <string.h>

static int32_t g_raw;
static unsigned g_reads;
static uint8_t g_last_channel;

int32_t xy_adc_dev_read(void *dev, uint8_t channel, uint32_t timeout)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_EQUAL_UINT32(XY_ACS712_DEFAULT_TIMEOUT_MS, timeout);
    g_last_channel = channel;
    g_reads++;
    return g_raw;
}

uint32_t xy_hal_sys_get_tick_count(void)
{
    return 424242U;
}

void setUp(void)
{
    g_raw = 2048; /* quiescent: Vcc/2 */
    g_reads = 0U;
    g_last_channel = 0U;
}

void tearDown(void) {}

static void test_init_deinit_and_null_guards(void)
{
    xy_acs712_t dev;
    int adc;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_acs712_init(NULL, &adc, 3U));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_acs712_init(&dev, NULL, 3U));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_acs712_init(&dev, &adc, 3U));
    TEST_ASSERT_TRUE(dev.initialized);
    TEST_ASSERT_EQUAL_PTR(&adc, dev.adc);
    TEST_ASSERT_EQUAL_UINT8(3U, dev.channel);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_acs712_deinit(&dev));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_NULL(dev.adc);

    /* deinit of an already-deinitialised device fails closed */
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_acs712_deinit(&dev));
}

static void test_read_converts_quiescent_raw_to_zero_current(void)
{
    xy_acs712_t dev;
    xy_acs712_sample_t sample;
    int adc;

    memset(&sample, 0xA5, sizeof(sample));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_acs712_init(&dev, &adc, 5U));
    g_raw = 2048; /* exactly Vcc/2 */

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_acs712_read(&dev, &sample));
    TEST_ASSERT_EQUAL_INT32(0, sample.current_ma);
    TEST_ASSERT_EQUAL_UINT32(424242U, sample.timestamp);
    TEST_ASSERT_EQUAL_UINT(1U, g_reads);
    TEST_ASSERT_EQUAL_UINT8(5U, g_last_channel);
}

static void test_read_converts_raw_to_positive_current(void)
{
    xy_acs712_t dev;
    xy_acs712_sample_t sample;
    int adc;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_acs712_init(&dev, &adc, 1U));
    /* raw 3072 = 3/4 scale -> (2.475V - 1.65V) / 66mV/A = 12.5 A */
    g_raw = 3072;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_acs712_read(&dev, &sample));
    TEST_ASSERT_EQUAL_INT32(12500, sample.current_ma);
}

static void test_read_converts_raw_to_negative_current(void)
{
    xy_acs712_t dev;
    xy_acs712_sample_t sample;
    int adc;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_acs712_init(&dev, &adc, 2U));
    /* raw 1024 = 1/4 scale -> (0.825V - 1.65V) / 66mV/A = -12.5 A */
    g_raw = 1024;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_acs712_read(&dev, &sample));
    TEST_ASSERT_EQUAL_INT32(-12500, sample.current_ma);
}

static void test_read_failure_is_atomic_and_preserves_output(void)
{
    xy_acs712_t dev;
    xy_acs712_sample_t sample;
    xy_acs712_sample_t snapshot;
    int adc;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_acs712_init(&dev, &adc, 4U));
    memset(&sample, 0x5A, sizeof(sample));
    snapshot = sample;
    g_raw = -1; /* ADC reports an error */

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_acs712_read(&dev, &sample));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &sample, sizeof(sample));
}

static void test_read_missing_handle_fails_closed_without_adc_access(void)
{
    xy_acs712_t dev;
    xy_acs712_sample_t sample;
    unsigned before;
    int adc;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_acs712_init(&dev, &adc, 6U));
    dev.adc = NULL;
    before = g_reads;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_acs712_read(&dev, &sample));
    TEST_ASSERT_EQUAL_UINT(before, g_reads);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_init_deinit_and_null_guards);
    RUN_TEST(test_read_converts_quiescent_raw_to_zero_current);
    RUN_TEST(test_read_converts_raw_to_positive_current);
    RUN_TEST(test_read_converts_raw_to_negative_current);
    RUN_TEST(test_read_failure_is_atomic_and_preserves_output);
    RUN_TEST(test_read_missing_handle_fails_closed_without_adc_access);

    return UNITY_END();
}
