#include "unity.h"

#include <stdint.h>
#include <string.h>

#include "xy_hal_error.h"
#include "xy_hal_adc.h"
#include "xy_hal_dac.h"
#include "xy_hal_dma.h"
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

static int g_dma_complete_events;
static xy_hal_dma_event_t g_dma_last_event;

static void dma_counting_callback(void *dma, xy_hal_dma_event_t event, void *arg)
{
    int *counter = (int *)arg;
    (void)dma;
    g_dma_last_event = event;
    if (counter != NULL) {
        (*counter)++;
    }
    g_dma_complete_events++;
}

static void test_dma_pc_state_and_events(void)
{
    static const xy_hal_dma_config_t cfg = {
        .direction = XY_HAL_DMA_DIR_MEM_TO_PERIPH,
        .mode = XY_HAL_DMA_MODE_NORMAL,
        .priority = XY_HAL_DMA_PRIORITY_MEDIUM,
        .periph_width = XY_HAL_DMA_WIDTH_BYTE,
        .mem_width = XY_HAL_DMA_WIDTH_BYTE,
        .periph_incr = XY_HAL_DMA_INCR_DISABLE,
        .mem_incr = XY_HAL_DMA_INCR_ENABLE,
    };
    static const xy_hal_dma_config_t circular = {
        .direction = XY_HAL_DMA_DIR_MEM_TO_PERIPH,
        .mode = XY_HAL_DMA_MODE_CIRCULAR,
        .priority = XY_HAL_DMA_PRIORITY_LOW,
        .periph_width = XY_HAL_DMA_WIDTH_BYTE,
        .mem_width = XY_HAL_DMA_WIDTH_BYTE,
        .periph_incr = XY_HAL_DMA_INCR_DISABLE,
        .mem_incr = XY_HAL_DMA_INCR_ENABLE,
    };
    int count = 0;

    g_dma_complete_events = 0;
    g_dma_last_event = XY_HAL_DMA_EVENT_ERROR;

    /* Nothing works before init - including the counter, whose register would hold anything. */
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_INIT, xy_hal_dma_start((void *)10, 0x1000U, 0x2000U, 4U));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_INIT, xy_hal_dma_stop((void *)10));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_INIT, xy_hal_dma_poll_complete((void *)10, 10U));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_INIT, xy_hal_dma_get_counter((void *)10));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_INIT, xy_hal_dma_deinit((void *)10));

    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_dma_init((void *)10, &cfg));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_ALREADY_INIT, xy_hal_dma_init((void *)10, &cfg));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_dma_register_callback(
                      (void *)10, (xy_hal_dma_event_t)7, dma_counting_callback, &count));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_dma_register_callback(
                      (void *)10, XY_HAL_DMA_EVENT_COMPLETE, dma_counting_callback, &count));

    /* Zero items is not a transfer; one past the counter width would be truncated by hardware. */
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_dma_start((void *)10, 0x1000U, 0x2000U, 0U));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM,
                      xy_hal_dma_start((void *)10, 0x1000U, 0x2000U, 0x10000U));

    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_dma_start((void *)10, 0x1000U, 0x2000U, 16U));
    /* The transfer is done, but the callback belongs to the interrupt handler: nothing arrives
     * until the vector runs, which is what the header documents. */
    TEST_ASSERT_EQUAL_INT(0, count);
    xy_hal_dma_irq_handler((void *)10);
    TEST_ASSERT_EQUAL_INT(1, count);
    xy_hal_dma_irq_handler((void *)10);
    TEST_ASSERT_EQUAL_INT(1, count);
    TEST_ASSERT_EQUAL(XY_HAL_DMA_EVENT_COMPLETE, g_dma_last_event);
    TEST_ASSERT_EQUAL_INT(0, xy_hal_dma_get_counter((void *)10));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_dma_poll_complete((void *)10, 0U));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_dma_stop((void *)10));

    /* Circular mode keeps the counter armed; the callback still fires once per start. */
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_dma_init((void *)11, &circular));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_dma_start((void *)11, 0x1000U, 0x2000U, 8U));
    TEST_ASSERT_EQUAL_INT(8, xy_hal_dma_get_counter((void *)11));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_dma_deinit((void *)11));

    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_dma_deinit((void *)10));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_INIT, xy_hal_dma_get_counter((void *)10));
}

static int g_timer_update_events;
static int g_timer_cc1_events;

static void timer_update_callback(void *timer, xy_hal_timer_event_t event, void *arg)
{
    (void)timer; (void)event; (void)arg;
    g_timer_update_events++;
}

static void timer_cc1_callback(void *timer, xy_hal_timer_event_t event, void *arg)
{
    (void)timer; (void)event; (void)arg;
    g_timer_cc1_events++;
}

