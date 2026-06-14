/**
 * @file test_addc.c
 * @brief ADC/DAC Helper Library Unit Tests using Unity Framework
 * @version 1.0.0
 * @date 2026-02-28
 */

#include <stdint.h>
#include <string.h>
#include "unity.h"

/* ADC header */
#include "xy_adc.h"

/* ==================== Test Fixtures ==================== */

static xy_adc_t test_adc;
static xy_dac_t test_dac;
static xy_adc_sample_t test_sample;

void setUp(void)
{
    memset(&test_adc, 0, sizeof(test_adc));
    memset(&test_dac, 0, sizeof(test_dac));
    memset(&test_sample, 0, sizeof(test_sample));
}

void tearDown(void)
{
    xy_adc_deinit(&test_adc);
    xy_dac_deinit(&test_dac);
}

/* ==================== Helper Functions ==================== */

static void adc_assert_ok(int result)
{
    TEST_ASSERT_EQUAL(XY_ADC_OK, result);
}

/* ==================== ADC Init Tests ==================== */

void test_adc_init(void)
{
    int result;

    /* Test valid initialization */
    result = xy_adc_init(&test_adc, 3300, ADC_RESOLUTION_12BIT);
    adc_assert_ok(result);
    TEST_ASSERT_TRUE(test_adc.initialized);
    TEST_ASSERT_EQUAL(3300, test_adc.ref_voltage_mv);
    TEST_ASSERT_EQUAL(ADC_RESOLUTION_12BIT, test_adc.res);
}

void test_adc_init_invalid_params(void)
{
    int result = xy_adc_init(NULL, 3300, ADC_RESOLUTION_12BIT);
    TEST_ASSERT_EQUAL(XY_ADC_INVALID_PARAM, result);
}

void test_adc_deinit(void)
{
    xy_adc_init(&test_adc, 3300, ADC_RESOLUTION_12BIT);

    int result = xy_adc_deinit(&test_adc);
    adc_assert_ok(result);
    TEST_ASSERT_FALSE(test_adc.initialized);
}

void test_adc_deinit_invalid_params(void)
{
    int result = xy_adc_deinit(NULL);
    TEST_ASSERT_EQUAL(XY_ADC_INVALID_PARAM, result);
}

/* ==================== ADC Channel Tests ==================== */

void test_adc_config_channel(void)
{
    xy_adc_init(&test_adc, 3300, ADC_RESOLUTION_12BIT);

    /* Enable channel */
    int result = xy_adc_config_channel(&test_adc, 0, true);
    adc_assert_ok(result);
    TEST_ASSERT_EQUAL(1, test_adc.channel_count);
    TEST_ASSERT_EQUAL(0, test_adc.channels[0].channel);
    TEST_ASSERT_TRUE(test_adc.channels[0].enabled);
}

void test_adc_config_channel_invalid(void)
{
    xy_adc_init(&test_adc, 3300, ADC_RESOLUTION_12BIT);

    /* Invalid channel */
    int result = xy_adc_config_channel(&test_adc, ADC_MAX_CHANNELS, true);
    TEST_ASSERT_EQUAL(XY_ADC_INVALID_PARAM, result);
}

void test_adc_config_channel_disable(void)
{
    xy_adc_init(&test_adc, 3300, ADC_RESOLUTION_12BIT);

    /* Enable then disable */
    xy_adc_config_channel(&test_adc, 0, true);
    int result = xy_adc_config_channel(&test_adc, 0, false);
    adc_assert_ok(result);
    TEST_ASSERT_FALSE(test_adc.channels[0].enabled);
}

/* ==================== ADC Sample Tests ==================== */

void test_adc_sample(void)
{
    xy_adc_init(&test_adc, 3300, ADC_RESOLUTION_12BIT);
    xy_adc_config_channel(&test_adc, 0, true);

    int result = xy_adc_sample(&test_adc, 0, &test_sample);
    adc_assert_ok(result);

    TEST_ASSERT_EQUAL(0, test_sample.channel);
    TEST_ASSERT_TRUE(test_sample.valid);
    /* Voltage should be around 50% of 3300mV = 1650mV */
    TEST_ASSERT_TRUE(test_sample.voltage_mv > 1000);
    TEST_ASSERT_TRUE(test_sample.voltage_mv < 2500);
}

void test_adc_sample_invalid_params(void)
{
    int result;

    /* NULL adc */
    result = xy_adc_sample(NULL, 0, &test_sample);
    TEST_ASSERT_EQUAL(XY_ADC_INVALID_PARAM, result);

    /* NULL sample */
    xy_adc_init(&test_adc, 3300, ADC_RESOLUTION_12BIT);
    result = xy_adc_sample(&test_adc, 0, NULL);
    TEST_ASSERT_EQUAL(XY_ADC_INVALID_PARAM, result);
}

