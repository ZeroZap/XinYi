/**
 * @file test_pandora_sensor_owners.c
 * @brief Pandora board-owned canonical sensor owners (AP3216C / ICM20608).
 *
 * `sensor_ap3216c_main.c` and `sensor_icm20608_main.c` are the two validation
 * images that own the onboard light/proximity and IMU devices. Before this
 * slice they owned those devices through the legacy `sensor_*` compatibility
 * wrappers (`ap3216c_create_light`, `icm20608_create_accel`) and the ICM image
 * even reached into `icm20608_priv_t` to find the canonical driver instance.
 *
 * Both images now own a canonical Device driver instance directly through
 * `pandora_ap3216c_owner.c` / `pandora_icm20608_owner.c`. This fixture pins the
 * board-owner contract against a host-side device model seated on the canonical
 * `xy_hal_i2c_*` seam: two I2C register files (0x1E / 0x68) that ACK only while
 * the modelled device is present, and a frame log that records every committed
 * transfer.
 *
 * Evidence boundary: host/compile contract only. No timing, bus-electrical,
 * clock-stretching, real-silicon, or board-runtime behaviour is claimed here.
 */

#include "unity.h"

#include "pandora_ap3216c_owner.h"
#include "pandora_icm20608_owner.h"

#include "xy_hal_delay.h"
#include "xy_hal_gpio.h"
#include "xy_hal_i2c.h"
#include "xy_hal_spi.h"
#include "xy_hal_sys.h"
#include "xy_hal_uart.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* --- fixture state ------------------------------------------------------ */

#define REG_SPACE 128U
#define FRAME_CAPACITY 48U
#define FRAME_BYTES 16U
#define AP_DEVICE 0U
#define ICM_DEVICE 1U

typedef struct {
    uint16_t address;
    uint32_t timeout;
    uint8_t read_not_write;
    uint8_t length;
    uint8_t bytes[FRAME_BYTES];
} frame_t;

static uint8_t g_regs[2][REG_SPACE];
static uint8_t g_reg_pointer[2];
static int g_present[2];
static unsigned int g_write_frames;
static unsigned int g_read_frames;
static unsigned int g_delay_ms;

static frame_t g_frames[FRAME_CAPACITY];
static unsigned int g_frame_count;

/* The board handle is opaque to the owners; this fixture never inspects it. */
static uint8_t g_ap_bus;
static uint8_t g_icm_bus;