static void test_timer_pc_state_and_events(void)
{
    static const xy_hal_timer_config_t cfg = {
        .prescaler = 99U,
        .period = 999U,
        .mode = XY_HAL_TIMER_COUNT_UP,
        .clock_div = XY_HAL_TIMER_CKDIV_1,
        .auto_reload_preload = 1U,
    };
    xy_hal_timer_config_t bad = cfg;

    g_timer_update_events = 0;
    g_timer_cc1_events = 0;

    /* Nothing works before init. */
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_INIT, xy_hal_timer_start((void *)20));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_INIT, xy_hal_timer_stop((void *)20));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_INIT, xy_hal_timer_get_counter((void *)20));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_INIT, xy_hal_timer_set_counter((void *)20, 1U));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_INIT, xy_hal_timer_set_period((void *)20, 1U));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_INIT, xy_hal_timer_enable_irq(
                      (void *)20, XY_HAL_TIMER_EVENT_UPDATE));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_INIT, xy_hal_timer_deinit((void *)20));

    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_timer_init((void *)20, &cfg));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_ALREADY_INIT, xy_hal_timer_init((void *)20, &cfg));

    /* The counter reads back what was written, and arming an interrupt does not move it. */
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_timer_set_counter((void *)20, 42U));
    TEST_ASSERT_EQUAL_INT(42, xy_hal_timer_get_counter((void *)20));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_timer_enable_irq((void *)20, XY_HAL_TIMER_EVENT_CC1));
    TEST_ASSERT_EQUAL_INT(42, xy_hal_timer_get_counter((void *)20));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_timer_start((void *)20));
    TEST_ASSERT_EQUAL_INT(42, xy_hal_timer_get_counter((void *)20));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_timer_stop((void *)20));

    /* Two events registered, one armed: only the armed one is delivered. */
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_timer_register_callback(
                      (void *)20, XY_HAL_TIMER_EVENT_UPDATE, timer_update_callback, NULL));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_timer_register_callback(
                      (void *)20, XY_HAL_TIMER_EVENT_CC1, timer_cc1_callback, NULL));
    xy_hal_timer_irq_handler((void *)20);
    TEST_ASSERT_EQUAL_INT(0, g_timer_update_events);
    TEST_ASSERT_EQUAL_INT(1, g_timer_cc1_events);

    /* Disarming stops delivery without touching the counter. */
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_timer_disable_irq((void *)20, XY_HAL_TIMER_EVENT_CC1));
    xy_hal_timer_irq_handler((void *)20);
    TEST_ASSERT_EQUAL_INT(1, g_timer_cc1_events);
    TEST_ASSERT_EQUAL_INT(42, xy_hal_timer_get_counter((void *)20));

    /* An event outside the enum is refused rather than treated as event 0. */
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_timer_enable_irq(
                      (void *)20, (xy_hal_timer_event_t)9));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_timer_register_callback(
                      (void *)20, (xy_hal_timer_event_t)9, timer_update_callback, NULL));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_timer_set_period((void *)20, 0U));

    bad.clock_div = (xy_hal_timer_ckdiv_t)7;
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_timer_init((void *)21, &bad));

    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_timer_deinit((void *)20));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_INIT, xy_hal_timer_get_counter((void *)20));
}

static int g_rtc_alarm_events;

static void rtc_alarm_callback(xy_hal_rtc_event_t event, void *arg)
{
    (void)event; (void)arg;
    g_rtc_alarm_events++;
}

/* Delivered by xy_hal_irq_pc.c; no public header declares it, so the test names it directly. */
extern void xy_hal_rtc_alarm_irq_handler(void *rtc, char alarm_id);
extern void xy_hal_rtc_pc_tick(void *rtc);

