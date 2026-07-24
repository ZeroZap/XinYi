#include "unity.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sensor_bmp280.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
    void *bus;
    uint8_t addr;
    uint8_t reg;
    uint8_t data[24];
    uint16_t len;
    int ret;
} mem_op_t;

static mem_op_t g_mem_reads[8];
static mem_op_t g_mem_writes[8];
static size_t g_mem_read_count;
static size_t g_mem_read_index;
static size_t g_mem_write_count;
static size_t g_mem_write_index;
static uint32_t g_tick;
static uint32_t g_delay_total;
static unsigned int g_delay_count;

uint32_t get_tick_ms(void)
{
    return g_tick;
}

void delay_ms(uint32_t ms)
{
    g_delay_total += ms;
    g_delay_count++;
    g_tick += ms;
}

int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_mem_reads), g_mem_read_index);
    mem_op_t *op = &g_mem_reads[g_mem_read_index++];
    TEST_ASSERT_EQUAL_PTR(op->bus, bus);
    TEST_ASSERT_EQUAL_UINT8(op->addr, addr);
    TEST_ASSERT_EQUAL_UINT8(op->reg, reg);
    TEST_ASSERT_EQUAL_UINT16(op->len, len);
    if (op->ret == SENSOR_EOK) {
        memcpy(data, op->data, len);
    }
    return op->ret;
}

int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_mem_writes), g_mem_write_index);
    mem_op_t *op = &g_mem_writes[g_mem_write_index++];
    TEST_ASSERT_EQUAL_PTR(op->bus, bus);
    TEST_ASSERT_EQUAL_UINT8(op->addr, addr);
    TEST_ASSERT_EQUAL_UINT8(op->reg, reg);
    TEST_ASSERT_EQUAL_UINT16(op->len, len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(op->data, data, len);
    return op->ret;
}

static void queue_mem_read(void *bus, uint8_t addr, uint8_t reg, const uint8_t *data, uint16_t len,
                           int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_mem_reads), g_mem_read_count);
    mem_op_t *op = &g_mem_reads[g_mem_read_count++];
    op->bus = bus;
    op->addr = addr;
    op->reg = reg;
    op->len = len;
    op->ret = ret;
    if (data != NULL) {
        memcpy(op->data, data, len);
    }
}

static void queue_mem_read8(void *bus, uint8_t addr, uint8_t reg, uint8_t value, int ret)
{
    queue_mem_read(bus, addr, reg, &value, 1U, ret);
}

static void queue_mem_write8(void *bus, uint8_t addr, uint8_t reg, uint8_t value, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_mem_writes), g_mem_write_count);
    mem_op_t *op = &g_mem_writes[g_mem_write_count++];
    op->bus = bus;
    op->addr = addr;
    op->reg = reg;
    op->len = 1U;
    op->ret = ret;
    op->data[0] = value;
}

void setUp(void)
{
    memset(g_mem_reads, 0, sizeof(g_mem_reads));
    memset(g_mem_writes, 0, sizeof(g_mem_writes));
    g_mem_read_count = 0;
    g_mem_read_index = 0;
    g_mem_write_count = 0;
    g_mem_write_index = 0;
    g_tick = 1000U;
    g_delay_total = 0U;
    g_delay_count = 0U;
}

void tearDown(void)
{
}

static void destroy_sensor(sensor_device_t *sensor)
{
    if (sensor != NULL) {
        SENSOR_FREE(sensor->priv_data);
        SENSOR_FREE(sensor);
    }
}

static void queue_standard_calibration(void *bus)
{
    const uint8_t calib[24] = {
        0x70, 0x6B, 0x43, 0x67, 0x18, 0xFC, 0x7D, 0x8E, 0x43, 0xD6, 0xD0, 0x0B,
        0x27, 0x0B, 0x8C, 0x00, 0xF9, 0xFF, 0x8C, 0x3C, 0xF8, 0xC6, 0x70, 0x17,
    };
    queue_mem_read(bus, BMP280_ADDR_DEFAULT, BMP280_REG_CALIB00, calib, sizeof(calib), SENSOR_EOK);
}