static int device_slot(uint16_t address) {
    if (address == XY_AP3216C_DEFAULT_ADDRESS) {
        return (int)AP_DEVICE;
    }
    if (address == XY_ICM20608_ADDR_DEFAULT) {
        return (int)ICM_DEVICE;
    }
    return -1;
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

/* --- canonical I2C HAL seam -------------------------------------------- */

xy_hal_error_t xy_hal_i2c_master_transmit(void* i2c, uint16_t dev_addr, const uint8_t* data,
                                          size_t len, uint32_t timeout) {
    int slot = device_slot(dev_addr);

    frame_record(dev_addr, timeout, 0U, data, len);
    if (i2c == NULL || slot < 0 || !g_present[slot]) {
        return XY_HAL_ERROR_IO;
    }
    if (data == NULL || len == 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    g_reg_pointer[slot] = data[0];
    if (len > 1U) {
        if ((size_t)g_reg_pointer[slot] + (len - 1U) > REG_SPACE) {
            return XY_HAL_ERROR_INVALID_PARAM;
        }
        memcpy(&g_regs[slot][g_reg_pointer[slot]], &data[1], len - 1U);
        ++g_write_frames; /* payload writes only; a bare select is not a write */
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_master_receive(void* i2c, uint16_t dev_addr, uint8_t* data, size_t len,
                                         uint32_t timeout) {
    int slot = device_slot(dev_addr);

    frame_record(dev_addr, timeout, 1U, NULL, len);
    if (i2c == NULL || slot < 0 || !g_present[slot]) {
        return XY_HAL_ERROR_IO;
    }
    if (data == NULL || len == 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if ((size_t)g_reg_pointer[slot] + len > REG_SPACE) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    memcpy(data, &g_regs[slot][g_reg_pointer[slot]], len);
    g_reg_pointer[slot] = (uint8_t)(g_reg_pointer[slot] + len);
    ++g_read_frames;
    return XY_HAL_OK;
}

void xy_hal_delay_ms(uint32_t ms) {
    g_delay_ms += ms;
}

/* `xy_device_timing.h` reads the canonical SYS tick for its OSAL-free path. */
uint32_t xy_hal_sys_get_tick_count(void) {
    return g_delay_ms;
}

uint32_t xy_hal_sys_get_tick_freq(void) {
    return 1000U;
}

/* The shared Device bus-helper TU also carries GPIO/SPI/UART helpers that this
 * I2C slice never reaches; keep them failing closed instead of linking the
 * whole PC HAL. */
xy_hal_error_t xy_hal_gpio_init(xy_hal_gpio_port_t port, uint8_t pin,
                                const xy_hal_gpio_config_t* config) {
    (void)port;
    (void)pin;
    (void)config;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

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

xy_hal_error_t xy_hal_gpio_toggle(xy_hal_gpio_port_t port, uint8_t pin) {
    (void)port;
    (void)pin;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

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

/* --- fixture helpers --------------------------------------------------- */

void setUp(void) {
    memset(g_regs, 0, sizeof(g_regs));
    memset(g_reg_pointer, 0, sizeof(g_reg_pointer));
    memset(g_frames, 0, sizeof(g_frames));
    g_present[AP_DEVICE] = 1;
    g_present[ICM_DEVICE] = 1;
    g_write_frames = 0U;
    g_read_frames = 0U;
    g_delay_ms = 0U;
    g_frame_count = 0U;
    /* A real ICM20608 always answers WHO_AM_I; the model must too. */
    g_regs[ICM_DEVICE][XY_ICM20608_REG_WHO_AM_I] = XY_ICM20608_WHO_AM_I;
}

void tearDown(void) {}

static const frame_t* frame_at(unsigned int index) {
    TEST_ASSERT_LESS_THAN_UINT(g_frame_count, index);
    return &g_frames[index];
}

static void assert_register_write(const frame_t* frame, uint16_t address, uint8_t reg,
                                  uint8_t value) {
    const uint8_t expected[] = {reg, value};

    TEST_ASSERT_EQUAL_UINT16(address, frame->address);
    TEST_ASSERT_EQUAL_UINT8(0U, frame->read_not_write);
    TEST_ASSERT_EQUAL_UINT8(2U, frame->length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, frame->bytes, sizeof(expected));
}

static void assert_register_select(const frame_t* frame, uint16_t address, uint8_t reg) {
    TEST_ASSERT_EQUAL_UINT16(address, frame->address);
    TEST_ASSERT_EQUAL_UINT8(0U, frame->read_not_write);
    TEST_ASSERT_EQUAL_UINT8(1U, frame->length);
    TEST_ASSERT_EQUAL_UINT8(reg, frame->bytes[0]);
}

/* Preload the AP3216C raws so a sample has deterministic, non-truncating data. */
static void program_ap3216c_raws(void) {
    g_regs[AP_DEVICE][XY_AP3216C_REG_IR_DATA_L] = 0xCDU;       /* IR low  */
    g_regs[AP_DEVICE][XY_AP3216C_REG_IR_DATA_L + 1U] = 0x02U;  /* IR high */
    g_regs[AP_DEVICE][XY_AP3216C_REG_ALS_DATA_L] = 0x34U;      /* ALS low */
    g_regs[AP_DEVICE][XY_AP3216C_REG_ALS_DATA_L + 1U] = 0x12U; /* ALS high */
    g_regs[AP_DEVICE][XY_AP3216C_REG_PS_DATA_L] = 0x05U;       /* PS low  */
    g_regs[AP_DEVICE][XY_AP3216C_REG_PS_DATA_L + 1U] = 0x03U;  /* PS high */
}

/* accel x=0x1000 y=0xF000 z=0x0400, temp=0x0800, gyro x=0x0100 y=0xFF00 z=0. */
static void program_icm20608_raws(void) {
    static const uint8_t raws[PANDORA_ICM20608_RAW_LEN] = {
        0x10U, 0x00U, 0xF0U, 0x00U, 0x04U, 0x00U, 0x08U,
        0x00U, 0x01U, 0x00U, 0xFFU, 0x00U, 0x00U, 0x00U,
    };

    memcpy(&g_regs[ICM_DEVICE][XY_ICM20608_REG_ACCEL_XOUT_H], raws, sizeof(raws));
}

/* --- AP3216C board owner ----------------------------------------------- */

static void test_ap3216c_owner_init_binds_the_canonical_driver(void) {
    pandora_ap3216c_owner_t owner;

    memset(&owner, 0, sizeof(owner));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, pandora_ap3216c_owner_init(&owner, &g_ap_bus));

    TEST_ASSERT_EQUAL_PTR(&g_ap_bus, owner.device.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_UINT16(XY_AP3216C_DEFAULT_ADDRESS, owner.device.i2c_dev.dev_addr);
    TEST_ASSERT_EQUAL_UINT32(100U, owner.device.i2c_dev.timeout);
    TEST_ASSERT_EQUAL_UINT8(XY_AP3216C_MODE_ALS_PS, owner.device.mode);
    TEST_ASSERT_EQUAL_UINT8(1U, owner.device.initialized);

    /* Reset, settle, mode, settle. */
    TEST_ASSERT_EQUAL_UINT(2U, g_frame_count);
    assert_register_write(frame_at(0U), XY_AP3216C_DEFAULT_ADDRESS, XY_AP3216C_REG_SYSTEM_CONFIG,
                          XY_AP3216C_MODE_RESET);
    assert_register_write(frame_at(1U), XY_AP3216C_DEFAULT_ADDRESS, XY_AP3216C_REG_SYSTEM_CONFIG,
                          XY_AP3216C_MODE_ALS_PS);
    TEST_ASSERT_EQUAL_UINT(100U, g_delay_ms);
    TEST_ASSERT_EQUAL_UINT8(XY_AP3216C_MODE_ALS_PS,
                            g_regs[AP_DEVICE][XY_AP3216C_REG_SYSTEM_CONFIG]);
}

static void test_ap3216c_owner_init_rejects_missing_arguments(void) {
    pandora_ap3216c_owner_t owner;
    pandora_ap3216c_owner_t before;

    memset(&owner, 0xAB, sizeof(owner));
    before = owner;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, pandora_ap3216c_owner_init(NULL, &g_ap_bus));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, pandora_ap3216c_owner_init(&owner, NULL));

    TEST_ASSERT_EQUAL_MEMORY(&before, &owner, sizeof(owner));
    TEST_ASSERT_EQUAL_UINT(0U, g_frame_count);
}

static void test_ap3216c_owner_init_propagates_a_bus_fault_and_keeps_state(void) {
    pandora_ap3216c_owner_t owner;
    pandora_ap3216c_owner_t before;

    memset(&owner, 0xAB, sizeof(owner));
    before = owner;
    g_present[AP_DEVICE] = 0;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR, pandora_ap3216c_owner_init(&owner, &g_ap_bus));

    /* Fail closed: the caller's owner is untouched, not zeroed. */
    TEST_ASSERT_EQUAL_MEMORY(&before, &owner, sizeof(owner));
    /* Only the reset write was attempted. */
    TEST_ASSERT_EQUAL_UINT(1U, g_frame_count);
    assert_register_write(frame_at(0U), XY_AP3216C_DEFAULT_ADDRESS, XY_AP3216C_REG_SYSTEM_CONFIG,
                          XY_AP3216C_MODE_RESET);
    TEST_ASSERT_EQUAL_UINT(0U, g_delay_ms);
}

static void test_ap3216c_owner_register_access_is_fail_closed_before_init(void) {
    pandora_ap3216c_owner_t owner;
    uint8_t value = 0U;
    uint8_t raw[PANDORA_AP3216C_RAW_LEN];
    pandora_ap3216c_sample_t sample;

    memset(&owner, 0, sizeof(owner));
    memset(raw, 0x5A, sizeof(raw));
    memset(&sample, 0x5A, sizeof(sample));

    TEST_ASSERT_EQUAL_INT(
        XY_DEVICE_INVALID_PARAM,
        pandora_ap3216c_owner_read_reg(&owner, XY_AP3216C_REG_SYSTEM_CONFIG, &value, 1U));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, pandora_ap3216c_owner_read_raw(&owner, raw));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, pandora_ap3216c_owner_sample(&owner, &sample));

    /* No bus traffic, no output mutation. */
    TEST_ASSERT_EQUAL_UINT(0U, g_frame_count);
    TEST_ASSERT_EQUAL_UINT8(0U, value);
    TEST_ASSERT_EQUAL_UINT8(0x5AU, raw[0]);
}

static void test_ap3216c_owner_raw_read_returns_the_six_byte_block(void) {
    pandora_ap3216c_owner_t owner;
    uint8_t raw[PANDORA_AP3216C_RAW_LEN];
    static const uint8_t expected[PANDORA_AP3216C_RAW_LEN] = {0xCDU, 0x02U, 0x34U,
                                                              0x12U, 0x05U, 0x03U};

    memset(&owner, 0, sizeof(owner));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, pandora_ap3216c_owner_init(&owner, &g_ap_bus));
    program_ap3216c_raws();
    g_frame_count = 0U;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, pandora_ap3216c_owner_read_raw(&owner, raw));

    TEST_ASSERT_EQUAL_UINT(2U, g_frame_count);
    assert_register_select(frame_at(0U), XY_AP3216C_DEFAULT_ADDRESS, XY_AP3216C_REG_IR_DATA_L);
    TEST_ASSERT_EQUAL_UINT8(1U, frame_at(1U)->read_not_write);
    TEST_ASSERT_EQUAL_UINT8(PANDORA_AP3216C_RAW_LEN, frame_at(1U)->length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, raw, sizeof(expected));
}

