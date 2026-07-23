#include "unity.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sensor_adxl362.h"
#include "sensor_bma400.h"
#include "sensor_kx023.h"
#include "sensor_lis2dw12.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
    void *bus;
    uint8_t reg;
    uint8_t data[8];
    uint16_t len;
    int ret;
} spi_op_t;

typedef struct {
    void *bus;
    uint8_t addr;
    uint8_t reg;
    uint8_t data[8];
    uint16_t len;
    int ret;
} i2c_op_t;

static spi_op_t g_spi_ops[32];
static i2c_op_t g_i2c_reads[32];
static i2c_op_t g_i2c_writes[32];
static spi_op_t g_spi_sends[32];
static spi_op_t g_spi_recvs[32];
static size_t g_spi_count;
static size_t g_spi_index;
static size_t g_i2c_read_count;
static size_t g_i2c_read_index;
static size_t g_i2c_write_count;
static size_t g_i2c_write_index;
static size_t g_spi_send_count;
static size_t g_spi_send_index;
static size_t g_spi_recv_count;
static size_t g_spi_recv_index;
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

int hal_spi_transfer(void *bus, uint8_t *tx, uint8_t *rx, uint16_t len)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_spi_ops), g_spi_index);
    spi_op_t *op = &g_spi_ops[g_spi_index++];
    TEST_ASSERT_EQUAL_PTR(op->bus, bus);
    TEST_ASSERT_EQUAL_UINT16(op->len + 2U, len);
    TEST_ASSERT_EQUAL_UINT8(op->reg, tx[1]);

    if (tx[0] == ADXL362_CMD_READ) {
        TEST_ASSERT_NOT_NULL(rx);
        if (op->ret == SENSOR_EOK) {
            memset(rx, 0, len);
            memcpy(&rx[2], op->data, op->len);
        }
    } else {
        TEST_ASSERT_EQUAL_UINT8(ADXL362_CMD_WRITE, tx[0]);
        TEST_ASSERT_EQUAL_UINT8_ARRAY(op->data, &tx[2], op->len);
    }

    return op->ret;
}

int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_i2c_reads), g_i2c_read_index);
    i2c_op_t *op = &g_i2c_reads[g_i2c_read_index++];
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
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_i2c_writes), g_i2c_write_index);
    i2c_op_t *op = &g_i2c_writes[g_i2c_write_index++];
    TEST_ASSERT_EQUAL_PTR(op->bus, bus);
    TEST_ASSERT_EQUAL_UINT8(op->addr, addr);
    TEST_ASSERT_EQUAL_UINT8(op->reg, reg);
    TEST_ASSERT_EQUAL_UINT16(op->len, len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(op->data, data, len);
    return op->ret;
}

int hal_spi_send(void *bus, uint8_t cs, uint8_t *data, uint16_t len)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_spi_sends), g_spi_send_index);
    spi_op_t *op = &g_spi_sends[g_spi_send_index++];
    TEST_ASSERT_EQUAL_PTR(op->bus, bus);
    TEST_ASSERT_EQUAL_UINT8(op->reg, cs);
    TEST_ASSERT_EQUAL_UINT16(op->len, len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(op->data, data, len);
    return op->ret;
}

int hal_spi_recv(void *bus, uint8_t cs, uint8_t *data, uint16_t len)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_spi_recvs), g_spi_recv_index);
    spi_op_t *op = &g_spi_recvs[g_spi_recv_index++];
    TEST_ASSERT_EQUAL_PTR(op->bus, bus);
    TEST_ASSERT_EQUAL_UINT8(op->reg, cs);
    TEST_ASSERT_EQUAL_UINT16(op->len, len);
    if (op->ret == SENSOR_EOK) {
        memcpy(data, op->data, len);
    }
    return op->ret;
}

static void queue_spi_read(void *bus, uint8_t reg, const uint8_t *data, uint16_t len, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_spi_ops), g_spi_count);
    spi_op_t *op = &g_spi_ops[g_spi_count++];
    op->bus = bus;
    op->reg = reg;
    op->len = len;
    op->ret = ret;
    if (data != NULL) {
        memcpy(op->data, data, len);
    }
}

static void queue_spi_read8(void *bus, uint8_t reg, uint8_t value, int ret)
{
    queue_spi_read(bus, reg, &value, 1U, ret);
}

static void queue_spi_write8(void *bus, uint8_t reg, uint8_t value, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_spi_ops), g_spi_count);
    spi_op_t *op = &g_spi_ops[g_spi_count++];
    op->bus = bus;
    op->reg = reg;
    op->len = 1U;
    op->ret = ret;
    op->data[0] = value;
}

