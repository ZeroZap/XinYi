#include "stm32l4xx_hal.h"
#include "pandora_fota_flash.h"
#include "pandora_soft_i2c.h"
#include "xy_device.h"
#include "xy_hal_gpio.h"
#include "xy_hal_uart.h"
#include "xy_hal_wdg.h"
#include "xy_sys.h"

static UART_HandleTypeDef uart1;


#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the source commit used for this image"
#endif

void _init(void) {}
void _fini(void) {}

void SysTick_Handler(void)
{
    HAL_IncTick();
}

static void fail(void)
{
    __disable_irq();
    for (;;) {
    }
}

static void uart_hex32(uint32_t value)
{
    static const char digits[] = "0123456789ABCDEF";
    uint8_t text[8];

    for (uint32_t index = 0; index < sizeof(text); ++index) {
        uint32_t shift = (uint32_t)(sizeof(text) - 1U - index) * 4U;
        text[index] = (uint8_t)digits[(value >> shift) & 0x0FU];
    }
    (void)xy_hal_uart_send(&uart1, text, sizeof(text), 100U);
}

static int aht10_init(xy_i2c_device_t *device)
{
    static const uint8_t command[] = {0xE1U, 0x08U, 0x00U};
    HAL_Delay(40U);
    if (xy_i2c_device_write(device, command, sizeof(command)) != XY_DEVICE_OK) {
        return 0;
    }
    HAL_Delay(10U);
    return 1;
}

static int aht10_measure(xy_i2c_device_t *device, uint32_t *humidity_milli_percent,
                         int32_t *temperature_milli_c)
{
    static const uint8_t command[] = {0xACU, 0x33U, 0x00U};
    uint8_t data[6];
    if (xy_i2c_device_write(device, command, sizeof(command)) != XY_DEVICE_OK) {
        return 0;
    }
    HAL_Delay(80U);
    if (xy_i2c_device_read(device, data, sizeof(data)) != XY_DEVICE_OK ||
        (data[0] & 0x80U) != 0U) {
        return 0;
    }
    uint32_t raw_humidity = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) |
                            ((uint32_t)data[3] >> 4);
    uint32_t raw_temperature = ((uint32_t)(data[3] & 0x0FU) << 16) |
                               ((uint32_t)data[4] << 8) | data[5];
    *humidity_milli_percent = (uint32_t)(((uint64_t)raw_humidity * 100000U) >> 20);
    *temperature_milli_c = (int32_t)(((uint64_t)raw_temperature * 200000U) >> 20) - 50000;
    return 1;
}

static void uart_text(const char *text)
{
    uint16_t length = 0;
    while (text[length] != '\0') ++length;
    (void)xy_hal_uart_send(&uart1, (const uint8_t *)text, length, 100U);
}

static void uart_u32(uint32_t value)
{
    uint8_t digits[10];
    uint32_t count = 0;
    do {
        digits[count++] = (uint8_t)('0' + value % 10U);
        value /= 10U;
    } while (value != 0U);
    while (count != 0U) {
        --count;
        (void)xy_hal_uart_send(&uart1, &digits[count], 1U, 100U);
    }
}

static void uart_i32(int32_t value)
{
    if (value < 0) {
        uart_text("-");
        uart_u32((uint32_t)(-(int64_t)value));
    } else {
        uart_u32((uint32_t)value);
    }
}

static void clock_init(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
        fail();
    }
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM = 1;
    osc.PLL.PLLN = 20;
    osc.PLL.PLLP = RCC_PLLP_DIV7;
    osc.PLL.PLLQ = RCC_PLLQ_DIV2;
    osc.PLL.PLLR = RCC_PLLR_DIV2;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
        fail();
    }
    clk.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                    RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_4) != HAL_OK) {
        fail();
    }
}

