#include "unity.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sensor_lsm6dsl.h"
#include "sensor_lsm6dso.h"
#include "sensor_lsm6dsr.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
    void *bus;
    uint8_t addr;
    uint8_t reg;
    uint8_t data;
    int ret;
} i2c_op_t;

typedef struct {
    void *bus;
    uint8_t cs;
    uint8_t data[2];
    uint16_t len;
    int ret;
} spi_send_op_t;

typedef struct {
    void *bus;
    uint8_t cs;
    uint8_t data;
    uint16_t len;
    int ret;
} spi_recv_op_t;

static i2c_op_t g_i2c_reads[96];
static i2c_op_t g_i2c_writes[96];
static size_t g_i2c_read_count;
static size_t g_i2c_read_index;
static size_t g_i2c_write_count;
static size_t g_i2c_write_index;
static spi_send_op_t g_spi_sends[96];
static spi_recv_op_t g_spi_recvs[96];
static size_t g_spi_send_count;
static size_t g_spi_send_index;
static size_t g_spi_recv_count;
static size_t g_spi_recv_index;
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
    TEST_ASSERT_EQUAL_UINT16(1U, len);
    TEST_ASSERT_LESS_THAN_UINT(g_i2c_read_count, g_i2c_read_index);
    i2c_op_t *op = &g_i2c_reads[g_i2c_read_index++];
    TEST_ASSERT_EQUAL_PTR(op->bus, bus);
    TEST_ASSERT_EQUAL_UINT8(op->addr, addr);
    TEST_ASSERT_EQUAL_UINT8(op->reg, reg);
    if (op->ret == SENSOR_EOK) {
        *data = op->data;
    }
    return op->ret;
}

int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    TEST_ASSERT_EQUAL_UINT16(1U, len);
    TEST_ASSERT_LESS_THAN_UINT(g_i2c_write_count, g_i2c_write_index);
    i2c_op_t *op = &g_i2c_writes[g_i2c_write_index++];
    TEST_ASSERT_EQUAL_PTR(op->bus, bus);
    TEST_ASSERT_EQUAL_UINT8(op->addr, addr);
    TEST_ASSERT_EQUAL_UINT8(op->reg, reg);
    TEST_ASSERT_EQUAL_UINT8(op->data, *data);
    return op->ret;
}

int hal_spi_send(void *bus, uint8_t cs, uint8_t *data, uint16_t len)
{
    TEST_ASSERT_LESS_THAN_UINT(g_spi_send_count, g_spi_send_index);
    spi_send_op_t *op = &g_spi_sends[g_spi_send_index++];
    TEST_ASSERT_EQUAL_PTR(op->bus, bus);
    TEST_ASSERT_EQUAL_UINT8(op->cs, cs);
    TEST_ASSERT_EQUAL_UINT16(op->len, len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(op->data, data, len);
    return op->ret;
}

int hal_spi_recv(void *bus, uint8_t cs, uint8_t *data, uint16_t len)
{
    TEST_ASSERT_LESS_THAN_UINT(g_spi_recv_count, g_spi_recv_index);
    spi_recv_op_t *op = &g_spi_recvs[g_spi_recv_index++];
    TEST_ASSERT_EQUAL_PTR(op->bus, bus);
    TEST_ASSERT_EQUAL_UINT8(op->cs, cs);
    TEST_ASSERT_EQUAL_UINT16(op->len, len);
    if (op->ret == SENSOR_EOK) {
        *data = op->data;
    }
    return op->ret;
}

static void queue_i2c_read8(void *bus, uint8_t addr, uint8_t reg, uint8_t data, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_i2c_reads), g_i2c_read_count);
    g_i2c_reads[g_i2c_read_count++] = (i2c_op_t){bus, addr, reg, data, ret};
}

static void queue_i2c_write8(void *bus, uint8_t addr, uint8_t reg, uint8_t data, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_i2c_writes), g_i2c_write_count);
    g_i2c_writes[g_i2c_write_count++] = (i2c_op_t){bus, addr, reg, data, ret};
}

