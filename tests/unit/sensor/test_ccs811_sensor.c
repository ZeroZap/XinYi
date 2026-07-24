#include "unity.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sensor_ccs811.h"

#define I2C_QUEUE_MAX 8U

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
    void *bus;
    uint8_t addr;
    uint8_t reg;
    uint8_t data[8];
    uint16_t len;
    int ret;
} i2c_mem_op_t;

typedef struct {
    void *bus;
    uint8_t addr;
    uint8_t data[2];
    uint16_t len;
    int ret;
} i2c_write_op_t;

static uint32_t g_tick;
static uint32_t g_delay_total_ms;
static i2c_mem_op_t g_reads[I2C_QUEUE_MAX];
static unsigned int g_read_count;
static unsigned int g_read_index;
static i2c_mem_op_t g_writes[I2C_QUEUE_MAX];
static unsigned int g_write_count;
static unsigned int g_write_index;
static i2c_write_op_t g_plain_writes[I2C_QUEUE_MAX];
static unsigned int g_plain_write_count;
static unsigned int g_plain_write_index;
static unsigned int g_i2c_unexpected;

uint32_t get_tick_ms(void)
{
    return g_tick;
}

void delay_ms(uint32_t ms)
{
    g_delay_total_ms += ms;
}

static void queue_read(void *bus, uint8_t addr, uint8_t reg, const uint8_t *data, uint16_t len,
                       int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(I2C_QUEUE_MAX, g_read_count);
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

static void queue_write(void *bus, uint8_t addr, uint8_t reg, const uint8_t *data, uint16_t len,
                        int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(I2C_QUEUE_MAX, g_write_count);
    i2c_mem_op_t *op = &g_writes[g_write_count++];
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
    queue_write(bus, addr, reg, &value, 1U, ret);
}

static void queue_plain_write(void *bus, uint8_t addr, const uint8_t *data, uint16_t len, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(I2C_QUEUE_MAX, g_plain_write_count);
    i2c_write_op_t *op = &g_plain_writes[g_plain_write_count++];
    op->bus = bus;
    op->addr = addr;
    op->len = len;
    op->ret = ret;
    memset(op->data, 0, sizeof(op->data));
    if (data != NULL && len > 0U) {
        TEST_ASSERT_LESS_OR_EQUAL_UINT(sizeof(op->data), len);
        memcpy(op->data, data, len);
    }
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

int hal_i2c_write(void *bus, uint8_t addr, uint8_t *data, uint16_t len)
{
    if (g_plain_write_index >= g_plain_write_count) {
        g_i2c_unexpected++;
        return -99;
    }

    const i2c_write_op_t *op = &g_plain_writes[g_plain_write_index++];
    TEST_ASSERT_EQUAL_PTR(op->bus, bus);
    TEST_ASSERT_EQUAL_UINT8(op->addr, addr);
    TEST_ASSERT_EQUAL_UINT16(op->len, len);
    if (len > 0U) {
        TEST_ASSERT_NOT_NULL(data);
        TEST_ASSERT_EQUAL_UINT8_ARRAY(op->data, data, len);
    }
    return op->ret;
}

void setUp(void)
{
    g_tick = 515151U;
    g_delay_total_ms = 0;
    memset(g_reads, 0, sizeof(g_reads));
    g_read_count = 0;
    g_read_index = 0;
    memset(g_writes, 0, sizeof(g_writes));
    g_write_count = 0;
    g_write_index = 0;
    memset(g_plain_writes, 0, sizeof(g_plain_writes));
    g_plain_write_count = 0;
    g_plain_write_index = 0;
    g_i2c_unexpected = 0;
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

static void assert_no_extra_i2c(void)
{
    TEST_ASSERT_EQUAL_UINT(g_read_count, g_read_index);
    TEST_ASSERT_EQUAL_UINT(g_write_count, g_write_index);
    TEST_ASSERT_EQUAL_UINT(g_plain_write_count, g_plain_write_index);
    TEST_ASSERT_EQUAL_UINT(0U, g_i2c_unexpected);
}

static void assert_output_unchanged(const sensor_data_t *actual, const sensor_data_t *expected)
{
    TEST_ASSERT_EQUAL_INT(expected->type, actual->type);
    TEST_ASSERT_EQUAL_UINT32(expected->value.val_uint32, actual->value.val_uint32);
    TEST_ASSERT_EQUAL_UINT32(expected->timestamp, actual->timestamp);
}

static void test_create_variants_set_identity_and_reject_null_names(void)
{
    int bus;
    sensor_device_t *co2 = ccs811_create_co2("ccs-co2", &bus);
    sensor_device_t *tvoc = ccs811_create_tvoc("ccs-tvoc", &bus);

    TEST_ASSERT_NULL(ccs811_create_co2(NULL, &bus));
    TEST_ASSERT_NULL(ccs811_create_tvoc(NULL, &bus));
    TEST_ASSERT_NOT_NULL(co2);
    TEST_ASSERT_NOT_NULL(tvoc);

    TEST_ASSERT_EQUAL_STRING("ccs-co2", co2->info.name);
    TEST_ASSERT_EQUAL_STRING("ams", co2->info.vendor);
    TEST_ASSERT_EQUAL_STRING("CCS811", co2->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_CO2, co2->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_PPM, co2->info.unit);
    TEST_ASSERT_EQUAL_UINT8(CCS811_ADDR_DEFAULT, ((ccs811_priv_t *)co2->priv_data)->i2c_addr);
    TEST_ASSERT_EQUAL_PTR(&bus, co2->bus);
    TEST_ASSERT_NOT_NULL(co2->ops->init);
    TEST_ASSERT_NOT_NULL(co2->ops->deinit);
    TEST_ASSERT_NOT_NULL(co2->ops->read);

    TEST_ASSERT_EQUAL_STRING("ccs-tvoc", tvoc->info.name);
    TEST_ASSERT_EQUAL_STRING("ams", tvoc->info.vendor);
    TEST_ASSERT_EQUAL_STRING("CCS811", tvoc->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_TVOC, tvoc->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_PPB, tvoc->info.unit);
    TEST_ASSERT_EQUAL_UINT8(CCS811_ADDR_DEFAULT, ((ccs811_priv_t *)tvoc->priv_data)->i2c_addr);
    TEST_ASSERT_EQUAL_PTR(&bus, tvoc->bus);

    destroy_sensor(co2);
    destroy_sensor(tvoc);
}

static void test_long_names_are_truncated_with_terminator(void)
{
    int bus;
    char long_name[SENSOR_NAME_MAX_LEN * 2U];
    memset(long_name, 'C', sizeof(long_name));
    long_name[sizeof(long_name) - 1U] = '\0';

    sensor_device_t *co2 = ccs811_create_co2(long_name, &bus);
    sensor_device_t *tvoc = ccs811_create_tvoc(long_name, &bus);

    TEST_ASSERT_NOT_NULL(co2);
    TEST_ASSERT_NOT_NULL(tvoc);
    TEST_ASSERT_EQUAL_UINT8('\0', co2->info.name[SENSOR_NAME_MAX_LEN - 1U]);
    TEST_ASSERT_EQUAL_UINT8('\0', tvoc->info.name[SENSOR_NAME_MAX_LEN - 1U]);
    TEST_ASSERT_EQUAL_UINT(SENSOR_NAME_MAX_LEN - 1U, strlen(co2->info.name));
    TEST_ASSERT_EQUAL_UINT(SENSOR_NAME_MAX_LEN - 1U, strlen(tvoc->info.name));

    destroy_sensor(co2);
    destroy_sensor(tvoc);
}

static void test_init_success_and_failure_paths_are_observable(void)
{
    int bus;
    sensor_device_t *sensor = ccs811_create_co2("ccs-init", &bus);
    const uint8_t hw_id = CCS811_HW_ID;
    const uint8_t app_start = CCS811_REG_APP_START;

    TEST_ASSERT_NOT_NULL(sensor);
    queue_read(&bus, CCS811_ADDR_DEFAULT, CCS811_REG_HW_ID, &hw_id, 1U, 0);
    queue_plain_write(&bus, CCS811_ADDR_DEFAULT, &app_start, 1U, 0);
    queue_write_u8(&bus, CCS811_ADDR_DEFAULT, CCS811_REG_MEAS_MODE, 0x10U, 0);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_UINT32(100U, g_delay_total_ms);
    assert_no_extra_i2c();

    setUp();
    queue_read(&bus, CCS811_ADDR_DEFAULT, CCS811_REG_HW_ID, NULL, 1U, -5);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->init(sensor));
    assert_no_extra_i2c();

    setUp();
    const uint8_t wrong_id = 0x00U;
    queue_read(&bus, CCS811_ADDR_DEFAULT, CCS811_REG_HW_ID, &wrong_id, 1U, 0);
    TEST_ASSERT_EQUAL_INT(SENSOR_ERROR, sensor->ops->init(sensor));
    assert_no_extra_i2c();

    setUp();
    queue_read(&bus, CCS811_ADDR_DEFAULT, CCS811_REG_HW_ID, &hw_id, 1U, 0);
    queue_plain_write(&bus, CCS811_ADDR_DEFAULT, &app_start, 1U, -7);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->init(sensor));
    assert_no_extra_i2c();

    setUp();
    queue_read(&bus, CCS811_ADDR_DEFAULT, CCS811_REG_HW_ID, &hw_id, 1U, 0);
    queue_plain_write(&bus, CCS811_ADDR_DEFAULT, &app_start, 1U, 0);
    queue_write_u8(&bus, CCS811_ADDR_DEFAULT, CCS811_REG_MEAS_MODE, 0x10U, -8);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->init(sensor));
    assert_no_extra_i2c();

    destroy_sensor(sensor);
}

