#include "unity.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sensor_vl53l0x.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
    void *bus;
    uint8_t addr;
    uint8_t reg;
    uint8_t data[16];
    uint16_t len;
    int ret;
} i2c_mem_op_t;

static i2c_mem_op_t g_reads[8];
static i2c_mem_op_t g_writes[8];
static unsigned int g_read_count;
static unsigned int g_read_index;
static unsigned int g_write_count;
static unsigned int g_write_index;
static uint32_t g_tick;
static uint32_t g_delay_total_ms;
static unsigned int g_delay_count;
static unsigned int g_i2c_unexpected;

uint32_t get_tick_ms(void)
{
    return g_tick;
}

uint32_t xy_os_tick_get(void)
{
    return g_tick;
}

void delay_ms(uint32_t ms)
{
    g_delay_total_ms += ms;
    g_delay_count++;
}

static void queue_read(void *bus, uint8_t addr, uint8_t reg, const uint8_t *data, uint16_t len,
                       int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_reads), g_read_count);
    i2c_mem_op_t *op = &g_reads[g_read_count++];
    op->bus = bus;
    op->addr = addr;
    op->reg = reg;
    op->len = len;
    op->ret = ret;
    memset(op->data, 0, sizeof(op->data));
    if (data != NULL && len > 0U) {
        TEST_ASSERT_LESS_OR_EQUAL_UINT(sizeof(op->data), len);
        memcpy(op->data, data, len);
    }
}

static void queue_write_u8(void *bus, uint8_t addr, uint8_t reg, uint8_t value, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_writes), g_write_count);
    i2c_mem_op_t *op = &g_writes[g_write_count++];
    op->bus = bus;
    op->addr = addr;
    op->reg = reg;
    op->len = 1U;
    op->ret = ret;
    op->data[0] = value;
}

int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    if (g_read_index >= g_read_count) {
        g_i2c_unexpected++;
        return -99;
    }

    const i2c_mem_op_t *op = &g_reads[g_read_index++];
    TEST_ASSERT_EQUAL_PTR(op->bus, bus);
    TEST_ASSERT_EQUAL_UINT8(op->addr, addr);
    TEST_ASSERT_EQUAL_UINT8(op->reg, reg);
    TEST_ASSERT_EQUAL_UINT16(op->len, len);
    if (op->ret == 0 && data != NULL && len > 0U) {
        memcpy(data, op->data, len);
    }
    return op->ret;
}

int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    if (g_write_index >= g_write_count) {
        g_i2c_unexpected++;
        return -99;
    }

    const i2c_mem_op_t *op = &g_writes[g_write_index++];
    TEST_ASSERT_EQUAL_PTR(op->bus, bus);
    TEST_ASSERT_EQUAL_UINT8(op->addr, addr);
    TEST_ASSERT_EQUAL_UINT8(op->reg, reg);
    TEST_ASSERT_EQUAL_UINT16(op->len, len);
    if (len > 0U) {
        TEST_ASSERT_NOT_NULL(data);
        TEST_ASSERT_EQUAL_UINT8_ARRAY(op->data, data, len);
    }
    return op->ret;
}

static void destroy_sensor(sensor_device_t *sensor)
{
    if (sensor != NULL) {
        SENSOR_FREE(sensor->priv_data);
        SENSOR_FREE(sensor);
    }
}

void setUp(void)
{
    memset(g_reads, 0, sizeof(g_reads));
    memset(g_writes, 0, sizeof(g_writes));
    g_read_count = 0;
    g_read_index = 0;
    g_write_count = 0;
    g_write_index = 0;
    g_tick = 1000U;
    g_delay_total_ms = 0U;
    g_delay_count = 0U;
    g_i2c_unexpected = 0U;
}

void tearDown(void)
{
}

static void assert_i2c_drained(void)
{
    TEST_ASSERT_EQUAL_UINT(g_read_count, g_read_index);
    TEST_ASSERT_EQUAL_UINT(g_write_count, g_write_index);
    TEST_ASSERT_EQUAL_UINT(0U, g_i2c_unexpected);
}

