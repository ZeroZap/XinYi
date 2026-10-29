#include "unity.h"

#include <stdint.h>
#include <string.h>

#include "xy_hal_error.h"
#include "xy_hal_gpio.h"
#include "xy_hal_i2c.h"
#include "xy_hal_pc.h"
#include "xy_hal_rtc.h"
#include "xy_hal_spi.h"
#include "xy_hal_sys.h"
#include "xy_hal_uart.h"
#include "fff.h"

DEFINE_FFF_GLOBALS;

FAKE_VOID_FUNC(gpio_irq_handler, void *)

void setUp(void)
{
    RESET_FAKE(gpio_irq_handler);
    FFF_RESET_HISTORY();
}

void tearDown(void)
{
}

static void test_hal_error_contract(void)
{
    TEST_ASSERT_EQUAL(0, XY_HAL_OK);
    TEST_ASSERT_LESS_THAN_INT(0, XY_HAL_ERROR);
    TEST_ASSERT_LESS_THAN_INT(0, XY_HAL_ERROR_INVALID_PARAM);
    TEST_ASSERT_LESS_THAN_INT(0, XY_HAL_ERROR_TIMEOUT);
    TEST_ASSERT_LESS_THAN_INT(0, XY_HAL_ERROR_BUSY);
    TEST_ASSERT_LESS_THAN_INT(0, XY_HAL_ERROR_NOT_SUPPORTED);
    TEST_ASSERT_LESS_THAN_INT(0, XY_HAL_ERROR_NO_MEMORY);
    TEST_ASSERT_LESS_THAN_INT(0, XY_HAL_ERROR_NOT_INIT);
    TEST_ASSERT_LESS_THAN_INT(0, XY_HAL_ERROR_ALREADY_INIT);
}

static void test_hal_sys_pc_clock_and_tick(void)
{
    xy_hal_sys_clock_info_t info;
    uint32_t tick0;
    uint32_t tick1;

    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_sys_get_clock_info(NULL));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_sys_get_clock_info(&info));
    TEST_ASSERT_EQUAL_UINT32(100000000u, info.sysclk);
    TEST_ASSERT_EQUAL_UINT32(100000000u, info.hclk);
    TEST_ASSERT_EQUAL_UINT32(50000000u, info.pclk1);
    TEST_ASSERT_EQUAL_UINT32(50000000u, info.pclk2);
    TEST_ASSERT_EQUAL_UINT32(50000000u, info.pclk3);
    TEST_ASSERT_EQUAL(1, info.hsi_ready);
    TEST_ASSERT_EQUAL(0, info.hse_ready);
    TEST_ASSERT_EQUAL(1, info.pll_ready);
    TEST_ASSERT_EQUAL_UINT32(XY_HAL_SYS_TICK_FREQ, xy_hal_sys_get_tick_freq());

    tick0 = xy_hal_sys_get_tick_count();
    xy_hal_sys_tick_irq_handler();
    tick1 = xy_hal_sys_get_tick_count();
    TEST_ASSERT_EQUAL_UINT32(11u, tick1 - tick0);
}

