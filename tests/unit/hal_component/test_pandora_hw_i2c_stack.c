/**
 * @file test_pandora_hw_i2c_stack.c
 * @brief Pandora hardware I2C3 -> Device -> canonical AP3216C vertical slice.
 *
 * `boards/pandora_stm32l475/pandora_hw_i2c.c` is the board-owned handle-based
 * transport for the onboard AP3216C. `pandora_i2c_owners` already pins how that
 * file configures PC0/PC1, the RCC gates and the vendor controller init, and
 * `sensor_ap3216c_device` already pins the canonical driver against stubbed
 * `xy_i2c_device_*` helpers. Neither proves the three layers agree.
 *
 * This fixture closes that seam for the *handle* transport style (the software
 * I2C4 style is covered by `pandora_device_stack`): a host-side AP3216C device
 * model sits on the canonical `xy_hal_i2c_*` seam, so the real board handle is
 * carried through the real `xy_i2c_device_*` helpers into the unmodified
 * `xy_ap3216c` Device driver, and every committed transfer is decoded back into
 * address/register/payload frames.
 *
 * Evidence boundary: host/compile contract only. No timing, bus-electrical,
 * clock-stretching, real-silicon, or board-runtime behaviour is claimed here.
 */

#include "unity.h"

#include "stm32l4xx_hal.h"

#include "pandora_hw_i2c.h"

#include "xy_ap3216c.h"
#include "xy_hal_delay.h"
#include "xy_hal_gpio.h"
#include "xy_hal_i2c.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* --- fake vendor HAL objects ------------------------------------------- */

/* `xy_ap3216c.h` -> `xy_dev_i2c.h` -> `xy_device.h` already pulls the PC port
 * struct in on this host platform, so reuse it instead of redeclaring the type. */
GPIO_TypeDef xy_test_fake_gpioc = {0U, 0U, 0U, 0U};
GPIO_TypeDef xy_test_fake_gpiod = {1U, 0U, 0U, 0U};
I2C_TypeDef xy_test_fake_i2c3;

#define PORT_C 0U
#define PORT_D 1U
#define PORT_FOREIGN 0xFFU
#define PIN_CAPACITY 16U

/* Real Pandora wiring of the hardware I2C3 bus used by the AP3216C. */
#define BUS_SCL_PORT PORT_C
#define BUS_SCL_PIN 0U
#define BUS_SDA_PORT PORT_C
#define BUS_SDA_PIN 1U

/* --- fixture state ----------------------------------------------------- */

static xy_hal_gpio_config_t g_config[2][PIN_CAPACITY];
static unsigned int g_config_count[2][PIN_CAPACITY];
static unsigned int g_rcc_gpioc;
static unsigned int g_rcc_i2c3;
static unsigned int g_delay_ms;

static unsigned int g_i2c_init_calls;
static void* g_i2c_init_handle;
static int g_i2c_init_instance_ok;
static uint32_t g_i2c_init_timing;
static xy_hal_i2c_config_t g_i2c_init_config;
static xy_hal_error_t g_next_i2c_init_result;
static xy_hal_error_t g_next_gpio_init_result;

/* AP3216C device model state. */
#define REG_SPACE 128U
#define FRAME_CAPACITY 24U
#define FRAME_BYTES 8U

static uint8_t g_regs[REG_SPACE];
static uint8_t g_reg_pointer;
static int g_device_present;
static unsigned int g_write_phases;
static unsigned int g_read_phases;

typedef struct {
    uint16_t address;
    uint32_t timeout;
    uint8_t read_not_write;
    uint8_t length;
    uint8_t bytes[FRAME_BYTES];
} frame_t;

static frame_t g_frames[FRAME_CAPACITY];
static unsigned int g_frame_count;

static void* g_board_handle;

static uint8_t port_tag(xy_hal_gpio_port_t port) {
    if (port == &xy_test_fake_gpioc) {
        return PORT_C;
    }
    if (port == &xy_test_fake_gpiod) {
        return PORT_D;
    }
    return PORT_FOREIGN;
}

/* --- frame log --------------------------------------------------------- */