void test_vl53l0x_create_sets_identity_and_contract_fields(void)
{
    void *bus = (void *)0x5300;
    TEST_ASSERT_NULL(vl53l0x_create(NULL, bus));

    sensor_device_t *sensor = vl53l0x_create("vl53_long_sensor_name_for_truncation", bus);

    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_NOT_NULL(sensor->priv_data);
    TEST_ASSERT_EQUAL_STRING_LEN("vl53_long_sensor_name_for_truncation", sensor->info.name,
                                 SENSOR_NAME_MAX_LEN - 1U);
    TEST_ASSERT_EQUAL_STRING("STMicroelectronics", sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING("VL53L0X", sensor->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_DISTANCE, sensor->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_MILLIMETER, sensor->info.unit);
    TEST_ASSERT_EQUAL_INT32(2000, sensor->info.range_max);
    TEST_ASSERT_EQUAL_INT32(30, sensor->info.range_min);
    TEST_ASSERT_EQUAL_UINT32(SENSOR_FLAG_HIGH_PRECISION, sensor->info.flags);
    TEST_ASSERT_EQUAL_PTR(bus, sensor->bus);
    TEST_ASSERT_EQUAL_INT(SENSOR_STATUS_IDLE, sensor->status);
    TEST_ASSERT_NOT_NULL(sensor->ops);
    TEST_ASSERT_NOT_NULL(sensor->ops->init);
    TEST_ASSERT_NOT_NULL(sensor->ops->read);
    TEST_ASSERT_NOT_NULL(sensor->ops->deinit);
    TEST_ASSERT_EQUAL_UINT8(VL53L0X_ADDR_DEFAULT, ((vl53l0x_priv_t *)sensor->priv_data)->i2c_addr);

    destroy_sensor(sensor);
}

void test_vl53l0x_init_checks_model_id(void)
{
    void *bus = (void *)0x5301;
    sensor_device_t *sensor = vl53l0x_create("tof", bus);
    TEST_ASSERT_NOT_NULL(sensor);

    uint8_t model = 0xEE;
    queue_read(bus, VL53L0X_ADDR_DEFAULT, VL53L0X_REG_IDENTIFICATION_MODEL_ID, &model, 1U, 0);

    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    assert_i2c_drained();

    destroy_sensor(sensor);
}

void test_vl53l0x_init_propagates_model_read_and_identity_failures(void)
{
    void *bus = (void *)0x5302;
    sensor_device_t *sensor = vl53l0x_create("tof", bus);
    TEST_ASSERT_NOT_NULL(sensor);

    queue_read(bus, VL53L0X_ADDR_DEFAULT, VL53L0X_REG_IDENTIFICATION_MODEL_ID, NULL, 1U, -5);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->init(sensor));

    uint8_t wrong_model = 0xEA;
    queue_read(bus, VL53L0X_ADDR_DEFAULT, VL53L0X_REG_IDENTIFICATION_MODEL_ID, &wrong_model, 1U,
               0);
    TEST_ASSERT_EQUAL_INT(SENSOR_ERROR, sensor->ops->init(sensor));
    assert_i2c_drained();

    destroy_sensor(sensor);
}

