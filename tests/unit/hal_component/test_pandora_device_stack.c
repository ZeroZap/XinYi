/**
 * @file test_pandora_device_stack.c
 * @brief Pandora I2C -> Device -> driver vertical slice on the host.
 *
 * The Pandora reference board owns its I2C transports itself:
 * `boards/pandora_stm32l475/pandora_soft_i2c.c` provides the canonical
 * `xy_hal_i2c_*` bit-bang backend for the onboard AHT10 / ICM20608 path, and
 * `pandora_hw_i2c.c` provides the handle-based I2C3 path for AP3216C.
 *
 * Each layer already had isolated coverage: `pandora_i2c_owners` decodes the
 * GPIO waveform of the transports alone, and the canonical driver tests stub
 * `xy_i2c_device_*`. Nothing proved the three layers agree with each other, so
 * this fixture closes that seam: a host-only I2C *slave* model is driven by the
 * real board transport, the real `xy_i2c_device_*` bus helpers and the canonical
 * `xy_aht10` / `xy_icm20608` Device drivers run unmodified on top of it.
 *
 * The slave model answers ACK/NACK per address, keeps a register file for the
 * register-addressed device and a command/frame model for the command-addressed
 * device, so the drivers' identity binding, configure-and-read-back and sample
 * decoding are observed end to end on the wire.
 *
 * Evidence boundary: host/compile contract only. No timing, bus-electrical,
 * clock-stretching, real-device, or board-runtime behaviour is claimed here.
 */

#include "unity.h"

#include "stm32l4xx_hal.h"

#include "pandora_soft_i2c.h"

#include "xy_aht10.h"
#include "xy_hal_delay.h"
#include "xy_hal_gpio.h"
#include "xy_hal_i2c.h"
#include "xy_icm20608.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* --- fake vendor HAL objects ------------------------------------------- */

/* `xy_device.h` already pulls the PC simulation port struct in on this host
 * platform, so reuse it instead of redeclaring the type. */
#ifdef HAL_PLATFORM_PC
GPIO_TypeDef xy_test_fake_gpioc = {0U, 0U, 0U, 0U};
GPIO_TypeDef xy_test_fake_gpiod = {1U, 0U, 0U, 0U};
#else
struct xy_hal_gpio_port {
    uint8_t tag;
};
GPIO_TypeDef xy_test_fake_gpioc = {0U};
GPIO_TypeDef xy_test_fake_gpiod = {1U};
#endif

#define PORT_C 0U
#define PORT_D 1U
#define PORT_FOREIGN 0xFFU
#define PIN_CAPACITY 16U

/* Real Pandora wiring of the software-I2C4 bus used by AHT10 and ICM20608. */
#define BUS_SCL_PORT PORT_D
#define BUS_SCL_PIN 6U
#define BUS_SDA_PORT PORT_C
#define BUS_SDA_PIN 1U

/* --- GPIO fixture ------------------------------------------------------ */

static xy_hal_gpio_config_t g_config[2][PIN_CAPACITY];
static unsigned int g_config_count[2][PIN_CAPACITY];
static unsigned int g_rcc_gpioc;
static unsigned int g_rcc_gpiod;
static unsigned int g_delay_ms;

static uint8_t g_scl_level;
static uint8_t g_sda_master_level;
static int g_sda_master_driving;
static uint8_t g_slave_drive = 1U;

static uint8_t port_tag(xy_hal_gpio_port_t port) {
    if (port == &xy_test_fake_gpioc) {
        return PORT_C;
    }
    if (port == &xy_test_fake_gpiod) {
        return PORT_D;
    }
    return PORT_FOREIGN;
}

xy_hal_error_t xy_hal_gpio_init(xy_hal_gpio_port_t port, uint8_t pin,
                                const xy_hal_gpio_config_t* config) {
    const uint8_t tag = port_tag(port);

    if (config == NULL || tag == PORT_FOREIGN || pin >= PIN_CAPACITY) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    g_config[tag][pin] = *config;
    ++g_config_count[tag][pin];
    if (tag == BUS_SDA_PORT && pin == BUS_SDA_PIN) {
        g_sda_master_driving = config->mode != XY_HAL_GPIO_MODE_INPUT;
    }
    return XY_HAL_OK;
}

/* --- I2C slave model --------------------------------------------------- */

typedef enum {
    DEV_NONE = 0,
    DEV_REGISTER, /* register-addressed device (ICM20608 style) */
    DEV_COMMAND,  /* command/stream device (AHT10 style) */
} dev_kind_t;

#define SLAVE_CAPACITY 2U
#define REG_SPACE 256U
#define FRAME_CAPACITY 96U
#define FRAME_BYTES 24U

typedef struct {
    uint8_t present;
    uint8_t address;
    dev_kind_t kind;
    uint8_t reg_pointer;
    uint8_t regs[REG_SPACE];
    uint8_t read_only[REG_SPACE];
    uint8_t write_buffer[8];
    uint8_t write_length;
    uint8_t frame[8];
    uint8_t frame_length;
    uint8_t frame_index;
    unsigned int init_commands;
    unsigned int measure_commands;
    unsigned int write_phases;
    unsigned int read_phases;
} slave_device_t;

static slave_device_t g_device[SLAVE_CAPACITY];