static void frame_record(uint16_t address, uint32_t timeout, uint8_t read_not_write,
                         const uint8_t* bytes, size_t length) {
    frame_t* frame;

    if (g_frame_count >= FRAME_CAPACITY) {
        return;
    }
    frame = &g_frames[g_frame_count];
    memset(frame, 0, sizeof(*frame));
    frame->address = address;
    frame->timeout = timeout;
    frame->read_not_write = read_not_write;
    frame->length = (uint8_t)(length > FRAME_BYTES ? FRAME_BYTES : length);
    if (bytes != NULL && frame->length > 0U) {
        memcpy(frame->bytes, bytes, frame->length);
    }
    ++g_frame_count;
}

/* --- AP3216C device model ---------------------------------------------- */

static int device_addressable(void* handle, uint16_t address) {
    return handle != NULL && handle == g_board_handle && g_device_present &&
           address == XY_AP3216C_DEFAULT_ADDRESS;
}

/* --- canonical HAL seam ------------------------------------------------ */

xy_hal_error_t xy_hal_gpio_init(xy_hal_gpio_port_t port, uint8_t pin,
                                const xy_hal_gpio_config_t* config) {
    const uint8_t tag = port_tag(port);

    if (config == NULL || tag == PORT_FOREIGN || pin >= PIN_CAPACITY) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (g_next_gpio_init_result != XY_HAL_OK) {
        return g_next_gpio_init_result;
    }
    g_config[tag][pin] = *config;
    ++g_config_count[tag][pin];
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_init(void* i2c, const xy_hal_i2c_config_t* config) {
    ++g_i2c_init_calls;
    g_i2c_init_handle = i2c;
    if (config != NULL) {
        g_i2c_init_config = *config;
    }
    if (i2c != NULL) {
        /* A zero-initialised static handle compiles and links cleanly while
         * still failing every vendor transfer; the identity must be set first. */
        const I2C_HandleTypeDef* handle = (const I2C_HandleTypeDef*)i2c;

        g_i2c_init_instance_ok = handle->Instance == I2C3;
        g_i2c_init_timing = handle->Init.Timing;
    }
    return g_next_i2c_init_result;
}

xy_hal_error_t xy_hal_i2c_master_transmit(void* i2c, uint16_t dev_addr, const uint8_t* data,
                                          size_t len, uint32_t timeout) {
    frame_record(dev_addr, timeout, 0U, data, len);
    if (!device_addressable(i2c, dev_addr)) {
        return XY_HAL_ERROR_IO;
    }
    if (data == NULL || len == 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    g_reg_pointer = data[0];
    if (len > 1U) {
        if ((size_t)g_reg_pointer + (len - 1U) > REG_SPACE) {
            return XY_HAL_ERROR_INVALID_PARAM;
        }
        memcpy(&g_regs[g_reg_pointer], &data[1], len - 1U);
    }
    ++g_write_phases;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_master_receive(void* i2c, uint16_t dev_addr, uint8_t* data, size_t len,
                                         uint32_t timeout) {
    frame_record(dev_addr, timeout, 1U, NULL, len);
    if (!device_addressable(i2c, dev_addr)) {
        return XY_HAL_ERROR_IO;
    }
    if (data == NULL || len == 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if ((size_t)g_reg_pointer + len > REG_SPACE) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    memcpy(data, &g_regs[g_reg_pointer], len);
    g_reg_pointer = (uint8_t)(g_reg_pointer + len);
    ++g_read_phases;
    return XY_HAL_OK;
}

void xy_hal_delay_ms(uint32_t ms) {
    g_delay_ms += ms;
}

void xy_test_fake_rcc_gpioc_enable(void) {
    ++g_rcc_gpioc;
}

void xy_test_fake_rcc_gpiod_enable(void) {
    /* Not enabled by the hardware I2C3 owner; kept for the shared fake header. */
}

void xy_test_fake_rcc_i2c3_enable(void) {
    ++g_rcc_i2c3;
}

void xy_test_fake_rcc_pwr_enable(void) {}

/* The canonical device bus helper TU also carries SPI/UART/GPIO helpers this
 * I2C slice never reaches; keep them failing closed instead of linking the
 * whole PC HAL (which would duplicate the transport owner under test). */
xy_hal_error_t xy_hal_spi_transmit(void* spi, const uint8_t* data, size_t len, uint32_t timeout) {
    (void)spi;
    (void)data;
    (void)len;
    (void)timeout;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_spi_receive(void* spi, uint8_t* data, size_t len, uint32_t timeout) {
    (void)spi;
    (void)data;
    (void)len;
    (void)timeout;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_spi_transmit_receive(void* spi, const uint8_t* tx_data, uint8_t* rx_data,
                                           size_t len, uint32_t timeout) {
    (void)spi;
    (void)tx_data;
    (void)rx_data;
    (void)len;
    (void)timeout;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_uart_send(void* uart, const uint8_t* data, size_t len, uint32_t timeout) {
    (void)uart;
    (void)data;
    (void)len;
    (void)timeout;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_uart_recv(void* uart, uint8_t* data, size_t len, uint32_t timeout) {
    (void)uart;
    (void)data;
    (void)len;
    (void)timeout;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_gpio_toggle(xy_hal_gpio_port_t port, uint8_t pin) {
    (void)port;
    (void)pin;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

/* The handle-based I2C3 transport never bit-bangs, so the GPIO data helpers
 * carried by the same TU must stay unused and fail closed. */
xy_hal_error_t xy_hal_gpio_write(xy_hal_gpio_port_t port, uint8_t pin, uint8_t value) {
    (void)port;
    (void)pin;
    (void)value;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int32_t xy_hal_gpio_read(xy_hal_gpio_port_t port, uint8_t pin) {
    (void)port;
    (void)pin;
    return -1;
}

/* --- fixture helpers --------------------------------------------------- */

void setUp(void) {
    memset(g_config, 0, sizeof(g_config));
    memset(g_config_count, 0, sizeof(g_config_count));
    memset(g_frames, 0, sizeof(g_frames));
    memset(g_regs, 0, sizeof(g_regs));
    memset(&g_i2c_init_config, 0, sizeof(g_i2c_init_config));
    g_rcc_gpioc = 0U;
    g_rcc_i2c3 = 0U;
    g_delay_ms = 0U;
    g_i2c_init_calls = 0U;
    g_i2c_init_handle = NULL;
    g_i2c_init_instance_ok = 0;
    g_i2c_init_timing = 0U;
    g_next_i2c_init_result = XY_HAL_OK;
    g_next_gpio_init_result = XY_HAL_OK;
    g_reg_pointer = 0U;
    g_device_present = 1;
    g_write_phases = 0U;
    g_read_phases = 0U;
    g_frame_count = 0U;
    g_board_handle = NULL;
}

void tearDown(void) {}

static void* board_bus(void) {
    g_board_handle = pandora_hw_i2c3_init();
    TEST_ASSERT_NOT_NULL(g_board_handle);
    return g_board_handle;
}

static const frame_t* frame_at(unsigned int index) {
    TEST_ASSERT_LESS_THAN_UINT(g_frame_count, index);
    return &g_frames[index];
}

static void assert_register_write(const frame_t* frame, uint8_t reg, uint8_t value) {
    const uint8_t expected[] = {reg, value};

    TEST_ASSERT_EQUAL_UINT16(XY_AP3216C_DEFAULT_ADDRESS, frame->address);
    TEST_ASSERT_EQUAL_UINT8(0U, frame->read_not_write);
    TEST_ASSERT_EQUAL_UINT8(2U, frame->length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, frame->bytes, sizeof(expected));
}

static void assert_register_select(const frame_t* frame, uint8_t reg) {
    TEST_ASSERT_EQUAL_UINT16(XY_AP3216C_DEFAULT_ADDRESS, frame->address);
    TEST_ASSERT_EQUAL_UINT8(0U, frame->read_not_write);
    TEST_ASSERT_EQUAL_UINT8(1U, frame->length);
    TEST_ASSERT_EQUAL_UINT8(reg, frame->bytes[0]);
}

static void assert_register_read(const frame_t* frame, uint8_t length) {
    TEST_ASSERT_EQUAL_UINT16(XY_AP3216C_DEFAULT_ADDRESS, frame->address);
    TEST_ASSERT_EQUAL_UINT8(1U, frame->read_not_write);
    TEST_ASSERT_EQUAL_UINT8(length, frame->length);
}

/* --- transport owner handed to the driver ------------------------------ */

static void test_the_driver_receives_the_board_owned_i2c3_handle(void) {
    xy_ap3216c_t ap;
    void* bus;

    memset(&ap, 0, sizeof(ap));
    bus = board_bus();

    /* Vendor HAL needs the peripheral identity before HAL_I2C_Init(). */
    TEST_ASSERT_EQUAL_UINT(1U, g_i2c_init_calls);
    TEST_ASSERT_EQUAL_PTR(bus, g_i2c_init_handle);
    TEST_ASSERT_EQUAL_INT(1, g_i2c_init_instance_ok);
    TEST_ASSERT_EQUAL_HEX32(0x10909CECU, g_i2c_init_timing);
    TEST_ASSERT_EQUAL_UINT32(100000U, g_i2c_init_config.clock_speed);
    TEST_ASSERT_EQUAL_INT(XY_HAL_I2C_ADDR_7BIT, g_i2c_init_config.addr_mode);
    TEST_ASSERT_EQUAL_UINT(1U, g_rcc_gpioc);
    TEST_ASSERT_EQUAL_UINT(1U, g_rcc_i2c3);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ap3216c_init(&ap, bus, XY_AP3216C_DEFAULT_ADDRESS,
                                                        XY_AP3216C_MODE_ALS_PS));
    TEST_ASSERT_EQUAL_PTR(bus, ap.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_UINT16(XY_AP3216C_DEFAULT_ADDRESS, ap.i2c_dev.dev_addr);
    TEST_ASSERT_EQUAL_UINT32(100U, ap.i2c_dev.timeout);
    /* The driver must not reconfigure the controller the board owns. */
    TEST_ASSERT_EQUAL_UINT(1U, g_i2c_init_calls);
}

/* --- initialisation on the wire ---------------------------------------- */

static void test_init_programs_reset_then_mode_on_the_wire(void) {
    xy_ap3216c_t ap;
    void* bus = board_bus();

    memset(&ap, 0, sizeof(ap));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ap3216c_init(&ap, bus, XY_AP3216C_DEFAULT_ADDRESS,
                                                        XY_AP3216C_MODE_ALS_PS));

    TEST_ASSERT_EQUAL_UINT(2U, g_write_phases);
    TEST_ASSERT_EQUAL_UINT(0U, g_read_phases);
    TEST_ASSERT_EQUAL_UINT(2U, g_frame_count);
    assert_register_write(frame_at(0U), XY_AP3216C_REG_SYSTEM_CONFIG, XY_AP3216C_MODE_RESET);
    assert_register_write(frame_at(1U), XY_AP3216C_REG_SYSTEM_CONFIG, XY_AP3216C_MODE_ALS_PS);
    /* Reset settling plus mode settling. */
    TEST_ASSERT_EQUAL_UINT(100U, g_delay_ms);

    /* Every transfer carries the driver's 100 ms transport timeout. */
    TEST_ASSERT_EQUAL_UINT32(100U, frame_at(0U)->timeout);
    TEST_ASSERT_EQUAL_UINT32(100U, frame_at(1U)->timeout);

    TEST_ASSERT_EQUAL_UINT8(1U, ap.initialized);
    TEST_ASSERT_EQUAL_UINT8(XY_AP3216C_MODE_ALS_PS, ap.mode);
    TEST_ASSERT_EQUAL_UINT8(XY_AP3216C_MODE_ALS_PS, g_regs[XY_AP3216C_REG_SYSTEM_CONFIG]);
    TEST_ASSERT_TRUE(ap.i2c_dev.base.initialized);
}

static void test_init_accepts_the_zero_address_as_the_board_default(void) {
    xy_ap3216c_t ap;
    void* bus = board_bus();

    memset(&ap, 0, sizeof(ap));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ap3216c_init(&ap, bus, 0U, XY_AP3216C_MODE_ALS));

    TEST_ASSERT_EQUAL_UINT8(XY_AP3216C_MODE_ALS, g_regs[XY_AP3216C_REG_SYSTEM_CONFIG]);
    TEST_ASSERT_EQUAL_UINT8(0U, ap.i2c_dev.dev_addr == XY_AP3216C_DEFAULT_ADDRESS ? 0U : 1U);
    TEST_ASSERT_EQUAL_UINT16(XY_AP3216C_DEFAULT_ADDRESS, frame_at(0U)->address);
}

static void test_init_fails_closed_when_the_reset_write_is_not_acknowledged(void) {
    xy_ap3216c_t ap;
    void* bus = board_bus();

    memset(&ap, 0xFF, sizeof(ap));
    g_device_present = 0;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR, xy_ap3216c_init(&ap, bus, XY_AP3216C_DEFAULT_ADDRESS,
                                                              XY_AP3216C_MODE_ALS_PS));
    /* Only the reset write was attempted; no mode write followed. */
    TEST_ASSERT_EQUAL_UINT(1U, g_frame_count);
    assert_register_write(frame_at(0U), XY_AP3216C_REG_SYSTEM_CONFIG, XY_AP3216C_MODE_RESET);
    TEST_ASSERT_EQUAL_UINT(0U, g_delay_ms);
    /* Controller configuration stays a board decision even on failure. */
    TEST_ASSERT_EQUAL_UINT(1U, g_i2c_init_calls);
    TEST_ASSERT_EQUAL_UINT8(0U, ap.initialized);
    TEST_ASSERT_FALSE(ap.i2c_dev.base.initialized);
}

static void test_a_failed_reinit_keeps_a_live_owner_and_its_cache(void) {
    xy_ap3216c_t ap;
    void* bus = board_bus();
    uint32_t lux = 0U;

    memset(&ap, 0, sizeof(ap));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ap3216c_init(&ap, bus, XY_AP3216C_DEFAULT_ADDRESS,
                                                        XY_AP3216C_MODE_ALS_PS));
    g_regs[XY_AP3216C_REG_ALS_DATA_L] = 0x10U;
    g_regs[XY_AP3216C_REG_ALS_DATA_L + 1U] = 0x00U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ap3216c_read_light(&ap, &lux));

    g_device_present = 0;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR, xy_ap3216c_init(&ap, bus, XY_AP3216C_DEFAULT_ADDRESS,
                                                              XY_AP3216C_MODE_ALS));
    TEST_ASSERT_EQUAL_UINT8(1U, ap.initialized);
    TEST_ASSERT_EQUAL_UINT8(XY_AP3216C_MODE_ALS_PS, ap.mode);
    TEST_ASSERT_EQUAL_PTR(bus, ap.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_UINT32(lux, ap.data.illuminance_millilux);
}

/* --- reads on the wire ------------------------------------------------- */

static void test_light_read_selects_the_als_low_register_and_scales_by_350(void) {
    xy_ap3216c_t ap;
    void* bus = board_bus();
    uint32_t lux = 0U;

    memset(&ap, 0, sizeof(ap));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ap3216c_init(&ap, bus, XY_AP3216C_DEFAULT_ADDRESS,
                                                        XY_AP3216C_MODE_ALS_PS));
    g_regs[XY_AP3216C_REG_ALS_DATA_L] = 0x34U;
    g_regs[XY_AP3216C_REG_ALS_DATA_L + 1U] = 0x12U;
    g_frame_count = 0U;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ap3216c_read_light(&ap, &lux));

    TEST_ASSERT_EQUAL_UINT(2U, g_frame_count);
    assert_register_select(frame_at(0U), XY_AP3216C_REG_ALS_DATA_L);
    assert_register_read(frame_at(1U), 2U);
    TEST_ASSERT_EQUAL_UINT32(4660U * 350U, lux);
    TEST_ASSERT_EQUAL_UINT32(lux, ap.data.illuminance_millilux);
}

static void test_proximity_read_masks_the_twelve_bit_field(void) {
    xy_ap3216c_t ap;
    void* bus = board_bus();
    uint16_t proximity = 0U;

    memset(&ap, 0, sizeof(ap));
    TEST_ASSERT_EQUAL_INT(
        XY_DEVICE_OK, xy_ap3216c_init(&ap, bus, XY_AP3216C_DEFAULT_ADDRESS, XY_AP3216C_MODE_PS));
    /* High byte carries only six data bits, low byte only four; the spare
     * bits (including the low byte's 0x40 range flag) must be masked away. */
    g_regs[XY_AP3216C_REG_PS_DATA_L] = 0xBAU;
    g_regs[XY_AP3216C_REG_PS_DATA_L + 1U] = 0xFFU;
    g_frame_count = 0U;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ap3216c_read_proximity(&ap, &proximity));

    TEST_ASSERT_EQUAL_UINT(2U, g_frame_count);
    assert_register_select(frame_at(0U), XY_AP3216C_REG_PS_DATA_L);
    assert_register_read(frame_at(1U), 2U);
    TEST_ASSERT_EQUAL_UINT16(0x3FAU, proximity);
    TEST_ASSERT_EQUAL_UINT16(proximity, ap.data.proximity_raw);
}

static void test_proximity_read_reports_io_error_when_the_object_bit_is_set(void) {
    xy_ap3216c_t ap;
    void* bus = board_bus();
    uint16_t proximity = 0U;

    memset(&ap, 0, sizeof(ap));
    TEST_ASSERT_EQUAL_INT(
        XY_DEVICE_OK, xy_ap3216c_init(&ap, bus, XY_AP3216C_DEFAULT_ADDRESS, XY_AP3216C_MODE_PS));
    g_regs[XY_AP3216C_REG_PS_DATA_L] = 0x05U;
    g_regs[XY_AP3216C_REG_PS_DATA_L + 1U] = 0x00U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ap3216c_read_proximity(&ap, &proximity));

    /* Bit 6 of the low byte means "object out of measuring range". */
    g_regs[XY_AP3216C_REG_PS_DATA_L] = 0x45U;
    proximity = 4321U;
    g_frame_count = 0U;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR, xy_ap3216c_read_proximity(&ap, &proximity));

    TEST_ASSERT_EQUAL_UINT16(4321U, proximity);
    TEST_ASSERT_EQUAL_UINT16(5U, ap.data.proximity_raw); /* cache untouched */
    TEST_ASSERT_EQUAL_UINT(2U, g_frame_count);           /* the read still happened */
}

static void test_ir_read_masks_the_ten_bit_field(void) {
    xy_ap3216c_t ap;
    void* bus = board_bus();
    uint16_t infrared = 0U;

    memset(&ap, 0, sizeof(ap));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ap3216c_init(&ap, bus, XY_AP3216C_DEFAULT_ADDRESS,
                                                        XY_AP3216C_MODE_ALS_PS));
    g_regs[XY_AP3216C_REG_IR_DATA_L] = 0xCDU;
    g_regs[XY_AP3216C_REG_IR_DATA_L + 1U] = 0xFFU;
    g_frame_count = 0U;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ap3216c_read_ir(&ap, &infrared));

    TEST_ASSERT_EQUAL_UINT(2U, g_frame_count);
    assert_register_select(frame_at(0U), XY_AP3216C_REG_IR_DATA_L);
    assert_register_read(frame_at(1U), 2U);
    TEST_ASSERT_EQUAL_UINT16(0x3CDU, infrared);
    TEST_ASSERT_EQUAL_UINT16(infrared, ap.data.infrared_raw);
}