void test_adc_sample_not_ready(void)
{
    /* Don't initialize */
    int result = xy_adc_sample(&test_adc, 0, &test_sample);
    TEST_ASSERT_EQUAL(XY_ADC_NOT_READY, result);
}

void test_adc_sample_multi(void)
{
    xy_adc_init(&test_adc, 3300, ADC_RESOLUTION_12BIT);
    xy_adc_config_channel(&test_adc, 0, true);
    xy_adc_config_channel(&test_adc, 1, true);

    xy_adc_sample_t samples[2];
    int result = xy_adc_sample_multi(&test_adc, samples, 2);

    TEST_ASSERT_TRUE(result >= 0);
    TEST_ASSERT_TRUE(samples[0].valid);
    TEST_ASSERT_TRUE(samples[1].valid);
}

/* ==================== ADC Conversion Tests ==================== */

void test_adc_raw_to_voltage(void)
{
    xy_adc_init(&test_adc, 3300, ADC_RESOLUTION_12BIT);

    /* 50% of full scale */
    uint32_t raw = 2048; /* 12-bit: 4095 full scale */
    uint32_t voltage = xy_adc_raw_to_voltage(&test_adc, raw);

    /* Should be around 1650mV (50% of 3300mV) */
    TEST_ASSERT_TRUE(voltage > 1500);
    TEST_ASSERT_TRUE(voltage < 1800);
}

void test_adc_voltage_to_raw(void)
{
    xy_adc_init(&test_adc, 3300, ADC_RESOLUTION_12BIT);

    /* 50% of reference */
    uint32_t voltage = 1650;
    uint32_t raw = xy_adc_voltage_to_raw(&test_adc, voltage);

    /* Should be around 2048 (50% of 4095) */
    TEST_ASSERT_TRUE(raw > 1800);
    TEST_ASSERT_TRUE(raw < 2300);
}

void test_adc_conversion_round_trip(void)
{
    xy_adc_init(&test_adc, 3300, ADC_RESOLUTION_12BIT);

    uint32_t original_voltage = 2000;
    uint32_t raw = xy_adc_voltage_to_raw(&test_adc, original_voltage);
    uint32_t converted_voltage = xy_adc_raw_to_voltage(&test_adc, raw);

    /* Should be close to original (within quantization error) */
    int32_t diff = (int32_t)converted_voltage - (int32_t)original_voltage;
    TEST_ASSERT_TRUE(diff > -10 && diff < 10);
}

void test_adc_conversion_invalid_params(void)
{
    /* NULL adc */
    uint32_t result = xy_adc_raw_to_voltage(NULL, 1000);
    TEST_ASSERT_EQUAL(0, result);

    result = xy_adc_voltage_to_raw(NULL, 1000);
    TEST_ASSERT_EQUAL(0, result);
}

/* ==================== DAC Init Tests ==================== */

void test_dac_init(void)
{
    int result;

    /* Test valid initialization */
    result = xy_dac_init(&test_dac, 3300, DAC_RESOLUTION_12BIT);
    adc_assert_ok(result);
    TEST_ASSERT_TRUE(test_dac.initialized);
    TEST_ASSERT_EQUAL(3300, test_dac.ref_voltage_mv);
    TEST_ASSERT_EQUAL(DAC_RESOLUTION_12BIT, test_dac.res);
}

void test_dac_init_invalid_params(void)
{
    int result = xy_dac_init(NULL, 3300, DAC_RESOLUTION_12BIT);
    TEST_ASSERT_EQUAL(XY_ADC_INVALID_PARAM, result);
}

void test_dac_deinit(void)
{
    xy_dac_init(&test_dac, 3300, DAC_RESOLUTION_12BIT);

    int result = xy_dac_deinit(&test_dac);
    adc_assert_ok(result);
    TEST_ASSERT_FALSE(test_dac.initialized);
}

void test_dac_deinit_invalid_params(void)
{
    int result = xy_dac_deinit(NULL);
    TEST_ASSERT_EQUAL(XY_ADC_INVALID_PARAM, result);
}

/* ==================== DAC Channel Tests ==================== */

void test_dac_config_channel(void)
{
    xy_dac_init(&test_dac, 3300, DAC_RESOLUTION_12BIT);

    /* Enable channel */
    int result = xy_dac_config_channel(&test_dac, 0, true);
    adc_assert_ok(result);
    TEST_ASSERT_EQUAL(1, test_dac.channel_count);
    TEST_ASSERT_EQUAL(0, test_dac.channels[0].channel);
    TEST_ASSERT_TRUE(test_dac.channels[0].enabled);
}

void test_dac_config_channel_invalid(void)
{
    xy_dac_init(&test_dac, 3300, DAC_RESOLUTION_12BIT);

    /* Invalid channel */
    int result = xy_dac_config_channel(&test_dac, DAC_MAX_CHANNELS, true);
    TEST_ASSERT_EQUAL(XY_ADC_INVALID_PARAM, result);
}