typedef struct {
    uint8_t address;
    uint8_t rw;
    uint8_t length;
    uint8_t bytes[FRAME_BYTES];
} frame_t;

static frame_t g_frames[FRAME_CAPACITY];
static unsigned int g_frame_count;
static frame_t g_frame;
static int g_frame_active;

typedef enum {
    ST_IDLE = 0,
    ST_ADDR,
    ST_ACK,
    ST_WDATA,
    ST_RDATA,
    ST_RACK,
    ST_IGNORE,
} slave_state_t;

typedef enum {
    AFTER_IGNORE = 0,
    AFTER_WDATA,
    AFTER_WDATA_CONTINUE,
    AFTER_RDATA,
} after_ack_t;

static slave_state_t g_state;
static after_ack_t g_after;
static int g_addressed;
static uint8_t g_read_write;
static uint8_t g_bit;
static uint8_t g_shift;
static uint8_t g_read_byte;

/* --- frame log --------------------------------------------------------- */

static void frame_begin(void) {
    memset(&g_frame, 0, sizeof(g_frame));
    g_frame.address = 0xFFU;
    g_frame_active = 1;
}

static void frame_note_address(uint8_t address_byte) {
    if (!g_frame_active) {
        frame_begin();
    }
    g_frame.address = address_byte;
    g_frame.rw = (uint8_t)(address_byte & 1U);
}

static void frame_note_byte(uint8_t value) {
    if (!g_frame_active || g_frame.length >= FRAME_BYTES) {
        return;
    }
    g_frame.bytes[g_frame.length] = value;
    ++g_frame.length;
}

static void frame_commit(void) {
    if (g_frame_active && g_frame.address != 0xFFU && g_frame_count < FRAME_CAPACITY) {
        g_frames[g_frame_count] = g_frame;
        ++g_frame_count;
    }
    g_frame_active = 0;
}

/* --- slave device models ----------------------------------------------- */

static slave_device_t* slave_device_by_address(uint8_t address) {
    for (unsigned int index = 0U; index < SLAVE_CAPACITY; ++index) {
        if (g_device[index].present && g_device[index].address == address) {
            return &g_device[index];
        }
    }
    return NULL;
}

static void slave_reset_devices(void) {
    memset(g_device, 0, sizeof(g_device));
    g_frame_count = 0U;
    g_frame_active = 0;
    g_state = ST_IDLE;
    g_addressed = 0;
    g_bit = 0U;
    g_shift = 0U;
}

static slave_device_t* slave_attach(uint8_t address, dev_kind_t kind) {
    for (unsigned int index = 0U; index < SLAVE_CAPACITY; ++index) {
        if (!g_device[index].present) {
            g_device[index].present = 1U;
            g_device[index].address = address;
            g_device[index].kind = kind;
            return &g_device[index];
        }
    }
    return NULL;
}

static void slave_add_read_only_register(slave_device_t* device, uint8_t reg) {
    device->read_only[reg] = 1U;
}

static void slave_write_byte(slave_device_t* device, uint8_t value) {
    if (device == NULL) {
        return;
    }
    if (device->write_length < sizeof(device->write_buffer)) {
        device->write_buffer[device->write_length] = value;
    }
    ++device->write_length;
    if (device->kind != DEV_REGISTER) {
        return;
    }
    if (device->write_length == 1U) {
        device->reg_pointer = value;
        return;
    }
    if (!device->read_only[device->reg_pointer]) {
        device->regs[device->reg_pointer] = value;
    }
    ++device->reg_pointer;
}

static void slave_begin_write(slave_device_t* device) {
    if (device == NULL) {
        return;
    }
    device->write_length = 0U;
    ++device->write_phases;
}

static void slave_end_write(slave_device_t* device) {
    static const uint8_t aht10_init[] = {0xE1U, 0x08U, 0x00U};
    static const uint8_t aht10_measure[] = {0xACU, 0x33U, 0x00U};

    if (device == NULL || device->kind != DEV_COMMAND || device->write_length == 0U) {
        return;
    }
    if (device->write_length == sizeof(aht10_init) &&
        memcmp(device->write_buffer, aht10_init, sizeof(aht10_init)) == 0) {
        ++device->init_commands;
        return;
    }
    if (device->write_length == sizeof(aht10_measure) &&
        memcmp(device->write_buffer, aht10_measure, sizeof(aht10_measure)) == 0) {
        ++device->measure_commands;
        device->frame_index = 0U;
    }
}

static uint8_t slave_read_byte(slave_device_t* device) {
    uint8_t value = 0xFFU;

    if (device == NULL) {
        return value;
    }
    if (device->kind == DEV_REGISTER) {
        value = device->regs[device->reg_pointer];
        ++device->reg_pointer;
    } else if (device->frame_index < device->frame_length) {
        value = device->frame[device->frame_index];
        ++device->frame_index;
    }
    frame_note_byte(value);
    return value;
}

/* --- I2C slave state machine ------------------------------------------- */

static void slave_on_start(void) {
    frame_commit();
    frame_begin();
    g_state = ST_ADDR;
    g_bit = 0U;
    g_shift = 0U;
    g_addressed = 0;
    g_slave_drive = 1U;
}