static void test_the_three_reads_use_disjoint_register_pairs(void) {
    xy_ap3216c_t ap;
    void* bus = board_bus();
    uint32_t lux = 0U;
    uint16_t proximity = 0U;
    uint16_t infrared = 0U;

    memset(&ap, 0, sizeof(ap));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ap3216c_init(&ap, bus, XY_AP3216C_DEFAULT_ADDRESS,
                                                        XY_AP3216C_MODE_ALS_PS));
    g_regs[XY_AP3216C_REG_IR_DATA_L] = 0x01U;
    g_regs[XY_AP3216C_REG_IR_DATA_L + 1U] = 0x00U;
    g_regs[XY_AP3216C_REG_ALS_DATA_L] = 0x02U;
    g_regs[XY_AP3216C_REG_ALS_DATA_L + 1U] = 0x00U;
    g_regs[XY_AP3216C_REG_PS_DATA_L] = 0x03U;
    g_regs[XY_AP3216C_REG_PS_DATA_L + 1U] = 0x00U;
    g_frame_count = 0U;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ap3216c_read_ir(&ap, &infrared));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ap3216c_read_light(&ap, &lux));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ap3216c_read_proximity(&ap, &proximity));

    TEST_ASSERT_EQUAL_UINT(6U, g_frame_count);
    for (unsigned int index = 0U; index < 6U; index += 2U) {
        assert_register_read(frame_at(index + 1U), 2U);
    }
    TEST_ASSERT_EQUAL_UINT8(XY_AP3216C_REG_IR_DATA_L, frame_at(0U)->bytes[0]);
    TEST_ASSERT_EQUAL_UINT8(XY_AP3216C_REG_ALS_DATA_L, frame_at(2U)->bytes[0]);
    TEST_ASSERT_EQUAL_UINT8(XY_AP3216C_REG_PS_DATA_L, frame_at(4U)->bytes[0]);
    TEST_ASSERT_EQUAL_UINT16(1U, infrared);
    TEST_ASSERT_EQUAL_UINT32(2U * 350U, lux);
    TEST_ASSERT_EQUAL_UINT16(3U, proximity);
}