void test_vl53l0x_public_ops_reject_null_inputs_without_i2c_side_effects(void)
{
    sensor_device_t *sensor = vl53l0x_create("tof", (void *)0x5305);
    sensor_data_t data = {.type = SENSOR_TYPE_CUSTOM, .value.val_uint32 = 42U};
    TEST_ASSERT_NOT_NULL(sensor);

    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->init(NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->deinit(NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->read(NULL, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->read(sensor, NULL));

    sensor->priv_data = NULL;
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->deinit(sensor));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_UINT(0U, g_read_index);
    TEST_ASSERT_EQUAL_UINT(0U, g_write_index);

    SENSOR_FREE(sensor);
}

void test_vl53l0x_read_converts_distance_and_timestamp(void)
{
    void *bus = (void *)0x5303;
    sensor_device_t *sensor = vl53l0x_create("tof", bus);
    sensor_data_t data = {.type = SENSOR_TYPE_CUSTOM, .value.val_uint32 = 0U, .timestamp = 0U};
    TEST_ASSERT_NOT_NULL(sensor);

    uint8_t range[12] = {0};
    range[10] = 0x01;
    range[11] = 0xF4;
    queue_write_u8(bus, VL53L0X_ADDR_DEFAULT, VL53L0X_REG_SYSRANGE_START, 0x01, 0);
    queue_read(bus, VL53L0X_ADDR_DEFAULT, VL53L0X_REG_RESULT_RANGE_STATUS, range,
               sizeof(range), 0);
    g_tick = 4321U;

    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_DISTANCE, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_MILLIMETER, data.unit);
    TEST_ASSERT_EQUAL_UINT32(500U, data.value.val_uint32);
    TEST_ASSERT_EQUAL_UINT32(4321U, data.timestamp);
    TEST_ASSERT_EQUAL_UINT8(92U, data.accuracy);
    TEST_ASSERT_EQUAL_UINT32(50U, g_delay_total_ms);
    TEST_ASSERT_EQUAL_UINT(1U, g_delay_count);
    assert_i2c_drained();

    destroy_sensor(sensor);
}

void test_vl53l0x_read_preserves_output_on_range_read_failure(void)
{
    void *bus = (void *)0x5304;
    sensor_device_t *sensor = vl53l0x_create("tof", bus);
    sensor_data_t data = {.type = SENSOR_TYPE_CUSTOM, .unit = SENSOR_UNIT_PPM,
                          .value.val_uint32 = 77U, .timestamp = 88U, .accuracy = 12U};
    TEST_ASSERT_NOT_NULL(sensor);

    queue_write_u8(bus, VL53L0X_ADDR_DEFAULT, VL53L0X_REG_SYSRANGE_START, 0x01, 0);
    queue_read(bus, VL53L0X_ADDR_DEFAULT, VL53L0X_REG_RESULT_RANGE_STATUS, NULL, 12U, -6);

    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_CUSTOM, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_PPM, data.unit);
    TEST_ASSERT_EQUAL_UINT32(77U, data.value.val_uint32);
    TEST_ASSERT_EQUAL_UINT32(88U, data.timestamp);
    TEST_ASSERT_EQUAL_UINT8(12U, data.accuracy);
    assert_i2c_drained();

    destroy_sensor(sensor);
}

void test_vl53l0x_read_propagates_start_write_failure_without_delay(void)
{
    void *bus = (void *)0x5306;
    sensor_device_t *sensor = vl53l0x_create("tof", bus);
    sensor_data_t data = {.type = SENSOR_TYPE_CUSTOM, .value.val_uint32 = 22U};
    TEST_ASSERT_NOT_NULL(sensor);

    queue_write_u8(bus, VL53L0X_ADDR_DEFAULT, VL53L0X_REG_SYSRANGE_START, 0x01, -7);

    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_CUSTOM, data.type);
    TEST_ASSERT_EQUAL_UINT32(22U, data.value.val_uint32);
    TEST_ASSERT_EQUAL_UINT32(0U, g_delay_total_ms);
    TEST_ASSERT_EQUAL_UINT(0U, g_delay_count);
    assert_i2c_drained();

    destroy_sensor(sensor);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_vl53l0x_create_sets_identity_and_contract_fields);
    RUN_TEST(test_vl53l0x_init_checks_model_id);
    RUN_TEST(test_vl53l0x_init_propagates_model_read_and_identity_failures);
    RUN_TEST(test_vl53l0x_public_ops_reject_null_inputs_without_i2c_side_effects);
    RUN_TEST(test_vl53l0x_read_converts_distance_and_timestamp);
    RUN_TEST(test_vl53l0x_read_preserves_output_on_range_read_failure);
    RUN_TEST(test_vl53l0x_read_propagates_start_write_failure_without_delay);
    return UNITY_END();
}