static sensor_device_t *create_initialized_pressure_sensor(int *fake_bus)
{
    sensor_device_t *sensor = bmp280_create_pressure("bmp280-main", fake_bus);
    TEST_ASSERT_NOT_NULL(sensor);
    queue_mem_read8(fake_bus, BMP280_ADDR_DEFAULT, BMP280_REG_CHIP_ID, BMP280_CHIP_ID, SENSOR_EOK);
    queue_mem_write8(fake_bus, BMP280_ADDR_DEFAULT, BMP280_REG_RESET, 0xB6U, SENSOR_EOK);
    queue_standard_calibration(fake_bus);
    queue_mem_write8(fake_bus, BMP280_ADDR_DEFAULT, BMP280_REG_CONFIG, 0x00U, SENSOR_EOK);
    queue_mem_write8(fake_bus, BMP280_ADDR_DEFAULT, BMP280_REG_CTRL_MEAS, 0x27U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    return sensor;
}

static void test_bmp280_create_defaults_and_init_reads_calibration(void)
{
    int fake_bus;
    sensor_device_t *sensor = create_initialized_pressure_sensor(&fake_bus);
    bmp280_priv_t *priv = (bmp280_priv_t *)sensor->priv_data;

    TEST_ASSERT_EQUAL_STRING("bmp280-main", sensor->info.name);
    TEST_ASSERT_EQUAL_STRING("Bosch", sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING("BMP280", sensor->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_PRESSURE, sensor->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_PASCAL, sensor->info.unit);
    TEST_ASSERT_EQUAL_INT32(110000, sensor->info.range_max);
    TEST_ASSERT_EQUAL_INT32(30000, sensor->info.range_min);
    TEST_ASSERT_EQUAL_UINT32(157U, sensor->info.max_odr);
    TEST_ASSERT_EQUAL_UINT32(26U, sensor->odr);
    TEST_ASSERT_EQUAL_PTR(&fake_bus, sensor->bus);
    TEST_ASSERT_EQUAL_UINT8(BMP280_ADDR_DEFAULT, priv->i2c_addr);
    TEST_ASSERT_EQUAL_UINT16(27504U, priv->dig_T1);
    TEST_ASSERT_EQUAL_INT16(26435, priv->dig_T2);
    TEST_ASSERT_EQUAL_INT16(-1000, priv->dig_T3);
    TEST_ASSERT_EQUAL_UINT16(36477U, priv->dig_P1);
    TEST_ASSERT_EQUAL_INT16(-10685, priv->dig_P2);
    TEST_ASSERT_EQUAL_INT16(3024, priv->dig_P3);
    TEST_ASSERT_EQUAL_UINT32(10U, g_delay_total);
    TEST_ASSERT_EQUAL_UINT(1U, g_delay_count);
    TEST_ASSERT_EQUAL_UINT(3U, g_mem_write_index);

    destroy_sensor(sensor);
}

static void test_bmp280_read_pressure_and_temperature_use_calibration(void)
{
    int fake_bus;
    uint8_t raw[6] = {0x65, 0x5A, 0xC0, 0x7E, 0xED, 0x00};
    sensor_data_t pressure = {0};
    sensor_data_t temperature = {0};
    sensor_device_t *pressure_sensor = create_initialized_pressure_sensor(&fake_bus);
    bmp280_priv_t *shared_priv = (bmp280_priv_t *)pressure_sensor->priv_data;
    sensor_device_t *temperature_sensor = bmp280_create_temperature("bmp280-temp", &fake_bus);

    TEST_ASSERT_NOT_NULL(temperature_sensor);
    SENSOR_FREE(temperature_sensor->priv_data);
    temperature_sensor->priv_data = shared_priv;

    queue_mem_read(&fake_bus, BMP280_ADDR_DEFAULT, BMP280_REG_PRESS_MSB, raw, sizeof(raw), SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, pressure_sensor->ops->read(pressure_sensor, &pressure));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_PRESSURE, pressure.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_PASCAL, pressure.unit);
    TEST_ASSERT_EQUAL_UINT32(100653U, pressure.value.val_uint32);
    TEST_ASSERT_EQUAL_UINT32(g_tick, pressure.timestamp);
    TEST_ASSERT_EQUAL_UINT8(98U, pressure.accuracy);

    queue_mem_read(&fake_bus, BMP280_ADDR_DEFAULT, BMP280_REG_PRESS_MSB, raw, sizeof(raw), SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, temperature_sensor->ops->read(temperature_sensor, &temperature));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_TEMPERATURE, temperature.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_CELSIUS, temperature.unit);
#if SENSOR_USE_FLOAT
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 25.08f, temperature.value.val_float);
#else
    TEST_ASSERT_EQUAL_INT32(2508, temperature.value.val_int32);