static void test_reads_fail_closed_when_the_device_stops_answering(void) {
    xy_ap3216c_t ap;
    void* bus = board_bus();
    uint32_t lux = 9U;
    uint16_t proximity = 9U;
    uint16_t infrared = 9U;

    memset(&ap, 0, sizeof(ap));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ap3216c_init(&ap, bus, XY_AP3216C_DEFAULT_ADDRESS,
                                                        XY_AP3216C_MODE_ALS_PS));
    g_device_present = 0;
    g_frame_count = 0U;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR, xy_ap3216c_read_light(&ap, &lux));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR, xy_ap3216c_read_proximity(&ap, &proximity));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR, xy_ap3216c_read_ir(&ap, &infrared));

    /* Each read stops after the rejected register select. */
    TEST_ASSERT_EQUAL_UINT(3U, g_frame_count);
    TEST_ASSERT_EQUAL_UINT(0U, g_read_phases);
    TEST_ASSERT_EQUAL_UINT32(9U, lux);
    TEST_ASSERT_EQUAL_UINT16(9U, proximity);
    TEST_ASSERT_EQUAL_UINT16(9U, infrared);
    TEST_ASSERT_EQUAL_UINT32(0U, ap.data.illuminance_millilux);
    TEST_ASSERT_EQUAL_UINT16(0U, ap.data.proximity_raw);
    TEST_ASSERT_EQUAL_UINT16(0U, ap.data.infrared_raw);
}