static void slave_on_stop(void) {
    slave_device_t* device = slave_device_by_address((uint8_t)(g_frame.address >> 1));

    if (g_addressed && device != NULL) {
        if (g_read_write == 0U) {
            slave_end_write(device);
        } else {
            ++device->read_phases;
        }
    }
    frame_commit();
    g_state = ST_IDLE;
    g_addressed = 0;
    g_bit = 0U;
    g_slave_drive = 1U;
}

static void slave_on_clock(void) {
    slave_device_t* device;

    switch (g_state) {
        case ST_ADDR:
            g_shift = (uint8_t)((g_shift << 1) | (g_sda_master_level & 1U));
            if (++g_bit < 8U) {
                break;
            }
            frame_note_address(g_shift);
            g_read_write = (uint8_t)(g_shift & 1U);
            device = slave_device_by_address((uint8_t)(g_shift >> 1));
            g_addressed = device != NULL;
            g_bit = 0U;
            g_slave_drive = g_addressed ? 0U : 1U;
            g_after = !g_addressed ? AFTER_IGNORE : (g_read_write ? AFTER_RDATA : AFTER_WDATA);
            g_state = ST_ACK;
            break;

        case ST_ACK:
            /* This clock sampled the ACK/NACK value latched above. */
            if (g_after == AFTER_WDATA) {
                slave_begin_write(slave_device_by_address((uint8_t)(g_frame.address >> 1)));
                g_state = ST_WDATA;
            } else if (g_after == AFTER_WDATA_CONTINUE) {
                g_state = ST_WDATA;
            } else if (g_after == AFTER_RDATA) {
                g_read_byte =
                    slave_read_byte(slave_device_by_address((uint8_t)(g_frame.address >> 1)));
                g_bit = 0U;
                g_state = ST_RDATA;
            } else {
                g_state = ST_IGNORE;
            }
            break;

        case ST_WDATA:
            g_shift = (uint8_t)((g_shift << 1) | (g_sda_master_level & 1U));
            if (++g_bit < 8U) {
                break;
            }
            frame_note_byte(g_shift);
            slave_write_byte(slave_device_by_address((uint8_t)(g_frame.address >> 1)), g_shift);
            g_bit = 0U;
            g_slave_drive = 0U;
            g_after = AFTER_WDATA_CONTINUE;
            g_state = ST_ACK;
            break;

        case ST_RDATA:
            /* The master samples this value after the clock, so the eighth bit must
             * stay stable until the ACK clock releases the line. */
            g_slave_drive = (uint8_t)((g_read_byte >> (7U - g_bit)) & 1U);
            if (++g_bit == 8U) {
                g_bit = 0U;
                g_state = ST_RACK;
            }
            break;

        case ST_RACK:
            g_slave_drive = 1U;
            if ((g_sda_master_level & 1U) == 0U) {
                g_read_byte =
                    slave_read_byte(slave_device_by_address((uint8_t)(g_frame.address >> 1)));
                g_bit = 0U;
                g_state = ST_RDATA;
            } else {
                g_slave_drive = 1U;
                g_state = ST_IGNORE;
            }
            break;

        case ST_IGNORE:
        case ST_IDLE:
        default:
            break;
    }
}

/* --- fake `xy_hal_gpio_*` seam ----------------------------------------- */

xy_hal_error_t xy_hal_gpio_write(xy_hal_gpio_port_t port, uint8_t pin, uint8_t value) {
    const uint8_t tag = port_tag(port);
    const uint8_t level = value ? 1U : 0U;
    const int is_scl = tag == BUS_SCL_PORT && pin == BUS_SCL_PIN;
    const int is_sda = tag == BUS_SDA_PORT && pin == BUS_SDA_PIN;

    if (is_sda) {
        if (g_sda_master_driving) {
            const uint8_t previous = g_sda_master_level;

            if (g_scl_level != 0U && previous != level) {
                if (level == 0U) {
                    slave_on_start();
                } else {
                    slave_on_stop();
                }
            }
            g_sda_master_level = level;
        }
        return XY_HAL_OK;
    }
    if (is_scl) {
        const uint8_t previous = g_scl_level;

        g_scl_level = level;
        if (previous == 0U && level == 1U) {
            slave_on_clock();
        }
        return XY_HAL_OK;
    }
    return XY_HAL_OK;
}

int32_t xy_hal_gpio_read(xy_hal_gpio_port_t port, uint8_t pin) {
    if (port_tag(port) == BUS_SDA_PORT && pin == BUS_SDA_PIN) {
        return (int32_t)g_slave_drive;
    }
    return 1;
}

void xy_test_fake_rcc_gpioc_enable(void) {
    ++g_rcc_gpioc;
}

void xy_test_fake_rcc_gpiod_enable(void) {
    ++g_rcc_gpiod;
}

void xy_hal_delay_ms(uint32_t ms) {
    g_delay_ms += ms;
}

/* The canonical device bus helper TU also carries SPI/UART/GPIO helpers that
 * this I2C slice never reaches; keep them failing closed instead of linking the
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

/* --- fixture helpers --------------------------------------------------- */