#endif
    TEST_ASSERT_EQUAL_UINT32(g_tick, temperature.timestamp);
    TEST_ASSERT_EQUAL_UINT8(98U, temperature.accuracy);

    temperature_sensor->priv_data = NULL;
    destroy_sensor(temperature_sensor);
    destroy_sensor(pressure_sensor);
}

static void test_bmp280_init_maps_chip_id_and_calibration_failures(void)
{
    int fake_bus;
    sensor_device_t *sensor = bmp280_create_pressure("bmp280-fail", &fake_bus);

    TEST_ASSERT_NOT_NULL(sensor);
    queue_mem_read8(&fake_bus, BMP280_ADDR_DEFAULT, BMP280_REG_CHIP_ID, 0x00U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_ERROR, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_UINT(0U, g_mem_write_index);
    TEST_ASSERT_EQUAL_UINT32(0U, g_delay_total);

    queue_mem_read8(&fake_bus, BMP280_ADDR_DEFAULT, BMP280_REG_CHIP_ID, BMP280_CHIP_ID, SENSOR_EOK);
    queue_mem_write8(&fake_bus, BMP280_ADDR_DEFAULT, BMP280_REG_RESET, 0xB6U, SENSOR_EOK);
    queue_mem_read(&fake_bus, BMP280_ADDR_DEFAULT, BMP280_REG_CALIB00, NULL, 24U, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_ERROR, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_UINT(1U, g_mem_write_index);
    TEST_ASSERT_EQUAL_UINT32(10U, g_delay_total);

    destroy_sensor(sensor);
}

static void test_bmp280_read_failure_preserves_output(void)
{
    int fake_bus;
    sensor_data_t data = {
        .type = SENSOR_TYPE_LIGHT,
        .unit = SENSOR_UNIT_LUX,
        .value.val_uint32 = 0x12345678U,
        .timestamp = 77U,
        .accuracy = 9U,
    };
    sensor_device_t *sensor = create_initialized_pressure_sensor(&fake_bus);

    queue_mem_read(&fake_bus, BMP280_ADDR_DEFAULT, BMP280_REG_PRESS_MSB, NULL, 6U, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_LIGHT, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_LUX, data.unit);
    TEST_ASSERT_EQUAL_UINT32(0x12345678U, data.value.val_uint32);
    TEST_ASSERT_EQUAL_UINT32(77U, data.timestamp);
    TEST_ASSERT_EQUAL_UINT8(9U, data.accuracy);

    destroy_sensor(sensor);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_bmp280_create_defaults_and_init_reads_calibration);
    RUN_TEST(test_bmp280_read_pressure_and_temperature_use_calibration);
    RUN_TEST(test_bmp280_init_maps_chip_id_and_calibration_failures);
    RUN_TEST(test_bmp280_read_failure_preserves_output);
    return UNITY_END();
}
