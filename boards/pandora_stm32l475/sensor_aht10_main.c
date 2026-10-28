/**
 * @file sensor_aht10_main.c
 * @brief Pandora AHT10 validation image on the canonical board-owned driver.
 *
 * The board owns one `xy_aht10` instance through `pandora_aht10_owner`; the
 * inline `xy_i2c_device_*` protocol copy that used to live in `main.c` is not
 * part of this image. U7 AHT10 sits on the board software-I2C4 bus (SCL=PD6,
 * SDA=PC1), which is the path the earlier AHT10 B1 evidence was taken on.
 */

#include "pandora_aht10_owner.h"
#include "pandora_platform_startup.h"
#include "pandora_soft_i2c.h"
#include "stm32l4xx_hal.h"
#include "xy_device.h"
#include "xy_hal_delay.h"
#include "xy_hal_sys.h"
#include "xy_hal_uart.h"

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the source commit used for this image"
#endif

/* AHT10 public range: 0..100 %RH and -40..85 C, in the driver's centi units. */
#define AHT10_HUMIDITY_MAX_CENTI 10000
#define AHT10_TEMPERATURE_MIN_CENTI (-4000)
#define AHT10_TEMPERATURE_MAX_CENTI 8500

static UART_HandleTypeDef uart1;
static pandora_aht10_owner_t aht10_owner;

void _init(void) {}
void _fini(void) {}
void SysTick_Handler(void) {
    xy_hal_sys_tick_irq_handler();
}
void delay_ms(uint32_t ms) {
    xy_hal_delay_ms(ms);
}
uint32_t get_tick_ms(void) {
    return xy_hal_sys_get_tick_count();
}

static void fail(void) {
    __disable_irq();
    for (;;) {
    }
}

static void uart_text(const char* text) {
    uint16_t length = 0U;
    while (text[length] != '\0') {
        ++length;
    }
    (void)xy_hal_uart_send(&uart1, (const uint8_t*)text, length, 100U);
}

static void uart_u32(uint32_t value) {
    uint8_t digits[10];
    uint32_t count = 0U;
    do {
        digits[count++] = (uint8_t)('0' + value % 10U);
        value /= 10U;
    } while (value != 0U);
    while (count != 0U) {
        --count;
        (void)xy_hal_uart_send(&uart1, &digits[count], 1U, 100U);
    }
}

static void uart_i32(int32_t value) {
    if (value < 0) {
        uart_text("-");
        uart_u32((uint32_t)(-(int64_t)value));
    } else {
        uart_u32((uint32_t)value);
    }
}

static void uart_hex8(uint8_t value) {
    static const char hex[] = "0123456789ABCDEF";
    char encoded[2] = {hex[value >> 4], hex[value & 0x0FU]};
    (void)xy_hal_uart_send(&uart1, (const uint8_t*)encoded, sizeof(encoded), 100U);
}

static void uart_init(void) {
    const xy_hal_gpio_config_t gpio = {XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_UP,
                                       XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_VERY_HIGH,
                                       GPIO_AF7_USART1};
    const xy_hal_uart_config_t uart = {115200U,
                                       XY_HAL_UART_WORDLEN_8B,
                                       XY_HAL_UART_STOPBITS_1,
                                       XY_HAL_UART_PARITY_NONE,
                                       XY_HAL_UART_FLOWCTRL_NONE,
                                       XY_HAL_UART_MODE_TX_RX};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();
    if (xy_hal_gpio_init(GPIOA, 9U, &gpio) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &gpio) != XY_HAL_OK) {
        fail();
    }
    uart1.Instance = USART1;
    if (xy_hal_uart_init(&uart1, &uart) != XY_HAL_OK) {
        fail();
    }
}

static void log_bus_scan(void* bus) {
    uint32_t ack_count = 0U;
    int aht10_seen = 0;

    for (uint32_t address = 0x08U; address <= 0x77U; ++address) {
        if (pandora_soft_i2c_probe(bus, (uint8_t)address)) {
            uart_text("I2C4_ACK=0x");
            uart_hex8((uint8_t)address);
            uart_text("\r\n");
            ++ack_count;
            if (address == XY_AHT10_DEFAULT_ADDRESS) {
                aht10_seen = 1;
            }
        }
    }
    uart_text("PANDORA_I2C4_SCAN_DONE count=");
    uart_u32(ack_count);
    uart_text("\r\n");
    if (!aht10_seen) {
        uart_text("PANDORA_AHT10_ACK_MISSING\r\n");
        fail();
    }
}

int main(void) {
    xy_aht10_data_t sample;
    void* i2c4;

    if (xy_hal_sys_init() != XY_HAL_OK) {
        fail();
    }
    if (pandora_platform_startup() != 0) {
        fail();
    }
    uart_init();
    uart_text("PANDORA AHT10 SENSOR READY\r\nFIRMWARE_COMMIT " XINYI_FIRMWARE_COMMIT
              "\r\nAHT10_BUS=SOFT_I2C4 SCL=PD6 SDA=PC1 ADDR=0x38\r\n");
    i2c4 = pandora_soft_i2c_init();
    uart_text("PANDORA_I2C4_INIT_DONE\r\n");
    log_bus_scan(i2c4);
    {
        xy_error_t init_result = pandora_aht10_owner_init(&aht10_owner, i2c4);

        if (init_result != XY_DEVICE_OK) {
            uart_text("PANDORA_AHT10_DRIVER_INIT_ERROR result=");
            uart_i32(init_result);
            uart_text("\r\n");
            fail();
        }
    }
    uart_text("AHT10_DRIVER_READY ADDR=0x38\r\n");

    for (;;) {
        if (pandora_aht10_owner_sample(&aht10_owner, &sample) != XY_DEVICE_OK) {
            uart_text("PANDORA_AHT10_READ_ERROR\r\n");
            fail();
        }
        if (sample.humidity_centi_pct > AHT10_HUMIDITY_MAX_CENTI ||
            sample.temperature_centi_c < AHT10_TEMPERATURE_MIN_CENTI ||
            sample.temperature_centi_c > AHT10_TEMPERATURE_MAX_CENTI) {
            uart_text("PANDORA_AHT10_RANGE_ERROR\r\n");
            fail();
        }
        uart_text("AHT10 SAMPLE hum_centi=");
        uart_u32(sample.humidity_centi_pct);
        uart_text(" temp_centi=");
        uart_i32(sample.temperature_centi_c);
        uart_text("\r\n");
        xy_hal_delay_ms(100U);
    }
}