static void test_rtc_pc_state_and_alarms(void)
{
    xy_hal_rtc_time_t time = { .hours = 12U, .minutes = 34U, .seconds = 56U, .subseconds = 0U };
    xy_hal_rtc_time_t read_back = { 0 };
    xy_hal_rtc_date_t date = { .weekday = 1U, .month = 10U, .date = 27U, .year = 2026U };
    xy_hal_rtc_date_t date_back = { 0 };
    xy_hal_rtc_alarm_t alarm_a = { 0 };
    xy_hal_rtc_alarm_t alarm_b = { 0 };
    xy_hal_rtc_alarm_t alarm_read = { 0 };
    xy_hal_rtc_time_t bad_time = { .hours = 24U, .minutes = 0U, .seconds = 0U, .subseconds = 0U };

    g_rtc_alarm_events = 0;

    /* Nothing works before init, and the timestamp is not an exception: it returns an error code. */
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_INIT,
                      xy_hal_rtc_get_time((void *)30, &read_back, XY_HAL_RTC_FORMAT_BIN));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_INIT, xy_hal_rtc_set_time(
                      (void *)30, &time, XY_HAL_RTC_FORMAT_BIN));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_INIT, xy_hal_rtc_enable_alarm((void *)30, 'A'));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_INIT, xy_hal_rtc_deinit((void *)30));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_INIT, xy_hal_rtc_get_timestamp((void *)30));

    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_rtc_init((void *)30));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_ALREADY_INIT, xy_hal_rtc_init((void *)30));

    /* What was written is what is read. */
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_rtc_set_time((void *)30, &time, XY_HAL_RTC_FORMAT_BIN));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_rtc_get_time((void *)30, &read_back, XY_HAL_RTC_FORMAT_BIN));
    TEST_ASSERT_EQUAL_UINT8(12U, read_back.hours);
    TEST_ASSERT_EQUAL_UINT8(56U, read_back.seconds);
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_rtc_set_date((void *)30, &date, XY_HAL_RTC_FORMAT_BIN));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_rtc_get_date((void *)30, &date_back, XY_HAL_RTC_FORMAT_BIN));
    TEST_ASSERT_EQUAL_UINT8(10U, date_back.month);
    TEST_ASSERT_EQUAL_UINT16(2026U, date_back.year);

    /* BCD is not produced by a backend that cannot produce it, and a bad field is not stored. */
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_SUPPORTED,
                      xy_hal_rtc_get_time((void *)30, &read_back, XY_HAL_RTC_FORMAT_BCD));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM,
                      xy_hal_rtc_set_time((void *)30, &bad_time, XY_HAL_RTC_FORMAT_BIN));

    /* Alarm A and alarm B are different slots: this is the defect worth not repeating. */
    alarm_a.time.hours = 7U;
    alarm_b.time.hours = 19U;
    alarm_a.alarm_mask = XY_HAL_RTC_ALARM_MASK_ALL;
    alarm_b.alarm_mask = XY_HAL_RTC_ALARM_MASK_HOURS;
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_INIT, xy_hal_rtc_enable_alarm((void *)30, 'A'));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_rtc_set_alarm((void *)30, &alarm_a, 'A'));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_rtc_set_alarm((void *)30, &alarm_b, 'B'));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_rtc_get_alarm((void *)30, &alarm_read, 'A'));
    TEST_ASSERT_EQUAL_UINT8(7U, alarm_read.time.hours);
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_rtc_get_alarm((void *)30, &alarm_read, 'B'));
    TEST_ASSERT_EQUAL_UINT8(19U, alarm_read.time.hours);
    /* An initialised RTC with nothing written to an alarm answers NOT_FOUND, not the other slot. */
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_rtc_init((void *)31));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_FOUND, xy_hal_rtc_get_alarm((void *)31, &alarm_read, 'A'));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_rtc_deinit((void *)31));

    /* An id that is not an alarm is refused, not folded into whichever alarm comes first. */
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_rtc_set_alarm((void *)30, &alarm_a, 'C'));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_rtc_get_alarm((void *)30, &alarm_read, 'C'));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_rtc_enable_alarm((void *)30, 'C'));

    /* A registered callback is reached by the interrupt entry, and only for an enabled alarm. */
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_rtc_register_callback(
                      (void *)30, XY_HAL_RTC_EVENT_ALARM_A, rtc_alarm_callback, NULL));
    xy_hal_rtc_alarm_irq_handler((void *)30, 'A');
    TEST_ASSERT_EQUAL_INT(0, g_rtc_alarm_events);
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_rtc_enable_alarm((void *)30, 'A'));
    xy_hal_rtc_alarm_irq_handler((void *)30, 'A');
    TEST_ASSERT_EQUAL_INT(1, g_rtc_alarm_events);
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_rtc_disable_alarm((void *)30, 'A'));
    xy_hal_rtc_alarm_irq_handler((void *)30, 'A');
    TEST_ASSERT_EQUAL_INT(1, g_rtc_alarm_events);

    /* The counter is seconds since the Unix epoch, the same answer the board backends give. */
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_rtc_set_timestamp((void *)30, 3661));
    TEST_ASSERT_EQUAL_INT64(3661, xy_hal_rtc_get_timestamp((void *)30));
    xy_hal_rtc_pc_tick((void *)30);
    TEST_ASSERT_EQUAL_INT64(3662, xy_hal_rtc_get_timestamp((void *)30));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_rtc_set_timestamp((void *)30, -1));

    /* A timestamp past midnight moves the date with it - the day is no longer just "not tracked". */
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_rtc_set_timestamp((void *)30, 86400));
    TEST_ASSERT_EQUAL_INT64(86400, xy_hal_rtc_get_timestamp((void *)30));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_rtc_get_date((void *)30, &date_back, XY_HAL_RTC_FORMAT_BIN));
    TEST_ASSERT_EQUAL_UINT8(2U, date_back.date);
    TEST_ASSERT_EQUAL_UINT8(1U, date_back.month);
    TEST_ASSERT_EQUAL_UINT16(1970U, date_back.year);
    /* 1970-01-02 was a Friday. */
    TEST_ASSERT_EQUAL_UINT8(5U, date_back.weekday);
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_rtc_set_timestamp((void *)30, 1793059200));
    TEST_ASSERT_EQUAL_INT64(1793059200, xy_hal_rtc_get_timestamp((void *)30));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_rtc_get_date((void *)30, &date_back, XY_HAL_RTC_FORMAT_BIN));
    TEST_ASSERT_EQUAL_UINT16(2026U, date_back.year);
    TEST_ASSERT_EQUAL_UINT8(10U, date_back.month);
    TEST_ASSERT_EQUAL_UINT8(27U, date_back.date);

    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_rtc_deinit((void *)30));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_INIT, xy_hal_rtc_get_timestamp((void *)30));
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

/*
 * The format argument, as this backend actually behaves. I first read the four functions, saw the
 * format validated and the value copied out unchanged, and concluded that BCD was ignored. It is not:
 * pc_rtc_format_ok refuses anything but binary with NOT_SUPPORTED, which is what the header asks of a
 * backend that cannot produce BCD. What was missing was a test - nothing pinned either the refusal or
 * the fact that a refused call leaves the RTC alone.
 */
