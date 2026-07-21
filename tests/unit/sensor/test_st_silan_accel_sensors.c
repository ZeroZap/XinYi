#include "unity.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sensor_lis2dh12.h"
#include "sensor_lis2dw12.h"
#include "sensor_sc7a20.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
    void *bus;
    uint8_t addr;
    uint8_t reg;
    uint8_t data[8];
    uint16_t len;
    int ret;
} i2c_op_t;

static i2c_op_t g_reads[64];
static i2c_op_t g_writes[64];
static size_t g_read_count;
static size_t g_read_index;
static size_t g_write_count;
static size_t g_write_index;
static uint32_t g_tick;

uint32_t get_tick_ms(void)
{
    return g_tick;
}

void delay_ms(uint32_t ms)
{
    g_tick += ms;
}

int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_reads), g_read_index);
    i2c_op_t *op = &g_reads[g_read_index++];
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
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_writes), g_write_index);
    i2c_op_t *op = &g_writes[g_write_index++];
    TEST_ASSERT_EQUAL_PTR(op->bus, bus);
    TEST_ASSERT_EQUAL_UINT8(op->addr, addr);
    TEST_ASSERT_EQUAL_UINT8(op->reg, reg);
    TEST_ASSERT_EQUAL_UINT16(op->len, len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(op->data, data, len);
    return op->ret;
}

int hal_spi_recv(void *bus, uint8_t cs, uint8_t *data, uint16_t len)
{
    (void)bus;
    (void)cs;
    (void)data;
    (void)len;
    return SENSOR_EIO;
}

int hal_spi_send(void *bus, uint8_t cs, uint8_t *data, uint16_t len)
{
    (void)bus;
    (void)cs;
    (void)data;
    (void)len;
    return SENSOR_EIO;
}

static void queue_read(void *bus, uint8_t addr, uint8_t reg, const uint8_t *data, uint16_t len, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_reads), g_read_count);
    i2c_op_t *op = &g_reads[g_read_count++];
    op->bus = bus;
    op->addr = addr;
    op->reg = reg;
    op->len = len;
    op->ret = ret;
    if (data != NULL) {
        memcpy(op->data, data, len);
    }
}

static void queue_read8(void *bus, uint8_t addr, uint8_t reg, uint8_t value, int ret)
{
    queue_read(bus, addr, reg, &value, 1U, ret);
}

static void queue_write8(void *bus, uint8_t addr, uint8_t reg, uint8_t value, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_writes), g_write_count);
    i2c_op_t *op = &g_writes[g_write_count++];
    op->bus = bus;
    op->addr = addr;
    op->reg = reg;
    op->len = 1U;
    op->ret = ret;
    op->data[0] = value;
}

