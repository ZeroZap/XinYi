#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "../../components/analog_devices/xy_adc.h"
#include "unity.h"
#include "xy_adc_ext.h"
#include "xy_hal_error.h"
#include "xy_hal_gpio.h"
#include "xy_hal_spi.h"
#include "fff.h"

DEFINE_FFF_GLOBALS;

FAKE_VOID_FUNC(xy_os_delay, uint32_t)
FAKE_VALUE_FUNC(xy_hal_error_t, xy_hal_gpio_init, xy_hal_gpio_port_t, uint8_t,
                const xy_hal_gpio_config_t *)
FAKE_VALUE_FUNC(xy_hal_error_t, xy_hal_gpio_write, xy_hal_gpio_port_t, uint8_t, uint8_t)
FAKE_VALUE_FUNC(int32_t, xy_hal_gpio_read, xy_hal_gpio_port_t, uint8_t)
FAKE_VALUE_FUNC(xy_hal_error_t, xy_hal_spi_transmit_receive, void *, const uint8_t *,
                uint8_t *, size_t, uint32_t)

static int g_gpio_init_count;
static int g_gpio_write_count;
static int g_spi_transfer_count;
static uint8_t g_last_gpio_pin;
static uint8_t g_last_gpio_value;
static uint8_t g_last_spi_tx[2];
static uint8_t g_next_spi_rx[2];
static int32_t g_gpio_read_value;

static void reset_fakes(void);
static xy_hal_error_t fake_gpio_init(xy_hal_gpio_port_t port, uint8_t pin,
                                     const xy_hal_gpio_config_t *config);
static xy_hal_error_t fake_gpio_write(xy_hal_gpio_port_t port, uint8_t pin, uint8_t value);
static int32_t fake_gpio_read(xy_hal_gpio_port_t port, uint8_t pin);
static xy_hal_error_t fake_spi_transmit_receive(void *spi, const uint8_t *tx_data,
                                                uint8_t *rx_data, size_t len,
                                                uint32_t timeout);

void xy_log_char(char ch)
{
    (void)ch;
}

void setUp(void)
{
    reset_fakes();
}

void tearDown(void)
{
}

static xy_hal_error_t fake_gpio_init(xy_hal_gpio_port_t port, uint8_t pin,
                                     const xy_hal_gpio_config_t *config)
{
    (void)port;
    TEST_ASSERT_NOT_NULL(config);
    g_gpio_init_count++;
    g_last_gpio_pin = pin;
    return XY_HAL_OK;
}

static xy_hal_error_t fake_gpio_write(xy_hal_gpio_port_t port, uint8_t pin, uint8_t value)
{
    (void)port;
    g_gpio_write_count++;
    g_last_gpio_pin = pin;
    g_last_gpio_value = value;
    return XY_HAL_OK;
}

static int32_t fake_gpio_read(xy_hal_gpio_port_t port, uint8_t pin)
{
    (void)port;
    (void)pin;
    return g_gpio_read_value;
}

static xy_hal_error_t fake_spi_transmit_receive(void *spi, const uint8_t *tx_data,
                                                uint8_t *rx_data, size_t len,
                                                uint32_t timeout)
{
    (void)spi;
    (void)timeout;
    TEST_ASSERT_NOT_NULL(tx_data);
    TEST_ASSERT_NOT_NULL(rx_data);
    TEST_ASSERT_EQUAL_UINT32(2U, len);
    g_spi_transfer_count++;
    memcpy(g_last_spi_tx, tx_data, 2);
    memcpy(rx_data, g_next_spi_rx, 2);
    return XY_HAL_OK;
}