static void test_rtc_pc_refuses_a_format_it_cannot_produce(void)
{
    xy_hal_rtc_time_t time = { .hours = 12U, .minutes = 34U, .seconds = 56U, .subseconds = 0U };
    xy_hal_rtc_date_t date = { .weekday = 2U, .month = 10U, .date = 27U, .year = 2026U };
    xy_hal_rtc_time_t read_back = { 0 };
    xy_hal_rtc_date_t date_back = { 0 };

    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_rtc_init((void *)31));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_rtc_set_time((void *)31, &time, XY_HAL_RTC_FORMAT_BIN));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_rtc_set_date((void *)31, &date, XY_HAL_RTC_FORMAT_BIN));

    /* All four entries refuse BCD rather than hand back binary digits the caller reads as BCD. */
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_SUPPORTED,
                      xy_hal_rtc_get_time((void *)31, &read_back, XY_HAL_RTC_FORMAT_BCD));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_SUPPORTED,
                      xy_hal_rtc_set_time((void *)31, &time, XY_HAL_RTC_FORMAT_BCD));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_SUPPORTED,
                      xy_hal_rtc_get_date((void *)31, &date_back, XY_HAL_RTC_FORMAT_BCD));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_SUPPORTED,
                      xy_hal_rtc_set_date((void *)31, &date, XY_HAL_RTC_FORMAT_BCD));

    /* A refused call leaves the RTC exactly as it was - a refusal that still wrote something would be
     * a refusal in the return code only. */
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_rtc_get_time((void *)31, &read_back, XY_HAL_RTC_FORMAT_BIN));
    TEST_ASSERT_EQUAL_UINT8(12U, read_back.hours);
    TEST_ASSERT_EQUAL_UINT8(34U, read_back.minutes);
    TEST_ASSERT_EQUAL_UINT8(56U, read_back.seconds);
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_rtc_get_date((void *)31, &date_back, XY_HAL_RTC_FORMAT_BIN));
    TEST_ASSERT_EQUAL_UINT16(2026U, date_back.year);
    TEST_ASSERT_EQUAL_UINT8(10U, date_back.month);
    TEST_ASSERT_EQUAL_UINT8(27U, date_back.date);

    /* The timestamp does not depend on the format the caller last asked for, because it never accepts
     * one - it reads binary through this backend. Midnight on 2026-10-27 plus the 12:34:56 set above. */
    TEST_ASSERT_EQUAL_INT64(1793059200 + 12 * 3600 + 34 * 60 + 56,
                            xy_hal_rtc_get_timestamp((void *)31));
    (void)xy_hal_rtc_deinit((void *)31);
}

static int g_adc_event_count;
static xy_hal_adc_evt_t g_adc_last_event;

static void adc_event_callback(void *adc, xy_hal_adc_evt_t event, void *arg)
{
    (void)adc;
    (void)arg;
    g_adc_event_count++;
    g_adc_last_event = event;
}

/*
 * The ADC contract a host can actually check: the full scale follows the configured resolution, the
 * raw value follows the voltage a test applied, a second read without a new conversion reports busy,
 * a rejected channel batch leaves the previous sampling times alone, and the window watchdog is only
 * raised for the watched channel outside its thresholds.
 */