static void test_ap3216c_owner_sample_decodes_all_three_channels(void) {
    pandora_ap3216c_owner_t owner;
    pandora_ap3216c_sample_t sample;

    memset(&owner, 0, sizeof(owner));
    memset(&sample, 0, sizeof(sample));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, pandora_ap3216c_owner_init(&owner, &g_ap_bus));
    program_ap3216c_raws();
    g_frame_count = 0U;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, pandora_ap3216c_owner_sample(&owner, &sample));

    /* ALS raw 0x1234 scaled by the driver's 350 millilux/LSB. */
    TEST_ASSERT_EQUAL_UINT32(0x1234U * 350U, sample.illuminance_millilux);
    /* PS high keeps only 6 bits, low only 4: 0x03,0x05 -> 0x35. */
    TEST_ASSERT_EQUAL_UINT16(0x35U, sample.proximity_raw);
    /* IR high keeps only 2 bits: 0x02,0xCD -> 0x2CD. */
    TEST_ASSERT_EQUAL_UINT16(0x2CDU, sample.infrared_raw);

    /* Three disjoint register pairs, each "select then read". */
    TEST_ASSERT_EQUAL_UINT(6U, g_frame_count);
    assert_register_select(frame_at(0U), XY_AP3216C_DEFAULT_ADDRESS, XY_AP3216C_REG_ALS_DATA_L);
    assert_register_select(frame_at(2U), XY_AP3216C_DEFAULT_ADDRESS, XY_AP3216C_REG_PS_DATA_L);
    assert_register_select(frame_at(4U), XY_AP3216C_DEFAULT_ADDRESS, XY_AP3216C_REG_IR_DATA_L);
    for (unsigned int index = 1U; index < 6U; index += 2U) {
        TEST_ASSERT_EQUAL_UINT8(1U, frame_at(index)->read_not_write);
        TEST_ASSERT_EQUAL_UINT8(2U, frame_at(index)->length);
    }
    /* The sample mirrors the driver cache the board reports. */
    TEST_ASSERT_EQUAL_UINT32(sample.illuminance_millilux, owner.device.data.illuminance_millilux);
    TEST_ASSERT_EQUAL_UINT16(sample.proximity_raw, owner.device.data.proximity_raw);
    TEST_ASSERT_EQUAL_UINT16(sample.infrared_raw, owner.device.data.infrared_raw);
}