static void test_gpio_pc_config_state_and_batch(void)
{
    struct xy_hal_gpio_port port_a = { .port_id = 0 };
    xy_hal_gpio_port_t port = &port_a;
    xy_hal_gpio_config_t config = {
        .mode = XY_HAL_GPIO_MODE_OUTPUT,
        .pull = XY_HAL_GPIO_PULL_UP,
        .otype = XY_HAL_GPIO_OTYPE_OD,
        .speed = XY_HAL_GPIO_SPEED_HIGH,
        .alternate = 3,
    };

    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_gpio_init(port, 16, &config));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_gpio_init(port, 1, NULL));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_gpio_init(port, 1, &config));

    TEST_ASSERT_EQUAL(XY_HAL_GPIO_MODE_OUTPUT, xy_hal_gpio_get_mode(port, 1));
    TEST_ASSERT_EQUAL(XY_HAL_GPIO_PULL_UP, xy_hal_gpio_get_pull(port, 1));
    TEST_ASSERT_EQUAL(XY_HAL_GPIO_OTYPE_OD, xy_hal_gpio_get_otype(port, 1));
    TEST_ASSERT_EQUAL(XY_HAL_GPIO_SPEED_HIGH, xy_hal_gpio_get_speed(port, 1));
    TEST_ASSERT_EQUAL(3, xy_hal_gpio_get_alternate(port, 1));

    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_gpio_set_mode(port, 1, XY_HAL_GPIO_MODE_INPUT));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_gpio_set_pull(port, 1, XY_HAL_GPIO_PULL_DOWN));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_gpio_set_otype(port, 1, XY_HAL_GPIO_OTYPE_PP));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_gpio_set_speed(port, 1, XY_HAL_GPIO_SPEED_VERY_HIGH));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_gpio_set_alternate(port, 1, 7));
    TEST_ASSERT_EQUAL(XY_HAL_GPIO_MODE_INPUT, xy_hal_gpio_get_mode(port, 1));
    TEST_ASSERT_EQUAL(XY_HAL_GPIO_PULL_DOWN, xy_hal_gpio_get_pull(port, 1));
    TEST_ASSERT_EQUAL(XY_HAL_GPIO_OTYPE_PP, xy_hal_gpio_get_otype(port, 1));
    TEST_ASSERT_EQUAL(XY_HAL_GPIO_SPEED_VERY_HIGH, xy_hal_gpio_get_speed(port, 1));
    TEST_ASSERT_EQUAL(7, xy_hal_gpio_get_alternate(port, 1));

    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_gpio_write(port, 1, 1));
    TEST_ASSERT_EQUAL(1, xy_hal_gpio_read(port, 1));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_gpio_toggle(port, 1));
    TEST_ASSERT_EQUAL(0, xy_hal_gpio_read(port, 1));

    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_gpio_write_batch(port, 0x000fu, 0x000au));
    TEST_ASSERT_EQUAL(0x000au, xy_hal_gpio_read_batch(port, 0x000fu));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_gpio_write(port, 16, 1));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_gpio_read(port, 16));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_gpio_write_batch(port, 0, 0));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_gpio_read_batch(port, 0));

    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_gpio_deinit(port, 1));
    TEST_ASSERT_EQUAL(XY_HAL_GPIO_MODE_INPUT, xy_hal_gpio_get_mode(port, 1));
}

static void test_gpio_pc_irq_and_extended_helpers(void)
{
    struct xy_hal_gpio_port port_a = { .port_id = 0 };
    xy_hal_gpio_port_t port = &port_a;
    xy_hal_gpio_config_t config = { .mode = XY_HAL_GPIO_MODE_OUTPUT };
    int arg = 77;

    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_gpio_init(port, 2, &config));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM,
                      xy_hal_gpio_attach_irq(port, 2, XY_HAL_GPIO_IRQ_RISING, NULL, &arg));
    TEST_ASSERT_EQUAL(XY_HAL_OK,
                      xy_hal_gpio_attach_irq(port, 2, XY_HAL_GPIO_IRQ_RISING, gpio_irq_handler, &arg));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_gpio_irq_enable(port, 2));
    TEST_ASSERT_EQUAL(0, xy_hal_gpio_get_irq_status(port, 2));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_gpio_clear_irq_status(port, 2));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_gpio_irq_disable(port, 2));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_gpio_detach_irq(port, 2));

    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_gpio_control(port, 2, 0, NULL));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_gpio_set_sleep_state(port, 2, 1));
    TEST_ASSERT_EQUAL(1, xy_hal_gpio_get_sleep_state(port, 2));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_gpio_get_driver_info(port, NULL));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_gpio_pinmux_config(port, 2, NULL));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_gpio_set_drive_strength(port, 2, 3));
    TEST_ASSERT_EQUAL(3, xy_hal_gpio_get_drive_strength(port, 2));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_gpio_set_slew_rate(port, 2, 2));
    TEST_ASSERT_EQUAL(2, xy_hal_gpio_get_slew_rate(port, 2));
    TEST_ASSERT_EQUAL_UINT(0U, gpio_irq_handler_fake.call_count);
}