static void test_adc_pc_conversion_and_channel_batch(void)
{
    static const xy_hal_adc_config_t config = {
        .resolution = XY_HAL_ADC_RESOLUTION_12B,
        .align = XY_HAL_ADC_DATAALIGN_RIGHT,
        .scan_mode = XY_HAL_ADC_SCAN_DISABLE,
        .continuous = XY_HAL_ADC_CONTINUOUS_DISABLE,
        .trigger_src = XY_HAL_ADC_TRIGGER_SOFTWARE,
        .clock_div = 4U,
        .sampling_time = 3U,
    };
    static const xy_hal_adc_channel_config_t accepted[2] = {
        { .channel = 1U, .rank = 1U, .sampling_time = 7U, .enabled = 1U },
        { .channel = 2U, .rank = 2U, .sampling_time = 9U, .enabled = 1U },
    };
    /* The second entry is refused, so *neither* may be applied. */
    static const xy_hal_adc_channel_config_t rejected[2] = {
        { .channel = 3U, .rank = 1U, .sampling_time = 11U, .enabled = 1U },
        { .channel = 200U, .rank = 2U, .sampling_time = 13U, .enabled = 1U },
    };
    xy_hal_adc_config_t bad = config;
    uint8_t multi_channels[2] = { 0U, 1U };
    uint32_t multi_values[2] = { 0U, 0U };
    uint32_t dma_values[3] = { 0U, 0U, 0U };
    int32_t multi_ret = 0;

    g_adc_event_count = 0;

    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_adc_init(NULL, &config));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_adc_init((void *)40, NULL));
    /* A handle this backend never saw is not a silent zero. */
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_INIT, xy_hal_adc_read((void *)40, 0U, 10U));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_INIT, xy_hal_adc_get_resolution((void *)40));
    TEST_ASSERT_EQUAL_UINT32(0U, xy_hal_adc_get_max_value((void *)40));

    bad.resolution = (xy_hal_adc_resolution_t)7;
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_adc_init((void *)40, &bad));
    bad = config;
    bad.align = (xy_hal_adc_align_t)2;
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_adc_init((void *)40, &bad));
    bad = config;
    bad.trigger_src = (xy_hal_adc_trigger_src_t)9;
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_adc_init((void *)40, &bad));
    bad = config;
    bad.enable_vbat = 2U;
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_adc_init((void *)40, &bad));

    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_adc_init((void *)40, &config));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_ALREADY_INIT, xy_hal_adc_init((void *)40, &config));
    TEST_ASSERT_EQUAL(XY_HAL_OK,
                      xy_hal_adc_register_callback((void *)40, adc_event_callback, NULL));
    TEST_ASSERT_EQUAL_INT(12, xy_hal_adc_get_resolution((void *)40));
    TEST_ASSERT_EQUAL_UINT32(4095U, xy_hal_adc_get_max_value((void *)40));
    /* An unset input reads 0 V, which is what an unconnected pin converts. */
    TEST_ASSERT_EQUAL_INT(0, xy_hal_adc_read((void *)40, 0U, 10U));
    TEST_ASSERT_EQUAL(1, g_adc_event_count);
    TEST_ASSERT_EQUAL(XY_HAL_ADC_EVENT_EOC, g_adc_last_event);

    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_adc_pc_set_input((void *)40, 0U, 1650U));
    TEST_ASSERT_EQUAL_UINT32(2047U, (uint32_t)xy_hal_adc_read((void *)40, 0U, 10U));
    /* The conversion flag is consumed by the non-blocking read and not re-converted. */
    TEST_ASSERT_EQUAL_INT(2047, xy_hal_adc_read_nb((void *)40, 0U));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_BUSY, xy_hal_adc_read_nb((void *)40, 0U));
    /* Calibration aborts the pending conversion, so the flag is gone again. */
    TEST_ASSERT_EQUAL_INT(2047, xy_hal_adc_read((void *)40, 0U, 10U));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_adc_calibrate((void *)40));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_BUSY, xy_hal_adc_read_nb((void *)40, 0U));

    TEST_ASSERT_EQUAL_UINT32(1649U, xy_hal_adc_value_to_mv((void *)40, 2047U, 3300U));
    /* Full scale is the configured resolution, so the same voltage is not the same count twice. */
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_adc_pc_set_vref((void *)40, 3300U));
    TEST_ASSERT_EQUAL_UINT32(0U, xy_hal_adc_value_to_mv((void *)40, 4096U, 3300U));
    TEST_ASSERT_EQUAL_UINT32(0U, xy_hal_adc_value_to_mv((void *)40, 2047U, 0U));

    /* Sampling time starts from the config-wide value on every channel. */
    TEST_ASSERT_EQUAL_INT(3, xy_hal_adc_get_sampling_time((void *)40, 1U));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM,
                      xy_hal_adc_config_channels((void *)40, rejected, 2U));
    TEST_ASSERT_EQUAL_INT(3, xy_hal_adc_get_sampling_time((void *)40, 3U));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_adc_config_channels((void *)40, accepted, 2U));
    TEST_ASSERT_EQUAL_INT(7, xy_hal_adc_get_sampling_time((void *)40, 1U));
    TEST_ASSERT_EQUAL_INT(9, xy_hal_adc_get_sampling_time((void *)40, 2U));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM,
                      xy_hal_adc_config_channels((void *)40, NULL, 1U));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM,
                      xy_hal_adc_config_channels((void *)40, accepted, 0U));

    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_adc_set_trigger((void *)40, XY_HAL_ADC_TRIGGER_TIM2_CC2));
    TEST_ASSERT_EQUAL_INT(XY_HAL_ADC_TRIGGER_TIM2_CC2, xy_hal_adc_get_trigger((void *)40));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM,
                      xy_hal_adc_set_trigger((void *)40, (xy_hal_adc_trigger_src_t)9));
    TEST_ASSERT_EQUAL(XY_HAL_OK,
                      xy_hal_adc_set_continuous((void *)40, XY_HAL_ADC_CONTINUOUS_ENABLE));
    TEST_ASSERT_EQUAL_INT(XY_HAL_ADC_CONTINUOUS_ENABLE, xy_hal_adc_get_continuous((void *)40));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_adc_set_prescaler((void *)40, 6U));
    TEST_ASSERT_EQUAL_INT(6, xy_hal_adc_get_prescaler((void *)40));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_adc_set_prescaler((void *)40, 0U));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_adc_set_oversampling((void *)40, 4U));
    TEST_ASSERT_EQUAL_INT(4, xy_hal_adc_get_oversampling((void *)40));

    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_adc_set_sampling_time((void *)40, 2U, 15U));
    TEST_ASSERT_EQUAL_INT(15, xy_hal_adc_get_sampling_time((void *)40, 2U));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM,
                      xy_hal_adc_set_sampling_time((void *)40, 32U, 15U));

    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_adc_start((void *)40));
    TEST_ASSERT_EQUAL_INT(1, xy_hal_adc_get_state((void *)40));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_adc_stop((void *)40));
    TEST_ASSERT_EQUAL_INT(0, xy_hal_adc_get_state((void *)40));

    multi_ret = xy_hal_adc_read_multi((void *)40, multi_channels, multi_values, 2U, 10U);
    TEST_ASSERT_EQUAL_INT(2, multi_ret);
    TEST_ASSERT_EQUAL_UINT32(2047U, multi_values[0]);
    TEST_ASSERT_EQUAL_UINT32(0U, multi_values[1]);
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM,
                      xy_hal_adc_read_multi((void *)40, multi_channels, NULL, 2U, 10U));

    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_adc_read_dma((void *)40, dma_values, 3U));
    TEST_ASSERT_EQUAL_UINT32(2047U, dma_values[0]);
    TEST_ASSERT_EQUAL_UINT32(0U, dma_values[2]);
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_adc_read_dma((void *)40, dma_values, 0U));

    /* A second instance has its own full scale: 8-bit is 255, not the 12-bit 4095. */
    bad = config;
    bad.resolution = XY_HAL_ADC_RESOLUTION_8B;
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_adc_init((void *)41, &bad));
    TEST_ASSERT_EQUAL_UINT32(255U, xy_hal_adc_get_max_value((void *)41));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_adc_pc_set_input((void *)41, 0U, 1650U));
    TEST_ASSERT_EQUAL_UINT32(127U, (uint32_t)xy_hal_adc_read((void *)41, 0U, 10U));

    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_adc_deinit((void *)41));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_INIT, xy_hal_adc_deinit((void *)41));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_adc_deinit((void *)40));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_INIT, xy_hal_adc_read((void *)40, 0U, 10U));
}