static void test_deinit_propagates_write_failure(void)
{
    int bus;
    sensor_device_t *sensor = ccs811_create_co2("ccs-deinit", &bus);

    TEST_ASSERT_NOT_NULL(sensor);
    queue_write_u8(&bus, CCS811_ADDR_DEFAULT, CCS811_REG_MEAS_MODE, 0x00U, -1);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->deinit(sensor));
    assert_no_extra_i2c();

    destroy_sensor(sensor);
}

static void test_read_co2_and_tvoc_use_algorithm_result_and_timestamp(void)
{
    int bus;
    sensor_device_t *co2 = ccs811_create_co2("ccs-co2", &bus);
    sensor_device_t *tvoc = ccs811_create_tvoc("ccs-tvoc", &bus);
    sensor_data_t data = {0};
    const uint8_t ready = 0x08U;
    const uint8_t result[8] = {0x04U, 0xD2U, 0x00U, 0x38U, 0U, 0U, 0U, 0U};

    TEST_ASSERT_NOT_NULL(co2);
    TEST_ASSERT_NOT_NULL(tvoc);

    queue_read(&bus, CCS811_ADDR_DEFAULT, CCS811_REG_STATUS, &ready, 1U, 0);
    queue_read(&bus, CCS811_ADDR_DEFAULT, CCS811_REG_ALG_RESULT, result, sizeof(result), 0);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, co2->ops->read(co2, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_CO2, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_PPM, data.unit);
    TEST_ASSERT_EQUAL_UINT32(1234U, data.value.val_uint32);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);
    TEST_ASSERT_EQUAL_UINT8(85U, data.accuracy);

    setUp();
    queue_read(&bus, CCS811_ADDR_DEFAULT, CCS811_REG_STATUS, &ready, 1U, 0);
    queue_read(&bus, CCS811_ADDR_DEFAULT, CCS811_REG_ALG_RESULT, result, sizeof(result), 0);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, tvoc->ops->read(tvoc, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_TVOC, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_PPB, data.unit);
    TEST_ASSERT_EQUAL_UINT32(56U, data.value.val_uint32);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);
    TEST_ASSERT_EQUAL_UINT8(85U, data.accuracy);
    assert_no_extra_i2c();

    destroy_sensor(co2);
    destroy_sensor(tvoc);
}

