#include "unity.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sensor_cms.h"
#include "sensor_dmp6100.h"
#include "sensor_hs_ads1100.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
    void *bus;
    uint8_t addr;
    uint8_t reg;
    uint8_t data[8];
    uint16_t len;
    int ret;
} i2c_op_t;

static i2c_op_t g_i2c_reads[24];
static i2c_op_t g_i2c_writes[8];
static size_t g_i2c_read_count;
static size_t g_i2c_read_index;
static size_t g_i2c_write_count;
static size_t g_i2c_write_index;
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

static void queue_i2c_read8(void *bus, uint8_t addr, uint8_t reg, uint8_t value, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_i2c_reads), g_i2c_read_count);
    i2c_op_t *op = &g_i2c_reads[g_i2c_read_count++];
    op->bus = bus;
    op->addr = addr;
    op->reg = reg;
    op->len = 1U;
    op->ret = ret;
    op->data[0] = value;
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

void setUp(void)
{
    memset(g_i2c_reads, 0, sizeof(g_i2c_reads));
    memset(g_i2c_writes, 0, sizeof(g_i2c_writes));
    g_i2c_read_count = 0;
    g_i2c_read_index = 0;
    g_i2c_write_count = 0;
    g_i2c_write_index = 0;
    g_tick = 13579U;
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

static void assert_i2c_drained(void)
{
    TEST_ASSERT_EQUAL_UINT(g_i2c_read_count, g_i2c_read_index);
    TEST_ASSERT_EQUAL_UINT(g_i2c_write_count, g_i2c_write_index);
}

static void assert_common_stub_accel(sensor_device_t *sensor, const char *name,
                                     const char *vendor, const char *model,
                                     uint8_t expected_addr)
{
    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_STRING(name, sensor->info.name);
    TEST_ASSERT_EQUAL_STRING(vendor, sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING(model, sensor->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_ACCELEROMETER, sensor->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_STATUS_IDLE, sensor->status);
    TEST_ASSERT_NOT_NULL(sensor->ops);
    TEST_ASSERT_NOT_NULL(sensor->ops->init);
    TEST_ASSERT_NULL(sensor->ops->deinit);
    TEST_ASSERT_NOT_NULL(sensor->ops->read);
    TEST_ASSERT_NOT_NULL(sensor->priv_data);
    TEST_ASSERT_EQUAL_UINT8(expected_addr, *(uint8_t *)sensor->priv_data);
}

static void queue_xyz_bytes(void *bus, uint8_t addr, uint8_t start_reg, const uint8_t raw[6])
{
    for (uint8_t i = 0U; i < 6U; ++i) {
        queue_i2c_read8(bus, addr, (uint8_t)(start_reg + i), raw[i], SENSOR_EOK);
    }
}

static void test_dmp6100_create_init_and_read_contract(void)
{
    int fake_bus;
    sensor_data_t data = {0};
    const uint8_t raw[6] = {0x01, 0x00, 0xFF, 0xFF, 0x00, 0x80};
    sensor_device_t *sensor = dmp6100_create("dmp6100-main", &fake_bus, 0U);

    assert_common_stub_accel(sensor, "dmp6100-main", "国产", "DMP6100", DMP6100_ADDR_DEFAULT);

    queue_i2c_write8(&fake_bus, DMP6100_ADDR_DEFAULT, DMP6100_REG_CTRL, 0x56U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));

    queue_xyz_bytes(&fake_bus, DMP6100_ADDR_DEFAULT, DMP6100_REG_DATA, raw);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_ACCELEROMETER, data.type);
    TEST_ASSERT_EQUAL_INT32(16, data.value.val_3axis.x);
    TEST_ASSERT_EQUAL_INT32(-16, data.value.val_3axis.y);
    TEST_ASSERT_EQUAL_INT32(-524288, data.value.val_3axis.z);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);
    assert_i2c_drained();

    destroy_sensor(sensor);
}