/*
 * The window watchdog, the internal-sensor sources and event dispatch. The point of the sensor
 * sources is that they refuse when the source was never enabled: a plausible constant is what a
 * disconnected channel looks like on a real part, and it is unreadable as evidence.
 */
static void test_adc_pc_watchdog_and_internal_sources(void)
{
    static const xy_hal_adc_config_t config = {
        .resolution = XY_HAL_ADC_RESOLUTION_12B,
        .align = XY_HAL_ADC_DATAALIGN_RIGHT,
        .scan_mode = XY_HAL_ADC_SCAN_DISABLE,
        .continuous = XY_HAL_ADC_CONTINUOUS_DISABLE,
        .trigger_src = XY_HAL_ADC_TRIGGER_SOFTWARE,
        .sampling_time = 3U,
    };
    uint32_t low = 0U;
    uint32_t high = 0U;

    g_adc_event_count = 0;
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_adc_init((void *)42, &config));
    TEST_ASSERT_EQUAL(XY_HAL_OK,
                      xy_hal_adc_register_callback((void *)42, adc_event_callback, NULL));

    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_adc_set_window_threshold((void *)42, 1000U, 2000U));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_adc_get_window_threshold((void *)42, &low, &high));
    TEST_ASSERT_EQUAL_UINT32(1000U, low);
    TEST_ASSERT_EQUAL_UINT32(2000U, high);
    /* A threshold above full scale describes a window the part cannot watch. */
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM,
                      xy_hal_adc_set_window_threshold((void *)42, 1000U, 4096U));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM,
                      xy_hal_adc_set_window_threshold((void *)42, 2000U, 1000U));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM,
                      xy_hal_adc_get_window_threshold((void *)42, NULL, &high));

    /* Inside the window, on the watched channel: EOC only. */
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_adc_enable_window_watchdog((void *)42, 1U));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_adc_pc_set_input((void *)42, 1U, 1500U));
    (void)xy_hal_adc_read((void *)42, 1U, 10U);
    TEST_ASSERT_EQUAL(1, g_adc_event_count);
    TEST_ASSERT_EQUAL(XY_HAL_ADC_EVENT_EOC, g_adc_last_event);

    /* Outside it: the watchdog is what distinguishes the sample from a normal conversion. */
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_adc_pc_set_input((void *)42, 1U, 3000U));
    TEST_ASSERT_EQUAL_UINT32(3722U, (uint32_t)xy_hal_adc_read((void *)42, 1U, 10U));
    TEST_ASSERT_EQUAL(3, g_adc_event_count);
    TEST_ASSERT_EQUAL(XY_HAL_ADC_EVENT_AWD, g_adc_last_event);

    /* The same out-of-window voltage on an unwatched channel raises nothing. */
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_adc_pc_set_input((void *)42, 2U, 3000U));
    (void)xy_hal_adc_read((void *)42, 2U, 10U);
    TEST_ASSERT_EQUAL(4, g_adc_event_count);
    TEST_ASSERT_EQUAL(XY_HAL_ADC_EVENT_EOC, g_adc_last_event);

    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_adc_disable_window_watchdog((void *)42, 1U));
    (void)xy_hal_adc_read((void *)42, 1U, 10U);
    TEST_ASSERT_EQUAL(5, g_adc_event_count);
    TEST_ASSERT_EQUAL(XY_HAL_ADC_EVENT_EOC, g_adc_last_event);
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM,
                      xy_hal_adc_enable_window_watchdog((void *)42, 32U));

    /* None of the three internal sources was enabled, so none may report a reading. */
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_SUPPORTED, xy_hal_adc_get_temp_value());
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_SUPPORTED, xy_hal_adc_get_vrefint_value());
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_SUPPORTED, xy_hal_adc_get_vbat_value());

    xy_hal_adc_pc_set_temperature(3125);
    xy_hal_adc_pc_set_vrefint(1200U);
    xy_hal_adc_pc_set_vbat(3000U);
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_adc_enable_temp_sensor());
    TEST_ASSERT_EQUAL_INT(3125, xy_hal_adc_get_temp_value());
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_adc_enable_vrefint());
    TEST_ASSERT_EQUAL_INT(1489, xy_hal_adc_get_vrefint_value());
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_adc_enable_vbat());
    TEST_ASSERT_EQUAL_INT(3722, xy_hal_adc_get_vbat_value());

    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_adc_disable_temp_sensor());
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_adc_disable_vrefint());
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_adc_disable_vbat());
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_SUPPORTED, xy_hal_adc_get_temp_value());
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_SUPPORTED, xy_hal_adc_get_vrefint_value());
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_SUPPORTED, xy_hal_adc_get_vbat_value());

    /* An event the backend is handed from outside reaches the registered callback. */
    xy_hal_adc_event_handler((void *)42, XY_HAL_ADC_EVENT_OVR, NULL);
    TEST_ASSERT_EQUAL(6, g_adc_event_count);
    TEST_ASSERT_EQUAL(XY_HAL_ADC_EVENT_OVR, g_adc_last_event);
    /* ... and a handle this backend never saw delivers nothing rather than crashing. */
    xy_hal_adc_event_handler((void *)99, XY_HAL_ADC_EVENT_OVR, NULL);
    TEST_ASSERT_EQUAL(6, g_adc_event_count);

    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_SUPPORTED, xy_hal_adc_control((void *)42, 1U, NULL));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_adc_deinit((void *)42));
}