/* --- teardown on the wire ---------------------------------------------- */

static void test_deinit_powers_the_device_down_on_the_wire(void) {
    xy_ap3216c_t ap;
    void* bus = board_bus();
    uint32_t lux = 0U;

    memset(&ap, 0, sizeof(ap));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ap3216c_init(&ap, bus, XY_AP3216C_DEFAULT_ADDRESS,
                                                        XY_AP3216C_MODE_ALS_PS));
    g_frame_count = 0U;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ap3216c_deinit(&ap));

    TEST_ASSERT_EQUAL_UINT(1U, g_frame_count);
    assert_register_write(frame_at(0U), XY_AP3216C_REG_SYSTEM_CONFIG, XY_AP3216C_MODE_POWER_DOWN);
    TEST_ASSERT_EQUAL_UINT8(0U, g_regs[XY_AP3216C_REG_SYSTEM_CONFIG]);
    TEST_ASSERT_EQUAL_UINT8(0U, ap.initialized);
    TEST_ASSERT_FALSE(ap.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(ap.i2c_dev.i2c_handle);

    /* A deinitialised driver must not touch the bus again. */
    g_frame_count = 0U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_ap3216c_read_light(&ap, &lux));
    TEST_ASSERT_EQUAL_UINT(0U, g_frame_count);
}