void setUp(void) {
    memset(g_config, 0, sizeof(g_config));
    memset(g_config_count, 0, sizeof(g_config_count));
    memset(g_frames, 0, sizeof(g_frames));
    g_rcc_gpioc = 0U;
    g_rcc_gpiod = 0U;
    g_delay_ms = 0U;
    g_scl_level = 0U;
    g_sda_master_level = 1U;
    g_sda_master_driving = 1;
    g_slave_drive = 1U;
    slave_reset_devices();
}

void tearDown(void) {}

/* A HT10 measurement frame: status 0x18, humidity raw 0x80000, temperature raw
 * 0x80000, i.e. exactly 50.00 %RH and 50.00 degC after the driver's fixed-point
 * conversion. */
static void aht10_prepare_frame(slave_device_t* device, uint8_t status, uint8_t third_byte,
                                uint8_t fourth, uint8_t fifth) {
    device->frame[0] = status;
    device->frame[1] = 0x80U;
    device->frame[2] = 0x00U;
    device->frame[3] = third_byte;
    device->frame[4] = fourth;
    device->frame[5] = fifth;
    device->frame_length = 6U;
    device->frame_index = 0U;
}

static slave_device_t* attach_icm20608(uint8_t address) {
    slave_device_t* device = slave_attach(address, DEV_REGISTER);

    TEST_ASSERT_NOT_NULL(device);
    device->regs[XY_ICM20608_REG_WHO_AM_I] = XY_ICM20608_WHO_AM_I;
    slave_add_read_only_register(device, XY_ICM20608_REG_WHO_AM_I);
    return device;
}

static slave_device_t* attach_aht10(void) {
    slave_device_t* device = slave_attach(XY_AHT10_DEFAULT_ADDRESS, DEV_COMMAND);

    TEST_ASSERT_NOT_NULL(device);
    aht10_prepare_frame(device, 0x18U, 0x08U, 0x00U, 0x00U);
    return device;
}

static const frame_t* frame_at(unsigned int index) {
    TEST_ASSERT_LESS_THAN_UINT(g_frame_count, index);
    return &g_frames[index];
}

static void assert_frame(const frame_t* frame, uint8_t address_byte, uint8_t rw,
                         const uint8_t* bytes, uint8_t length) {
    TEST_ASSERT_EQUAL_UINT8(address_byte, frame->address);
    TEST_ASSERT_EQUAL_UINT8(rw, frame->rw);
    TEST_ASSERT_EQUAL_UINT8(length, frame->length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(bytes, frame->bytes, length);
}

/* --- transport owner --------------------------------------------------- */

static void test_board_bus_is_the_icm20608_and_aht10_transport(void) {
    void* bus = pandora_soft_i2c_init();

    TEST_ASSERT_NOT_NULL(bus);
    TEST_ASSERT_EQUAL_PTR(pandora_soft_i2c4_init(), bus); /* legacy alias, same bus */
    TEST_ASSERT_EQUAL_UINT(2U, g_rcc_gpioc);
    TEST_ASSERT_EQUAL_UINT(2U, g_rcc_gpiod);
    TEST_ASSERT_EQUAL_UINT(2U, g_config_count[BUS_SCL_PORT][BUS_SCL_PIN]);
    TEST_ASSERT_EQUAL_UINT(2U, g_config_count[BUS_SDA_PORT][BUS_SDA_PIN]);
    TEST_ASSERT_EQUAL_INT(XY_HAL_GPIO_MODE_OUTPUT, g_config[BUS_SCL_PORT][BUS_SCL_PIN].mode);
    TEST_ASSERT_EQUAL_INT(XY_HAL_GPIO_OTYPE_OD, g_config[BUS_SCL_PORT][BUS_SCL_PIN].otype);
    TEST_ASSERT_EQUAL_INT(XY_HAL_GPIO_PULL_UP, g_config[BUS_SCL_PORT][BUS_SCL_PIN].pull);
    TEST_ASSERT_EQUAL_UINT(0U, g_config_count[PORT_C][0]); /* I2C3 pins stay untouched */
    TEST_ASSERT_EQUAL_UINT(0U, g_config_count[PORT_C][4]);
}

/* --- AHT10 through the whole stack ------------------------------------- */

static void test_aht10_init_programs_the_calibration_command_on_the_wire(void) {
    static const uint8_t expected[] = {0xE1U, 0x08U, 0x00U};
    slave_device_t* slave = attach_aht10();
    void* bus = pandora_soft_i2c_init();
    xy_aht10_t aht10;

    memset(&aht10, 0, sizeof(aht10));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_aht10_init(&aht10, bus, XY_AHT10_DEFAULT_ADDRESS));
    TEST_ASSERT_EQUAL_UINT(1U, slave->init_commands);
    TEST_ASSERT_EQUAL_UINT(10U, g_delay_ms);
    TEST_ASSERT_EQUAL_UINT(1U, g_frame_count);
    assert_frame(frame_at(0U), (uint8_t)(XY_AHT10_DEFAULT_ADDRESS << 1), 0U, expected,
                 sizeof(expected));

    TEST_ASSERT_EQUAL_UINT8(1U, aht10.initialized);
    TEST_ASSERT_EQUAL_PTR(bus, aht10.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_UINT16(XY_AHT10_DEFAULT_ADDRESS, aht10.i2c_dev.dev_addr);
    TEST_ASSERT_TRUE(aht10.i2c_dev.base.initialized);
}

static void test_aht10_read_decodes_the_measured_frame(void) {
    static const uint8_t measure[] = {0xACU, 0x33U, 0x00U};
    slave_device_t* slave = attach_aht10();
    void* bus = pandora_soft_i2c_init();
    xy_aht10_t aht10;
    xy_aht10_data_t data;

    memset(&aht10, 0, sizeof(aht10));
    memset(&data, 0, sizeof(data));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_aht10_init(&aht10, bus, XY_AHT10_DEFAULT_ADDRESS));
    g_delay_ms = 0U;
    g_frame_count = 0U;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_aht10_read(&aht10, &data));

    TEST_ASSERT_EQUAL_UINT(1U, slave->measure_commands);
    TEST_ASSERT_EQUAL_UINT(80U, g_delay_ms);
    TEST_ASSERT_EQUAL_UINT(2U, g_frame_count);
    assert_frame(frame_at(0U), (uint8_t)(XY_AHT10_DEFAULT_ADDRESS << 1), 0U, measure,
                 sizeof(measure));
    /* Read frame: address+R then the six data bytes the slave served. */
    TEST_ASSERT_EQUAL_UINT8((uint8_t)((XY_AHT10_DEFAULT_ADDRESS << 1) | 1U), frame_at(1U)->address);
    TEST_ASSERT_EQUAL_UINT8(1U, frame_at(1U)->rw);
    TEST_ASSERT_EQUAL_UINT8(6U, frame_at(1U)->length);

    TEST_ASSERT_EQUAL_INT32(5000, data.temperature_centi_c);
    TEST_ASSERT_EQUAL_UINT32(5000U, data.humidity_centi_pct);
    TEST_ASSERT_EQUAL_INT32(data.temperature_centi_c, aht10.data.temperature_centi_c);
    TEST_ASSERT_EQUAL_UINT32(data.humidity_centi_pct, aht10.data.humidity_centi_pct);
}