/* ==================== DAC Output Tests ==================== */

void test_dac_set_voltage(void)
{
    xy_dac_init(&test_dac, 3300, DAC_RESOLUTION_12BIT);
    xy_dac_config_channel(&test_dac, 0, true);

    int result = xy_dac_set_voltage(&test_dac, 0, 1650);
    adc_assert_ok(result);
}

void test_dac_set_raw(void)
{
    xy_dac_init(&test_dac, 3300, DAC_RESOLUTION_12BIT);
    xy_dac_config_channel(&test_dac, 0, true);

    int result = xy_dac_set_raw(&test_dac, 0, 2048);
    adc_assert_ok(result);
}

void test_dac_get_voltage(void)
{
    xy_dac_init(&test_dac, 3300, DAC_RESOLUTION_12BIT);
    xy_dac_config_channel(&test_dac, 0, true);

    uint32_t voltage = xy_dac_get_voltage(&test_dac, 0);
    /* Should return a valid voltage */
    TEST_ASSERT_TRUE(voltage > 0);
    TEST_ASSERT_TRUE(voltage <= 3300);
}

void test_dac_operations_invalid_params(void)
{
    int result;

    /* NULL dac */
    result = xy_dac_set_voltage(NULL, 0, 1650);
    TEST_ASSERT_EQUAL(XY_ADC_INVALID_PARAM, result);

    result = xy_dac_set_raw(NULL, 0, 2048);
    TEST_ASSERT_EQUAL(XY_ADC_INVALID_PARAM, result);

    uint32_t voltage = xy_dac_get_voltage(NULL, 0);
    TEST_ASSERT_EQUAL(0, voltage);
}

/* ==================== Helper Macro Tests ==================== */

void test_adc_max_value_macro(void)
{
    /* Test ADC_MAX_VALUE for different resolutions */
    TEST_ASSERT_EQUAL(255, ADC_MAX_VALUE(ADC_RESOLUTION_8BIT));
    TEST_ASSERT_EQUAL(1023, ADC_MAX_VALUE(ADC_RESOLUTION_10BIT));
    TEST_ASSERT_EQUAL(4095, ADC_MAX_VALUE(ADC_RESOLUTION_12BIT));
    TEST_ASSERT_EQUAL(16383, ADC_MAX_VALUE(ADC_RESOLUTION_14BIT));
    TEST_ASSERT_EQUAL(65535, ADC_MAX_VALUE(ADC_RESOLUTION_16BIT));
}

void test_dac_max_value_macro(void)
{
    /* Test DAC_MAX_VALUE for different resolutions */
    TEST_ASSERT_EQUAL(255, DAC_MAX_VALUE(DAC_RESOLUTION_8BIT));
    TEST_ASSERT_EQUAL(1023, DAC_MAX_VALUE(DAC_RESOLUTION_10BIT));
    TEST_ASSERT_EQUAL(4095, DAC_MAX_VALUE(DAC_RESOLUTION_12BIT));
}

/* ==================== Main ==================== */

int main(void)
{
    UNITY_BEGIN();

    /* ADC Init Tests */
    RUN_TEST(test_adc_init);
    RUN_TEST(test_adc_init_invalid_params);
    RUN_TEST(test_adc_deinit);
    RUN_TEST(test_adc_deinit_invalid_params);

    /* ADC Channel Tests */
    RUN_TEST(test_adc_config_channel);
    RUN_TEST(test_adc_config_channel_invalid);
    RUN_TEST(test_adc_config_channel_disable);

    /* ADC Sample Tests */
    RUN_TEST(test_adc_sample);
    RUN_TEST(test_adc_sample_invalid_params);
    RUN_TEST(test_adc_sample_not_ready);
    RUN_TEST(test_adc_sample_multi);

    /* ADC Conversion Tests */
    RUN_TEST(test_adc_raw_to_voltage);
    RUN_TEST(test_adc_voltage_to_raw);
    RUN_TEST(test_adc_conversion_round_trip);
    RUN_TEST(test_adc_conversion_invalid_params);

    /* DAC Init Tests */
    RUN_TEST(test_dac_init);
    RUN_TEST(test_dac_init_invalid_params);
    RUN_TEST(test_dac_deinit);
    RUN_TEST(test_dac_deinit_invalid_params);

    /* DAC Channel Tests */
    RUN_TEST(test_dac_config_channel);
    RUN_TEST(test_dac_config_channel_invalid);

    /* DAC Output Tests */
    RUN_TEST(test_dac_set_voltage);
    RUN_TEST(test_dac_set_raw);
    RUN_TEST(test_dac_get_voltage);
    RUN_TEST(test_dac_operations_invalid_params);

    /* Helper Macro Tests */
    RUN_TEST(test_adc_max_value_macro);
    RUN_TEST(test_dac_max_value_macro);

    return UNITY_END();
}