static void test_uart_i2c_spi_pc_smoke(void)
{
    uint8_t tx[4] = {1, 2, 3, 4};
    uint8_t rx[4] = {0xff, 0xff, 0xff, 0xff};
    xy_hal_uart_config_t uart_cfg = {
        .baudrate = 115200U,
        .wordlen = XY_HAL_UART_WORDLEN_8B,
        .stopbits = XY_HAL_UART_STOPBITS_1,
        .parity = XY_HAL_UART_PARITY_NONE,
        .flowctrl = XY_HAL_UART_FLOWCTRL_NONE,
        .mode = XY_HAL_UART_MODE_TX_RX,
    };
    xy_hal_i2c_config_t i2c_cfg = {
        .clock_speed = 100000U,
        .addr_mode = XY_HAL_I2C_ADDR_7BIT,
        .duty_cycle = XY_HAL_I2C_DUTY_2,
        .own_address = 0U,
        .general_call_mode = 0U,
    };
    xy_hal_spi_config_t spi_cfg = {
        .mode = XY_HAL_SPI_MODE_0,
        .direction = XY_HAL_SPI_DIR_2LINES,
        .datasize = XY_HAL_SPI_DATASIZE_8BIT,
        .firstbit = XY_HAL_SPI_FIRSTBIT_MSB,
        .nss = XY_HAL_SPI_NSS_SOFT,
        .baudrate_prescaler = 2U,
        .is_master = 1U,
    };

    /* A transfer before init used to "work"; it is the one answer a board would never give. */
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_INIT, xy_hal_uart_send((void *)1, tx, sizeof(tx), 10));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_INIT,
                      xy_hal_i2c_master_transmit((void *)1, 0x50, tx, sizeof(tx), 10));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_INIT,
                      xy_hal_spi_transmit((void *)1, tx, sizeof(tx), 10));

    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_uart_init((void *)1, &uart_cfg));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_i2c_init((void *)1, &i2c_cfg));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_spi_init((void *)1, &spi_cfg));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_ALREADY_INIT, xy_hal_uart_init((void *)1, &uart_cfg));

    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_uart_send((void *)1, tx, sizeof(tx), 10));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_uart_recv((void *)1, rx, sizeof(rx), 10));
    TEST_ASSERT_EQUAL_UINT8(0, rx[0]);
    TEST_ASSERT_EQUAL_UINT8(0, rx[3]);

    memset(rx, 0xff, sizeof(rx));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_i2c_master_transmit((void *)1, 0x50, tx, sizeof(tx), 10));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_i2c_master_receive((void *)1, 0x50, rx, sizeof(rx), 10));
    TEST_ASSERT_EQUAL_UINT8(0, rx[0]);
    TEST_ASSERT_EQUAL_UINT8(0, rx[3]);

    memset(rx, 0xff, sizeof(rx));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_spi_transmit((void *)1, tx, sizeof(tx), 10));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_spi_receive((void *)1, rx, sizeof(rx), 10));
    TEST_ASSERT_EQUAL_UINT8(0, rx[0]);
    TEST_ASSERT_EQUAL_UINT8(0, rx[3]);
    memset(rx, 0xff, sizeof(rx));
    TEST_ASSERT_EQUAL(XY_HAL_OK,
                      xy_hal_spi_transmit_receive((void *)1, tx, rx, sizeof(rx), 10));
    TEST_ASSERT_EQUAL_UINT8(0, rx[0]);
    TEST_ASSERT_EQUAL_UINT8(0, rx[3]);

    /* Deinit puts the port back where it started, and the transfer says so. */
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_uart_deinit((void *)1));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_INIT, xy_hal_uart_send((void *)1, tx, sizeof(tx), 10));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_INIT, xy_hal_uart_deinit((void *)1));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_i2c_deinit((void *)1));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_spi_deinit((void *)1));
}