static void queue_i2c_read(void *bus, uint8_t addr, uint8_t reg, const uint8_t *data, uint16_t len, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_i2c_reads), g_i2c_read_count);
    i2c_op_t *op = &g_i2c_reads[g_i2c_read_count++];
    op->bus = bus;
    op->addr = addr;
    op->reg = reg;
    op->len = len;
    op->ret = ret;
    if (data != NULL) {
        memcpy(op->data, data, len);
    }
}

static void queue_i2c_read8(void *bus, uint8_t addr, uint8_t reg, uint8_t value, int ret)
{
    queue_i2c_read(bus, addr, reg, &value, 1U, ret);
}

static void queue_i2c_write8(void *bus, uint8_t addr, uint8_t reg, uint8_t value, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_i2c_writes), g_i2c_write_count);
    i2c_op_t *op = &g_i2c_writes[g_i2c_write_count++];
    op->bus = bus;
    op->addr = addr;
    op->reg = reg;
    op->len = 1U;
    op->ret = ret;
    op->data[0] = value;
}

static void queue_spi_send(void *bus, uint8_t cs, const uint8_t *data, uint16_t len, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_spi_sends), g_spi_send_count);
    spi_op_t *op = &g_spi_sends[g_spi_send_count++];
    op->bus = bus;
    op->reg = cs;
    op->len = len;
    op->ret = ret;
    memcpy(op->data, data, len);
}

static void queue_spi_recv(void *bus, uint8_t cs, const uint8_t *data, uint16_t len, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_spi_recvs), g_spi_recv_count);
    spi_op_t *op = &g_spi_recvs[g_spi_recv_count++];
    op->bus = bus;
    op->reg = cs;
    op->len = len;
    op->ret = ret;
    if (data != NULL) {
        memcpy(op->data, data, len);
    }
}

static void queue_spi_register_read8(void *bus, uint8_t cs, uint8_t reg, uint8_t value, int send_ret,
                                     int recv_ret)
{
    uint8_t tx = reg | 0x80U;
    queue_spi_send(bus, cs, &tx, 1U, send_ret);
    if (send_ret == SENSOR_EOK) {
        queue_spi_recv(bus, cs, &value, 1U, recv_ret);
    }
}

static void queue_spi_register_write8(void *bus, uint8_t cs, uint8_t reg, uint8_t value, int ret)
{
    uint8_t tx[2] = { (uint8_t)(reg & 0x7FU), value };
    queue_spi_send(bus, cs, tx, sizeof(tx), ret);
}

