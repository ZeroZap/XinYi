/**
 * @file hal_quickstart.c
 * @brief The HAL quick start this repository's HAL documentation teaches - as a program.
 *
 * `components/hal/README.md` and `docs/hal/HAL_UNIFIED_API.md` used to show
 * `xy_hal_gpio_bind("GPIOC.13")`, `xy_hal_uart_configure()` and an `xy_hal_gpio_t` handle. None of
 * those names exist: the HAL takes a *platform handle* (the vendor peripheral pointer on a board, a
 * handle object on the PC simulation) and every entry is `xy_hal_<peripheral>_<verb>(handle, ...)`.
 * A reader who copied the documented sample got a compile error, and nothing in the repository
 * noticed because prose is not built.
 *
 * So the documented sample is this file. It is compiled and run by CTest `hal_quickstart` against
 * the real PC backend, and the code blocks in the two documents are excerpts of it - one fact
 * source, so a broken HAL contract fails a test instead of quietly ageing in a markdown block.
 *
 * Scope of what this proves: the *host* backend's contract (argument validation, state, and the
 * answers a sample can observe). It does not claim any MCU runtime, and it does not claim that a
 * name exists on every platform - `docs/validation/hal-platform-evidence-matrix.md` is the fact
 * source for per-platform coverage.
 *
 * Deliberately no `assert()`: the default build type is Release (`NDEBUG`), where `assert()` is a
 * no-op and the sample would then check nothing while still exiting 0.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "xy_hal.h"
#include "xy_hal_delay.h"
#include "xy_hal_error.h"
#include "xy_hal_gpio.h"
#include "xy_hal_i2c.h"
#include "xy_hal_pc.h"
#include "xy_hal_spi.h"
#include "xy_hal_sys.h"
#include "xy_hal_uart.h"

static int g_checks;
static int g_failures;

static void check(int condition, const char *what)
{
    g_checks++;
    if (condition == 0) {
        g_failures++;
        printf("FAIL %s\n", what);
    }
}

/* ---- GPIO ---------------------------------------------------------------------------------- */

static void gpio_quickstart(void)
{
    /* The handle is the platform's peripheral, not a name string: `GPIOA` on a board (a
     * `GPIO_TypeDef *`), a handle object on the PC simulation. */
    struct xy_hal_gpio_port port_a = {.port_id = 0};
    xy_hal_gpio_port_t port = &port_a;

    xy_hal_gpio_config_t output_cfg = {
        .mode = XY_HAL_GPIO_MODE_OUTPUT,
        .pull = XY_HAL_GPIO_PULL_NONE,
        .otype = XY_HAL_GPIO_OTYPE_PP,
        .speed = XY_HAL_GPIO_SPEED_LOW,
        .alternate = 0,
    };

    check(xy_hal_gpio_init(port, 5, &output_cfg) == XY_HAL_OK, "gpio init PA5");
    check(xy_hal_gpio_write(port, 5, 1) == XY_HAL_OK, "gpio write 1");
    check(xy_hal_gpio_read(port, 5) == 1, "gpio read back 1");
    check(xy_hal_gpio_toggle(port, 5) == XY_HAL_OK, "gpio toggle");
    check(xy_hal_gpio_read(port, 5) == 0, "gpio read back 0 after toggle");

    /* Batch forms take a pin mask and a value mask. */
    check(xy_hal_gpio_write_batch(port, 0x0003U, 0x0001U) == XY_HAL_OK, "gpio write batch");
    check(xy_hal_gpio_read_batch(port, 0x0003U) == 0x0001, "gpio read batch");

    /* Refusals are part of the contract, not an afterthought: pin 15 is the last legal pin. */
    check(xy_hal_gpio_init(port, 16, &output_cfg) == XY_HAL_ERROR_INVALID_PARAM,
          "gpio init rejects pin 16");
    check(xy_hal_gpio_init(port, 6, NULL) == XY_HAL_ERROR_INVALID_PARAM,
          "gpio init rejects NULL config");
    check(xy_hal_gpio_write_batch(port, 0x0000U, 0U) == XY_HAL_ERROR_INVALID_PARAM,
          "gpio write batch rejects an empty mask");

    check(xy_hal_gpio_deinit(port, 5) == XY_HAL_OK, "gpio deinit PA5");
}