void setUp(void)
{
    memset(g_reads, 0, sizeof(g_reads));
    memset(g_writes, 0, sizeof(g_writes));
    g_read_count = 0;
    g_read_index = 0;
    g_write_count = 0;
    g_write_index = 0;
    g_tick = 30000U;
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

static void assert_common_accel(sensor_device_t *sensor, const char *name, const char *vendor,
                                const char *model, int32_t resolution, int32_t max_odr,
                                int32_t odr)
{
    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_STRING(name, sensor->info.name);
    TEST_ASSERT_EQUAL_STRING(vendor, sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING(model, sensor->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_ACCELEROMETER, sensor->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_MILLI_G, sensor->info.unit);
    TEST_ASSERT_EQUAL_INT32(2000, sensor->info.range_max);
    TEST_ASSERT_EQUAL_INT32(-2000, sensor->info.range_min);
    TEST_ASSERT_EQUAL_INT32(resolution, sensor->info.resolution);
    TEST_ASSERT_EQUAL_INT32(max_odr, sensor->info.max_odr);
    TEST_ASSERT_EQUAL_INT32(odr, sensor->odr);
    TEST_ASSERT_EQUAL_INT(SENSOR_STATUS_IDLE, sensor->status);
    TEST_ASSERT_NOT_NULL(sensor->ops->init);
    TEST_ASSERT_NOT_NULL(sensor->ops->deinit);
    TEST_ASSERT_NOT_NULL(sensor->ops->read);
}

static void test_lis2dh12_init_read_config_deinit_and_errors(void)
{
    int fake_bus;
    sensor_data_t data = {0};
    uint8_t raw[6] = {0x00, 0x10, 0x00, 0x80, 0xF0, 0x7F};
    uint32_t cfg;
    sensor_device_t *sensor = lis2dh12_create("lis2dh12-main", &fake_bus);

    assert_common_accel(sensor, "lis2dh12-main", "STMicroelectronics", "LIS2DH12", 12, 400, 10);
    TEST_ASSERT_EQUAL_UINT8(LIS2DH12_ADDR_DEFAULT, ((lis2dh12_priv_t *)sensor->priv_data)->i2c_addr);
    TEST_ASSERT_EQUAL_INT(LIS2DH12_MODE_LOW_POWER, ((lis2dh12_priv_t *)sensor->priv_data)->mode);

    queue_read8(&fake_bus, LIS2DH12_ADDR_DEFAULT, LIS2DH12_REG_WHOAMI, LIS2DH12_WHOAMI_VALUE, SENSOR_EOK);
    queue_write8(&fake_bus, LIS2DH12_ADDR_DEFAULT, LIS2DH12_REG_CTRL1, LIS2DH12_ODR_10HZ | 0x07U, SENSOR_EOK);
    queue_write8(&fake_bus, LIS2DH12_ADDR_DEFAULT, LIS2DH12_REG_CTRL4, LIS2DH12_RANGE_2G | 0x08U, SENSOR_EOK);
    queue_write8(&fake_bus, LIS2DH12_ADDR_DEFAULT, LIS2DH12_REG_TEMP_CFG, 0xC0U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_INT(LIS2DH12_ODR_10HZ, ((lis2dh12_priv_t *)sensor->priv_data)->odr);
    TEST_ASSERT_EQUAL_INT(LIS2DH12_RANGE_2G, ((lis2dh12_priv_t *)sensor->priv_data)->range);
    TEST_ASSERT_EQUAL_UINT8(2U, ((lis2dh12_priv_t *)sensor->priv_data)->range_g);

    queue_read(&fake_bus, LIS2DH12_ADDR_DEFAULT, LIS2DH12_REG_OUT_X_L | 0x80U, raw, sizeof(raw), SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_ACCELEROMETER, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_MILLI_G, data.unit);
    TEST_ASSERT_EQUAL_INT32(0, data.value.val_3axis.x);
    TEST_ASSERT_EQUAL_INT32(0, data.value.val_3axis.y);
    TEST_ASSERT_EQUAL_INT32(0, data.value.val_3axis.z);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);
    TEST_ASSERT_EQUAL_UINT8(94U, data.accuracy);

    cfg = 8U;
    queue_write8(&fake_bus, LIS2DH12_ADDR_DEFAULT, LIS2DH12_REG_CTRL4, LIS2DH12_RANGE_8G | 0x08U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->config(sensor, SENSOR_CFG_RANGE, &cfg));
    TEST_ASSERT_EQUAL_UINT8(8U, ((lis2dh12_priv_t *)sensor->priv_data)->range_g);
    cfg = 100U;
    queue_write8(&fake_bus, LIS2DH12_ADDR_DEFAULT, LIS2DH12_REG_CTRL1, LIS2DH12_ODR_100HZ | 0x07U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->config(sensor, SENSOR_CFG_ODR, &cfg));
    TEST_ASSERT_EQUAL_UINT32(100U, sensor->odr);

    queue_write8(&fake_bus, LIS2DH12_ADDR_DEFAULT, LIS2DH12_REG_CTRL1, LIS2DH12_ODR_POWER_DOWN, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->deinit(sensor));
    queue_read8(&fake_bus, LIS2DH12_ADDR_DEFAULT, LIS2DH12_REG_WHOAMI, 0x00U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_ERROR, sensor->ops->init(sensor));
    queue_read(&fake_bus, LIS2DH12_ADDR_DEFAULT, LIS2DH12_REG_OUT_X_L | 0x80U, NULL, 6U, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->read(sensor, &data));

    destroy_sensor(sensor);
}

static void test_lis2dw12_i2c_init_read_helpers_deinit_and_errors(void)
{
    int fake_bus;
    sensor_data_t data = {0};
    uint8_t raw[6] = {0x00, 0x01, 0x00, 0xFF, 0x00, 0x40};
    sensor_device_t *sensor = lis2dw12_create("lis2dw12-main", &fake_bus, 0U);

    assert_common_accel(sensor, "lis2dw12-main", "STMicro", "LIS2DW12", 14, 800, 100);
    TEST_ASSERT_EQUAL_UINT8(LIS2DW12_ADDR_DEFAULT, ((lis2dw12_priv_t *)sensor->priv_data)->i2c_addr);
    TEST_ASSERT_EQUAL_UINT8(LIS2DW12_SPI_CS_NONE, ((lis2dw12_priv_t *)sensor->priv_data)->spi_cs);

    queue_read8(&fake_bus, LIS2DW12_ADDR_DEFAULT, LIS2DW12_REG_WHOAMI, LIS2DW12_WHOAMI_VALUE, SENSOR_EOK);
    queue_write8(&fake_bus, LIS2DW12_ADDR_DEFAULT, LIS2DW12_REG_CTRL2, 0x04U, SENSOR_EOK);
    queue_write8(&fake_bus, LIS2DW12_ADDR_DEFAULT, LIS2DW12_REG_CTRL1,
                 (uint8_t)((LIS2DW12_MODE_LOW_POWER << 5) | (LIS2DW12_RATE_100HZ << 2) | LIS2DW12_RANGE_2G), SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_UINT8(2U, ((lis2dw12_priv_t *)sensor->priv_data)->range);
    TEST_ASSERT_EQUAL_UINT8(100U, ((lis2dw12_priv_t *)sensor->priv_data)->rate);
    TEST_ASSERT_EQUAL_UINT8(LIS2DW12_MODE_LOW_POWER, ((lis2dw12_priv_t *)sensor->priv_data)->mode);

    for (uint8_t i = 0; i < sizeof(raw); ++i) {
        queue_read8(&fake_bus, LIS2DW12_ADDR_DEFAULT, (uint8_t)(LIS2DW12_REG_OUT_X_L + i), raw[i], SENSOR_EOK);
    }
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT32(256, data.value.val_3axis.x);
    TEST_ASSERT_EQUAL_INT32(-256, data.value.val_3axis.y);
    TEST_ASSERT_EQUAL_INT32(16384, data.value.val_3axis.z);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);
    TEST_ASSERT_EQUAL_UINT8(95U, data.accuracy);

    queue_read8(&fake_bus, LIS2DW12_ADDR_DEFAULT, LIS2DW12_REG_CTRL1, 0xA0U, SENSOR_EOK);
    queue_write8(&fake_bus, LIS2DW12_ADDR_DEFAULT, LIS2DW12_REG_CTRL1, 0xA2U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(0, lis2dw12_set_range(sensor, LIS2DW12_RANGE_8G));
    TEST_ASSERT_EQUAL_UINT8(8U, ((lis2dw12_priv_t *)sensor->priv_data)->range);
    queue_read8(&fake_bus, LIS2DW12_ADDR_DEFAULT, LIS2DW12_REG_CTRL1, 0x03U, SENSOR_EOK);
    queue_write8(&fake_bus, LIS2DW12_ADDR_DEFAULT, LIS2DW12_REG_CTRL1, 0x17U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(0, lis2dw12_set_rate(sensor, LIS2DW12_RATE_100HZ));
    TEST_ASSERT_EQUAL_UINT8(100U, ((lis2dw12_priv_t *)sensor->priv_data)->rate);
    queue_read8(&fake_bus, LIS2DW12_ADDR_DEFAULT, LIS2DW12_REG_CTRL5, 0x00U, SENSOR_EOK);
    queue_write8(&fake_bus, LIS2DW12_ADDR_DEFAULT, LIS2DW12_REG_CTRL5, 0x40U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(0, lis2dw12_enable_high_pass(sensor, 1U));

    queue_write8(&fake_bus, LIS2DW12_ADDR_DEFAULT, LIS2DW12_REG_CTRL1, LIS2DW12_RATE_POWER_DOWN << 2, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->deinit(sensor));
    queue_read8(&fake_bus, LIS2DW12_ADDR_DEFAULT, LIS2DW12_REG_WHOAMI, 0x00U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_ERROR, sensor->ops->init(sensor));

    destroy_sensor(sensor);
}

static void test_sc7a20_init_read_config_deinit_and_errors(void)
{
    int fake_bus;
    sensor_data_t data = {0};
    uint8_t raw[6] = {0x00, 0x10, 0x00, 0x80, 0xF0, 0x7F};
    uint32_t cfg;
    sensor_device_t *sensor = sc7a20_create("sc7a20-main", &fake_bus);

    assert_common_accel(sensor, "sc7a20-main", "Silan", "SC7A20", 12, 400, 100);
    TEST_ASSERT_EQUAL_UINT8(SC7A20_ADDR_DEFAULT, ((sc7a20_priv_t *)sensor->priv_data)->i2c_addr);
    TEST_ASSERT_EQUAL_UINT8(2U, ((sc7a20_priv_t *)sensor->priv_data)->range);

    queue_read8(&fake_bus, SC7A20_ADDR_DEFAULT, SC7A20_REG_WHOAMI, SC7A20_WHOAMI_VALUE, SENSOR_EOK);
    queue_write8(&fake_bus, SC7A20_ADDR_DEFAULT, SC7A20_REG_CTRL_REG1, SC7A20_ODR_100HZ | 0x07U, SENSOR_EOK);
    queue_write8(&fake_bus, SC7A20_ADDR_DEFAULT, SC7A20_REG_CTRL_REG4, SC7A20_RANGE_2G | 0x08U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_UINT8(SC7A20_ODR_100HZ | 0x07U, ((sc7a20_priv_t *)sensor->priv_data)->odr_reg);
    TEST_ASSERT_EQUAL_UINT8(2U, ((sc7a20_priv_t *)sensor->priv_data)->range);

    queue_read(&fake_bus, SC7A20_ADDR_DEFAULT, SC7A20_REG_OUT_X_L | 0x80U, raw, sizeof(raw), SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT32(0, data.value.val_3axis.x);
    TEST_ASSERT_EQUAL_INT32(0, data.value.val_3axis.y);
    TEST_ASSERT_EQUAL_INT32(0, data.value.val_3axis.z);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);
    TEST_ASSERT_EQUAL_UINT8(95U, data.accuracy);

    cfg = 16U;
    queue_write8(&fake_bus, SC7A20_ADDR_DEFAULT, SC7A20_REG_CTRL_REG4, SC7A20_RANGE_16G | 0x08U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->config(sensor, SENSOR_CFG_RANGE, &cfg));
    TEST_ASSERT_EQUAL_UINT8(16U, ((sc7a20_priv_t *)sensor->priv_data)->range);
    cfg = 25U;
    queue_write8(&fake_bus, SC7A20_ADDR_DEFAULT, SC7A20_REG_CTRL_REG1, SC7A20_ODR_25HZ | 0x07U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->config(sensor, SENSOR_CFG_ODR, &cfg));
    TEST_ASSERT_EQUAL_UINT32(25U, sensor->odr);

    queue_write8(&fake_bus, SC7A20_ADDR_DEFAULT, SC7A20_REG_CTRL_REG1, SC7A20_ODR_POWER_DOWN, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->deinit(sensor));
    queue_read8(&fake_bus, SC7A20_ADDR_DEFAULT, SC7A20_REG_WHOAMI, 0x00U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_ERROR, sensor->ops->init(sensor));
    queue_read(&fake_bus, SC7A20_ADDR_DEFAULT, SC7A20_REG_OUT_X_L | 0x80U, NULL, 6U, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->read(sensor, &data));

    destroy_sensor(sensor);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_lis2dh12_init_read_config_deinit_and_errors);
    RUN_TEST(test_lis2dw12_i2c_init_read_helpers_deinit_and_errors);
    RUN_TEST(test_sc7a20_init_read_config_deinit_and_errors);
    return UNITY_END();
}