static void test_ap3216c_owner_sample_preserves_the_previous_value_on_range_fault(void) {
    pandora_ap3216c_owner_t owner;
    pandora_ap3216c_sample_t sample;

    memset(&owner, 0, sizeof(owner));
    memset(&sample, 0, sizeof(sample));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, pandora_ap3216c_owner_init(&owner, &g_ap_bus));
    program_ap3216c_raws();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, pandora_ap3216c_owner_sample(&owner, &sample));
    TEST_ASSERT_EQUAL_UINT16(0x35U, sample.proximity_raw);

    /* Bit 6 of the PS low byte means "object out of measuring range". */
    g_regs[AP_DEVICE][XY_AP3216C_REG_PS_DATA_L] = 0x45U;
    sample.illuminance_millilux = 11U;
    sample.proximity_raw = 22U;
    sample.infrared_raw = 33U;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR, pandora_ap3216c_owner_sample(&owner, &sample));

    /* Atomic: the light channel already succeeded but must not be committed. */
    TEST_ASSERT_EQUAL_UINT32(11U, sample.illuminance_millilux);
    TEST_ASSERT_EQUAL_UINT16(22U, sample.proximity_raw);
    TEST_ASSERT_EQUAL_UINT16(33U, sample.infrared_raw);
}

static void test_ap3216c_owner_sample_preserves_output_when_the_bus_dies(void) {
    pandora_ap3216c_owner_t owner;
    pandora_ap3216c_sample_t sample;

    memset(&owner, 0, sizeof(owner));
    memset(&sample, 0, sizeof(sample));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, pandora_ap3216c_owner_init(&owner, &g_ap_bus));
    sample.illuminance_millilux = 7U;
    sample.proximity_raw = 8U;
    sample.infrared_raw = 9U;
    g_present[AP_DEVICE] = 0;
    g_frame_count = 0U;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR, pandora_ap3216c_owner_sample(&owner, &sample));

    TEST_ASSERT_EQUAL_UINT32(7U, sample.illuminance_millilux);
    TEST_ASSERT_EQUAL_UINT16(8U, sample.proximity_raw);
    TEST_ASSERT_EQUAL_UINT16(9U, sample.infrared_raw);
    /* The first register select is rejected; no read phase is attempted. */
    TEST_ASSERT_EQUAL_UINT(1U, g_frame_count);
    TEST_ASSERT_EQUAL_UINT(0U, g_read_frames);
}