static void test_aht10_read_reports_busy_and_preserves_the_cached_sample(void) {
    slave_device_t* slave = attach_aht10();
    void* bus = pandora_soft_i2c_init();
    xy_aht10_t aht10;
    xy_aht10_data_t data;

    memset(&aht10, 0, sizeof(aht10));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_aht10_init(&aht10, bus, XY_AHT10_DEFAULT_ADDRESS));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_aht10_read(&aht10, &data));

    aht10_prepare_frame(slave, 0x98U, 0x08U, 0x00U, 0x00U); /* busy bit set */
    data.temperature_centi_c = 1234;
    data.humidity_centi_pct = 5678U;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_BUSY, xy_aht10_read(&aht10, &data));
    TEST_ASSERT_EQUAL_INT32(1234, data.temperature_centi_c);
    TEST_ASSERT_EQUAL_UINT32(5678U, data.humidity_centi_pct);
    TEST_ASSERT_EQUAL_INT32(5000, aht10.data.temperature_centi_c);
    TEST_ASSERT_EQUAL_UINT32(5000U, aht10.data.humidity_centi_pct);
}

static void test_aht10_init_fails_closed_when_the_bus_nacks(void) {
    void* bus = pandora_soft_i2c_init();
    xy_aht10_t aht10;

    memset(&aht10, 0xFF, sizeof(aht10));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR, xy_aht10_init(&aht10, bus, XY_AHT10_DEFAULT_ADDRESS));
    TEST_ASSERT_EQUAL_UINT8(0U, aht10.initialized);
    TEST_ASSERT_FALSE(aht10.i2c_dev.base.initialized);
    TEST_ASSERT_EQUAL_UINT(1U, g_frame_count);
    TEST_ASSERT_EQUAL_UINT8(0U, frame_at(0U)->length);
}

/* --- ICM20608 through the whole stack ---------------------------------- */

static void test_icm20608_init_binds_the_identity_read_over_the_board_bus(void) {
    static const uint8_t select_identity[] = {XY_ICM20608_REG_WHO_AM_I};
    slave_device_t* slave = attach_icm20608(XY_ICM20608_ADDR_DEFAULT);
    void* bus = pandora_soft_i2c_init();
    xy_icm20608_t icm;

    memset(&icm, 0, sizeof(icm));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_icm20608_init_i2c(&icm, bus, XY_ICM20608_ADDR_DEFAULT));

    assert_frame(frame_at(0U), (uint8_t)(XY_ICM20608_ADDR_DEFAULT << 1), 0U, select_identity,
                 sizeof(select_identity));
    TEST_ASSERT_EQUAL_UINT8((uint8_t)((XY_ICM20608_ADDR_DEFAULT << 1) | 1U), frame_at(1U)->address);
    TEST_ASSERT_EQUAL_UINT8(XY_ICM20608_WHO_AM_I, frame_at(1U)->bytes[0]);
    TEST_ASSERT_EQUAL_UINT(16U, slave->write_phases);
    TEST_ASSERT_EQUAL_UINT(8U, slave->read_phases);

    TEST_ASSERT_EQUAL_UINT8(1U, icm.initialized);
    TEST_ASSERT_EQUAL_UINT8(1U, icm.configuration_synchronized);
    TEST_ASSERT_EQUAL_PTR(bus, icm.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_HEX16(XY_ICM20608_ADDR_DEFAULT, icm.address);
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_TRANSPORT_I2C, icm.transport);
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_ACCEL_RANGE_4G, icm.accel_range);
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_GYRO_RANGE_500DPS, icm.gyro_range);
    TEST_ASSERT_EQUAL_UINT16(100U, icm.odr_hz);
}