static void gpio_uart_init(void)
{
    const xy_hal_gpio_config_t led_gpio = {
        XY_HAL_GPIO_MODE_OUTPUT, XY_HAL_GPIO_PULL_NONE, XY_HAL_GPIO_OTYPE_PP,
        XY_HAL_GPIO_SPEED_LOW, 0U,
    };
    const xy_hal_gpio_config_t key_gpio = {
        XY_HAL_GPIO_MODE_INPUT, XY_HAL_GPIO_PULL_UP, XY_HAL_GPIO_OTYPE_PP,
        XY_HAL_GPIO_SPEED_LOW, 0U,
    };
    const xy_hal_gpio_config_t uart_gpio = {
        XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_UP, XY_HAL_GPIO_OTYPE_PP,
        XY_HAL_GPIO_SPEED_VERY_HIGH, GPIO_AF7_USART1,
    };
    const xy_hal_uart_config_t uart_config = {
        115200U, XY_HAL_UART_WORDLEN_8B, XY_HAL_UART_STOPBITS_1, XY_HAL_UART_PARITY_NONE,
        XY_HAL_UART_FLOWCTRL_NONE, XY_HAL_UART_MODE_TX_RX,
    };
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    __HAL_RCC_USART1_CLK_ENABLE();

    if (xy_hal_gpio_init(GPIOE, 7U, &led_gpio) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOD, 8U, &key_gpio) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOD, 9U, &key_gpio) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOD, 10U, &key_gpio) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 9U, &uart_gpio) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &uart_gpio) != XY_HAL_OK ||
        xy_hal_uart_init(&uart1, &uart_config) != XY_HAL_OK) {
        fail();
    }
}