static int g_dac_ready_events;
static xy_hal_dac_channel_t g_dac_last_channel;

static void dac_ready_callback(void *dac, xy_hal_dac_channel_t channel, xy_hal_dac_event_t event,
                               void *arg)
{
    (void)dac;
    (void)arg;
    TEST_ASSERT_EQUAL(XY_HAL_DAC_EVENT_READY, event);
    g_dac_ready_events++;
    g_dac_last_channel = channel;
}

/*
 * The DAC contract: a value outside the configured resolution is refused rather than truncated, the
 * alignment must match the one the channel was configured with, a software trigger is refused on a
 * part configured for a timer trigger, and the waveform generators advance the latched value.
 */
static void test_dac_pc_output_contract(void)
{
    static const xy_hal_dac_config_t config = {
        .resolution = XY_HAL_DAC_RESOLUTION_12B,
        .align = XY_HAL_DAC_DATAALIGN_RIGHT,
        .trigger_src = XY_HAL_DAC_TRIGGER_SOFTWARE,
        .wave = XY_HAL_DAC_WAVE_NONE,
        .buffer_enable = 1U,
    };
    static const uint32_t dma_samples[3] = { 100U, 200U, 300U };
    static const uint32_t dma_bad[2] = { 100U, 5000U };
    xy_hal_dac_config_t bad = config;
    uint32_t seen = 0U;
    uint32_t distinct = 0U;
    uint32_t previous = 0xFFFFFFFFU;
    int index;

    g_dac_ready_events = 0;

    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_dac_init(NULL, &config));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_dac_init((void *)60, NULL));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_INIT,
                      xy_hal_dac_get_value((void *)60, XY_HAL_DAC_CHANNEL_1));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_INIT,
                      xy_hal_dac_set_value((void *)60, XY_HAL_DAC_CHANNEL_1, 1U,
                                           XY_HAL_DAC_DATAALIGN_RIGHT));

    bad.resolution = (xy_hal_dac_resolution_t)2;
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_dac_init((void *)60, &bad));
    bad = config;
    bad.align = (xy_hal_dac_align_t)2;
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_dac_init((void *)60, &bad));
    bad = config;
    bad.trigger_src = (xy_hal_dac_trigger_src_t)3;
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_dac_init((void *)60, &bad));
    bad = config;
    bad.wave = (xy_hal_dac_wave_t)3;
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_dac_init((void *)60, &bad));
    bad = config;
    bad.buffer_enable = 2U;
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_dac_init((void *)60, &bad));
    bad = config;
    bad.resolution = XY_HAL_DAC_RESOLUTION_8B;
    bad.align = XY_HAL_DAC_DATAALIGN_LEFT;
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, xy_hal_dac_init((void *)60, &bad));

    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_dac_init((void *)60, &config));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_ALREADY_INIT, xy_hal_dac_init((void *)60, &config));
    TEST_ASSERT_EQUAL_INT(0, xy_hal_dac_get_value((void *)60, XY_HAL_DAC_CHANNEL_1));

    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM,
                      xy_hal_dac_set_value((void *)60, XY_HAL_DAC_CHANNEL_1, 4096U,
                                           XY_HAL_DAC_DATAALIGN_RIGHT));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM,
                      xy_hal_dac_set_value((void *)60, XY_HAL_DAC_CHANNEL_1, 1U,
                                           XY_HAL_DAC_DATAALIGN_LEFT));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM,
                      xy_hal_dac_set_value((void *)60, (xy_hal_dac_channel_t)2, 1U,
                                           XY_HAL_DAC_DATAALIGN_RIGHT));
    TEST_ASSERT_EQUAL(XY_HAL_OK,
                      xy_hal_dac_set_value((void *)60, XY_HAL_DAC_CHANNEL_1, 4095U,
                                           XY_HAL_DAC_DATAALIGN_RIGHT));
    TEST_ASSERT_EQUAL_INT(4095, xy_hal_dac_get_value((void *)60, XY_HAL_DAC_CHANNEL_1));

    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_dac_register_callback((void *)60, XY_HAL_DAC_CHANNEL_2,
                                                              dac_ready_callback, NULL));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_dac_start_output((void *)60, XY_HAL_DAC_CHANNEL_2));
    TEST_ASSERT_EQUAL(1, g_dac_ready_events);
    TEST_ASSERT_EQUAL(XY_HAL_DAC_CHANNEL_2, g_dac_last_channel);
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_dac_stop_output((void *)60, XY_HAL_DAC_CHANNEL_2));

    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM,
                      xy_hal_dac_software_trigger((void *)60, XY_HAL_DAC_CHANNEL_1));

    /* Triangle: from 0 it climbs to the programmed amplitude, then turns around. */
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_dac_set_value((void *)60, XY_HAL_DAC_CHANNEL_1, 0U,
                                                      XY_HAL_DAC_DATAALIGN_RIGHT));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM,
                      xy_hal_dac_generate_triangle((void *)60, XY_HAL_DAC_CHANNEL_1, 0U));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM,
                      xy_hal_dac_generate_triangle((void *)60, XY_HAL_DAC_CHANNEL_1, 4096U));
    TEST_ASSERT_EQUAL(XY_HAL_OK,
                      xy_hal_dac_generate_triangle((void *)60, XY_HAL_DAC_CHANNEL_1, 7U));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_dac_start_output((void *)60, XY_HAL_DAC_CHANNEL_1));
    for (index = 0; index < 3; index++) {
        TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_dac_software_trigger((void *)60, XY_HAL_DAC_CHANNEL_1));
    }
    TEST_ASSERT_EQUAL_INT(3, xy_hal_dac_get_value((void *)60, XY_HAL_DAC_CHANNEL_1));
    for (index = 0; index < 4; index++) {
        (void)xy_hal_dac_software_trigger((void *)60, XY_HAL_DAC_CHANNEL_1);
    }
    TEST_ASSERT_EQUAL_INT(7, xy_hal_dac_get_value((void *)60, XY_HAL_DAC_CHANNEL_1));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_dac_software_trigger((void *)60, XY_HAL_DAC_CHANNEL_1));
    TEST_ASSERT_EQUAL_INT(6, xy_hal_dac_get_value((void *)60, XY_HAL_DAC_CHANNEL_1));
    for (index = 0; index < 6; index++) {
        (void)xy_hal_dac_software_trigger((void *)60, XY_HAL_DAC_CHANNEL_1);
    }
    TEST_ASSERT_EQUAL_INT(0, xy_hal_dac_get_value((void *)60, XY_HAL_DAC_CHANNEL_1));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_dac_software_trigger((void *)60, XY_HAL_DAC_CHANNEL_1));
    TEST_ASSERT_EQUAL_INT(1, xy_hal_dac_get_value((void *)60, XY_HAL_DAC_CHANNEL_1));

    /* Noise: bounded by the programmed mask, and not a constant. */
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM,
                      xy_hal_dac_generate_noise((void *)60, XY_HAL_DAC_CHANNEL_1, 12U));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_dac_generate_noise((void *)60, XY_HAL_DAC_CHANNEL_1, 5U));
    for (index = 0; index < 8; index++) {
        (void)xy_hal_dac_software_trigger((void *)60, XY_HAL_DAC_CHANNEL_1);
        seen = (uint32_t)xy_hal_dac_get_value((void *)60, XY_HAL_DAC_CHANNEL_1);
        TEST_ASSERT_LESS_OR_EQUAL_UINT32(63U, seen);
        if (seen != previous) {
            distinct++;
            previous = seen;
        }
    }
    TEST_ASSERT_GREATER_THAN_UINT32(1U, distinct);

    /* A trigger configured for a timer is not a software trigger. */
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_dac_deinit((void *)60));
    bad = config;
    bad.trigger_src = XY_HAL_DAC_TRIGGER_TIMER;
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_dac_init((void *)60, &bad));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_dac_start_output((void *)60, XY_HAL_DAC_CHANNEL_1));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_SUPPORTED,
                      xy_hal_dac_software_trigger((void *)60, XY_HAL_DAC_CHANNEL_1));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_dac_stop_output((void *)60, XY_HAL_DAC_CHANNEL_1));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM,
                      xy_hal_dac_software_trigger((void *)60, XY_HAL_DAC_CHANNEL_1));

    /* DMA: the host completes the transfer synchronously, and refuses a sample set it cannot honour
     * instead of writing the part of it that happens to fit. */
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_dac_output_dma((void *)60, XY_HAL_DAC_CHANNEL_1,
                                                       dma_samples, 3U));
    TEST_ASSERT_EQUAL_INT(300, xy_hal_dac_get_value((void *)60, XY_HAL_DAC_CHANNEL_1));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM,
                      xy_hal_dac_output_dma((void *)60, XY_HAL_DAC_CHANNEL_1, dma_bad, 2U));
    TEST_ASSERT_EQUAL_INT(300, xy_hal_dac_get_value((void *)60, XY_HAL_DAC_CHANNEL_1));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM,
                      xy_hal_dac_output_dma((void *)60, XY_HAL_DAC_CHANNEL_1, dma_samples, 0U));

    TEST_ASSERT_EQUAL_UINT32(1650U, xy_hal_dac_value_to_mv((void *)60, 2048U, 3300U));
    TEST_ASSERT_EQUAL_UINT32(0U, xy_hal_dac_value_to_mv((void *)60, 4096U, 3300U));

    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_dac_deinit((void *)60));
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_NOT_INIT,
                      xy_hal_dac_get_value((void *)60, XY_HAL_DAC_CHANNEL_1));
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
    RUN_TEST(test_dma_pc_state_and_events);
    RUN_TEST(test_timer_pc_state_and_events);
    RUN_TEST(test_rtc_pc_state_and_alarms);
    RUN_TEST(test_rtc_pc_refuses_a_format_it_cannot_produce);
    RUN_TEST(test_rtc_backup_register_pc_contract);
    RUN_TEST(test_sys_irq_control_pc_contract);
    RUN_TEST(test_adc_pc_conversion_and_channel_batch);
    RUN_TEST(test_adc_pc_watchdog_and_internal_sources);
    RUN_TEST(test_dac_pc_output_contract);
    return UNITY_END();
}
