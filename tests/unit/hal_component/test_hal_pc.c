#include "unity.h"

#include <stdint.h>
#include <string.h>

#include "xy_hal_error.h"
#include "xy_hal_gpio.h"
#include "xy_hal_i2c.h"
#include "xy_hal_pc.h"
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
    TEST_ASSERT_TRUE(XY_HAL_ERROR < 0);
    TEST_ASSERT_TRUE(XY_HAL_ERROR_INVALID_PARAM < 0);
    TEST_ASSERT_TRUE(XY_HAL_ERROR_TIMEOUT < 0);
    TEST_ASSERT_TRUE(XY_HAL_ERROR_BUSY < 0);
    TEST_ASSERT_TRUE(XY_HAL_ERROR_NOT_SUPPORTED < 0);
    TEST_ASSERT_TRUE(XY_HAL_ERROR_NO_MEMORY < 0);
    TEST_ASSERT_TRUE(XY_HAL_ERROR_NOT_INIT < 0);
    TEST_ASSERT_TRUE(XY_HAL_ERROR_ALREADY_INIT < 0);
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
    tick1 = xy_hal_sys_get_tick_count();
    TEST_ASSERT_EQUAL_UINT32(10u, tick1 - tick0);
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
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_spi_transmit_receive((void *)1, tx, rx, sizeof(rx), 10));
    TEST_ASSERT_EQUAL_UINT8(0, rx[0]);
    TEST_ASSERT_EQUAL_UINT8(0, rx[3]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_hal_error_contract);
    RUN_TEST(test_hal_sys_pc_clock_and_tick);
    RUN_TEST(test_gpio_pc_config_state_and_batch);
    RUN_TEST(test_gpio_pc_irq_and_extended_helpers);
    RUN_TEST(test_uart_i2c_spi_pc_smoke);
    return UNITY_END();
}