/* --- ICM20608 board owner ---------------------------------------------- */

static void test_icm20608_owner_init_programs_the_canonical_sequence(void) {
    pandora_icm20608_owner_t owner;

    memset(&owner, 0, sizeof(owner));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, pandora_icm20608_owner_init(&owner, &g_icm_bus));

    TEST_ASSERT_EQUAL_PTR(&g_icm_bus, owner.device.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_UINT8(XY_ICM20608_ADDR_DEFAULT, owner.device.address);
    TEST_ASSERT_EQUAL_UINT8(1U, owner.device.configuration_synchronized);
    TEST_ASSERT_EQUAL_UINT8(1U, owner.device.initialized);
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_ACCEL_RANGE_4G, owner.device.accel_range);
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_GYRO_RANGE_500DPS, owner.device.gyro_range);
    TEST_ASSERT_EQUAL_UINT16(100U, owner.device.odr_hz);

    /* Identity is read before anything is written. */
    assert_register_select(frame_at(0U), XY_ICM20608_ADDR_DEFAULT, XY_ICM20608_REG_WHO_AM_I);
    TEST_ASSERT_EQUAL_UINT8(1U, frame_at(1U)->read_not_write);

    /* Reset, settle, then the documented power/range/ODR/filter writes. */
    assert_register_write(frame_at(2U), XY_ICM20608_ADDR_DEFAULT, XY_ICM20608_REG_PWR_MGMT_1,
                          0x80U);
    assert_register_write(frame_at(3U), XY_ICM20608_ADDR_DEFAULT, XY_ICM20608_REG_PWR_MGMT_1,
                          0x01U);
    TEST_ASSERT_EQUAL_UINT(100U, g_delay_ms);
    assert_register_write(frame_at(4U), XY_ICM20608_ADDR_DEFAULT, XY_ICM20608_REG_PWR_MGMT_2,
                          0x00U);
    assert_register_write(frame_at(5U), XY_ICM20608_ADDR_DEFAULT, XY_ICM20608_REG_GYRO_CONFIG,
                          0x08U);
    assert_register_write(frame_at(6U), XY_ICM20608_ADDR_DEFAULT, XY_ICM20608_REG_ACCEL_CONFIG,
                          0x08U);
    assert_register_write(frame_at(7U), XY_ICM20608_ADDR_DEFAULT, XY_ICM20608_REG_SMPLRT_DIV,
                          0x09U);
    assert_register_write(frame_at(8U), XY_ICM20608_ADDR_DEFAULT, XY_ICM20608_REG_CONFIG, 0x04U);
    assert_register_write(frame_at(9U), XY_ICM20608_ADDR_DEFAULT, XY_ICM20608_REG_ACCEL_CONFIG2,
                          0x04U);
    TEST_ASSERT_EQUAL_UINT8(0x01U, g_regs[ICM_DEVICE][XY_ICM20608_REG_PWR_MGMT_1]);
    TEST_ASSERT_EQUAL_UINT8(0x09U, g_regs[ICM_DEVICE][XY_ICM20608_REG_SMPLRT_DIV]);
}