static void test_icm20608_init_rejects_a_foreign_identity_without_configuring(void) {
    slave_device_t* slave = attach_icm20608(XY_ICM20608_ADDR_DEFAULT);
    void* bus = pandora_soft_i2c_init();
    xy_icm20608_t icm;

    slave->regs[XY_ICM20608_REG_WHO_AM_I] = 0x00U;
    memset(&icm, 0, sizeof(icm));

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_NOT_FOUND,
                          xy_icm20608_init_i2c(&icm, bus, XY_ICM20608_ADDR_DEFAULT));
    TEST_ASSERT_EQUAL_UINT8(0U, icm.initialized);
    TEST_ASSERT_EQUAL_UINT8(0U, slave->regs[XY_ICM20608_REG_PWR_MGMT_1]);
    TEST_ASSERT_EQUAL_UINT(2U, g_frame_count); /* identity select + identity read only */
    TEST_ASSERT_EQUAL_UINT(0U, g_delay_ms);
}

static void test_icm20608_init_writes_the_documented_configuration_sequence(void) {
    static const uint8_t writes[][2] = {
        {XY_ICM20608_REG_PWR_MGMT_1, 0x80U},   {XY_ICM20608_REG_PWR_MGMT_1, 0x01U},
        {XY_ICM20608_REG_PWR_MGMT_2, 0x00U},   {XY_ICM20608_REG_GYRO_CONFIG, 0x08U},
        {XY_ICM20608_REG_ACCEL_CONFIG, 0x08U}, {XY_ICM20608_REG_SMPLRT_DIV, 0x09U},
        {XY_ICM20608_REG_CONFIG, 0x04U},       {XY_ICM20608_REG_ACCEL_CONFIG2, 0x04U},
    };
    static const uint8_t verify_order[] = {
        XY_ICM20608_REG_PWR_MGMT_1,    XY_ICM20608_REG_PWR_MGMT_2, XY_ICM20608_REG_GYRO_CONFIG,
        XY_ICM20608_REG_ACCEL_CONFIG,  XY_ICM20608_REG_SMPLRT_DIV, XY_ICM20608_REG_CONFIG,
        XY_ICM20608_REG_ACCEL_CONFIG2,
    };
    slave_device_t* slave = attach_icm20608(XY_ICM20608_ADDR_DEFAULT);
    void* bus = pandora_soft_i2c_init();
    xy_icm20608_t icm;

    memset(&icm, 0, sizeof(icm));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_icm20608_init_i2c(&icm, bus, XY_ICM20608_ADDR_DEFAULT));

    /* Frames 0/1 are the identity select and read; frames 2..9 are the
     * configuration writes. */
    for (unsigned int index = 0U; index < sizeof(writes) / sizeof(writes[0]); ++index) {
        const frame_t* frame = frame_at(index + 2U);

        TEST_ASSERT_EQUAL_UINT8((uint8_t)(XY_ICM20608_ADDR_DEFAULT << 1), frame->address);
        TEST_ASSERT_EQUAL_UINT8(0U, frame->rw);
        TEST_ASSERT_EQUAL_UINT8(2U, frame->length);
        TEST_ASSERT_EQUAL_UINT8(writes[index][0], frame->bytes[0]);
        TEST_ASSERT_EQUAL_UINT8(writes[index][1], frame->bytes[1]);
    }
    /* Then one select+read pair per register, in the documented verify order. */
    for (unsigned int index = 0U; index < sizeof(verify_order); ++index) {
        const frame_t* select = frame_at(10U + index * 2U);
        const frame_t* readback = frame_at(11U + index * 2U);

        TEST_ASSERT_EQUAL_UINT8(1U, select->length);
        TEST_ASSERT_EQUAL_UINT8(verify_order[index], select->bytes[0]);
        TEST_ASSERT_EQUAL_UINT8(1U, readback->rw);
        TEST_ASSERT_EQUAL_UINT8(1U, readback->length);
        TEST_ASSERT_EQUAL_UINT8(slave->regs[verify_order[index]], readback->bytes[0]);
    }
    TEST_ASSERT_EQUAL_UINT(24U, g_frame_count);
    TEST_ASSERT_EQUAL_UINT(100U, g_delay_ms);
}

static void test_icm20608_init_fails_when_the_readback_disagrees(void) {
    slave_device_t* slave = attach_icm20608(XY_ICM20608_ADDR_DEFAULT);
    void* bus = pandora_soft_i2c_init();
    xy_icm20608_t icm;

    /* Sticky register: the device keeps reporting the power-on default. */
    slave->regs[XY_ICM20608_REG_ACCEL_CONFIG2] = 0x00U;
    slave_add_read_only_register(slave, XY_ICM20608_REG_ACCEL_CONFIG2);
    memset(&icm, 0, sizeof(icm));

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_FAIL,
                          xy_icm20608_init_i2c(&icm, bus, XY_ICM20608_ADDR_DEFAULT));
    TEST_ASSERT_EQUAL_UINT8(0U, icm.initialized);
}