static void reset_fakes(void)
{
    RESET_FAKE(xy_os_delay);
    RESET_FAKE(xy_hal_gpio_init);
    RESET_FAKE(xy_hal_gpio_write);
    RESET_FAKE(xy_hal_gpio_read);
    RESET_FAKE(xy_hal_spi_transmit_receive);
    FFF_RESET_HISTORY();

    xy_hal_gpio_init_fake.custom_fake = fake_gpio_init;
    xy_hal_gpio_write_fake.custom_fake = fake_gpio_write;
    xy_hal_gpio_read_fake.custom_fake = fake_gpio_read;
    xy_hal_spi_transmit_receive_fake.custom_fake = fake_spi_transmit_receive;

    g_gpio_init_count = 0;
    g_gpio_write_count = 0;
    g_spi_transfer_count = 0;
    g_last_gpio_pin = 0;
    g_last_gpio_value = 0;
    g_last_spi_tx[0] = g_last_spi_tx[1] = 0;
    g_next_spi_rx[0] = g_next_spi_rx[1] = 0;
    g_gpio_read_value = 0;
}

static void test_adc_dac_core_contracts(void)
{
    xy_adc_t adc;
    xy_adc_sample_t samples[ADC_MAX_CHANNELS];

    TEST_ASSERT_EQUAL_INT(XY_ADC_INVALID_PARAM, xy_adc_init(NULL, 3300, ADC_RESOLUTION_12BIT));
    TEST_ASSERT_EQUAL_INT(XY_ADC_OK, xy_adc_init(&adc, 3300, ADC_RESOLUTION_12BIT));
    TEST_ASSERT_TRUE(adc.initialized);
    TEST_ASSERT_EQUAL_UINT16(3300, adc.ref_voltage_mv);
    TEST_ASSERT_EQUAL_INT(ADC_RESOLUTION_12BIT, adc.res);

    TEST_ASSERT_EQUAL_INT(XY_ADC_INVALID_PARAM, xy_adc_config_channel(NULL, 0, true));
    TEST_ASSERT_EQUAL_INT(XY_ADC_INVALID_PARAM, xy_adc_config_channel(&adc, ADC_MAX_CHANNELS, true));
    TEST_ASSERT_EQUAL_INT(XY_ADC_OK, xy_adc_config_channel(&adc, 3, true));
    TEST_ASSERT_EQUAL_INT(XY_ADC_OK, xy_adc_config_channel(&adc, 5, true));
    TEST_ASSERT_EQUAL_UINT8(2, adc.channel_count);
    TEST_ASSERT_EQUAL_INT(XY_ADC_OK, xy_adc_config_channel(&adc, 3, false));
    TEST_ASSERT_FALSE(adc.channels[0].enabled);

    TEST_ASSERT_EQUAL_UINT16(3300, xy_adc_raw_to_voltage(&adc, 4095));
    TEST_ASSERT_EQUAL_UINT16(4095, xy_adc_voltage_to_raw(&adc, 3300));

    memset(samples, 0, sizeof(samples));
    TEST_ASSERT_EQUAL_INT(1, xy_adc_sample_multi(&adc, samples, 4));
    TEST_ASSERT_EQUAL_UINT8(5, samples[0].channel);
    TEST_ASSERT_TRUE(samples[0].valid);

    TEST_ASSERT_EQUAL_INT(XY_ADC_OK, xy_adc_deinit(&adc));
    TEST_ASSERT_FALSE(adc.initialized);
    TEST_ASSERT_EQUAL_INT(XY_ADC_NOT_READY, xy_adc_sample(&adc, 0, &samples[0]));

    xy_dac_t dac;
    TEST_ASSERT_EQUAL_INT(XY_ADC_INVALID_PARAM, xy_dac_init(NULL, 3300, DAC_RESOLUTION_12BIT));
    TEST_ASSERT_EQUAL_INT(XY_ADC_OK, xy_dac_init(&dac, 3300, DAC_RESOLUTION_12BIT));
    TEST_ASSERT_EQUAL_INT(XY_ADC_OK, xy_dac_config_channel(&dac, 0, true));
    TEST_ASSERT_EQUAL_INT(XY_ADC_INVALID_PARAM, xy_dac_config_channel(&dac, DAC_MAX_CHANNELS, true));
    TEST_ASSERT_EQUAL_INT(XY_ADC_OK, xy_dac_set_voltage(&dac, 0, 1650));
    TEST_ASSERT_EQUAL_INT(XY_ADC_OK, xy_dac_set_raw(&dac, 0, 2048));
    TEST_ASSERT_EQUAL_UINT16(1650, xy_dac_get_voltage(&dac, 0));
    TEST_ASSERT_EQUAL_INT(XY_ADC_OK, xy_dac_deinit(&dac));
}