static void test_icm20608_owner_init_rejects_a_foreign_identity_before_writing(void) {
    pandora_icm20608_owner_t owner;
    pandora_icm20608_owner_t before;

    memset(&owner, 0xAB, sizeof(owner));
    before = owner;
    g_regs[ICM_DEVICE][XY_ICM20608_REG_WHO_AM_I] = 0x71U; /* MPU6500 identity */

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_NOT_FOUND, pandora_icm20608_owner_init(&owner, &g_icm_bus));

    TEST_ASSERT_EQUAL_MEMORY(&before, &owner, sizeof(owner));
    /* Only the identity read happened; nothing was configured. */
    TEST_ASSERT_EQUAL_UINT(2U, g_frame_count);
    TEST_ASSERT_EQUAL_UINT(0U, g_write_frames);
    TEST_ASSERT_EQUAL_UINT8(0x71U, g_regs[ICM_DEVICE][XY_ICM20608_REG_WHO_AM_I]);
    TEST_ASSERT_EQUAL_UINT8(0x00U, g_regs[ICM_DEVICE][XY_ICM20608_REG_PWR_MGMT_1]);
    TEST_ASSERT_EQUAL_UINT(0U, g_delay_ms);
}

static void test_icm20608_owner_init_rejects_missing_arguments(void) {
    pandora_icm20608_owner_t owner;

    memset(&owner, 0, sizeof(owner));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, pandora_icm20608_owner_init(NULL, &g_icm_bus));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, pandora_icm20608_owner_init(&owner, NULL));
    TEST_ASSERT_EQUAL_UINT(0U, g_frame_count);
}

static void test_icm20608_owner_access_is_fail_closed_before_init(void) {
    pandora_icm20608_owner_t owner;
    uint8_t value = 0U;
    uint8_t raw[PANDORA_ICM20608_RAW_LEN];
    xy_icm20608_sample_t sample;

    memset(&owner, 0, sizeof(owner));
    memset(raw, 0x5A, sizeof(raw));
    memset(&sample, 0x5A, sizeof(sample));

    TEST_ASSERT_EQUAL_INT(
        XY_DEVICE_INVALID_PARAM,
        pandora_icm20608_owner_read_reg(&owner, XY_ICM20608_REG_WHO_AM_I, &value, 1U));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, pandora_icm20608_owner_read_raw(&owner, raw));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, pandora_icm20608_owner_sample(&owner, &sample));

    TEST_ASSERT_EQUAL_UINT(0U, g_frame_count);
    TEST_ASSERT_EQUAL_UINT8(0U, value);
    TEST_ASSERT_EQUAL_UINT8(0x5AU, raw[0]);
}

static void test_icm20608_owner_raw_read_starts_at_the_accel_block(void) {
    pandora_icm20608_owner_t owner;
    uint8_t raw[PANDORA_ICM20608_RAW_LEN];
    static const uint8_t expected[PANDORA_ICM20608_RAW_LEN] = {
        0x10U, 0x00U, 0xF0U, 0x00U, 0x04U, 0x00U, 0x08U,
        0x00U, 0x01U, 0x00U, 0xFFU, 0x00U, 0x00U, 0x00U,
    };

    memset(&owner, 0, sizeof(owner));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, pandora_icm20608_owner_init(&owner, &g_icm_bus));
    program_icm20608_raws();
    g_frame_count = 0U;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, pandora_icm20608_owner_read_raw(&owner, raw));

    TEST_ASSERT_EQUAL_UINT(2U, g_frame_count);
    assert_register_select(frame_at(0U), XY_ICM20608_ADDR_DEFAULT, XY_ICM20608_REG_ACCEL_XOUT_H);
    TEST_ASSERT_EQUAL_UINT8(1U, frame_at(1U)->read_not_write);
    TEST_ASSERT_EQUAL_UINT8(PANDORA_ICM20608_RAW_LEN, frame_at(1U)->length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, raw, sizeof(expected));
}