static void test_deinit_reports_a_bus_fault_and_keeps_the_owner_live(void) {
    xy_ap3216c_t ap;
    void* bus = board_bus();

    memset(&ap, 0, sizeof(ap));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ap3216c_init(&ap, bus, XY_AP3216C_DEFAULT_ADDRESS,
                                                        XY_AP3216C_MODE_ALS_PS));
    g_device_present = 0;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR, xy_ap3216c_deinit(&ap));
    TEST_ASSERT_EQUAL_UINT8(1U, ap.initialized);
    TEST_ASSERT_TRUE(ap.i2c_dev.base.initialized);
    TEST_ASSERT_EQUAL_PTR(bus, ap.i2c_dev.i2c_handle);
}

/* --- runner ------------------------------------------------------------ */

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_the_driver_receives_the_board_owned_i2c3_handle);
    RUN_TEST(test_init_programs_reset_then_mode_on_the_wire);
    RUN_TEST(test_init_accepts_the_zero_address_as_the_board_default);
    RUN_TEST(test_init_fails_closed_when_the_reset_write_is_not_acknowledged);
    RUN_TEST(test_a_failed_reinit_keeps_a_live_owner_and_its_cache);
    RUN_TEST(test_light_read_selects_the_als_low_register_and_scales_by_350);
    RUN_TEST(test_proximity_read_masks_the_twelve_bit_field);
    RUN_TEST(test_proximity_read_reports_io_error_when_the_object_bit_is_set);
    RUN_TEST(test_ir_read_masks_the_ten_bit_field);
    RUN_TEST(test_the_three_reads_use_disjoint_register_pairs);
    RUN_TEST(test_reads_fail_closed_when_the_device_stops_answering);
    RUN_TEST(test_deinit_powers_the_device_down_on_the_wire);
    RUN_TEST(test_deinit_reports_a_bus_fault_and_keeps_the_owner_live);
    return UNITY_END();
}