static void test_icm20608_init_nacks_the_absent_alternate_address(void) {
    slave_device_t* slave = attach_icm20608(XY_ICM20608_ADDR_DEFAULT);
    void* bus = pandora_soft_i2c_init();
    xy_icm20608_t icm;

    memset(&icm, 0, sizeof(icm));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR,
                          xy_icm20608_init_i2c(&icm, bus, XY_ICM20608_ADDR_ALT));
    TEST_ASSERT_EQUAL_UINT(0U, slave->write_phases);
    TEST_ASSERT_EQUAL_UINT(1U, g_frame_count);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)(XY_ICM20608_ADDR_ALT << 1), frame_at(0U)->address);
    TEST_ASSERT_EQUAL_UINT8(0U, frame_at(0U)->length);
}

static void icm20608_program_sample_registers(slave_device_t* slave) {
    static const uint8_t sample[14] = {
        0x40U, 0x00U, /* accel X = +16384 -> +2000 mg at 4 g */
        0x00U, 0x00U, /* accel Y = 0 */
        0xC0U, 0x00U, /* accel Z = -16384 -> -2000 mg */
        0x0CU, 0xC4U, /* temperature raw 3268 -> 35.00 degC */
        0x20U, 0x00U, /* gyro X = +8192 -> +125000 mdps at 500 dps */
        0x00U, 0x00U, /* gyro Y = 0 */
        0xE0U, 0x00U, /* gyro Z = -8192 -> -125000 mdps */
    };

    memcpy(&slave->regs[XY_ICM20608_REG_ACCEL_XOUT_H], sample, sizeof(sample));
}

static void test_icm20608_read_sample_decodes_the_fourteen_byte_block(void) {
    static const uint8_t select_block[] = {XY_ICM20608_REG_ACCEL_XOUT_H};
    slave_device_t* slave = attach_icm20608(XY_ICM20608_ADDR_DEFAULT);
    void* bus = pandora_soft_i2c_init();
    xy_icm20608_t icm;
    xy_icm20608_sample_t sample;

    memset(&icm, 0, sizeof(icm));
    memset(&sample, 0, sizeof(sample));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_icm20608_init_i2c(&icm, bus, XY_ICM20608_ADDR_DEFAULT));
    icm20608_program_sample_registers(slave);
    g_frame_count = 0U;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_icm20608_read_sample(&icm, &sample));

    TEST_ASSERT_EQUAL_UINT(2U, g_frame_count);
    assert_frame(frame_at(0U), (uint8_t)(XY_ICM20608_ADDR_DEFAULT << 1), 0U, select_block,
                 sizeof(select_block));
    TEST_ASSERT_EQUAL_UINT8(14U, frame_at(1U)->length);
    TEST_ASSERT_EQUAL_INT32(2000, sample.accel.x_mg);
    TEST_ASSERT_EQUAL_INT32(0, sample.accel.y_mg);
    TEST_ASSERT_EQUAL_INT32(-2000, sample.accel.z_mg);
    TEST_ASSERT_EQUAL_INT32(3500, sample.temperature_centi_c);
    TEST_ASSERT_EQUAL_INT32(125000, sample.gyro.x_mdps);
    TEST_ASSERT_EQUAL_INT32(0, sample.gyro.y_mdps);
    TEST_ASSERT_EQUAL_INT32(-125000, sample.gyro.z_mdps);
    TEST_ASSERT_EQUAL_INT32(sample.accel.x_mg, icm.accel.x_mg);
    TEST_ASSERT_EQUAL_INT32(sample.temperature_centi_c, icm.temperature_centi_c);
}

static void test_icm20608_read_sample_preserves_the_cache_on_a_bus_fault(void) {
    slave_device_t* slave = attach_icm20608(XY_ICM20608_ADDR_DEFAULT);
    void* bus = pandora_soft_i2c_init();
    xy_icm20608_t icm;
    xy_icm20608_sample_t sample;

    memset(&icm, 0, sizeof(icm));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_icm20608_init_i2c(&icm, bus, XY_ICM20608_ADDR_DEFAULT));
    icm20608_program_sample_registers(slave);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_icm20608_read_sample(&icm, &sample));

    slave->present = 0U;
    sample.accel.x_mg = 7;
    sample.accel.y_mg = 8;
    sample.accel.z_mg = 9;
    sample.gyro.x_mdps = 11;
    sample.gyro.y_mdps = 12;
    sample.gyro.z_mdps = 13;
    sample.temperature_centi_c = 14;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR, xy_icm20608_read_sample(&icm, &sample));
    TEST_ASSERT_EQUAL_INT32(7, sample.accel.x_mg);
    TEST_ASSERT_EQUAL_INT32(8, sample.accel.y_mg);
    TEST_ASSERT_EQUAL_INT32(9, sample.accel.z_mg);
    TEST_ASSERT_EQUAL_INT32(11, sample.gyro.x_mdps);
    TEST_ASSERT_EQUAL_INT32(12, sample.gyro.y_mdps);
    TEST_ASSERT_EQUAL_INT32(13, sample.gyro.z_mdps);
    TEST_ASSERT_EQUAL_INT32(14, sample.temperature_centi_c);
    TEST_ASSERT_EQUAL_INT32(2000, icm.accel.x_mg);
    TEST_ASSERT_EQUAL_INT32(3500, icm.temperature_centi_c);
}