static void test_read_failures_preserve_output_and_return_specific_status(void)
{
    int bus;
    sensor_device_t *sensor = ccs811_create_co2("ccs-read", &bus);
    sensor_data_t data = {.type = SENSOR_TYPE_CUSTOM, .value.val_uint32 = 0xA5A5U, .timestamp = 9U};
    sensor_data_t snapshot = data;
    const uint8_t not_ready = 0x00U;
    const uint8_t ready = 0x08U;

    TEST_ASSERT_NOT_NULL(sensor);

    queue_read(&bus, CCS811_ADDR_DEFAULT, CCS811_REG_STATUS, &not_ready, 1U, 0);
    TEST_ASSERT_EQUAL_INT(SENSOR_EBUSY, sensor->ops->read(sensor, &data));
    assert_output_unchanged(&data, &snapshot);
    assert_no_extra_i2c();

    setUp();
    data = snapshot;
    queue_read(&bus, CCS811_ADDR_DEFAULT, CCS811_REG_STATUS, &ready, 1U, 0);
    queue_read(&bus, CCS811_ADDR_DEFAULT, CCS811_REG_ALG_RESULT, NULL, 8U, -1);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->read(sensor, &data));
    assert_output_unchanged(&data, &snapshot);
    assert_no_extra_i2c();

    destroy_sensor(sensor);
}

static void test_public_ops_reject_null_inputs_and_missing_private_data(void)
{
    int bus;
    sensor_device_t *sensor = ccs811_create_co2("ccs-guard", &bus);
    sensor_data_t data = {.type = SENSOR_TYPE_CUSTOM, .value.val_uint32 = 7U, .timestamp = 11U};
    sensor_data_t snapshot = data;

    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->init(NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->deinit(NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->read(NULL, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->read(sensor, NULL));
    assert_output_unchanged(&data, &snapshot);

    SENSOR_FREE(sensor->priv_data);
    sensor->priv_data = NULL;
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->deinit(sensor));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->read(sensor, &data));
    assert_output_unchanged(&data, &snapshot);
    assert_no_extra_i2c();

    destroy_sensor(sensor);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_create_variants_set_identity_and_reject_null_names);
    RUN_TEST(test_long_names_are_truncated_with_terminator);
    RUN_TEST(test_init_success_and_failure_paths_are_observable);
    RUN_TEST(test_deinit_propagates_write_failure);
    RUN_TEST(test_read_co2_and_tvoc_use_algorithm_result_and_timestamp);
    RUN_TEST(test_read_failures_preserve_output_and_return_specific_status);
    RUN_TEST(test_public_ops_reject_null_inputs_and_missing_private_data);
    return UNITY_END();
}