void setUp(void)
{
    memset(g_spi_ops, 0, sizeof(g_spi_ops));
    memset(g_i2c_reads, 0, sizeof(g_i2c_reads));
    memset(g_i2c_writes, 0, sizeof(g_i2c_writes));
    memset(g_spi_sends, 0, sizeof(g_spi_sends));
    memset(g_spi_recvs, 0, sizeof(g_spi_recvs));
    g_spi_count = 0;
    g_spi_index = 0;
    g_i2c_read_count = 0;
    g_i2c_read_index = 0;
    g_i2c_write_count = 0;
    g_i2c_write_index = 0;
    g_spi_send_count = 0;
    g_spi_send_index = 0;
    g_spi_recv_count = 0;
    g_spi_recv_index = 0;
    g_tick = 24680U;
    g_delay_total = 0;
    g_delay_count = 0;
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
                                const char *model, int32_t max_odr, int32_t odr)
{
    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_STRING(name, sensor->info.name);
    TEST_ASSERT_EQUAL_STRING(vendor, sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING(model, sensor->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_ACCELEROMETER, sensor->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_MILLI_G, sensor->info.unit);
    TEST_ASSERT_EQUAL_INT32(2000, sensor->info.range_max);
    TEST_ASSERT_EQUAL_INT32(-2000, sensor->info.range_min);
    TEST_ASSERT_EQUAL_INT32(max_odr, sensor->info.max_odr);
    TEST_ASSERT_EQUAL_INT32(odr, sensor->odr);
    TEST_ASSERT_EQUAL_INT(SENSOR_STATUS_IDLE, sensor->status);
    TEST_ASSERT_NOT_NULL(sensor->ops);
    TEST_ASSERT_NOT_NULL(sensor->ops->init);
    TEST_ASSERT_NOT_NULL(sensor->ops->deinit);
    TEST_ASSERT_NOT_NULL(sensor->ops->read);
}

static void test_adxl362_create_init_read_deinit_and_error_paths(void)
{
    int fake_spi;
    sensor_data_t data = {0};
    uint8_t raw[6] = {0x34, 0x01, 0x00, 0x08, 0xFF, 0x07};
    sensor_device_t *sensor = adxl362_create("adxl362-main", &fake_spi);

    assert_common_accel(sensor, "adxl362-main", "Analog Devices", "ADXL362", 400, 100);
    TEST_ASSERT_EQUAL_PTR(&fake_spi, sensor->bus);
    TEST_ASSERT_EQUAL_PTR(&fake_spi, ((adxl362_priv_t *)sensor->priv_data)->spi_bus);

    queue_spi_read8(&fake_spi, ADXL362_REG_DEVID_AD, ADXL362_DEVID_AD, SENSOR_EOK);
    queue_spi_write8(&fake_spi, ADXL362_REG_FILTER_CTL,
                     (uint8_t)((ADXL362_ODR_100HZ << 3) | ADXL362_RANGE_2G), SENSOR_EOK);
    queue_spi_write8(&fake_spi, ADXL362_REG_POWER_CTL, ADXL362_MODE_MEASUREMENT, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_INT(ADXL362_ODR_100HZ, ((adxl362_priv_t *)sensor->priv_data)->odr);
    TEST_ASSERT_EQUAL_INT(ADXL362_RANGE_2G, ((adxl362_priv_t *)sensor->priv_data)->range);
    TEST_ASSERT_EQUAL_UINT8(2U, ((adxl362_priv_t *)sensor->priv_data)->range_g);
    TEST_ASSERT_EQUAL_INT(ADXL362_MODE_MEASUREMENT, ((adxl362_priv_t *)sensor->priv_data)->mode);

    queue_spi_read(&fake_spi, ADXL362_REG_XDATA, raw, sizeof(raw), SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_ACCELEROMETER, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_MILLI_G, data.unit);
    TEST_ASSERT_EQUAL_INT32(0x0134, data.value.val_3axis.x);
    TEST_ASSERT_EQUAL_INT32(-2048, data.value.val_3axis.y);
    TEST_ASSERT_EQUAL_INT32(2047, data.value.val_3axis.z);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);
    TEST_ASSERT_EQUAL_UINT8(95U, data.accuracy);

    queue_spi_write8(&fake_spi, ADXL362_REG_POWER_CTL, ADXL362_MODE_STANDBY, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->deinit(sensor));

    queue_spi_read8(&fake_spi, ADXL362_REG_DEVID_AD, 0x00U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_ERROR, sensor->ops->init(sensor));
    queue_spi_read(&fake_spi, ADXL362_REG_XDATA, NULL, 6U, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->read(sensor, &data));

    destroy_sensor(sensor);
}

static void test_adxl362_propagates_config_write_failures_without_cache_updates(void)
{
    int fake_spi;
    sensor_device_t *sensor = adxl362_create("adxl362-write-fail", &fake_spi);

    TEST_ASSERT_NOT_NULL(sensor);
    queue_spi_read8(&fake_spi, ADXL362_REG_DEVID_AD, ADXL362_DEVID_AD, SENSOR_EOK);
    queue_spi_write8(&fake_spi, ADXL362_REG_FILTER_CTL,
                     (uint8_t)((ADXL362_ODR_100HZ << 3) | ADXL362_RANGE_2G), SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_INT(0, ((adxl362_priv_t *)sensor->priv_data)->odr);
    TEST_ASSERT_EQUAL_INT(0, ((adxl362_priv_t *)sensor->priv_data)->range);
    TEST_ASSERT_EQUAL_UINT8(0U, ((adxl362_priv_t *)sensor->priv_data)->range_g);
    TEST_ASSERT_EQUAL_INT(0, ((adxl362_priv_t *)sensor->priv_data)->mode);

    setUp();
    queue_spi_read8(&fake_spi, ADXL362_REG_DEVID_AD, ADXL362_DEVID_AD, SENSOR_EOK);
    queue_spi_write8(&fake_spi, ADXL362_REG_FILTER_CTL,
                     (uint8_t)((ADXL362_ODR_100HZ << 3) | ADXL362_RANGE_2G), SENSOR_EOK);
    queue_spi_write8(&fake_spi, ADXL362_REG_POWER_CTL, ADXL362_MODE_MEASUREMENT, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_INT(ADXL362_ODR_100HZ, ((adxl362_priv_t *)sensor->priv_data)->odr);
    TEST_ASSERT_EQUAL_INT(ADXL362_RANGE_2G, ((adxl362_priv_t *)sensor->priv_data)->range);
    TEST_ASSERT_EQUAL_UINT8(2U, ((adxl362_priv_t *)sensor->priv_data)->range_g);
    TEST_ASSERT_EQUAL_INT(0, ((adxl362_priv_t *)sensor->priv_data)->mode);

    setUp();
    queue_spi_write8(&fake_spi, ADXL362_REG_POWER_CTL, ADXL362_MODE_STANDBY, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->deinit(sensor));

    destroy_sensor(sensor);
}

static void test_bma400_create_init_read_and_deinit(void)
{
    int fake_bus;
    sensor_data_t data = {0};
    uint8_t raw[6] = {0x00, 0x01, 0x00, 0x08, 0xFF, 0x07};
    sensor_device_t *sensor = bma400_create("bma400-main", &fake_bus);

    assert_common_accel(sensor, "bma400-main", "Bosch", "BMA400", 800, 25);
    TEST_ASSERT_EQUAL_PTR(&fake_bus, sensor->bus);
    TEST_ASSERT_EQUAL_UINT8(BMA400_ADDR_DEFAULT, ((bma400_priv_t *)sensor->priv_data)->i2c_addr);

    queue_i2c_read8(&fake_bus, BMA400_ADDR_DEFAULT, BMA400_REG_CHIPID, BMA400_CHIP_ID, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, BMA400_ADDR_DEFAULT, BMA400_REG_CMD, 0xB6U, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, BMA400_ADDR_DEFAULT, BMA400_REG_ACC_CONFIG0,
                     BMA400_POWER_MODE_LOW_POWER, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, BMA400_ADDR_DEFAULT, BMA400_REG_ACC_CONFIG1,
                     (uint8_t)(BMA400_RANGE_2G << 6), SENSOR_EOK);
    queue_i2c_write8(&fake_bus, BMA400_ADDR_DEFAULT, BMA400_REG_ACC_CONFIG2,
                     BMA400_ODR_25HZ, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_UINT32(10U, g_delay_total);
    TEST_ASSERT_EQUAL_INT(BMA400_POWER_MODE_LOW_POWER, ((bma400_priv_t *)sensor->priv_data)->power_mode);
    TEST_ASSERT_EQUAL_INT(BMA400_RANGE_2G, ((bma400_priv_t *)sensor->priv_data)->range);
    TEST_ASSERT_EQUAL_UINT8(2U, ((bma400_priv_t *)sensor->priv_data)->range_g);
    TEST_ASSERT_EQUAL_INT(BMA400_ODR_25HZ, ((bma400_priv_t *)sensor->priv_data)->odr);

    queue_i2c_read(&fake_bus, BMA400_ADDR_DEFAULT, BMA400_REG_ACC_X_LSB, raw, sizeof(raw), SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_ACCELEROMETER, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_MILLI_G, data.unit);
    TEST_ASSERT_EQUAL_INT32(0, data.value.val_3axis.x);
    TEST_ASSERT_EQUAL_INT32(0, data.value.val_3axis.y);
    TEST_ASSERT_EQUAL_INT32(0, data.value.val_3axis.z);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);
    TEST_ASSERT_EQUAL_UINT8(93U, data.accuracy);

    queue_i2c_write8(&fake_bus, BMA400_ADDR_DEFAULT, BMA400_REG_ACC_CONFIG0,
                     BMA400_POWER_MODE_SLEEP, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->deinit(sensor));

    destroy_sensor(sensor);
}

static void test_bma400_error_paths(void)
{
    int fake_bus;
    sensor_data_t data = {0};
    sensor_device_t *sensor = bma400_create("bma400-err", &fake_bus);

    TEST_ASSERT_NOT_NULL(sensor);
    queue_i2c_read8(&fake_bus, BMA400_ADDR_DEFAULT, BMA400_REG_CHIPID, 0x00U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_ERROR, sensor->ops->init(sensor));
    queue_i2c_read(&fake_bus, BMA400_ADDR_DEFAULT, BMA400_REG_ACC_X_LSB, NULL, 6U, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->read(sensor, &data));

    destroy_sensor(sensor);
}

static void test_bma400_init_propagates_each_config_write_failure(void)
{
    int fake_bus;
    sensor_device_t *sensor = bma400_create("bma400-write-fail", &fake_bus);

    TEST_ASSERT_NOT_NULL(sensor);
    queue_i2c_read8(&fake_bus, BMA400_ADDR_DEFAULT, BMA400_REG_CHIPID, BMA400_CHIP_ID, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, BMA400_ADDR_DEFAULT, BMA400_REG_CMD, 0xB6U, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_UINT32(0U, g_delay_total);
    TEST_ASSERT_EQUAL_INT(0, ((bma400_priv_t *)sensor->priv_data)->power_mode);

    setUp();
    queue_i2c_read8(&fake_bus, BMA400_ADDR_DEFAULT, BMA400_REG_CHIPID, BMA400_CHIP_ID, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, BMA400_ADDR_DEFAULT, BMA400_REG_CMD, 0xB6U, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, BMA400_ADDR_DEFAULT, BMA400_REG_ACC_CONFIG0,
                     BMA400_POWER_MODE_LOW_POWER, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_UINT32(10U, g_delay_total);
    TEST_ASSERT_EQUAL_INT(0, ((bma400_priv_t *)sensor->priv_data)->power_mode);

    setUp();
    queue_i2c_read8(&fake_bus, BMA400_ADDR_DEFAULT, BMA400_REG_CHIPID, BMA400_CHIP_ID, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, BMA400_ADDR_DEFAULT, BMA400_REG_CMD, 0xB6U, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, BMA400_ADDR_DEFAULT, BMA400_REG_ACC_CONFIG0,
                     BMA400_POWER_MODE_LOW_POWER, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, BMA400_ADDR_DEFAULT, BMA400_REG_ACC_CONFIG1,
                     (uint8_t)(BMA400_RANGE_2G << 6), SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_INT(BMA400_POWER_MODE_LOW_POWER,
                          ((bma400_priv_t *)sensor->priv_data)->power_mode);
    TEST_ASSERT_EQUAL_INT(0, ((bma400_priv_t *)sensor->priv_data)->range);
    TEST_ASSERT_EQUAL_UINT8(0U, ((bma400_priv_t *)sensor->priv_data)->range_g);

    setUp();
    queue_i2c_read8(&fake_bus, BMA400_ADDR_DEFAULT, BMA400_REG_CHIPID, BMA400_CHIP_ID, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, BMA400_ADDR_DEFAULT, BMA400_REG_CMD, 0xB6U, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, BMA400_ADDR_DEFAULT, BMA400_REG_ACC_CONFIG0,
                     BMA400_POWER_MODE_LOW_POWER, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, BMA400_ADDR_DEFAULT, BMA400_REG_ACC_CONFIG1,
                     (uint8_t)(BMA400_RANGE_2G << 6), SENSOR_EOK);
    queue_i2c_write8(&fake_bus, BMA400_ADDR_DEFAULT, BMA400_REG_ACC_CONFIG2,
                     BMA400_ODR_25HZ, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_INT(BMA400_RANGE_2G, ((bma400_priv_t *)sensor->priv_data)->range);
    TEST_ASSERT_EQUAL_UINT8(2U, ((bma400_priv_t *)sensor->priv_data)->range_g);
    TEST_ASSERT_EQUAL_INT(0, ((bma400_priv_t *)sensor->priv_data)->odr);

    destroy_sensor(sensor);
}

static void test_kx023_create_init_read_deinit_and_error_paths(void)
{
    int fake_bus;
    sensor_data_t data = {0};
    uint8_t raw[6] = {0x00, 0x40, 0x00, 0xC0, 0x00, 0x20};
    sensor_device_t *sensor = kx023_create("kx023-main", &fake_bus);

    assert_common_accel(sensor, "kx023-main", "Kionix", "KX023", 100, 12);
    TEST_ASSERT_EQUAL_PTR(&fake_bus, sensor->bus);
    TEST_ASSERT_EQUAL_UINT8(KX023_ADDR_DEFAULT, ((kx023_priv_t *)sensor->priv_data)->i2c_addr);

    queue_i2c_read8(&fake_bus, KX023_ADDR_DEFAULT, KX023_REG_WHO_AM_I, KX023_WHO_AM_I_VALUE, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, KX023_ADDR_DEFAULT, KX023_REG_SOFT_REST, 0x80U, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, KX023_ADDR_DEFAULT, KX023_REG_CNTL1, 0x00U, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, KX023_ADDR_DEFAULT, KX023_REG_ODCNTL, KX023_ODR_12_5HZ, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, KX023_ADDR_DEFAULT, KX023_REG_CNTL1, KX023_MODE_LOW_POWER, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_UINT32(10U, g_delay_total);
    TEST_ASSERT_EQUAL_INT(KX023_ODR_12_5HZ, ((kx023_priv_t *)sensor->priv_data)->odr);
    TEST_ASSERT_EQUAL_INT(KX023_MODE_LOW_POWER, ((kx023_priv_t *)sensor->priv_data)->mode);

    queue_i2c_read(&fake_bus, KX023_ADDR_DEFAULT, KX023_REG_XOUT_L, raw, sizeof(raw), SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_ACCELEROMETER, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_MILLI_G, data.unit);
    TEST_ASSERT_EQUAL_INT32(1000, data.value.val_3axis.x);
    TEST_ASSERT_EQUAL_INT32(-1000, data.value.val_3axis.y);
    TEST_ASSERT_EQUAL_INT32(500, data.value.val_3axis.z);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);
    TEST_ASSERT_EQUAL_UINT8(92U, data.accuracy);

    queue_i2c_write8(&fake_bus, KX023_ADDR_DEFAULT, KX023_REG_CNTL1, KX023_MODE_STANDBY, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->deinit(sensor));

    queue_i2c_read8(&fake_bus, KX023_ADDR_DEFAULT, KX023_REG_WHO_AM_I, 0x00U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_ERROR, sensor->ops->init(sensor));
    queue_i2c_read(&fake_bus, KX023_ADDR_DEFAULT, KX023_REG_XOUT_L, NULL, 6U, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->read(sensor, &data));

    destroy_sensor(sensor);
}

static void test_kx023_propagates_config_write_failures(void)
{
    int fake_bus;
    sensor_device_t *sensor = kx023_create("kx023-write-fail", &fake_bus);

    TEST_ASSERT_NOT_NULL(sensor);
    queue_i2c_read8(&fake_bus, KX023_ADDR_DEFAULT, KX023_REG_WHO_AM_I, KX023_WHO_AM_I_VALUE,
                    SENSOR_EOK);
    queue_i2c_write8(&fake_bus, KX023_ADDR_DEFAULT, KX023_REG_SOFT_REST, 0x80U, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_UINT32(0U, g_delay_total);

    setUp();
    queue_i2c_read8(&fake_bus, KX023_ADDR_DEFAULT, KX023_REG_WHO_AM_I, KX023_WHO_AM_I_VALUE,
                    SENSOR_EOK);
    queue_i2c_write8(&fake_bus, KX023_ADDR_DEFAULT, KX023_REG_SOFT_REST, 0x80U, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, KX023_ADDR_DEFAULT, KX023_REG_CNTL1, 0x00U, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_UINT32(10U, g_delay_total);
    TEST_ASSERT_EQUAL_INT(0, ((kx023_priv_t *)sensor->priv_data)->odr);
    TEST_ASSERT_EQUAL_INT(0, ((kx023_priv_t *)sensor->priv_data)->mode);

    setUp();
    queue_i2c_read8(&fake_bus, KX023_ADDR_DEFAULT, KX023_REG_WHO_AM_I, KX023_WHO_AM_I_VALUE,
                    SENSOR_EOK);
    queue_i2c_write8(&fake_bus, KX023_ADDR_DEFAULT, KX023_REG_SOFT_REST, 0x80U, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, KX023_ADDR_DEFAULT, KX023_REG_CNTL1, 0x00U, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, KX023_ADDR_DEFAULT, KX023_REG_ODCNTL, KX023_ODR_12_5HZ,
                     SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_INT(0, ((kx023_priv_t *)sensor->priv_data)->odr);
    TEST_ASSERT_EQUAL_INT(0, ((kx023_priv_t *)sensor->priv_data)->mode);

    setUp();
    queue_i2c_read8(&fake_bus, KX023_ADDR_DEFAULT, KX023_REG_WHO_AM_I, KX023_WHO_AM_I_VALUE,
                    SENSOR_EOK);
    queue_i2c_write8(&fake_bus, KX023_ADDR_DEFAULT, KX023_REG_SOFT_REST, 0x80U, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, KX023_ADDR_DEFAULT, KX023_REG_CNTL1, 0x00U, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, KX023_ADDR_DEFAULT, KX023_REG_ODCNTL, KX023_ODR_12_5HZ,
                     SENSOR_EOK);
    queue_i2c_write8(&fake_bus, KX023_ADDR_DEFAULT, KX023_REG_CNTL1, KX023_MODE_LOW_POWER,
                     SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_INT(KX023_ODR_12_5HZ, ((kx023_priv_t *)sensor->priv_data)->odr);
    TEST_ASSERT_EQUAL_INT(0, ((kx023_priv_t *)sensor->priv_data)->mode);

    setUp();
    queue_i2c_write8(&fake_bus, KX023_ADDR_DEFAULT, KX023_REG_CNTL1, KX023_MODE_STANDBY,
                     SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->deinit(sensor));

    destroy_sensor(sensor);
}

static void test_lis2dw12_create_init_read_deinit_and_setters(void)
{
    int fake_bus;
    sensor_data_t data = {0};
    sensor_device_t *sensor = lis2dw12_create("lis2dw12-main", &fake_bus, 0U);

    assert_common_accel(sensor, "lis2dw12-main", "STMicro", "LIS2DW12", 800, 100);
    TEST_ASSERT_EQUAL_PTR(&fake_bus, sensor->bus);
    TEST_ASSERT_EQUAL_UINT8(LIS2DW12_ADDR_DEFAULT, ((lis2dw12_priv_t *)sensor->priv_data)->i2c_addr);
    TEST_ASSERT_EQUAL_UINT8(LIS2DW12_SPI_CS_NONE, ((lis2dw12_priv_t *)sensor->priv_data)->spi_cs);

    queue_i2c_read8(&fake_bus, LIS2DW12_ADDR_DEFAULT, LIS2DW12_REG_WHOAMI,
                    LIS2DW12_WHOAMI_VALUE, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, LIS2DW12_ADDR_DEFAULT, LIS2DW12_REG_CTRL2, 0x04U,
                     SENSOR_EOK);
    queue_i2c_write8(&fake_bus, LIS2DW12_ADDR_DEFAULT, LIS2DW12_REG_CTRL1,
                     (uint8_t)((LIS2DW12_MODE_LOW_POWER << 5) | (LIS2DW12_RATE_100HZ << 2) |
                               LIS2DW12_RANGE_2G),
                     SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_UINT32(10U, g_delay_total);
    TEST_ASSERT_EQUAL_INT(2, ((lis2dw12_priv_t *)sensor->priv_data)->range);
    TEST_ASSERT_EQUAL_INT(100, ((lis2dw12_priv_t *)sensor->priv_data)->rate);
    TEST_ASSERT_EQUAL_INT(LIS2DW12_MODE_LOW_POWER, ((lis2dw12_priv_t *)sensor->priv_data)->mode);

    uint8_t raw[6] = {0x10, 0x00, 0xF0, 0xFF, 0x00, 0x08};
    for (uint8_t i = 0; i < sizeof(raw); i++) {
        queue_i2c_read8(&fake_bus, LIS2DW12_ADDR_DEFAULT, (uint8_t)(LIS2DW12_REG_OUT_X_L + i),
                        raw[i], SENSOR_EOK);
    }
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_ACCELEROMETER, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_MILLI_G, data.unit);
    TEST_ASSERT_EQUAL_INT32(16, data.value.val_3axis.x);
    TEST_ASSERT_EQUAL_INT32(-16, data.value.val_3axis.y);
    TEST_ASSERT_EQUAL_INT32(2048, data.value.val_3axis.z);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);
    TEST_ASSERT_EQUAL_UINT8(95U, data.accuracy);

    queue_i2c_read8(&fake_bus, LIS2DW12_ADDR_DEFAULT, LIS2DW12_REG_CTRL1, 0xA0U, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, LIS2DW12_ADDR_DEFAULT, LIS2DW12_REG_CTRL1, 0xA2U,
                     SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, lis2dw12_set_range(sensor, LIS2DW12_RANGE_8G));
    TEST_ASSERT_EQUAL_INT(8, ((lis2dw12_priv_t *)sensor->priv_data)->range);

    queue_i2c_read8(&fake_bus, LIS2DW12_ADDR_DEFAULT, LIS2DW12_REG_CTRL1, 0xA2U, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, LIS2DW12_ADDR_DEFAULT, LIS2DW12_REG_CTRL1,
                     (uint8_t)((0xA2U & 0xC3U) | (LIS2DW12_RATE_400HZ << 2)), SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, lis2dw12_set_rate(sensor, LIS2DW12_RATE_400HZ));
    TEST_ASSERT_EQUAL_INT(400, ((lis2dw12_priv_t *)sensor->priv_data)->rate);

    queue_i2c_read8(&fake_bus, LIS2DW12_ADDR_DEFAULT, LIS2DW12_REG_CTRL1, 0x1FU, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, LIS2DW12_ADDR_DEFAULT, LIS2DW12_REG_CTRL1,
                     (uint8_t)(0x1FU | (LIS2DW12_MODE_HIGH_PERF << 5)), SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, lis2dw12_set_mode(sensor, LIS2DW12_MODE_HIGH_PERF));
    TEST_ASSERT_EQUAL_INT(LIS2DW12_MODE_HIGH_PERF, ((lis2dw12_priv_t *)sensor->priv_data)->mode);

    queue_i2c_read8(&fake_bus, LIS2DW12_ADDR_DEFAULT, LIS2DW12_REG_CTRL5, 0x00U, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, LIS2DW12_ADDR_DEFAULT, LIS2DW12_REG_CTRL5, 0x40U,
                     SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, lis2dw12_enable_high_pass(sensor, 1U));

    queue_i2c_write8(&fake_bus, LIS2DW12_ADDR_DEFAULT, LIS2DW12_REG_CTRL1,
                     (uint8_t)(LIS2DW12_RATE_POWER_DOWN << 2), SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->deinit(sensor));

    destroy_sensor(sensor);
}

static void test_lis2dw12_propagates_i2c_and_spi_failures_without_cache_updates(void)
{
    int fake_bus;
    sensor_device_t *sensor = lis2dw12_create("lis2dw12-fail", &fake_bus, LIS2DW12_ADDR_ALT);

    TEST_ASSERT_NOT_NULL(sensor);
    queue_i2c_read8(&fake_bus, LIS2DW12_ADDR_ALT, LIS2DW12_REG_WHOAMI, 0x00U, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->init(sensor));

    setUp();
    queue_i2c_read8(&fake_bus, LIS2DW12_ADDR_ALT, LIS2DW12_REG_WHOAMI, 0x00U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_ERROR, sensor->ops->init(sensor));

    setUp();
    queue_i2c_read8(&fake_bus, LIS2DW12_ADDR_ALT, LIS2DW12_REG_WHOAMI,
                    LIS2DW12_WHOAMI_VALUE, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, LIS2DW12_ADDR_ALT, LIS2DW12_REG_CTRL2, 0x04U, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_UINT32(0U, g_delay_total);
    TEST_ASSERT_EQUAL_INT(0, ((lis2dw12_priv_t *)sensor->priv_data)->range);

    setUp();
    queue_i2c_read8(&fake_bus, LIS2DW12_ADDR_ALT, LIS2DW12_REG_WHOAMI,
                    LIS2DW12_WHOAMI_VALUE, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, LIS2DW12_ADDR_ALT, LIS2DW12_REG_CTRL2, 0x04U, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, LIS2DW12_ADDR_ALT, LIS2DW12_REG_CTRL1,
                     (uint8_t)((LIS2DW12_MODE_LOW_POWER << 5) | (LIS2DW12_RATE_100HZ << 2) |
                               LIS2DW12_RANGE_2G),
                     SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_UINT32(10U, g_delay_total);
    TEST_ASSERT_EQUAL_INT(0, ((lis2dw12_priv_t *)sensor->priv_data)->range);

    setUp();
    ((lis2dw12_priv_t *)sensor->priv_data)->range = 4U;
    queue_i2c_read8(&fake_bus, LIS2DW12_ADDR_ALT, LIS2DW12_REG_CTRL1, 0x00U, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, LIS2DW12_ADDR_ALT, LIS2DW12_REG_CTRL1, LIS2DW12_RANGE_16G,
                     SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, lis2dw12_set_range(sensor, LIS2DW12_RANGE_16G));
    TEST_ASSERT_EQUAL_INT(4, ((lis2dw12_priv_t *)sensor->priv_data)->range);

    setUp();
    ((lis2dw12_priv_t *)sensor->priv_data)->rate = 25U;
    queue_i2c_read8(&fake_bus, LIS2DW12_ADDR_ALT, LIS2DW12_REG_CTRL1, 0x00U, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, lis2dw12_set_rate(sensor, LIS2DW12_RATE_800HZ));
    TEST_ASSERT_EQUAL_INT(25, ((lis2dw12_priv_t *)sensor->priv_data)->rate);

    setUp();
    ((lis2dw12_priv_t *)sensor->priv_data)->mode = LIS2DW12_MODE_LOW_POWER;
    queue_i2c_read8(&fake_bus, LIS2DW12_ADDR_ALT, LIS2DW12_REG_CTRL1, 0x00U, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, LIS2DW12_ADDR_ALT, LIS2DW12_REG_CTRL1,
                     (uint8_t)(LIS2DW12_MODE_NORMAL << 5), SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, lis2dw12_set_mode(sensor, LIS2DW12_MODE_NORMAL));
    TEST_ASSERT_EQUAL_INT(LIS2DW12_MODE_LOW_POWER, ((lis2dw12_priv_t *)sensor->priv_data)->mode);

    setUp();
    queue_i2c_read8(&fake_bus, LIS2DW12_ADDR_ALT, LIS2DW12_REG_CTRL5, 0x00U, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, lis2dw12_enable_high_pass(sensor, 1U));

    setUp();
    queue_i2c_write8(&fake_bus, LIS2DW12_ADDR_ALT, LIS2DW12_REG_CTRL1,
                     (uint8_t)(LIS2DW12_RATE_POWER_DOWN << 2), SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->deinit(sensor));

    destroy_sensor(sensor);

    setUp();
    sensor = lis2dw12_create_spi("lis2dw12-spi", &fake_bus, 3U);
    TEST_ASSERT_NOT_NULL(sensor);
    queue_spi_register_read8(&fake_bus, 3U, LIS2DW12_REG_WHOAMI, LIS2DW12_WHOAMI_VALUE,
                             SENSOR_EIO, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_UINT(0U, g_spi_recv_index);

    setUp();
    queue_spi_register_read8(&fake_bus, 3U, LIS2DW12_REG_WHOAMI, LIS2DW12_WHOAMI_VALUE,
                             SENSOR_EOK, SENSOR_EOK);
    queue_spi_register_write8(&fake_bus, 3U, LIS2DW12_REG_CTRL2, 0x04U, SENSOR_EOK);
    queue_spi_register_write8(&fake_bus, 3U, LIS2DW12_REG_CTRL1,
                              (uint8_t)((LIS2DW12_MODE_LOW_POWER << 5) |
                                        (LIS2DW12_RATE_100HZ << 2) | LIS2DW12_RANGE_2G),
                              SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_INT(2, ((lis2dw12_priv_t *)sensor->priv_data)->range);

    destroy_sensor(sensor);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_adxl362_create_init_read_deinit_and_error_paths);
    RUN_TEST(test_adxl362_propagates_config_write_failures_without_cache_updates);
    RUN_TEST(test_bma400_create_init_read_and_deinit);
    RUN_TEST(test_bma400_error_paths);
    RUN_TEST(test_bma400_init_propagates_each_config_write_failure);
    RUN_TEST(test_kx023_create_init_read_deinit_and_error_paths);
    RUN_TEST(test_kx023_propagates_config_write_failures);
    RUN_TEST(test_lis2dw12_create_init_read_deinit_and_setters);
    RUN_TEST(test_lis2dw12_propagates_i2c_and_spi_failures_without_cache_updates);
    return UNITY_END();
}
