/**
 * @file test_pandora_i2c_owners.c
 * @brief Host contract for the Pandora board-owned I2C transports.
 *
 * `boards/pandora_stm32l475/pandora_soft_i2c.c` and `pandora_hw_i2c.c` are the
 * only I2C transports the reference board uses for its onboard AHT10 /
 * ICM20608 (software I2C) and AP3216C (hardware I2C3) paths, and they had no
 * host coverage at all. Both files sit on the canonical `xy_hal_gpio_*` /
 * `xy_hal_i2c_*` seams, so a host fixture can drive them with a fake vendor
 * HAL header and decode the resulting GPIO waveform back into I2C frames.
 *
 * Evidence boundary: host/compile contract only. No timing, bus-electrical,
 * clock-stretching, or real-device behaviour is claimed here.
 */

#include "unity.h"

#include "stm32l4xx_hal.h"

#include "pandora_hw_i2c.h"
#include "pandora_soft_i2c.h"

#include "xy_hal_gpio.h"
#include "xy_hal_i2c.h"

#include <stdint.h>
#include <string.h>

/* --- fake vendor HAL objects ------------------------------------------- */

struct xy_hal_gpio_port {
    uint8_t tag;
};

#define PORT_C 0U
#define PORT_D 1U
#define PORT_FOREIGN 0xFFU
#define PIN_CAPACITY 16U

GPIO_TypeDef xy_test_fake_gpioc = {PORT_C};
GPIO_TypeDef xy_test_fake_gpiod = {PORT_D};
I2C_TypeDef xy_test_fake_i2c3;

/* --- GPIO fixture ------------------------------------------------------ */

typedef enum {
    EV_WRITE = 0,
    EV_READ,
    EV_CONFIG,
} gpio_event_kind_t;

typedef struct {
    gpio_event_kind_t kind;
    uint8_t port;
    uint8_t pin;
    uint8_t value;
} gpio_event_t;

#define EVENT_CAPACITY 8192U
#define READ_SCRIPT_CAPACITY 128U

static gpio_event_t g_events[EVENT_CAPACITY];
static unsigned int g_event_count;
static int g_event_overflow;

static int32_t g_read_script[READ_SCRIPT_CAPACITY];
static unsigned int g_read_script_len;
static unsigned int g_read_script_steps;
static int32_t g_idle_read_level;

static xy_hal_gpio_config_t g_config[2][PIN_CAPACITY];
static unsigned int g_config_count[2][PIN_CAPACITY];

static unsigned int g_init_calls;
static unsigned int g_fail_init_at;
static unsigned int g_rcc_gpioc;
static unsigned int g_rcc_gpiod;
static unsigned int g_rcc_i2c3;

static unsigned int g_i2c_init_calls;
static void *g_i2c_init_handle;
static int g_i2c_init_instance_ok;
static xy_hal_i2c_config_t g_i2c_init_config;
static xy_hal_error_t g_next_i2c_init_result;

static uint8_t port_tag(xy_hal_gpio_port_t port)
{
    if (port == &xy_test_fake_gpioc) {
        return PORT_C;
    }
    if (port == &xy_test_fake_gpiod) {
        return PORT_D;
    }
    return PORT_FOREIGN;
}

static void record_event(gpio_event_kind_t kind, xy_hal_gpio_port_t port, uint8_t pin,
                         uint8_t value)
{
    if (g_event_count < EVENT_CAPACITY) {
        g_events[g_event_count].kind = kind;
        g_events[g_event_count].port = port_tag(port);
        g_events[g_event_count].pin = pin;
        g_events[g_event_count].value = value;
    } else {
        g_event_overflow = 1;
    }
    ++g_event_count;
}