static void test_icm20608_verify_configuration_rereads_through_the_transport(void) {
    slave_device_t* slave = attach_icm20608(XY_ICM20608_ADDR_DEFAULT);
    void* bus = pandora_soft_i2c_init();
    xy_icm20608_t icm;

    memset(&icm, 0, sizeof(icm));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_icm20608_init_i2c(&icm, bus, XY_ICM20608_ADDR_DEFAULT));
    g_frame_count = 0U;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_icm20608_verify_configuration(&icm));
    TEST_ASSERT_EQUAL_UINT(18U, g_frame_count); /* nine select+read pairs */
    TEST_ASSERT_EQUAL_UINT8(XY_ICM20608_REG_WHO_AM_I, frame_at(0U)->bytes[0]);

    slave->regs[XY_ICM20608_REG_SMPLRT_DIV] = 0x7FU;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_FAIL, xy_icm20608_verify_configuration(&icm));
    TEST_ASSERT_EQUAL_UINT8(0U, icm.configuration_synchronized);
}

/* --- two devices sharing the board-owned bus --------------------------- */

static void test_aht10_and_icm20608_share_the_bus_without_crosstalk(void) {
    slave_device_t* aht10_slave = attach_aht10();
    slave_device_t* icm_slave = attach_icm20608(XY_ICM20608_ADDR_DEFAULT);
    void* bus = pandora_soft_i2c_init();
    xy_aht10_t aht10;
    xy_aht10_data_t data;
    xy_icm20608_t icm;
    xy_icm20608_sample_t sample;

    memset(&aht10, 0, sizeof(aht10));
    memset(&icm, 0, sizeof(icm));
    memset(&data, 0, sizeof(data));
    memset(&sample, 0, sizeof(sample));
    icm20608_program_sample_registers(icm_slave);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_aht10_init(&aht10, bus, XY_AHT10_DEFAULT_ADDRESS));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_aht10_read(&aht10, &data));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_icm20608_init_i2c(&icm, bus, XY_ICM20608_ADDR_DEFAULT));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_icm20608_read_sample(&icm, &sample));

    TEST_ASSERT_EQUAL_UINT(1U, aht10_slave->init_commands);
    TEST_ASSERT_EQUAL_UINT(1U, aht10_slave->measure_commands);
    TEST_ASSERT_EQUAL_UINT(0U, aht10_slave->reg_pointer);
    TEST_ASSERT_EQUAL_UINT8(XY_ICM20608_WHO_AM_I, icm_slave->regs[XY_ICM20608_REG_WHO_AM_I]);
    TEST_ASSERT_EQUAL_INT32(5000, data.temperature_centi_c);
    TEST_ASSERT_EQUAL_INT32(2000, sample.accel.x_mg);

    /* Every committed frame targets exactly one of the two owned addresses. */
    TEST_ASSERT_GREATER_THAN_UINT(0U, g_frame_count);
    for (unsigned int index = 0U; index < g_frame_count; ++index) {
        const uint8_t address = (uint8_t)(g_frames[index].address >> 1);

        TEST_ASSERT_TRUE(address == XY_AHT10_DEFAULT_ADDRESS ||
                         address == XY_ICM20608_ADDR_DEFAULT);
    }

    /* An address nobody answers still fails closed and touches no device. */
    {
        static const uint8_t payload[] = {XY_ICM20608_REG_WHO_AM_I};
        const unsigned int icm_writes = icm_slave->write_phases;

        TEST_ASSERT_EQUAL_INT(
            XY_HAL_ERROR_IO, xy_hal_i2c_master_transmit(bus, 0x69U, payload, sizeof(payload), 10U));
        TEST_ASSERT_EQUAL_UINT(icm_writes, icm_slave->write_phases);
        TEST_ASSERT_EQUAL_UINT(2U, aht10_slave->write_phases);
    }
}

/* --- runner ------------------------------------------------------------ */

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_board_bus_is_the_icm20608_and_aht10_transport);
    RUN_TEST(test_aht10_init_programs_the_calibration_command_on_the_wire);
    RUN_TEST(test_aht10_read_decodes_the_measured_frame);
    RUN_TEST(test_aht10_read_reports_busy_and_preserves_the_cached_sample);
    RUN_TEST(test_aht10_init_fails_closed_when_the_bus_nacks);
    RUN_TEST(test_icm20608_init_binds_the_identity_read_over_the_board_bus);
    RUN_TEST(test_icm20608_init_rejects_a_foreign_identity_without_configuring);
    RUN_TEST(test_icm20608_init_writes_the_documented_configuration_sequence);
    RUN_TEST(test_icm20608_init_fails_when_the_readback_disagrees);
    RUN_TEST(test_icm20608_init_nacks_the_absent_alternate_address);
    RUN_TEST(test_icm20608_read_sample_decodes_the_fourteen_byte_block);
    RUN_TEST(test_icm20608_read_sample_preserves_the_cache_on_a_bus_fault);
    RUN_TEST(test_icm20608_verify_configuration_rereads_through_the_transport);
    RUN_TEST(test_aht10_and_icm20608_share_the_bus_without_crosstalk);
    return UNITY_END();
}