/* ---- UART ---------------------------------------------------------------------------------- */

static void uart_quickstart(void)
{
    /* On a board this is `&huart2`, the CubeMX handle. On the host the backend keys its state off
     * the handle pointer, so any distinct object works. */
    xy_hal_uart_t uart = {.instance = 2, .baudrate = 0, .fp = NULL};
    void *handle = &uart;

    xy_hal_uart_config_t cfg = {
        .baudrate = 115200,
        .wordlen = XY_HAL_UART_WORDLEN_8B,
        .stopbits = XY_HAL_UART_STOPBITS_1,
        .parity = XY_HAL_UART_PARITY_NONE,
        .flowctrl = XY_HAL_UART_FLOWCTRL_NONE,
        .mode = XY_HAL_UART_MODE_TX_RX,
    };
    const char *msg = "Hello UART!";
    uint8_t rx[8];

    check(xy_hal_uart_init(handle, &cfg) == XY_HAL_OK, "uart init 115200 8N1");
    check(xy_hal_uart_init(handle, &cfg) == XY_HAL_ERROR_ALREADY_INIT, "uart re-init refused");
    check(xy_hal_uart_send(handle, (const uint8_t *)msg, strlen(msg), 100) == XY_HAL_OK,
          "uart send");
    check(xy_hal_uart_send(handle, (const uint8_t *)msg, 0, 100) == XY_HAL_ERROR_INVALID_PARAM,
          "uart send rejects a zero length");

    memset(rx, 0xA5, sizeof(rx));
    check(xy_hal_uart_recv(handle, rx, sizeof(rx), 100) == XY_HAL_OK, "uart recv");
    check(rx[0] == 0 && rx[sizeof(rx) - 1] == 0, "uart recv fills the buffer");

    check(xy_hal_uart_deinit(handle) == XY_HAL_OK, "uart deinit");
    check(xy_hal_uart_send(handle, (const uint8_t *)msg, strlen(msg), 100) ==
              XY_HAL_ERROR_NOT_INIT,
          "uart send after deinit is NOT_INIT");

    /* A zero baudrate is refused rather than defaulted: a port configured at 0 would look
     * initialised and move nothing. */
    check(xy_hal_uart_init(handle, &(xy_hal_uart_config_t){0}) == XY_HAL_ERROR_INVALID_PARAM,
          "uart init rejects baudrate 0");
}

/* ---- I2C ----------------------------------------------------------------------------------- */

static void i2c_quickstart(void)
{
    /* On a board this is `&hi2c1`. */
    xy_hal_i2c_t i2c = {.instance = 1, .speed = 0, .current_addr = 0};
    void *handle = &i2c;

    xy_hal_i2c_config_t cfg = {
        .clock_speed = 400000, /* 400 kHz */
        .addr_mode = XY_HAL_I2C_ADDR_7BIT,
        .duty_cycle = XY_HAL_I2C_DUTY_2,
        .own_address = 0,
        .general_call_mode = 0,
    };
    uint8_t value = 0x55;
    uint8_t readback = 0;

    check(xy_hal_i2c_init(handle, &cfg) == XY_HAL_OK, "i2c init 400 kHz");

    /* Register access: a memory write/read pair, which is what a sensor register is. */
    check(xy_hal_i2c_mem_write(handle, 0x68, 0x0000, &value, 1, 100) == XY_HAL_OK,
          "i2c mem write");
    check(xy_hal_i2c_mem_read(handle, 0x68, 0x0000, &readback, 1, 100) == XY_HAL_OK,
          "i2c mem read");
    check(xy_hal_i2c_master_transmit(handle, 0x68, &value, 1, 100) == XY_HAL_OK,
          "i2c master transmit");

    /* Addresses are 7-bit and unshifted here - the convention every backend documents. */
    check(xy_hal_i2c_master_transmit(handle, 0x80, &value, 1, 100) == XY_HAL_ERROR_INVALID_PARAM,
          "i2c rejects a shifted/8-bit address");

    check(xy_hal_i2c_deinit(handle) == XY_HAL_OK, "i2c deinit");

    /* 10-bit addressing is declared for the shape of the API and implemented by no backend; init
     * refuses it instead of programming a mode no transfer can use. */
    {
        xy_hal_i2c_t ten_bit = {.instance = 2, .speed = 0, .current_addr = 0};
        xy_hal_i2c_config_t ten_bit_cfg = cfg;

        ten_bit_cfg.addr_mode = XY_HAL_I2C_ADDR_10BIT;
        check(xy_hal_i2c_init(&ten_bit, &ten_bit_cfg) == XY_HAL_ERROR_NOT_SUPPORTED,
              "i2c init refuses 10-bit addressing");
    }
}