static void test_uart_i2c_spi_pc_refuses_bad_input(void)
{
    uint8_t tx[4] = {1, 2, 3, 4};
    uint8_t rx[4] = {0};
    xy_hal_uart_config_t uart_cfg = {
        .baudrate = 115200U,
        .wordlen = XY_HAL_UART_WORDLEN_8B,
        .stopbits = XY_HAL_UART_STOPBITS_1,
        .parity = XY_HAL_UART_PARITY_NONE,
        .flowctrl = XY_HAL_UART_FLOWCTRL_NONE,
        .mode = XY_HAL_UART_MODE_TX_RX,
    };
    xy_hal_i2c_config_t i2c_cfg = {
        .clock_speed = 100000U,
        .addr_mode = XY_HAL_I2C_ADDR_7BIT,
        .duty_cycle = XY_HAL_I2C_DUTY_2,
        .own_address = 0U,
        .general_call_mode = 0U,
    };
    xy_hal_spi_config_t spi_cfg = {
        .mode = XY_HAL_SPI_MODE_0,
        .direction = XY_HAL_SPI_DIR_2LINES,
        .datasize = XY_HAL_SPI_DATASIZE_8BIT,
        .firstbit = XY_HAL_SPI_FIRSTBIT_MSB,
        .nss = XY_HAL_SPI_NSS_SOFT,
        .baudrate_prescaler = 2U,
        .is_master = 1U,
    };
    xy_hal_uart_config_t bad_uart = uart_cfg;
    xy_hal_spi_config_t wide_spi = spi_cfg;

    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_uart_init((void *)2, &uart_cfg));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_i2c_init((void *)2, &i2c_cfg));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_spi_init((void *)2, &spi_cfg));

    /* Zero length is not a transfer. */
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_uart_send((void *)2, tx, 0U, 10));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_uart_recv((void *)2, rx, 0U, 10));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM,
                      xy_hal_i2c_master_transmit((void *)2, 0x50, tx, 0U, 10));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_spi_transmit((void *)2, tx, 0U, 10));

    /* I2C addresses are 7-bit and unshifted. */
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM,
                      xy_hal_i2c_master_transmit((void *)2, 0xA0U, tx, sizeof(tx), 10));

    /* A null buffer with a real length is a caller bug, not a no-op. */
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM,
                      xy_hal_uart_send((void *)2, NULL, sizeof(tx), 10));

    /* 10-bit addressing is declared and implemented nowhere; the host backend must agree. */
    {
        xy_hal_i2c_config_t ten_bit = i2c_cfg;
        ten_bit.addr_mode = XY_HAL_I2C_ADDR_10BIT;
        TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_SUPPORTED, xy_hal_i2c_init((void *)3, &ten_bit));
    }

    /* An enum outside its range is refused rather than defaulted to something else. */
    bad_uart.parity = (xy_hal_uart_parity_t)0x7F;
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_uart_init((void *)4, &bad_uart));

    /* On a 16-bit bus `len` is bytes and the count must be whole frames. */
    wide_spi.datasize = XY_HAL_SPI_DATASIZE_16BIT;
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_spi_init((void *)5, &wide_spi));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_spi_transmit((void *)5, tx, 3U, 10));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_spi_transmit((void *)5, tx, 4U, 10));

    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_uart_deinit((void *)2));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_i2c_deinit((void *)2));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_spi_deinit((void *)2));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_spi_deinit((void *)5));
}

static void test_rtc_backup_register_pc_contract(void)
{
    uint32_t value = 0xAAAAAAAAU;

    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_rtc_backup_read(0U, NULL));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM,
                      xy_hal_rtc_backup_read(XY_HAL_RTC_BACKUP_REGISTER_COUNT, &value));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM,
                      xy_hal_rtc_backup_write(XY_HAL_RTC_BACKUP_REGISTER_COUNT, value));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_rtc_backup_write(0U, 0x12345678U));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_rtc_backup_read(0U, &value));
    TEST_ASSERT_EQUAL_HEX32(0x12345678U, value);
}

static void test_sys_irq_control_pc_contract(void)
{
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM,
                      xy_hal_sys_set_irq_priority(UINT32_MAX, 5U));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM,
                      xy_hal_sys_set_irq_priority(1U, UINT32_MAX));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_sys_set_irq_priority(1U, 5U));
    TEST_ASSERT_EQUAL_INT(5, xy_hal_sys_get_irq_priority(1U));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_sys_enable_irq_num(1U));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_sys_disable_irq_num(1U));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_sys_enable_irq_num(UINT32_MAX));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_sys_disable_irq_num(UINT32_MAX));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_hal_error_contract);
    RUN_TEST(test_hal_sys_pc_clock_and_tick);
    RUN_TEST(test_gpio_pc_config_state_and_batch);
    RUN_TEST(test_gpio_pc_irq_and_extended_helpers);
    RUN_TEST(test_uart_i2c_spi_pc_smoke);
    RUN_TEST(test_uart_i2c_spi_pc_refuses_bad_input);
    RUN_TEST(test_rtc_backup_register_pc_contract);
    RUN_TEST(test_sys_irq_control_pc_contract);
    return UNITY_END();
}