static void test_cms_create_init_and_read_contract(void)
{
    int fake_bus;
    sensor_data_t data = {0};
    const uint8_t raw[6] = {0x34, 0x12, 0x00, 0x80, 0xFF, 0x7F};
    sensor_device_t *sensor = cms_create("cms-main", &fake_bus, 0x1AU);

    assert_common_stub_accel(sensor, "cms-main", "CRmicro", "CMS", 0x1AU);

    queue_i2c_write8(&fake_bus, 0x1AU, CMS_REG_CTRL1, 0x57U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));

    queue_xyz_bytes(&fake_bus, 0x1AU, CMS_REG_OUT_X_L, raw);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_ACCELEROMETER, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_MILLI_G, data.unit);
    TEST_ASSERT_EQUAL_INT32(0x1234, data.value.val_3axis.x);
    TEST_ASSERT_EQUAL_INT32(-32768, data.value.val_3axis.y);
    TEST_ASSERT_EQUAL_INT32(32767, data.value.val_3axis.z);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);
    assert_i2c_drained();

    destroy_sensor(sensor);
}

static void test_hs_ads1100_create_init_and_read_contract(void)
{
    int fake_bus;
    sensor_data_t data = {0};
    const uint8_t raw[6] = {0x78, 0x56, 0xFE, 0xFF, 0x00, 0x80};
    sensor_device_t *sensor = hs_ads1100_create("hsads-main", &fake_bus, 0U);

    assert_common_stub_accel(sensor, "hsads-main", "Hangshun", "HS-ADS1100",
                             HS_ADS1100_ADDR_DEFAULT);

    queue_i2c_write8(&fake_bus, HS_ADS1100_ADDR_DEFAULT, HS_ADS1100_REG_CTRL1, 0x57U,
                     SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));

    queue_xyz_bytes(&fake_bus, HS_ADS1100_ADDR_DEFAULT, HS_ADS1100_REG_OUT_X_L, raw);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_ACCELEROMETER, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_MILLI_G, data.unit);
    TEST_ASSERT_EQUAL_INT32(0x5678, data.value.val_3axis.x);
    TEST_ASSERT_EQUAL_INT32(-2, data.value.val_3axis.y);
    TEST_ASSERT_EQUAL_INT32(-32768, data.value.val_3axis.z);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);
    assert_i2c_drained();

    destroy_sensor(sensor);
}

static void test_long_names_are_truncated_with_terminator(void)
{
    int fake_bus;
    char long_name[SENSOR_NAME_MAX_LEN * 2U];
    memset(long_name, 'A', sizeof(long_name));
    long_name[sizeof(long_name) - 1U] = '\0';

    sensor_device_t *dmp6100 = dmp6100_create(long_name, &fake_bus, 0U);
    sensor_device_t *cms = cms_create(long_name, &fake_bus, 0U);
    sensor_device_t *hs_ads1100 = hs_ads1100_create(long_name, &fake_bus, 0U);

    TEST_ASSERT_NOT_NULL(dmp6100);
    TEST_ASSERT_NOT_NULL(cms);
    TEST_ASSERT_NOT_NULL(hs_ads1100);
    TEST_ASSERT_EQUAL_UINT8('\0', dmp6100->info.name[SENSOR_NAME_MAX_LEN - 1U]);
    TEST_ASSERT_EQUAL_UINT8('\0', cms->info.name[SENSOR_NAME_MAX_LEN - 1U]);
    TEST_ASSERT_EQUAL_UINT8('\0', hs_ads1100->info.name[SENSOR_NAME_MAX_LEN - 1U]);
    TEST_ASSERT_EQUAL_UINT(SENSOR_NAME_MAX_LEN - 1U, strlen(dmp6100->info.name));
    TEST_ASSERT_EQUAL_UINT(SENSOR_NAME_MAX_LEN - 1U, strlen(cms->info.name));
    TEST_ASSERT_EQUAL_UINT(SENSOR_NAME_MAX_LEN - 1U, strlen(hs_ads1100->info.name));

    destroy_sensor(dmp6100);
    destroy_sensor(cms);
    destroy_sensor(hs_ads1100);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_dmp6100_create_init_and_read_contract);
    RUN_TEST(test_cms_create_init_and_read_contract);
    RUN_TEST(test_hs_ads1100_create_init_and_read_contract);
    RUN_TEST(test_long_names_are_truncated_with_terminator);
    return UNITY_END();
}