/* ---- SPI ----------------------------------------------------------------------------------- */

static void spi_quickstart(void)
{
    /* On a board this is `&hspi1`. */
    xy_hal_spi_t spi = {.instance = 1, .speed_hz = 0, .mode = 0};
    void *handle = &spi;

    xy_hal_spi_config_t cfg = {
        .mode = XY_HAL_SPI_MODE_0,
        .direction = XY_HAL_SPI_DIR_2LINES,
        .datasize = XY_HAL_SPI_DATASIZE_8BIT,
        .firstbit = XY_HAL_SPI_FIRSTBIT_MSB,
        .nss = XY_HAL_SPI_NSS_SOFT,
        .baudrate_prescaler = 8,
        .is_master = 1,
    };
    uint8_t tx[2] = {0x9F, 0x00};
    uint8_t rx[2] = {0, 0};

    check(xy_hal_spi_init(handle, &cfg) == XY_HAL_OK, "spi init mode 0");
    check(xy_hal_spi_transmit_receive(handle, tx, rx, sizeof(tx), 100) == XY_HAL_OK,
          "spi full-duplex transfer");
    check(xy_hal_spi_transmit(handle, tx, sizeof(tx), 100) == XY_HAL_OK, "spi transmit");
    check(xy_hal_spi_deinit(handle) == XY_HAL_OK, "spi deinit");

    /* A transfer on a handle that was never initialised must not look like success. */
    check(xy_hal_spi_transmit(handle, tx, sizeof(tx), 100) == XY_HAL_ERROR_NOT_INIT,
          "spi transmit before init is NOT_INIT");
}

/* ---- SYS / delay --------------------------------------------------------------------------- */

static void sys_quickstart(void)
{
    xy_hal_sys_clock_info_t info;
    uint32_t first;
    uint32_t second;

    check(xy_hal_sys_get_clock_info(NULL) == XY_HAL_ERROR_INVALID_PARAM,
          "sys clock info rejects NULL");
    check(xy_hal_sys_get_clock_info(&info) == XY_HAL_OK, "sys clock info");
    check(info.sysclk != 0, "sys clock info reports a non-zero sysclk");

    first = xy_hal_sys_get_tick_count();
    second = xy_hal_sys_get_tick_count();
    check((int32_t)(second - first) >= 0, "sys tick does not run backwards");
    check(xy_hal_sys_get_tick_freq() == XY_HAL_SYS_TICK_FREQ, "sys tick frequency");

    /* The delay contract is "returns"; a host cannot witness elapsed wall time, so this sample does
     * not assert a duration. On a board the millisecond budget is a separate, measured claim. */
    xy_hal_delay_ms(1);
    xy_hal_delay_us(10);
}

int main(void)
{
    gpio_quickstart();
    uart_quickstart();
    i2c_quickstart();
    spi_quickstart();
    sys_quickstart();

    printf("hal quickstart: %d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