static void test_mcp3008_spi_command_and_result(void)
{
    reset_fakes();
    xy_mcp3008_t mcp;
    int fake_spi;
    TEST_ASSERT_EQUAL_INT(-1, xy_mcp3008_init(NULL, &fake_spi, 10));
    TEST_ASSERT_EQUAL_INT(-1, xy_mcp3008_init(&mcp, NULL, 10));
    TEST_ASSERT_EQUAL_INT(0, xy_mcp3008_init(&mcp, &fake_spi, 10));
    TEST_ASSERT_EQUAL_PTR(&fake_spi, mcp.spi_handle);
    TEST_ASSERT_EQUAL_UINT8(10, mcp.cs_pin);
    TEST_ASSERT_EQUAL_INT(1, g_gpio_init_count);
    TEST_ASSERT_EQUAL_UINT(1U, xy_hal_gpio_init_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(10, xy_hal_gpio_init_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT8(10, g_last_gpio_pin);
    TEST_ASSERT_EQUAL_UINT8(1, g_last_gpio_value);

    uint16_t value = 0;
    g_next_spi_rx[0] = 0x03;
    g_next_spi_rx[1] = 0xFE;
    TEST_ASSERT_EQUAL_INT(0, xy_mcp3008_read(&mcp, 2, &value));
    TEST_ASSERT_EQUAL_INT(1, g_spi_transfer_count);
    TEST_ASSERT_EQUAL_UINT(1U, xy_hal_spi_transmit_receive_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&fake_spi, xy_hal_spi_transmit_receive_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT(2U, xy_hal_spi_transmit_receive_fake.arg3_val);
    TEST_ASSERT_EQUAL_HEX8(0x01, g_last_spi_tx[0]);
    TEST_ASSERT_EQUAL_HEX8(0xA0, g_last_spi_tx[1]);
    TEST_ASSERT_EQUAL_HEX16(0x1FF, value);
    TEST_ASSERT_EQUAL_UINT8(1, g_last_gpio_value);

    TEST_ASSERT_EQUAL_INT(-1, xy_mcp3008_read(&mcp, 8, &value));
    TEST_ASSERT_EQUAL_INT(-1, xy_mcp3008_read(&mcp, 0, NULL));
}

static void test_hx711_lifecycle_timeout_and_read(void)
{
    reset_fakes();
    xy_hx711_t hx;
    TEST_ASSERT_EQUAL_INT(-1, xy_hx711_init(NULL, 4, 5));
    TEST_ASSERT_EQUAL_INT(0, xy_hx711_init(&hx, 4, 5));
    TEST_ASSERT_EQUAL_UINT8(4, hx.pd_sck_pin);
    TEST_ASSERT_EQUAL_UINT8(5, hx.dout_pin);
    TEST_ASSERT_EQUAL_UINT8(1, hx.gain);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, hx.scale);

    int32_t value = 0;
    g_gpio_read_value = 1;
    TEST_ASSERT_EQUAL_INT(-1, xy_hx711_read(&hx, &value));

    g_gpio_read_value = 0;
    TEST_ASSERT_EQUAL_INT(0, xy_hx711_read(&hx, &value));
    TEST_ASSERT_GREATER_THAN_INT(24, g_gpio_write_count);
    TEST_ASSERT_GREATER_THAN_UINT(24U, xy_hal_gpio_write_fake.call_count);
    TEST_ASSERT_GREATER_THAN_UINT(1U, xy_hal_gpio_read_fake.call_count);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_adc_dac_core_contracts);
    RUN_TEST(test_mcp3008_spi_command_and_result);
    RUN_TEST(test_hx711_lifecycle_timeout_and_read);
    return UNITY_END();
}