static void queue_spi_send1(void *bus, uint8_t cs, uint8_t data, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_spi_sends), g_spi_send_count);
    spi_send_op_t *op = &g_spi_sends[g_spi_send_count++];
    op->bus = bus;
    op->cs = cs;
    op->data[0] = data;
    op->len = 1U;
    op->ret = ret;
}

static void queue_spi_send2(void *bus, uint8_t cs, uint8_t reg, uint8_t data, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_spi_sends), g_spi_send_count);
    spi_send_op_t *op = &g_spi_sends[g_spi_send_count++];
    op->bus = bus;
    op->cs = cs;
    op->data[0] = reg;
    op->data[1] = data;
    op->len = 2U;
    op->ret = ret;
}

static void queue_spi_read8(void *bus, uint8_t cs, uint8_t reg, uint8_t data, int ret)
{
    queue_spi_send1(bus, cs, (uint8_t)(reg | 0x80U), SENSOR_EOK);
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_spi_recvs), g_spi_recv_count);
    g_spi_recvs[g_spi_recv_count++] = (spi_recv_op_t){bus, cs, data, 1U, ret};
}

void setUp(void)
{
    memset(g_i2c_reads, 0, sizeof(g_i2c_reads));
    memset(g_i2c_writes, 0, sizeof(g_i2c_writes));
    memset(g_spi_sends, 0, sizeof(g_spi_sends));
    memset(g_spi_recvs, 0, sizeof(g_spi_recvs));
    g_i2c_read_count = 0;
    g_i2c_read_index = 0;
    g_i2c_write_count = 0;
    g_i2c_write_index = 0;
    g_spi_send_count = 0;
    g_spi_send_index = 0;
    g_spi_recv_count = 0;
    g_spi_recv_index = 0;
    g_tick = 42000U;
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

static void assert_accel(sensor_device_t *sensor, const char *name, const char *model, int32_t max_odr)
{
    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_STRING(name, sensor->info.name);
    TEST_ASSERT_EQUAL_STRING("STMicro", sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING(model, sensor->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_ACCELEROMETER, sensor->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_MILLI_G, sensor->info.unit);
    TEST_ASSERT_EQUAL_INT32(2000, sensor->info.range_max);
    TEST_ASSERT_EQUAL_INT32(-2000, sensor->info.range_min);
    TEST_ASSERT_EQUAL_INT32(16, sensor->info.resolution);
    TEST_ASSERT_EQUAL_INT32(max_odr, sensor->info.max_odr);
    TEST_ASSERT_EQUAL_INT32(104, sensor->odr);
    TEST_ASSERT_NOT_NULL(sensor->ops->init);
    TEST_ASSERT_NOT_NULL(sensor->ops->deinit);
    TEST_ASSERT_NOT_NULL(sensor->ops->read);
}

static void assert_gyro(sensor_device_t *sensor, const char *name, const char *model, int32_t max_odr)
{
    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_STRING(name, sensor->info.name);
    TEST_ASSERT_EQUAL_STRING("STMicro", sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING(model, sensor->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_GYROSCOPE, sensor->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_DEGREE_PER_SECOND, sensor->info.unit);
    TEST_ASSERT_EQUAL_INT32(250, sensor->info.range_max);
    TEST_ASSERT_EQUAL_INT32(-250, sensor->info.range_min);
    TEST_ASSERT_EQUAL_INT32(16, sensor->info.resolution);
    TEST_ASSERT_EQUAL_INT32(max_odr, sensor->info.max_odr);
    TEST_ASSERT_EQUAL_INT32(104, sensor->odr);
}

static void queue_lsm6dsl_spi_init(void *bus, uint8_t cs, uint8_t whoami)
{
    queue_spi_read8(bus, cs, LSM6DSL_REG_WHOAMI, whoami, SENSOR_EOK);
    queue_spi_send2(bus, cs, LSM6DSL_REG_CTRL3_C & 0x7FU, 0x01U, SENSOR_EOK);
    queue_spi_send2(bus, cs, LSM6DSL_REG_CTRL4_C & 0x7FU, 0x00U, SENSOR_EOK);
    queue_spi_send2(bus, cs, LSM6DSL_REG_CTRL1_XL & 0x7FU,
                    (uint8_t)((LSM6DSL_ACCEL_RATE_104Hz << 4) | LSM6DSL_ACCEL_RANGE_2G), SENSOR_EOK);
    queue_spi_send2(bus, cs, LSM6DSL_REG_CTRL2_G & 0x7FU,
                    (uint8_t)((LSM6DSL_GYRO_RATE_104Hz << 4) | LSM6DSL_GYRO_RANGE_250DPS), SENSOR_EOK);
}

static void queue_lsm6dso_i2c_init(void *bus, uint8_t addr, uint8_t whoami)
{
    queue_i2c_read8(bus, addr, LSM6DSO_REG_WHOAMI, whoami, SENSOR_EOK);
    queue_i2c_write8(bus, addr, LSM6DSO_REG_CTRL3_C, 0x01U, SENSOR_EOK);
    queue_i2c_write8(bus, addr, LSM6DSO_REG_CTRL4_C, 0x00U, SENSOR_EOK);
    queue_i2c_write8(bus, addr, LSM6DSO_REG_CTRL1_XL,
                     (uint8_t)((LSM6DSO_ACCEL_RATE_104Hz << 4) | LSM6DSO_ACCEL_RANGE_2G), SENSOR_EOK);
    queue_i2c_write8(bus, addr, LSM6DSO_REG_CTRL2_G,
                     (uint8_t)((LSM6DSO_GYRO_RATE_104Hz << 4) | LSM6DSO_GYRO_RANGE_250DPS), SENSOR_EOK);
}

static void queue_lsm6dsr_i2c_init(void *bus, uint8_t addr, uint8_t whoami)
{
    queue_i2c_read8(bus, addr, LSM6DSR_REG_WHOAMI, whoami, SENSOR_EOK);
    queue_i2c_write8(bus, addr, LSM6DSR_REG_CTRL3_C, 0x01U, SENSOR_EOK);
    queue_i2c_write8(bus, addr, LSM6DSR_REG_CTRL4_C, 0x00U, SENSOR_EOK);
    queue_i2c_write8(bus, addr, LSM6DSR_REG_CTRL1_XL,
                     (uint8_t)((LSM6DSR_ACCEL_RATE_104Hz << 4) | LSM6DSR_ACCEL_RANGE_2G), SENSOR_EOK);
    queue_i2c_write8(bus, addr, LSM6DSR_REG_CTRL2_G,
                     (uint8_t)((LSM6DSR_GYRO_RATE_104Hz << 4) | LSM6DSR_GYRO_RANGE_250DPS), SENSOR_EOK);
}

static void test_lsm6dsl_spi_accel_gyro_init_read_deinit_and_helpers(void)
{
    int fake_bus;
    sensor_data_t data = {0};
    const uint8_t cs = 3U;
    uint8_t raw[6] = {0x00, 0x10, 0x00, 0xF0, 0x00, 0x40};
    sensor_device_t *accel = lsm6dsl_create_spi_accel("lsm6dsl-acc", &fake_bus, cs);
    sensor_device_t *gyro = lsm6dsl_create_spi_gyro("lsm6dsl-gyr", &fake_bus, cs);

    assert_accel(accel, "lsm6dsl-acc", "LSM6DSL", 6660);
    assert_gyro(gyro, "lsm6dsl-gyr", "LSM6DSL", 6660);
    TEST_ASSERT_EQUAL_UINT8(cs, ((lsm6dsl_priv_t *)accel->priv_data)->spi_cs);
    TEST_ASSERT_EQUAL_UINT8(cs, ((lsm6dsl_priv_t *)gyro->priv_data)->spi_cs);

    queue_lsm6dsl_spi_init(&fake_bus, cs, LSM6DSL_WHOAMI_VALUE);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->init(accel));
    TEST_ASSERT_EQUAL_UINT8(2U, ((lsm6dsl_priv_t *)accel->priv_data)->accel_range);
    TEST_ASSERT_EQUAL_UINT8(250U, ((lsm6dsl_priv_t *)accel->priv_data)->gyro_range);

    for (uint8_t i = 0; i < sizeof(raw); ++i) {
        queue_spi_read8(&fake_bus, cs, (uint8_t)(LSM6DSL_REG_OUTX_L_XL + i), raw[i], SENSOR_EOK);
    }
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->read(accel, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_ACCELEROMETER, data.type);
    TEST_ASSERT_EQUAL_INT32(249, data.value.val_3axis.x);
    TEST_ASSERT_EQUAL_INT32(-249, data.value.val_3axis.y);
    TEST_ASSERT_EQUAL_INT32(999, data.value.val_3axis.z);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);

    ((lsm6dsl_priv_t *)gyro->priv_data)->gyro_range = 250U;
    for (uint8_t i = 0; i < sizeof(raw); ++i) {
        queue_spi_read8(&fake_bus, cs, (uint8_t)(LSM6DSL_REG_OUTX_L_G + i), raw[i], SENSOR_EOK);
    }
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, gyro->ops->read(gyro, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_GYROSCOPE, data.type);
    TEST_ASSERT_EQUAL_INT32(36, data.value.val_3axis.x);
    TEST_ASSERT_EQUAL_INT32(-36, data.value.val_3axis.y);
    TEST_ASSERT_EQUAL_INT32(147, data.value.val_3axis.z);

    queue_spi_read8(&fake_bus, cs, LSM6DSL_REG_CTRL1_XL, 0x40U, SENSOR_EOK);
    queue_spi_send2(&fake_bus, cs, LSM6DSL_REG_CTRL1_XL & 0x7FU, 0x20U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(0, lsm6dsl_set_accel_range(accel, LSM6DSL_ACCEL_RANGE_4G));
    queue_spi_read8(&fake_bus, cs, LSM6DSL_REG_CTRL2_G, 0x40U, SENSOR_EOK);
    queue_spi_send2(&fake_bus, cs, LSM6DSL_REG_CTRL2_G & 0x7FU, 0x20U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(0, lsm6dsl_set_gyro_range(accel, LSM6DSL_GYRO_RANGE_500DPS));

    queue_spi_send2(&fake_bus, cs, LSM6DSL_REG_CTRL1_XL & 0x7FU, 0x00U, SENSOR_EOK);
    queue_spi_send2(&fake_bus, cs, LSM6DSL_REG_CTRL2_G & 0x7FU, 0x00U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->deinit(accel));
    queue_spi_read8(&fake_bus, cs, LSM6DSL_REG_WHOAMI, 0x00U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_ERROR, accel->ops->init(accel));

    destroy_sensor(accel);
    destroy_sensor(gyro);
}

static void test_lsm6dso_i2c_accel_gyro_init_read_helpers_and_errors(void)
{
    int fake_bus;
    sensor_data_t data = {0};
    uint8_t raw[6] = {0x00, 0x20, 0x00, 0xE0, 0x00, 0x10};
    sensor_device_t *accel = lsm6dso_create_accel("lsm6dso-acc", &fake_bus, 0U);
    sensor_device_t *gyro = lsm6dso_create_gyro("lsm6dso-gyr", &fake_bus, LSM6DSO_ADDR_ALT);

    assert_accel(accel, "lsm6dso-acc", "LSM6DSO", 6660);
    assert_gyro(gyro, "lsm6dso-gyr", "LSM6DSO", 6660);
    TEST_ASSERT_EQUAL_UINT8(LSM6DSO_ADDR_DEFAULT, ((lsm6dso_priv_t *)accel->priv_data)->i2c_addr);
    TEST_ASSERT_EQUAL_UINT8(LSM6DSO_ADDR_ALT, ((lsm6dso_priv_t *)gyro->priv_data)->i2c_addr);

    queue_lsm6dso_i2c_init(&fake_bus, LSM6DSO_ADDR_DEFAULT, LSM6DSO_WHOAMI_VALUE);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->init(accel));
    TEST_ASSERT_EQUAL_UINT8(2U, ((lsm6dso_priv_t *)accel->priv_data)->accel_range);
    TEST_ASSERT_EQUAL_UINT8(104U, ((lsm6dso_priv_t *)accel->priv_data)->gyro_rate);

    for (uint8_t i = 0; i < sizeof(raw); ++i) {
        queue_i2c_read8(&fake_bus, LSM6DSO_ADDR_DEFAULT, (uint8_t)(LSM6DSO_REG_OUTX_L_XL + i), raw[i], SENSOR_EOK);
    }
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->read(accel, &data));
    TEST_ASSERT_EQUAL_INT32(499, data.value.val_3axis.x);
    TEST_ASSERT_EQUAL_INT32(-499, data.value.val_3axis.y);
    TEST_ASSERT_EQUAL_INT32(249, data.value.val_3axis.z);

    for (uint8_t i = 0; i < sizeof(raw); ++i) {
        queue_i2c_read8(&fake_bus, LSM6DSO_ADDR_ALT, (uint8_t)(LSM6DSO_REG_OUTX_L_G + i), raw[i], SENSOR_EOK);
    }
    ((lsm6dso_priv_t *)gyro->priv_data)->gyro_range = 250U;
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, gyro->ops->read(gyro, &data));
    TEST_ASSERT_EQUAL_INT32(73, data.value.val_3axis.x);
    TEST_ASSERT_EQUAL_INT32(-73, data.value.val_3axis.y);
    TEST_ASSERT_EQUAL_INT32(36, data.value.val_3axis.z);

    queue_i2c_read8(&fake_bus, LSM6DSO_ADDR_DEFAULT, LSM6DSO_REG_CTRL1_XL, 0x0FU, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, LSM6DSO_ADDR_DEFAULT, LSM6DSO_REG_CTRL1_XL, 0x2FU, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(0, lsm6dso_set_accel_range(accel, LSM6DSO_ACCEL_RANGE_4G));
    queue_i2c_read8(&fake_bus, LSM6DSO_ADDR_DEFAULT, LSM6DSO_REG_CTRL1_XL, 0xF0U, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, LSM6DSO_ADDR_DEFAULT, LSM6DSO_REG_CTRL1_XL, 0xF5U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(0, lsm6dso_set_accel_rate(accel, LSM6DSO_ACCEL_RATE_208Hz));

    queue_i2c_write8(&fake_bus, LSM6DSO_ADDR_DEFAULT, LSM6DSO_REG_CTRL1_XL, 0x00U, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, LSM6DSO_ADDR_DEFAULT, LSM6DSO_REG_CTRL2_G, 0x00U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->deinit(accel));
    queue_i2c_read8(&fake_bus, LSM6DSO_ADDR_DEFAULT, LSM6DSO_REG_WHOAMI, 0x00U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_ERROR, accel->ops->init(accel));
    queue_i2c_read8(&fake_bus, LSM6DSO_ADDR_DEFAULT, LSM6DSO_REG_OUTX_L_XL, 0U, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, accel->ops->read(accel, &data));

    destroy_sensor(accel);
    destroy_sensor(gyro);
}

static void test_lsm6dsr_i2c_accel_gyro_init_read_helpers_and_errors(void)
{
    int fake_bus;
    sensor_data_t data = {0};
    uint8_t raw[6] = {0x00, 0x08, 0x00, 0xF8, 0x00, 0x04};
    sensor_device_t *accel = lsm6dsr_create_accel("lsm6dsr-acc", &fake_bus, 0U);
    sensor_device_t *gyro = lsm6dsr_create_gyro("lsm6dsr-gyr", &fake_bus, 0U);

    assert_accel(accel, "lsm6dsr-acc", "LSM6DSR", 8000);
    assert_gyro(gyro, "lsm6dsr-gyr", "LSM6DSR", 8000);
    TEST_ASSERT_EQUAL_UINT8(LSM6DSR_ADDR_DEFAULT, ((lsm6dsr_priv_t *)accel->priv_data)->i2c_addr);
    TEST_ASSERT_EQUAL_UINT8(LSM6DSR_SPI_CS_NONE, ((lsm6dsr_priv_t *)accel->priv_data)->spi_cs);

    queue_lsm6dsr_i2c_init(&fake_bus, LSM6DSR_ADDR_DEFAULT, LSM6DSR_WHOAMI_VALUE);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->init(accel));
    TEST_ASSERT_EQUAL_UINT8(2U, ((lsm6dsr_priv_t *)accel->priv_data)->accel_range);
    TEST_ASSERT_EQUAL_UINT8(104U, ((lsm6dsr_priv_t *)accel->priv_data)->gyro_rate);

    for (uint8_t i = 0; i < sizeof(raw); ++i) {
        queue_i2c_read8(&fake_bus, LSM6DSR_ADDR_DEFAULT, (uint8_t)(LSM6DSR_REG_OUTX_L_XL + i), raw[i], SENSOR_EOK);
    }
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->read(accel, &data));
    TEST_ASSERT_EQUAL_INT32(124, data.value.val_3axis.x);
    TEST_ASSERT_EQUAL_INT32(-124, data.value.val_3axis.y);
    TEST_ASSERT_EQUAL_INT32(62, data.value.val_3axis.z);

    for (uint8_t i = 0; i < sizeof(raw); ++i) {
        queue_i2c_read8(&fake_bus, LSM6DSR_ADDR_DEFAULT, (uint8_t)(LSM6DSR_REG_OUTX_L_G + i), raw[i], SENSOR_EOK);
    }
    ((lsm6dsr_priv_t *)gyro->priv_data)->gyro_range = 250U;
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, gyro->ops->read(gyro, &data));
    TEST_ASSERT_EQUAL_INT32(18, data.value.val_3axis.x);
    TEST_ASSERT_EQUAL_INT32(-18, data.value.val_3axis.y);
    TEST_ASSERT_EQUAL_INT32(9, data.value.val_3axis.z);

    queue_i2c_read8(&fake_bus, LSM6DSR_ADDR_DEFAULT, LSM6DSR_REG_CTRL2_G, 0x0FU, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, LSM6DSR_ADDR_DEFAULT, LSM6DSR_REG_CTRL2_G, 0x2FU, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(0, lsm6dsr_set_gyro_range(accel, LSM6DSR_GYRO_RANGE_500DPS));
    queue_i2c_read8(&fake_bus, LSM6DSR_ADDR_DEFAULT, LSM6DSR_REG_CTRL2_G, 0xF0U, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, LSM6DSR_ADDR_DEFAULT, LSM6DSR_REG_CTRL2_G, 0xF5U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(0, lsm6dsr_set_gyro_rate(accel, LSM6DSR_GYRO_RATE_208Hz));

    queue_i2c_write8(&fake_bus, LSM6DSR_ADDR_DEFAULT, LSM6DSR_REG_CTRL1_XL, 0x00U, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, LSM6DSR_ADDR_DEFAULT, LSM6DSR_REG_CTRL2_G, 0x00U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->deinit(accel));
    queue_i2c_read8(&fake_bus, LSM6DSR_ADDR_DEFAULT, LSM6DSR_REG_WHOAMI, 0x00U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_ERROR, accel->ops->init(accel));
    queue_i2c_read8(&fake_bus, LSM6DSR_ADDR_DEFAULT, LSM6DSR_REG_OUTX_L_G, 0U, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, gyro->ops->read(gyro, &data));

    destroy_sensor(accel);
    destroy_sensor(gyro);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_lsm6dsl_spi_accel_gyro_init_read_deinit_and_helpers);
    RUN_TEST(test_lsm6dso_i2c_accel_gyro_init_read_helpers_and_errors);
    RUN_TEST(test_lsm6dsr_i2c_accel_gyro_init_read_helpers_and_errors);
    return UNITY_END();
}