xy_hal_error_t xy_hal_gpio_init(xy_hal_gpio_port_t port, uint8_t pin,
                                const xy_hal_gpio_config_t *config)
{
    const uint8_t tag = port_tag(port);

    ++g_init_calls;
    if (config == NULL || tag == PORT_FOREIGN || pin >= PIN_CAPACITY) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    g_config[tag][pin] = *config;
    ++g_config_count[tag][pin];
    record_event(EV_CONFIG, port, pin, (uint8_t)config->mode);
    if (g_fail_init_at != 0U && g_init_calls == g_fail_init_at) {
        return XY_HAL_ERROR_IO;
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_write(xy_hal_gpio_port_t port, uint8_t pin, uint8_t value)
{
    record_event(EV_WRITE, port, pin, value ? 1U : 0U);
    return XY_HAL_OK;
}

int32_t xy_hal_gpio_read(xy_hal_gpio_port_t port, uint8_t pin)
{
    int32_t value = g_idle_read_level;

    ++g_read_script_steps;
    if (g_read_script_len > 0U) {
        value = g_read_script[0];
        --g_read_script_len;
        for (unsigned int index = 0U; index < g_read_script_len; ++index) {
            g_read_script[index] = g_read_script[index + 1U];
        }
    }
    record_event(EV_READ, port, pin, (uint8_t)(value != 0));
    return value;
}

void xy_test_fake_rcc_gpioc_enable(void)
{
    ++g_rcc_gpioc;
}

void xy_test_fake_rcc_gpiod_enable(void)
{
    ++g_rcc_gpiod;
}

void xy_test_fake_rcc_i2c3_enable(void)
{
    ++g_rcc_i2c3;
}

xy_hal_error_t xy_hal_i2c_init(void *i2c, const xy_hal_i2c_config_t *config)
{
    I2C_HandleTypeDef *handle = i2c;

    ++g_i2c_init_calls;
    g_i2c_init_handle = i2c;
    g_i2c_init_instance_ok = (handle != NULL && handle->Instance == I2C3) ? 1 : 0;
    if (config != NULL) {
        g_i2c_init_config = *config;
    }
    return g_next_i2c_init_result;
}

/* --- read-script helpers ----------------------------------------------- */

#define ACK_LEVEL 0U
#define NACK_LEVEL 1U

static void script_read(uint8_t level)
{
    if (g_read_script_len < READ_SCRIPT_CAPACITY) {
        g_read_script[g_read_script_len] = (int32_t)level;
        ++g_read_script_len;
    }
}

static void script_byte_bits(uint8_t value)
{
    for (int bit = 7; bit >= 0; --bit) {
        script_read((uint8_t)((value >> bit) & 1U));
    }
}

/* --- waveform decoder -------------------------------------------------- */

typedef struct {
    uint8_t scl_port;
    uint8_t scl_pin;
    uint8_t sda_port;
    uint8_t sda_pin;
} bus_pins_t;

static const bus_pins_t BUS_I2C3 = {PORT_C, 0U, PORT_C, 1U};
static const bus_pins_t BUS_I2C4 = {PORT_D, 6U, PORT_C, 1U};

typedef struct {
    uint8_t bit;
    uint8_t master_driven;
} sample_t;

#define MAX_SAMPLES 512U
#define TXN_MAX_FRAMES 64U
#define TXN_MAX_PHASES 8U

typedef struct {
    unsigned int starts;
    unsigned int stops;
    unsigned int frame_count;
    uint8_t byte[TXN_MAX_FRAMES];
    unsigned int ack[TXN_MAX_FRAMES];
    unsigned int phase_count;
    unsigned int phase_frames[TXN_MAX_PHASES];
    int misaligned;
    int overflow;
} transaction_t;

static uint8_t peek_read_level(unsigned int from)
{
    for (unsigned int index = from + 1U; index <= from + 3U && index < g_event_count &&
                                      index < EVENT_CAPACITY;
         ++index) {
        if (g_events[index].kind == EV_READ) {
            return g_events[index].value ? 1U : 0U;
        }
    }
    return 1U;
}

static void append_phase(transaction_t *txn, const sample_t *phase, unsigned int len)
{
    unsigned int offset = 0U;
    unsigned int frames = 0U;

    while (offset + 9U <= len) {
        uint8_t value = 0U;
        for (unsigned int bit = 0U; bit < 8U; ++bit) {
            value = (uint8_t)((value << 1) | phase[offset + bit].bit);
        }
        if (txn->frame_count < TXN_MAX_FRAMES) {
            txn->byte[txn->frame_count] = value;
            txn->ack[txn->frame_count] = phase[offset + 8U].bit;
            ++txn->frame_count;
        } else {
            txn->overflow = 1;
        }
        offset += 9U;
        ++frames;
    }
    if (offset != len) {
        txn->misaligned = 1;
    }
    if (txn->phase_count < TXN_MAX_PHASES) {
        txn->phase_frames[txn->phase_count] = frames;
        ++txn->phase_count;
    }
}

/*
 * Rebuild the I2C frames the board transport produced from the recorded GPIO
 * activity: START/STOP are SDA transitions while SCL is high, and every byte is
 * the eight SDA levels sampled on SCL rising edges followed by the ACK clock.
 * The SCL rise inside a STOP is not a data clock, so it is dropped per phase.
 */
static void capture_decode(const bus_pins_t *bus, transaction_t *txn)
{
    sample_t phase[MAX_SAMPLES];
    unsigned int phase_len = 0U;
    uint8_t scl = 1U;
    uint8_t sda_level = 1U;
    uint8_t sda_driven = 1U;

    memset(txn, 0, sizeof(*txn));

    for (unsigned int index = 0U; index < g_event_count && index < EVENT_CAPACITY; ++index) {
        const gpio_event_t *event = &g_events[index];

        if (event->kind == EV_CONFIG) {
            if (event->port == bus->sda_port && event->pin == bus->sda_pin) {
                sda_driven = (uint8_t)(event->value != (uint8_t)XY_HAL_GPIO_MODE_INPUT);
            }
            continue;
        }
        if (event->kind == EV_WRITE && event->port == bus->sda_port &&
            event->pin == bus->sda_pin) {
            const uint8_t previous = sda_level;
            sda_level = event->value ? 1U : 0U;
            if (scl == 1U && previous != sda_level) {
                if (sda_level == 0U) {
                    ++txn->starts;
                    phase_len = 0U;
                } else {
                    ++txn->stops;
                    if (phase_len > 0U) {
                        --phase_len;
                    }
                    append_phase(txn, phase, phase_len);
                    phase_len = 0U;
                }
            }
            continue;
        }
        if (event->kind == EV_WRITE && event->port == bus->scl_port &&
            event->pin == bus->scl_pin) {
            const uint8_t previous_scl = scl;
            scl = event->value ? 1U : 0U;
            if (previous_scl == 0U && scl == 1U && phase_len < MAX_SAMPLES) {
                phase[phase_len].bit = (sda_driven ? sda_level : peek_read_level(index)) ? 1U : 0U;
                phase[phase_len].master_driven = sda_driven ? 1U : 0U;
                ++phase_len;
            }
            continue;
        }
    }
    if (phase_len > 0U) {
        append_phase(txn, phase, phase_len);
    }
}

static void assert_clean_capture(const transaction_t *txn)
{
    TEST_ASSERT_EQUAL_INT(0, g_event_overflow);
    TEST_ASSERT_EQUAL_INT(0, txn->overflow);
    TEST_ASSERT_EQUAL_INT(0, txn->misaligned);
}

static void assert_only_bus_pins_touched(const bus_pins_t *bus)
{
    for (unsigned int index = 0U; index < g_event_count && index < EVENT_CAPACITY; ++index) {
        const gpio_event_t *event = &g_events[index];
        int on_sda;
        int on_scl;

        if (event->kind == EV_READ) {
            continue;
        }
        on_sda = (event->port == bus->sda_port && event->pin == bus->sda_pin);
        on_scl = (event->port == bus->scl_port && event->pin == bus->scl_pin);
        TEST_ASSERT_TRUE_MESSAGE(on_sda || on_scl,
                                 "board soft-I2C touched a pin outside the bus mapping");
    }
}

static void assert_no_bus_activity(void)
{
    TEST_ASSERT_EQUAL_UINT(0U, g_event_count);
}

/* --- Unity boilerplate ------------------------------------------------- */

void setUp(void)
{
    memset(g_events, 0, sizeof(g_events));
    g_event_count = 0U;
    g_event_overflow = 0;

    memset(g_read_script, 0, sizeof(g_read_script));
    g_read_script_len = 0U;
    g_read_script_steps = 0U;
    g_idle_read_level = 1;

    memset(g_config, 0, sizeof(g_config));
    memset(g_config_count, 0, sizeof(g_config_count));

    g_init_calls = 0U;
    g_fail_init_at = 0U;
    g_rcc_gpioc = 0U;
    g_rcc_gpiod = 0U;
    g_rcc_i2c3 = 0U;

    g_i2c_init_calls = 0U;
    g_i2c_init_handle = NULL;
    g_i2c_init_instance_ok = 0;
    memset(&g_i2c_init_config, 0, sizeof(g_i2c_init_config));
    g_next_i2c_init_result = XY_HAL_OK;
}

void tearDown(void)
{
}

/* --- software I2C: bus wiring ------------------------------------------ */

static void test_soft_i2c3_init_wires_scl_pc0_and_sda_pc1(void)
{
    void *bus = pandora_soft_i2c3_init();

    TEST_ASSERT_NOT_NULL(bus);
    TEST_ASSERT_EQUAL_UINT(1U, g_config_count[PORT_C][0]);
    TEST_ASSERT_EQUAL_UINT(1U, g_config_count[PORT_C][1]);
    TEST_ASSERT_EQUAL_INT(XY_HAL_GPIO_MODE_OUTPUT, g_config[PORT_C][0].mode);
    TEST_ASSERT_EQUAL_INT(XY_HAL_GPIO_MODE_OUTPUT, g_config[PORT_C][1].mode);
    TEST_ASSERT_EQUAL_INT(XY_HAL_GPIO_PULL_UP, g_config[PORT_C][0].pull);
    TEST_ASSERT_EQUAL_INT(XY_HAL_GPIO_PULL_UP, g_config[PORT_C][1].pull);
    TEST_ASSERT_EQUAL_INT(XY_HAL_GPIO_OTYPE_OD, g_config[PORT_C][0].otype);
    TEST_ASSERT_EQUAL_INT(XY_HAL_GPIO_OTYPE_OD, g_config[PORT_C][1].otype);
    TEST_ASSERT_EQUAL_INT(XY_HAL_GPIO_SPEED_HIGH, g_config[PORT_C][0].speed);
    TEST_ASSERT_EQUAL_INT(XY_HAL_GPIO_SPEED_HIGH, g_config[PORT_C][1].speed);
    TEST_ASSERT_EQUAL_UINT(1U, g_rcc_gpioc);
    TEST_ASSERT_EQUAL_UINT(0U, g_rcc_gpiod);
}

static void test_soft_i2c4_init_wires_scl_pd6_and_sda_pc1(void)
{
    void *bus = pandora_soft_i2c4_init();

    TEST_ASSERT_NOT_NULL(bus);
    TEST_ASSERT_EQUAL_UINT(1U, g_config_count[PORT_D][6]);
    TEST_ASSERT_EQUAL_UINT(1U, g_config_count[PORT_C][1]);
    TEST_ASSERT_EQUAL_UINT(0U, g_config_count[PORT_C][0]);
    TEST_ASSERT_EQUAL_INT(XY_HAL_GPIO_MODE_OUTPUT, g_config[PORT_D][6].mode);
    /* soft_i2c_init() enables both port clocks unconditionally. */
    TEST_ASSERT_EQUAL_UINT(1U, g_rcc_gpioc);
    TEST_ASSERT_EQUAL_UINT(1U, g_rcc_gpiod);
}

static void test_soft_i2c_init_is_the_i2c4_bus_alias(void)
{
    void *i2c3 = pandora_soft_i2c3_init();
    void *i2c4 = pandora_soft_i2c4_init();
    void *alias = pandora_soft_i2c_init();

    TEST_ASSERT_NOT_NULL(i2c3);
    TEST_ASSERT_NOT_NULL(i2c4);
    TEST_ASSERT_EQUAL_PTR(i2c4, alias);
    TEST_ASSERT_TRUE(i2c3 != alias);
}

static void test_soft_init_returns_null_when_sda_configuration_fails(void)
{
    g_fail_init_at = 1U;

    TEST_ASSERT_NULL(pandora_soft_i2c3_init());
    TEST_ASSERT_EQUAL_UINT(1U, g_event_count);
    TEST_ASSERT_EQUAL_INT(EV_CONFIG, g_events[0].kind);
}

static void test_soft_init_returns_null_when_scl_configuration_fails(void)
{
    g_fail_init_at = 2U;

    TEST_ASSERT_NULL(pandora_soft_i2c3_init());
}

static void test_soft_init_releases_both_lines_after_configuring(void)
{
    transaction_t txn;
    unsigned int last = 0U;

    TEST_ASSERT_NOT_NULL(pandora_soft_i2c3_init());
    TEST_ASSERT_TRUE(g_event_count >= 2U);

    /* The bus must be left idle-high: SDA then SCL driven to 1. */
    last = g_event_count - 1U;
    TEST_ASSERT_EQUAL_INT(EV_WRITE, g_events[last - 1U].kind);
    TEST_ASSERT_EQUAL_INT(EV_WRITE, g_events[last].kind);
    TEST_ASSERT_EQUAL_UINT(PORT_C, g_events[last - 1U].port);
    TEST_ASSERT_EQUAL_UINT(1U, g_events[last - 1U].pin);
    TEST_ASSERT_EQUAL_UINT(1U, g_events[last - 1U].value);
    TEST_ASSERT_EQUAL_UINT(PORT_C, g_events[last].port);
    TEST_ASSERT_EQUAL_UINT(0U, g_events[last].pin);
    TEST_ASSERT_EQUAL_UINT(1U, g_events[last].value);

    capture_decode(&BUS_I2C3, &txn);
    assert_clean_capture(&txn);
    TEST_ASSERT_EQUAL_UINT(0U, txn.frame_count);
}

/* --- software I2C: address probe --------------------------------------- */

static void test_probe_reports_ack_and_emits_address_write_frame(void)
{
    void *bus = pandora_soft_i2c3_init();
    transaction_t txn;

    g_event_count = 0U;
    script_read(ACK_LEVEL);

    TEST_ASSERT_EQUAL_INT(1, pandora_soft_i2c_probe(bus, 0x38U));

    capture_decode(&BUS_I2C3, &txn);
    assert_clean_capture(&txn);
    assert_only_bus_pins_touched(&BUS_I2C3);
    TEST_ASSERT_EQUAL_UINT(1U, txn.starts);
    TEST_ASSERT_EQUAL_UINT(1U, txn.stops);
    TEST_ASSERT_EQUAL_UINT(1U, txn.frame_count);
    TEST_ASSERT_EQUAL_HEX8(0x70U, txn.byte[0]);
    TEST_ASSERT_EQUAL_UINT(ACK_LEVEL, txn.ack[0]);
}

static void test_probe_reports_nack_and_still_issues_stop(void)
{
    void *bus = pandora_soft_i2c3_init();
    transaction_t txn;

    g_event_count = 0U;
    script_read(NACK_LEVEL);

    TEST_ASSERT_EQUAL_INT(0, pandora_soft_i2c_probe(bus, 0x38U));

    capture_decode(&BUS_I2C3, &txn);
    assert_clean_capture(&txn);
    TEST_ASSERT_EQUAL_UINT(1U, txn.starts);
    TEST_ASSERT_EQUAL_UINT(1U, txn.stops);
    TEST_ASSERT_EQUAL_UINT(1U, txn.frame_count);
    TEST_ASSERT_EQUAL_UINT(NACK_LEVEL, txn.ack[0]);
}

static void test_probe_rejects_reserved_address_without_bus_activity(void)
{
    void *bus = pandora_soft_i2c3_init();

    g_event_count = 0U;
    TEST_ASSERT_EQUAL_INT(0, pandora_soft_i2c_probe(bus, 0x80U));
    assert_no_bus_activity();
}

static void test_probe_rejects_foreign_bus_without_bus_activity(void)
{
    int foreign = 0;

    TEST_ASSERT_EQUAL_INT(0, pandora_soft_i2c_probe(&foreign, 0x38U));
    assert_no_bus_activity();
}

static void test_probe_uses_the_requested_bus_pins(void)
{
    void *bus = pandora_soft_i2c4_init();
    transaction_t txn;

    g_event_count = 0U;
    script_read(ACK_LEVEL);

    TEST_ASSERT_EQUAL_INT(1, pandora_soft_i2c_probe(bus, 0x1EU));

    capture_decode(&BUS_I2C4, &txn);
    assert_clean_capture(&txn);
    assert_only_bus_pins_touched(&BUS_I2C4);
    TEST_ASSERT_EQUAL_HEX8(0x3CU, txn.byte[0]);
}

/* --- software I2C: master transmit ------------------------------------- */

static void test_transmit_writes_address_then_payload_msb_first(void)
{
    void *bus = pandora_soft_i2c3_init();
    const uint8_t payload[2] = {0xA5U, 0x5AU};
    transaction_t txn;

    g_event_count = 0U;
    script_read(ACK_LEVEL);
    script_read(ACK_LEVEL);
    script_read(ACK_LEVEL);

    TEST_ASSERT_EQUAL_INT(XY_HAL_OK,
                          xy_hal_i2c_master_transmit(bus, 0x38U, payload, sizeof(payload), 100U));

    capture_decode(&BUS_I2C3, &txn);
    assert_clean_capture(&txn);
    assert_only_bus_pins_touched(&BUS_I2C3);
    TEST_ASSERT_EQUAL_UINT(1U, txn.starts);
    TEST_ASSERT_EQUAL_UINT(1U, txn.stops);
    TEST_ASSERT_EQUAL_UINT(3U, txn.frame_count);
    TEST_ASSERT_EQUAL_HEX8(0x70U, txn.byte[0]);
    TEST_ASSERT_EQUAL_HEX8(0xA5U, txn.byte[1]);
    TEST_ASSERT_EQUAL_HEX8(0x5AU, txn.byte[2]);
    TEST_ASSERT_EQUAL_UINT(ACK_LEVEL, txn.ack[0]);
    TEST_ASSERT_EQUAL_UINT(ACK_LEVEL, txn.ack[1]);
    TEST_ASSERT_EQUAL_UINT(ACK_LEVEL, txn.ack[2]);
}

static void test_transmit_propagates_address_nack_and_skips_payload(void)
{
    void *bus = pandora_soft_i2c3_init();
    const uint8_t payload[2] = {0x11U, 0x22U};
    transaction_t txn;

    g_event_count = 0U;
    script_read(NACK_LEVEL);

    TEST_ASSERT_EQUAL_INT(XY_HAL_ERROR_IO,
                          xy_hal_i2c_master_transmit(bus, 0x38U, payload, sizeof(payload), 100U));

    capture_decode(&BUS_I2C3, &txn);
    assert_clean_capture(&txn);
    TEST_ASSERT_EQUAL_UINT(1U, txn.frame_count);
    TEST_ASSERT_EQUAL_HEX8(0x70U, txn.byte[0]);
    TEST_ASSERT_EQUAL_UINT(NACK_LEVEL, txn.ack[0]);
    TEST_ASSERT_EQUAL_UINT(1U, txn.stops);
}

static void test_transmit_stops_at_first_nacked_payload_byte(void)
{
    void *bus = pandora_soft_i2c3_init();
    const uint8_t payload[3] = {0x01U, 0x02U, 0x03U};
    transaction_t txn;

    g_event_count = 0U;
    script_read(ACK_LEVEL);
    script_read(NACK_LEVEL);

    TEST_ASSERT_EQUAL_INT(XY_HAL_ERROR_IO,
                          xy_hal_i2c_master_transmit(bus, 0x38U, payload, sizeof(payload), 100U));

    capture_decode(&BUS_I2C3, &txn);
    assert_clean_capture(&txn);
    TEST_ASSERT_EQUAL_UINT(2U, txn.frame_count);
    TEST_ASSERT_EQUAL_HEX8(0x01U, txn.byte[1]);
    TEST_ASSERT_EQUAL_UINT(NACK_LEVEL, txn.ack[1]);
    TEST_ASSERT_EQUAL_UINT(1U, txn.stops);
}

static void test_transmit_rejects_invalid_arguments_without_bus_activity(void)
{
    void *bus = pandora_soft_i2c3_init();
    const uint8_t payload[1] = {0x5AU};
    int foreign = 0;

    g_event_count = 0U;
    TEST_ASSERT_EQUAL_INT(XY_HAL_ERROR_INVALID_PARAM,
                          xy_hal_i2c_master_transmit(bus, 0x38U, NULL, 1U, 100U));
    TEST_ASSERT_EQUAL_INT(XY_HAL_ERROR_INVALID_PARAM,
                          xy_hal_i2c_master_transmit(bus, 0x38U, payload, 0U, 100U));
    TEST_ASSERT_EQUAL_INT(XY_HAL_ERROR_INVALID_PARAM,
                          xy_hal_i2c_master_transmit(bus, 0x80U, payload, 1U, 100U));
    TEST_ASSERT_EQUAL_INT(XY_HAL_ERROR_INVALID_PARAM,
                          xy_hal_i2c_master_transmit(&foreign, 0x38U, payload, 1U, 100U));
    assert_no_bus_activity();
}

static void test_transmit_accepts_the_maximum_seven_bit_address(void)
{
    void *bus = pandora_soft_i2c3_init();
    const uint8_t payload[1] = {0x00U};
    transaction_t txn;

    g_event_count = 0U;
    script_read(ACK_LEVEL);
    script_read(ACK_LEVEL);

    TEST_ASSERT_EQUAL_INT(XY_HAL_OK,
                          xy_hal_i2c_master_transmit(bus, 0x7FU, payload, sizeof(payload), 100U));

    capture_decode(&BUS_I2C3, &txn);
    assert_clean_capture(&txn);
    TEST_ASSERT_EQUAL_HEX8(0xFEU, txn.byte[0]);
}

/* --- software I2C: master receive -------------------------------------- */

static void test_receive_reads_address_with_read_bit_then_payload_msb_first(void)
{
    void *bus = pandora_soft_i2c3_init();
    uint8_t buffer[2] = {0xEEU, 0xEEU};
    transaction_t txn;

    g_event_count = 0U;
    script_read(ACK_LEVEL);
    script_byte_bits(0x12U);
    script_byte_bits(0x34U);

    TEST_ASSERT_EQUAL_INT(XY_HAL_OK,
                          xy_hal_i2c_master_receive(bus, 0x38U, buffer, sizeof(buffer), 100U));

    TEST_ASSERT_EQUAL_HEX8(0x12U, buffer[0]);
    TEST_ASSERT_EQUAL_HEX8(0x34U, buffer[1]);

    capture_decode(&BUS_I2C3, &txn);
    assert_clean_capture(&txn);
    assert_only_bus_pins_touched(&BUS_I2C3);
    TEST_ASSERT_EQUAL_UINT(1U, txn.starts);
    TEST_ASSERT_EQUAL_UINT(1U, txn.stops);
    TEST_ASSERT_EQUAL_UINT(3U, txn.frame_count);
    TEST_ASSERT_EQUAL_HEX8(0x71U, txn.byte[0]);
    TEST_ASSERT_EQUAL_HEX8(0x12U, txn.byte[1]);
    TEST_ASSERT_EQUAL_HEX8(0x34U, txn.byte[2]);
    TEST_ASSERT_EQUAL_UINT(ACK_LEVEL, txn.ack[1]);
    TEST_ASSERT_EQUAL_UINT(NACK_LEVEL, txn.ack[2]);
}

static void test_receive_nacks_a_single_byte_read(void)
{
    void *bus = pandora_soft_i2c3_init();
    uint8_t buffer[1] = {0x00U};
    transaction_t txn;

    g_event_count = 0U;
    script_read(ACK_LEVEL);
    script_byte_bits(0x5AU);

    TEST_ASSERT_EQUAL_INT(XY_HAL_OK,
                          xy_hal_i2c_master_receive(bus, 0x38U, buffer, sizeof(buffer), 100U));

    TEST_ASSERT_EQUAL_HEX8(0x5AU, buffer[0]);

    capture_decode(&BUS_I2C3, &txn);
    assert_clean_capture(&txn);
    TEST_ASSERT_EQUAL_UINT(2U, txn.frame_count);
    TEST_ASSERT_EQUAL_UINT(NACK_LEVEL, txn.ack[1]);
}

static void test_receive_propagates_address_nack_and_preserves_output(void)
{
    void *bus = pandora_soft_i2c3_init();
    uint8_t buffer[2] = {0xEEU, 0xEEU};
    transaction_t txn;

    g_event_count = 0U;
    script_read(NACK_LEVEL);

    TEST_ASSERT_EQUAL_INT(XY_HAL_ERROR_IO,
                          xy_hal_i2c_master_receive(bus, 0x38U, buffer, sizeof(buffer), 100U));

    TEST_ASSERT_EQUAL_HEX8(0xEEU, buffer[0]);
    TEST_ASSERT_EQUAL_HEX8(0xEEU, buffer[1]);

    capture_decode(&BUS_I2C3, &txn);
    assert_clean_capture(&txn);
    TEST_ASSERT_EQUAL_UINT(1U, txn.frame_count);
    TEST_ASSERT_EQUAL_UINT(1U, txn.stops);
}

static void test_receive_rejects_invalid_arguments_without_bus_activity(void)
{
    void *bus = pandora_soft_i2c3_init();
    uint8_t buffer[1] = {0x00U};
    int foreign = 0;

    g_event_count = 0U;
    TEST_ASSERT_EQUAL_INT(XY_HAL_ERROR_INVALID_PARAM,
                          xy_hal_i2c_master_receive(bus, 0x38U, NULL, 1U, 100U));
    TEST_ASSERT_EQUAL_INT(XY_HAL_ERROR_INVALID_PARAM,
                          xy_hal_i2c_master_receive(bus, 0x38U, buffer, 0U, 100U));
    TEST_ASSERT_EQUAL_INT(XY_HAL_ERROR_INVALID_PARAM,
                          xy_hal_i2c_master_receive(bus, 0x80U, buffer, 1U, 100U));
    TEST_ASSERT_EQUAL_INT(XY_HAL_ERROR_INVALID_PARAM,
                          xy_hal_i2c_master_receive(&foreign, 0x38U, buffer, 1U, 100U));
    assert_no_bus_activity();
}

/* --- software I2C: register (memory) access ---------------------------- */

static void test_mem_write_sends_register_then_payload(void)
{
    void *bus = pandora_soft_i2c3_init();
    const uint8_t payload[2] = {0x40U, 0x80U};
    transaction_t txn;

    g_event_count = 0U;
    script_read(ACK_LEVEL); /* address */
    script_read(ACK_LEVEL); /* register */
    script_read(ACK_LEVEL); /* payload byte 0 */
    script_read(ACK_LEVEL); /* payload byte 1 */

    TEST_ASSERT_EQUAL_INT(
        XY_HAL_OK, xy_hal_i2c_mem_write(bus, 0x68U, 0x6BU, payload, sizeof(payload), 100U));

    capture_decode(&BUS_I2C3, &txn);
    assert_clean_capture(&txn);
    assert_only_bus_pins_touched(&BUS_I2C3);
    TEST_ASSERT_EQUAL_UINT(1U, txn.starts);
    TEST_ASSERT_EQUAL_UINT(1U, txn.stops);
    TEST_ASSERT_EQUAL_UINT(4U, txn.frame_count);
    TEST_ASSERT_EQUAL_HEX8(0xD0U, txn.byte[0]);
    TEST_ASSERT_EQUAL_HEX8(0x6BU, txn.byte[1]);
    TEST_ASSERT_EQUAL_HEX8(0x40U, txn.byte[2]);
    TEST_ASSERT_EQUAL_HEX8(0x80U, txn.byte[3]);
}

static void test_mem_write_propagates_register_nack(void)
{
    void *bus = pandora_soft_i2c3_init();
    const uint8_t payload[1] = {0x00U};
    transaction_t txn;

    g_event_count = 0U;
    script_read(ACK_LEVEL);
    script_read(NACK_LEVEL);

    TEST_ASSERT_EQUAL_INT(
        XY_HAL_ERROR_IO, xy_hal_i2c_mem_write(bus, 0x68U, 0x6BU, payload, sizeof(payload), 100U));

    capture_decode(&BUS_I2C3, &txn);
    assert_clean_capture(&txn);
    TEST_ASSERT_EQUAL_UINT(2U, txn.frame_count);
    TEST_ASSERT_EQUAL_UINT(NACK_LEVEL, txn.ack[1]);
    TEST_ASSERT_EQUAL_UINT(1U, txn.stops);
}

static void test_mem_write_rejects_widened_register_address(void)
{
    void *bus = pandora_soft_i2c3_init();
    const uint8_t payload[1] = {0x00U};

    g_event_count = 0U;
    TEST_ASSERT_EQUAL_INT(
        XY_HAL_ERROR_INVALID_PARAM,
        xy_hal_i2c_mem_write(bus, 0x68U, 0x0100U, payload, sizeof(payload), 100U));
    assert_no_bus_activity();
}

static void test_mem_write_rejects_invalid_arguments(void)
{
    void *bus = pandora_soft_i2c3_init();
    const uint8_t payload[1] = {0x00U};
    int foreign = 0;

    g_event_count = 0U;
    TEST_ASSERT_EQUAL_INT(XY_HAL_ERROR_INVALID_PARAM,
                          xy_hal_i2c_mem_write(bus, 0x68U, 0x6BU, NULL, 1U, 100U));
    TEST_ASSERT_EQUAL_INT(XY_HAL_ERROR_INVALID_PARAM,
                          xy_hal_i2c_mem_write(bus, 0x68U, 0x6BU, payload, 0U, 100U));
    TEST_ASSERT_EQUAL_INT(XY_HAL_ERROR_INVALID_PARAM,
                          xy_hal_i2c_mem_write(bus, 0x80U, 0x6BU, payload, 1U, 100U));
    TEST_ASSERT_EQUAL_INT(XY_HAL_ERROR_INVALID_PARAM,
                          xy_hal_i2c_mem_write(&foreign, 0x68U, 0x6BU, payload, 1U, 100U));
    assert_no_bus_activity();
}

static void test_mem_read_writes_register_then_restarts_with_read_address(void)
{
    void *bus = pandora_soft_i2c3_init();
    uint8_t buffer[2] = {0x00U, 0x00U};
    transaction_t txn;

    g_event_count = 0U;
    script_read(ACK_LEVEL); /* write-phase address */
    script_read(ACK_LEVEL); /* register byte */
    script_read(ACK_LEVEL); /* read-phase address */
    script_byte_bits(0xAAU);
    script_byte_bits(0x55U);

    TEST_ASSERT_EQUAL_INT(XY_HAL_OK,
                          xy_hal_i2c_mem_read(bus, 0x68U, 0x75U, buffer, sizeof(buffer), 100U));

    TEST_ASSERT_EQUAL_HEX8(0xAAU, buffer[0]);
    TEST_ASSERT_EQUAL_HEX8(0x55U, buffer[1]);

    capture_decode(&BUS_I2C3, &txn);
    assert_clean_capture(&txn);
    assert_only_bus_pins_touched(&BUS_I2C3);
    TEST_ASSERT_EQUAL_UINT(2U, txn.starts);
    TEST_ASSERT_EQUAL_UINT(2U, txn.stops);
    TEST_ASSERT_EQUAL_UINT(2U, txn.phase_count);
    TEST_ASSERT_EQUAL_UINT(2U, txn.phase_frames[0]);
    TEST_ASSERT_EQUAL_UINT(3U, txn.phase_frames[1]);
    TEST_ASSERT_EQUAL_HEX8(0xD0U, txn.byte[0]);
    TEST_ASSERT_EQUAL_HEX8(0x75U, txn.byte[1]);
    TEST_ASSERT_EQUAL_HEX8(0xD1U, txn.byte[2]);
    TEST_ASSERT_EQUAL_HEX8(0xAAU, txn.byte[3]);
    TEST_ASSERT_EQUAL_HEX8(0x55U, txn.byte[4]);
    TEST_ASSERT_EQUAL_UINT(NACK_LEVEL, txn.ack[4]);
}

static void test_mem_read_propagates_read_address_nack_and_preserves_output(void)
{
    void *bus = pandora_soft_i2c3_init();
    uint8_t buffer[2] = {0xEEU, 0xEEU};
    transaction_t txn;

    g_event_count = 0U;
    script_read(ACK_LEVEL);
    script_read(ACK_LEVEL);
    script_read(NACK_LEVEL);

    TEST_ASSERT_EQUAL_INT(XY_HAL_ERROR_IO,
                          xy_hal_i2c_mem_read(bus, 0x68U, 0x75U, buffer, sizeof(buffer), 100U));

    TEST_ASSERT_EQUAL_HEX8(0xEEU, buffer[0]);
    TEST_ASSERT_EQUAL_HEX8(0xEEU, buffer[1]);

    capture_decode(&BUS_I2C3, &txn);
    assert_clean_capture(&txn);
    TEST_ASSERT_EQUAL_UINT(3U, txn.frame_count);
    TEST_ASSERT_EQUAL_UINT(NACK_LEVEL, txn.ack[2]);
    TEST_ASSERT_EQUAL_UINT(2U, txn.stops);
}

static void test_mem_read_rejects_widened_register_address(void)
{
    void *bus = pandora_soft_i2c3_init();
    uint8_t buffer[1] = {0x00U};

    g_event_count = 0U;
    TEST_ASSERT_EQUAL_INT(XY_HAL_ERROR_INVALID_PARAM,
                          xy_hal_i2c_mem_read(bus, 0x68U, 0x0100U, buffer, sizeof(buffer), 100U));
    assert_no_bus_activity();
}

/* --- software I2C: device readiness ------------------------------------ */

static void test_is_device_ready_acks_a_present_device(void)
{
    void *bus = pandora_soft_i2c3_init();
    transaction_t txn;

    g_event_count = 0U;
    script_read(ACK_LEVEL);

    TEST_ASSERT_EQUAL_INT(XY_HAL_OK, xy_hal_i2c_is_device_ready(bus, 0x68U, 1U, 100U));

    capture_decode(&BUS_I2C3, &txn);
    assert_clean_capture(&txn);
    TEST_ASSERT_EQUAL_UINT(1U, txn.frame_count);
    TEST_ASSERT_EQUAL_HEX8(0xD0U, txn.byte[0]);
    TEST_ASSERT_EQUAL_UINT(ACK_LEVEL, txn.ack[0]);
}

static void test_is_device_ready_reports_missing_device(void)
{
    void *bus = pandora_soft_i2c3_init();

    g_event_count = 0U;
    script_read(NACK_LEVEL);

    TEST_ASSERT_EQUAL_INT(XY_HAL_ERROR_IO, xy_hal_i2c_is_device_ready(bus, 0x68U, 1U, 100U));
}

static void test_is_device_ready_rejects_zero_trials_and_reserved_address(void)
{
    void *bus = pandora_soft_i2c3_init();

    g_event_count = 0U;
    TEST_ASSERT_EQUAL_INT(XY_HAL_ERROR_INVALID_PARAM,
                          xy_hal_i2c_is_device_ready(bus, 0x68U, 0U, 100U));
    TEST_ASSERT_EQUAL_INT(XY_HAL_ERROR_INVALID_PARAM,
                          xy_hal_i2c_is_device_ready(bus, 0x80U, 1U, 100U));
    TEST_ASSERT_EQUAL_INT(XY_HAL_ERROR_INVALID_PARAM,
                          xy_hal_i2c_is_device_ready(NULL, 0x68U, 1U, 100U));
    assert_no_bus_activity();
}

/* --- hardware I2C3 ----------------------------------------------------- */

static void test_hw_i2c3_init_assigns_instance_before_controller_init(void)
{
    void *handle = pandora_hw_i2c3_init();

    TEST_ASSERT_NOT_NULL(handle);
    TEST_ASSERT_EQUAL_UINT(1U, g_i2c_init_calls);
    TEST_ASSERT_EQUAL_PTR(handle, g_i2c_init_handle);
    /* Vendor HAL needs the peripheral identity before HAL_I2C_Init(). */
    TEST_ASSERT_EQUAL_INT(1, g_i2c_init_instance_ok);
    TEST_ASSERT_EQUAL_UINT32(100000U, g_i2c_init_config.clock_speed);
    TEST_ASSERT_EQUAL_INT(XY_HAL_I2C_ADDR_7BIT, g_i2c_init_config.addr_mode);
    TEST_ASSERT_EQUAL_INT(XY_HAL_I2C_DUTY_2, g_i2c_init_config.duty_cycle);
    TEST_ASSERT_EQUAL_UINT16(0U, g_i2c_init_config.own_address);
    TEST_ASSERT_EQUAL_UINT8(0U, g_i2c_init_config.general_call_mode);
    TEST_ASSERT_EQUAL_UINT(1U, g_rcc_gpioc);
    TEST_ASSERT_EQUAL_UINT(1U, g_rcc_i2c3);
}

static void test_hw_i2c3_init_configures_i2c3_alternate_function(void)
{
    TEST_ASSERT_NOT_NULL(pandora_hw_i2c3_init());

    TEST_ASSERT_EQUAL_UINT(1U, g_config_count[PORT_C][0]);
    TEST_ASSERT_EQUAL_UINT(1U, g_config_count[PORT_C][1]);
    TEST_ASSERT_EQUAL_INT(XY_HAL_GPIO_MODE_AF, g_config[PORT_C][0].mode);
    TEST_ASSERT_EQUAL_INT(XY_HAL_GPIO_MODE_AF, g_config[PORT_C][1].mode);
    TEST_ASSERT_EQUAL_INT(XY_HAL_GPIO_PULL_UP, g_config[PORT_C][0].pull);
    TEST_ASSERT_EQUAL_INT(XY_HAL_GPIO_OTYPE_OD, g_config[PORT_C][0].otype);
    TEST_ASSERT_EQUAL_INT(XY_HAL_GPIO_SPEED_VERY_HIGH, g_config[PORT_C][0].speed);
    TEST_ASSERT_EQUAL_UINT8(GPIO_AF4_I2C3, g_config[PORT_C][0].alternate);
    TEST_ASSERT_EQUAL_UINT8(GPIO_AF4_I2C3, g_config[PORT_C][1].alternate);
    TEST_ASSERT_EQUAL_UINT(0U, g_config_count[PORT_D][6]);
}

static void test_hw_i2c3_init_returns_null_when_pin_configuration_fails(void)
{
    g_fail_init_at = 1U;

    TEST_ASSERT_NULL(pandora_hw_i2c3_init());
    TEST_ASSERT_EQUAL_UINT(0U, g_i2c_init_calls);
}

static void test_hw_i2c3_init_returns_null_when_controller_init_fails(void)
{
    g_next_i2c_init_result = XY_HAL_ERROR_FAIL;

    TEST_ASSERT_NULL(pandora_hw_i2c3_init());
    TEST_ASSERT_EQUAL_UINT(1U, g_i2c_init_calls);
}

static void test_hw_i2c3_init_reuses_the_board_owned_handle(void)
{
    void *first = pandora_hw_i2c3_init();
    void *second = pandora_hw_i2c3_init();

    TEST_ASSERT_NOT_NULL(first);
    TEST_ASSERT_EQUAL_PTR(first, second);
    TEST_ASSERT_EQUAL_UINT(2U, g_i2c_init_calls);
}

/* --- runner ------------------------------------------------------------ */

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_soft_i2c3_init_wires_scl_pc0_and_sda_pc1);
    RUN_TEST(test_soft_i2c4_init_wires_scl_pd6_and_sda_pc1);
    RUN_TEST(test_soft_i2c_init_is_the_i2c4_bus_alias);
    RUN_TEST(test_soft_init_returns_null_when_sda_configuration_fails);
    RUN_TEST(test_soft_init_returns_null_when_scl_configuration_fails);
    RUN_TEST(test_soft_init_releases_both_lines_after_configuring);
    RUN_TEST(test_probe_reports_ack_and_emits_address_write_frame);
    RUN_TEST(test_probe_reports_nack_and_still_issues_stop);
    RUN_TEST(test_probe_rejects_reserved_address_without_bus_activity);
    RUN_TEST(test_probe_rejects_foreign_bus_without_bus_activity);
    RUN_TEST(test_probe_uses_the_requested_bus_pins);
    RUN_TEST(test_transmit_writes_address_then_payload_msb_first);
    RUN_TEST(test_transmit_propagates_address_nack_and_skips_payload);
    RUN_TEST(test_transmit_stops_at_first_nacked_payload_byte);
    RUN_TEST(test_transmit_rejects_invalid_arguments_without_bus_activity);
    RUN_TEST(test_transmit_accepts_the_maximum_seven_bit_address);
    RUN_TEST(test_receive_reads_address_with_read_bit_then_payload_msb_first);
    RUN_TEST(test_receive_nacks_a_single_byte_read);
    RUN_TEST(test_receive_propagates_address_nack_and_preserves_output);
    RUN_TEST(test_receive_rejects_invalid_arguments_without_bus_activity);
    RUN_TEST(test_mem_write_sends_register_then_payload);
    RUN_TEST(test_mem_write_propagates_register_nack);
    RUN_TEST(test_mem_write_rejects_widened_register_address);
    RUN_TEST(test_mem_write_rejects_invalid_arguments);
    RUN_TEST(test_mem_read_writes_register_then_restarts_with_read_address);
    RUN_TEST(test_mem_read_propagates_read_address_nack_and_preserves_output);
    RUN_TEST(test_mem_read_rejects_widened_register_address);
    RUN_TEST(test_is_device_ready_acks_a_present_device);
    RUN_TEST(test_is_device_ready_reports_missing_device);
    RUN_TEST(test_is_device_ready_rejects_zero_trials_and_reserved_address);
    RUN_TEST(test_hw_i2c3_init_assigns_instance_before_controller_init);
    RUN_TEST(test_hw_i2c3_init_configures_i2c3_alternate_function);
    RUN_TEST(test_hw_i2c3_init_returns_null_when_pin_configuration_fails);
    RUN_TEST(test_hw_i2c3_init_returns_null_when_controller_init_fails);
    RUN_TEST(test_hw_i2c3_init_reuses_the_board_owned_handle);
    return UNITY_END();
}