int main(void)
{
    static const uint8_t banner[] = "PANDORA STM32L475VE XINYI SMOKE OK\r\n";
    static const uint8_t firmware_commit[] = "FIRMWARE_COMMIT " XINYI_FIRMWARE_COMMIT "\r\n";
    static const uint8_t key0[] = "KEY0\r\n";
    static const uint8_t aht_ack[] = "AHT10 0x38 ACK\r\n";
    static const uint8_t aht_nack[] = "AHT10 0x38 NACK\r\n";
    uint32_t humidity_milli_percent = 0;
    int32_t temperature_milli_c = 0;
    uint32_t reset_reason = 0;
    uint32_t chip_id[3] = {0};
    xy_i2c_device_t aht10 = {0};
    const xy_fota_metadata_flash_t *metadata_backend;
    xy_fota_metadata_t metadata = {
        .active_version = 1U,
        .min_version = 1U,
        .active_slot = 0U,
        .pending_slot = XY_FOTA_METADATA_NO_SLOT,
    };
    xy_fota_metadata_t committed;
    xy_fota_metadata_t loaded;
    IWDG_HandleTypeDef watchdog = {0};
    const xy_hal_iwdg_config_t watchdog_config = {
        .prescaler = IWDG_PRESCALER_32,
        .reload = 125U,
        .timeout_ms = 0U,
    };

    HAL_Init();
    clock_init();
    gpio_uart_init();
    xy_sys_init();
    if (xy_sys_reboot_reason(&reset_reason) != XY_OK || xy_sys_get_chip_id(chip_id) != XY_OK) {
        fail();
    }
    uart_text("SYS_RESET_CSR 0x");
    uart_hex32(reset_reason);
    uart_text("\r\nSYS_CHIP_ID ");
    uart_hex32(chip_id[0]);
    uart_hex32(chip_id[1]);
    uart_hex32(chip_id[2]);
    uart_text("\r\n");
    if ((reset_reason & RCC_CSR_IWDGRSTF) != 0U) {
        uart_text("SYS_RESET_KIND WATCHDOG\r\nSYS_WATCHDOG_RESET_OK\r\n");
    } else if ((reset_reason & RCC_CSR_SFTRSTF) != 0U) {
        uart_text("SYS_RESET_KIND SOFTWARE\r\nSYS_SOFTWARE_RESET_OK\r\n");
        uart_text("SYS_WATCHDOG_RESET_REQUEST\r\n");
        HAL_Delay(250U);
        watchdog.Instance = IWDG;
        if (xy_hal_iwdg_init(&watchdog, &watchdog_config) != XY_HAL_OK) {
            fail();
        }
        for (;;) {
        }
    } else if ((reset_reason & RCC_CSR_PINRSTF) != 0U &&
               (reset_reason & RCC_CSR_BORRSTF) == 0U) {
        uart_text("SYS_RESET_KIND EXTERNAL_PIN\r\nSYS_EXTERNAL_PIN_RESET_OK\r\n");
        uart_text("SYS_SOFTWARE_RESET_REQUEST\r\n");
        HAL_Delay(250U);
        (void)xy_sys_reset(1);
        fail();
    } else {
        uart_text("SYS_RESET_KIND POWER_ON\r\nSYS_SOFTWARE_RESET_REQUEST\r\n");
        HAL_Delay(250U);
        (void)xy_sys_reset(1);
        fail();
    }
    uart_text("FOTA_FIRMWARE_COMMIT " XINYI_FIRMWARE_COMMIT "\r\n");
    metadata_backend = pandora_fota_metadata_backend();
    if (xy_fota_metadata_flash_validate(metadata_backend) != XY_FOTA_OK) {
        fail();
    }
    if (xy_fota_metadata_flash_load(metadata_backend, &loaded) == XY_FOTA_NO_IMAGE) {
        if (xy_fota_metadata_flash_commit(metadata_backend, &metadata, &committed) != XY_FOTA_OK ||
            xy_fota_metadata_flash_load(metadata_backend, &loaded) != XY_FOTA_OK ||
            loaded.generation != committed.generation || loaded.active_version != 1U ||
            loaded.active_slot != 0U) {
            fail();
        }
        uart_text("FOTA_METADATA_INITIALIZED slot=0 version=1\r\n");
    }

    if ((loaded.flags & XY_FOTA_METADATA_FLAG_PENDING) == 0U && loaded.active_slot == 0U &&
        loaded.active_version == 1U) {
        if (xy_fota_metadata_boot_handoff(1U, 2U, (void *)metadata_backend) != XY_FOTA_OK) {
            fail();
        }
        uart_text("FOTA_BOOT_HANDOFF_COMMITTED slot=1 version=2\r\n");
        HAL_Delay(250U);
        (void)xy_sys_reset(1);
        fail();
    }

    if ((loaded.flags & XY_FOTA_METADATA_FLAG_PENDING) != 0U && loaded.pending_slot == 1U &&
        loaded.pending_version == 2U && loaded.boot_attempts == 0U) {
        bool rollback_required = true;
        if (xy_fota_metadata_boot_attempt(3U, &rollback_required, (void *)metadata_backend) !=
                XY_FOTA_OK ||
            rollback_required) {
            fail();
        }
        uart_text("FOTA_BOOT_ATTEMPT_COMMITTED count=1\r\n");
        HAL_Delay(250U);
        (void)xy_sys_reset(1);
        fail();
    }

    if ((loaded.flags & XY_FOTA_METADATA_FLAG_PENDING) != 0U && loaded.pending_slot == 1U &&
        loaded.pending_version == 2U && loaded.boot_attempts == 1U) {
        if (xy_fota_metadata_boot_confirm(1U, 2U, (void *)metadata_backend) != XY_FOTA_OK ||
            xy_fota_metadata_flash_load(metadata_backend, &loaded) != XY_FOTA_OK) {
            fail();
        }
        uart_text("FOTA_BOOT_CONFIRM_COMMITTED slot=1 version=2\r\n");
    }

    if ((loaded.flags & XY_FOTA_METADATA_FLAG_PENDING) == 0U) {
        if (loaded.active_slot != 1U || loaded.active_version != 2U || loaded.min_version != 2U ||
            loaded.boot_attempts != 0U) {
            fail();
        }
        uart_text("FOTA_BOOT_CONTRACT_OK active_slot=1 version=2 min_version=2\r\n");
        if (loaded.generation == 4U) {
            if (xy_fota_metadata_boot_handoff(0U, 1U, (void *)metadata_backend) !=
                    XY_FOTA_VERSION_ERROR ||
                xy_fota_metadata_flash_load(metadata_backend, &committed) != XY_FOTA_OK ||
                committed.generation != loaded.generation ||
                committed.active_slot != loaded.active_slot ||
                committed.active_version != loaded.active_version ||
                committed.min_version != loaded.min_version ||
                committed.pending_slot != loaded.pending_slot ||
                committed.pending_version != loaded.pending_version ||
                committed.flags != loaded.flags) {
                fail();
            }
            uart_text("FOTA_ANTI_ROLLBACK_REJECTED version=1 floor=2\r\n");
            if (xy_fota_metadata_boot_handoff(0U, 3U, (void *)metadata_backend) != XY_FOTA_OK) {
                fail();
            }
            uart_text("FOTA_ROLLBACK_HANDOFF_COMMITTED slot=0 version=3\r\n");
            HAL_Delay(250U);
            (void)xy_sys_reset(1);
            fail();
        }
    } else if (loaded.pending_slot == 0U && loaded.pending_version == 3U &&
               loaded.boot_attempts == 0U) {
        bool rollback_required = true;
        if (xy_fota_metadata_boot_attempt(2U, &rollback_required, (void *)metadata_backend) !=
                XY_FOTA_OK ||
            rollback_required) {
            fail();
        }
        uart_text("FOTA_ROLLBACK_ATTEMPT_COMMITTED count=1\r\n");
        HAL_Delay(250U);
        (void)xy_sys_reset(1);
        fail();
    } else if (loaded.pending_slot == 0U && loaded.pending_version == 3U &&
               loaded.boot_attempts == 1U) {
        bool rollback_required = false;
        if (xy_fota_metadata_boot_attempt(2U, &rollback_required, (void *)metadata_backend) !=
                XY_FOTA_OK ||
            !rollback_required ||
            xy_fota_metadata_flash_load(metadata_backend, &loaded) != XY_FOTA_OK ||
            (loaded.flags & XY_FOTA_METADATA_FLAG_PENDING) != 0U || loaded.active_slot != 1U ||
            loaded.active_version != 2U || loaded.min_version != 2U ||
            loaded.boot_attempts != 0U) {
            fail();
        }
        uart_text("FOTA_AUTOMATIC_ROLLBACK_COMMITTED active_slot=1 version=2\r\n");
    } else {
        fail();
    }
    uart_text("FOTA_METADATA_FLASH_OK\r\n");
    if (xy_i2c_device_init(&aht10, pandora_soft_i2c_init(), 0x38U, 100U) != XY_DEVICE_OK) {
        fail();
    }
    int aht_initialized = aht10_init(&aht10);
    for (;;) {
        (void)xy_hal_gpio_toggle(GPIOE, 7U);
        (void)xy_hal_uart_send(&uart1, banner, sizeof(banner) - 1U, 100U);
        (void)xy_hal_uart_send(&uart1, firmware_commit, sizeof(firmware_commit) - 1U, 100U);
        if (aht_initialized &&
            aht10_measure(&aht10, &humidity_milli_percent, &temperature_milli_c)) {
            (void)xy_hal_uart_send(&uart1, aht_ack, sizeof(aht_ack) - 1U, 100U);
            uart_text("AHT10 RH_milli_percent=");
            uart_u32(humidity_milli_percent);
            uart_text(" T_milli_c=");
            uart_i32(temperature_milli_c);
            uart_text("\r\n");
        } else {
            (void)xy_hal_uart_send(&uart1, aht_nack, sizeof(aht_nack) - 1U, 100U);
            aht_initialized = aht10_init(&aht10);
        }
        if (xy_hal_gpio_read(GPIOD, 10U) == 0) {
            (void)xy_hal_uart_send(&uart1, key0, sizeof(key0) - 1U, 100U);
        }
        HAL_Delay(500U);
    }
}