static void test_icm20608_owner_sample_scales_the_fourteen_byte_block(void) {
    pandora_icm20608_owner_t owner;
    xy_icm20608_sample_t sample;

    memset(&owner, 0, sizeof(owner));
    memset(&sample, 0, sizeof(sample));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, pandora_icm20608_owner_init(&owner, &g_icm_bus));
    program_icm20608_raws();
    g_frame_count = 0U;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, pandora_icm20608_owner_sample(&owner, &sample));

    /* 4 g full scale = 4000 mg over 32768 counts. */
    TEST_ASSERT_EQUAL_INT32(500, sample.accel.x_mg);
    TEST_ASSERT_EQUAL_INT32(-500, sample.accel.y_mg);
    TEST_ASSERT_EQUAL_INT32(125, sample.accel.z_mg);
    /* 500 dps full scale = 500000 mdps over 32768 counts. */
    TEST_ASSERT_EQUAL_INT32(3906, sample.gyro.x_mdps);
    TEST_ASSERT_EQUAL_INT32(-3906, sample.gyro.y_mdps);
    TEST_ASSERT_EQUAL_INT32(0, sample.gyro.z_mdps);
    /* 2048 counts / 3268 per degree + 25 C offset. */
    TEST_ASSERT_EQUAL_INT32(3126, sample.temperature_centi_c);

    /* One 14-byte burst from ACCEL_XOUT_H. */
    TEST_ASSERT_EQUAL_UINT(2U, g_frame_count);
    assert_register_select(frame_at(0U), XY_ICM20608_ADDR_DEFAULT, XY_ICM20608_REG_ACCEL_XOUT_H);
    TEST_ASSERT_EQUAL_UINT8(PANDORA_ICM20608_RAW_LEN, frame_at(1U)->length);
}

static void test_icm20608_owner_sample_preserves_output_when_the_bus_dies(void) {
    pandora_icm20608_owner_t owner;
    xy_icm20608_sample_t sample;

    memset(&owner, 0, sizeof(owner));
    memset(&sample, 0, sizeof(sample));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, pandora_icm20608_owner_init(&owner, &g_icm_bus));
    program_icm20608_raws();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, pandora_icm20608_owner_sample(&owner, &sample));
    TEST_ASSERT_EQUAL_INT32(500, sample.accel.x_mg);

    sample.accel.x_mg = 1234;
    sample.gyro.z_mdps = 5678;
    sample.temperature_centi_c = 2500;
    g_present[ICM_DEVICE] = 0;
    g_frame_count = 0U;
    g_read_frames = 0U;
    g_write_frames = 0U;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR, pandora_icm20608_owner_sample(&owner, &sample));

    TEST_ASSERT_EQUAL_INT32(1234, sample.accel.x_mg);
    TEST_ASSERT_EQUAL_INT32(5678, sample.gyro.z_mdps);
    TEST_ASSERT_EQUAL_INT32(2500, sample.temperature_centi_c);
    TEST_ASSERT_EQUAL_UINT(1U, g_frame_count);
    TEST_ASSERT_EQUAL_UINT(0U, g_read_frames);
}

/* --- runner ------------------------------------------------------------ */

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ap3216c_owner_init_binds_the_canonical_driver);
    RUN_TEST(test_ap3216c_owner_init_rejects_missing_arguments);
    RUN_TEST(test_ap3216c_owner_init_propagates_a_bus_fault_and_keeps_state);
    RUN_TEST(test_ap3216c_owner_register_access_is_fail_closed_before_init);
    RUN_TEST(test_ap3216c_owner_raw_read_returns_the_six_byte_block);
    RUN_TEST(test_ap3216c_owner_sample_decodes_all_three_channels);
    RUN_TEST(test_ap3216c_owner_sample_preserves_the_previous_value_on_range_fault);
    RUN_TEST(test_ap3216c_owner_sample_preserves_output_when_the_bus_dies);
    RUN_TEST(test_icm20608_owner_init_programs_the_canonical_sequence);
    RUN_TEST(test_icm20608_owner_init_rejects_a_foreign_identity_before_writing);
    RUN_TEST(test_icm20608_owner_init_rejects_missing_arguments);
    RUN_TEST(test_icm20608_owner_access_is_fail_closed_before_init);
    RUN_TEST(test_icm20608_owner_raw_read_starts_at_the_accel_block);
    RUN_TEST(test_icm20608_owner_sample_scales_the_fourteen_byte_block);
    RUN_TEST(test_icm20608_owner_sample_preserves_output_when_the_bus_dies);
    return UNITY_END();
}
