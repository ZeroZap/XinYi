#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "../../components/analog_devices/xy_adc.h"
#include "xy_adc_ext.h"
#include "xy_hal_error.h"
#include "xy_hal_gpio.h"
#include "xy_hal_spi.h"

static int g_gpio_init_count;
static int g_gpio_write_count;
static int g_spi_transfer_count;
static uint8_t g_last_gpio_pin;
static uint8_t g_last_gpio_value;
static uint8_t g_last_spi_tx[2];
static uint8_t g_next_spi_rx[2];
static int32_t g_gpio_read_value;

void xy_log_char(char ch)
{
    (void)ch;
}

void xy_os_delay(uint32_t ms)
{
    (void)ms;
}

xy_hal_error_t xy_hal_gpio_init(xy_hal_gpio_port_t port, uint8_t pin,
                                const xy_hal_gpio_config_t *config)
{
    (void)port;
    assert(config != NULL);
    g_gpio_init_count++;
    g_last_gpio_pin = pin;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_write(xy_hal_gpio_port_t port, uint8_t pin, uint8_t value)
{
    (void)port;
    g_gpio_write_count++;
    g_last_gpio_pin = pin;
    g_last_gpio_value = value;
    return XY_HAL_OK;
}

int32_t xy_hal_gpio_read(xy_hal_gpio_port_t port, uint8_t pin)
{
    (void)port;
    (void)pin;
    return g_gpio_read_value;
}

xy_hal_error_t xy_hal_spi_transmit_receive(void *spi, const uint8_t *tx_data,
                                           uint8_t *rx_data, size_t len,
                                           uint32_t timeout)
{
    (void)spi;
    (void)timeout;
    assert(tx_data != NULL);
    assert(rx_data != NULL);
    assert(len == 2);
    g_spi_transfer_count++;
    memcpy(g_last_spi_tx, tx_data, 2);
    memcpy(rx_data, g_next_spi_rx, 2);
    return XY_HAL_OK;
}

static void reset_fakes(void)
{
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

    assert(xy_adc_init(NULL, 3300, ADC_RESOLUTION_12BIT) == XY_ADC_INVALID_PARAM);
    assert(xy_adc_init(&adc, 3300, ADC_RESOLUTION_12BIT) == XY_ADC_OK);
    assert(adc.initialized);
    assert(adc.ref_voltage_mv == 3300);
    assert(adc.res == ADC_RESOLUTION_12BIT);

    assert(xy_adc_config_channel(NULL, 0, true) == XY_ADC_INVALID_PARAM);
    assert(xy_adc_config_channel(&adc, ADC_MAX_CHANNELS, true) == XY_ADC_INVALID_PARAM);
    assert(xy_adc_config_channel(&adc, 3, true) == XY_ADC_OK);
    assert(xy_adc_config_channel(&adc, 5, true) == XY_ADC_OK);
    assert(adc.channel_count == 2);
    assert(xy_adc_config_channel(&adc, 3, false) == XY_ADC_OK);
    assert(!adc.channels[0].enabled);

    assert(xy_adc_raw_to_voltage(&adc, 4095) == 3300);
    assert(xy_adc_voltage_to_raw(&adc, 3300) == 4095);

    memset(samples, 0, sizeof(samples));
    assert(xy_adc_sample_multi(&adc, samples, 4) == 1);
    assert(samples[0].channel == 5);
    assert(samples[0].valid);

    assert(xy_adc_deinit(&adc) == XY_ADC_OK);
    assert(!adc.initialized);
    assert(xy_adc_sample(&adc, 0, &samples[0]) == XY_ADC_NOT_READY);

    xy_dac_t dac;
    assert(xy_dac_init(NULL, 3300, DAC_RESOLUTION_12BIT) == XY_ADC_INVALID_PARAM);
    assert(xy_dac_init(&dac, 3300, DAC_RESOLUTION_12BIT) == XY_ADC_OK);
    assert(xy_dac_config_channel(&dac, 0, true) == XY_ADC_OK);
    assert(xy_dac_config_channel(&dac, DAC_MAX_CHANNELS, true) == XY_ADC_INVALID_PARAM);
    assert(xy_dac_set_voltage(&dac, 0, 1650) == XY_ADC_OK);
    assert(xy_dac_set_raw(&dac, 0, 2048) == XY_ADC_OK);
    assert(xy_dac_get_voltage(&dac, 0) == 1650);
    assert(xy_dac_deinit(&dac) == XY_ADC_OK);
}

static void test_mcp3008_spi_command_and_result(void)
{
    reset_fakes();
    xy_mcp3008_t mcp;
    int fake_spi;
    assert(xy_mcp3008_init(NULL, &fake_spi, 10) == -1);
    assert(xy_mcp3008_init(&mcp, NULL, 10) == -1);
    assert(xy_mcp3008_init(&mcp, &fake_spi, 10) == 0);
    assert(mcp.spi_handle == &fake_spi);
    assert(mcp.cs_pin == 10);
    assert(g_gpio_init_count == 1);
    assert(g_last_gpio_pin == 10);
    assert(g_last_gpio_value == 1);

    uint16_t value = 0;
    g_next_spi_rx[0] = 0x03;
    g_next_spi_rx[1] = 0xFE;
    assert(xy_mcp3008_read(&mcp, 2, &value) == 0);
    assert(g_spi_transfer_count == 1);
    assert(g_last_spi_tx[0] == 0x01);
    assert(g_last_spi_tx[1] == 0xA0);
    assert(value == 0x1FF);
    assert(g_last_gpio_value == 1);

    assert(xy_mcp3008_read(&mcp, 8, &value) == -1);
    assert(xy_mcp3008_read(&mcp, 0, NULL) == -1);
}

static void test_hx711_lifecycle_timeout_and_read(void)
{
    reset_fakes();
    xy_hx711_t hx;
    assert(xy_hx711_init(NULL, 4, 5) == -1);
    assert(xy_hx711_init(&hx, 4, 5) == 0);
    assert(hx.pd_sck_pin == 4);
    assert(hx.dout_pin == 5);
    assert(hx.gain == 1);
    assert(hx.scale == 1.0f);

    int32_t value = 0;
    g_gpio_read_value = 1;
    assert(xy_hx711_read(&hx, &value) == -1);

    g_gpio_read_value = 0;
    assert(xy_hx711_read(&hx, &value) == 0);
    assert(g_gpio_write_count > 24);
}

int main(void)
{
    test_adc_dac_core_contracts();
    test_mcp3008_spi_command_and_result();
    test_hx711_lifecycle_timeout_and_read();
    return 0;
}
