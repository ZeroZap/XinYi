#include "unity.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sensor_ap3216c.h"

#define I2C_QUEUE_MAX 8U

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
    void *bus;
    uint8_t addr;
    uint8_t reg;
    uint8_t data[4];
    uint16_t len;
    int ret;
} i2c_read_op_t;

typedef struct {
    void *bus;
    uint8_t addr;
    uint8_t reg;
    uint8_t data[2];
    uint16_t len;
    int ret;
} i2c_write_op_t;

static uint32_t g_tick;
static uint32_t g_delay_total_ms;
static i2c_read_op_t g_i2c_reads[I2C_QUEUE_MAX];
static unsigned int g_i2c_read_count;
static unsigned int g_i2c_read_index;
static i2c_write_op_t g_i2c_writes[I2C_QUEUE_MAX];
static unsigned int g_i2c_write_count;
static unsigned int g_i2c_write_index;
static unsigned int g_i2c_unexpected;

uint32_t get_tick_ms(void)
{
    return g_tick;
}

void delay_ms(uint32_t ms)
{
    g_delay_total_ms += ms;
}

static void queue_i2c_read(void *bus, uint8_t addr, uint8_t reg, const uint8_t *data, uint16_t len,
                           int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(I2C_QUEUE_MAX, g_i2c_read_count);
    i2c_read_op_t *op = &g_i2c_reads[g_i2c_read_count++];
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

static void queue_i2c_write(void *bus, uint8_t addr, uint8_t reg, const uint8_t *data,
                            uint16_t len, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(I2C_QUEUE_MAX, g_i2c_write_count);
    i2c_write_op_t *op = &g_i2c_writes[g_i2c_write_count++];
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

int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    if (g_i2c_read_index >= g_i2c_read_count) {
        g_i2c_unexpected++;
        return -99;
    }

    const i2c_read_op_t *op = &g_i2c_reads[g_i2c_read_index++];
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
    if (g_i2c_write_index >= g_i2c_write_count) {
        g_i2c_unexpected++;
        return -99;
    }

    const i2c_write_op_t *op = &g_i2c_writes[g_i2c_write_index++];
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

void setUp(void)
{
    g_tick = 424242U;
    g_delay_total_ms = 0;
    memset(g_i2c_reads, 0, sizeof(g_i2c_reads));
    g_i2c_read_count = 0;
    g_i2c_read_index = 0;
    memset(g_i2c_writes, 0, sizeof(g_i2c_writes));
    g_i2c_write_count = 0;
    g_i2c_write_index = 0;
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
    TEST_ASSERT_EQUAL_UINT(g_i2c_read_count, g_i2c_read_index);
    TEST_ASSERT_EQUAL_UINT(g_i2c_write_count, g_i2c_write_index);
    TEST_ASSERT_EQUAL_UINT(0U, g_i2c_unexpected);
}

static void test_create_variants_set_identity_and_reject_null_names(void)
{
    int bus;
    sensor_device_t *light = ap3216c_create_light("ap-light", &bus);
    sensor_device_t *prox = ap3216c_create_proximity("ap-prox", &bus);
    sensor_device_t *ir = ap3216c_create_ir("ap-ir", &bus);

    TEST_ASSERT_NULL(ap3216c_create_light(NULL, &bus));
    TEST_ASSERT_NULL(ap3216c_create_proximity(NULL, &bus));
    TEST_ASSERT_NULL(ap3216c_create_ir(NULL, &bus));
    TEST_ASSERT_NOT_NULL(light);
    TEST_ASSERT_NOT_NULL(prox);
    TEST_ASSERT_NOT_NULL(ir);
    TEST_ASSERT_EQUAL_STRING("ap-light", light->info.name);
    TEST_ASSERT_EQUAL_STRING("ap-prox", prox->info.name);
    TEST_ASSERT_EQUAL_STRING("ap-ir", ir->info.name);
    TEST_ASSERT_EQUAL_STRING("Liteon", light->info.vendor);
    TEST_ASSERT_EQUAL_STRING("AP3216C", prox->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_AMBIENT_LIGHT, light->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_PROXIMITY, prox->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_IR, ir->info.type);
    TEST_ASSERT_EQUAL_UINT8(AP3216C_ADDR_DEFAULT, ((ap3216c_priv_t *)light->priv_data)->i2c_addr);
    TEST_ASSERT_EQUAL_UINT8(AP3216C_MODE_ALS_PS_IR, ((ap3216c_priv_t *)prox->priv_data)->mode);
    TEST_ASSERT_EQUAL_PTR(&bus, ir->bus);
    TEST_ASSERT_NOT_NULL(light->ops->init);
    TEST_ASSERT_NOT_NULL(prox->ops->deinit);
    TEST_ASSERT_NOT_NULL(ir->ops->read);

    destroy_sensor(light);
    destroy_sensor(prox);
    destroy_sensor(ir);
}

static void test_init_deinit_propagate_config_write_failures(void)
{
    int bus;
    sensor_device_t *sensor = ap3216c_create_light("ap-init", &bus);
    uint8_t reset = 0x04U;
    uint8_t mode = AP3216C_MODE_ALS_PS_IR;
    uint8_t off = AP3216C_MODE_POWER_DOWN;

    TEST_ASSERT_NOT_NULL(sensor);
    queue_i2c_write(&bus, AP3216C_ADDR_DEFAULT, AP3216C_REG_SYS_CONFIG, &reset, 1U, -5);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_UINT32(0U, g_delay_total_ms);
    assert_no_extra_i2c();

    queue_i2c_write(&bus, AP3216C_ADDR_DEFAULT, AP3216C_REG_SYS_CONFIG, &reset, 1U, 0);
    queue_i2c_write(&bus, AP3216C_ADDR_DEFAULT, AP3216C_REG_SYS_CONFIG, &mode, 1U, -6);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_UINT32(50U, g_delay_total_ms);
    assert_no_extra_i2c();

    queue_i2c_write(&bus, AP3216C_ADDR_DEFAULT, AP3216C_REG_SYS_CONFIG, &reset, 1U, 0);
    queue_i2c_write(&bus, AP3216C_ADDR_DEFAULT, AP3216C_REG_SYS_CONFIG, &mode, 1U, 0);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_UINT32(150U, g_delay_total_ms);
    assert_no_extra_i2c();

    queue_i2c_write(&bus, AP3216C_ADDR_DEFAULT, AP3216C_REG_SYS_CONFIG, &off, 1U, -7);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->deinit(sensor));
    assert_no_extra_i2c();

    queue_i2c_write(&bus, AP3216C_ADDR_DEFAULT, AP3216C_REG_SYS_CONFIG, &off, 1U, 0);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->deinit(sensor));
    assert_no_extra_i2c();

    destroy_sensor(sensor);
}

static void test_light_proximity_and_ir_reads_convert_raw_values(void)
{
    int bus;
    sensor_device_t *light = ap3216c_create_light("ap-light-read", &bus);
    sensor_device_t *prox = ap3216c_create_proximity("ap-prox-read", &bus);
    sensor_device_t *ir = ap3216c_create_ir("ap-ir-read", &bus);
    sensor_data_t data = {0};
    uint8_t als[] = {0x34U, 0x12U};
    uint8_t ps[] = {0x8AU, 0x21U};
    uint8_t ir_raw[] = {0xAAU, 0x03U};

    TEST_ASSERT_NOT_NULL(light);
    TEST_ASSERT_NOT_NULL(prox);
    TEST_ASSERT_NOT_NULL(ir);

    queue_i2c_read(&bus, AP3216C_ADDR_DEFAULT, AP3216C_REG_ALS_DATA_L, als, sizeof(als), 0);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, light->ops->read(light, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_AMBIENT_LIGHT, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_LUX, data.unit);
    TEST_ASSERT_EQUAL_UINT32(1631U, data.value.val_uint32);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);
    TEST_ASSERT_EQUAL_UINT8(90U, data.accuracy);

    g_tick = 123U;
    queue_i2c_read(&bus, AP3216C_ADDR_DEFAULT, AP3216C_REG_PS_DATA_L, ps, sizeof(ps), 0);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, prox->ops->read(prox, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_PROXIMITY, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_NONE, data.unit);
    TEST_ASSERT_EQUAL_INT32(0x21AU, data.value.val_int32);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);
    TEST_ASSERT_EQUAL_UINT8(85U, data.accuracy);

    g_tick = 456U;
    queue_i2c_read(&bus, AP3216C_ADDR_DEFAULT, AP3216C_REG_IR_DATA_L, ir_raw, sizeof(ir_raw), 0);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, ir->ops->read(ir, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_IR, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_NONE, data.unit);
    TEST_ASSERT_EQUAL_UINT32(0x3AAU, data.value.val_uint32);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);
    TEST_ASSERT_EQUAL_UINT8(85U, data.accuracy);
    assert_no_extra_i2c();

    destroy_sensor(light);
    destroy_sensor(prox);
    destroy_sensor(ir);
}

static void test_read_failures_and_overflow_preserve_output(void)
{
    int bus;
    sensor_device_t *light = ap3216c_create_light("ap-light-fail", &bus);
    sensor_device_t *prox = ap3216c_create_proximity("ap-prox-fail", &bus);
    sensor_device_t *ir = ap3216c_create_ir("ap-ir-fail", &bus);
    sensor_data_t data = {.type = SENSOR_TYPE_CUSTOM, .unit = SENSOR_UNIT_PPM,
                          .value.val_uint32 = 0xA5A5U, .timestamp = 999U, .accuracy = 1U};
    sensor_data_t snapshot = data;
    uint8_t overflow[] = {0x40U, 0x00U};

    TEST_ASSERT_NOT_NULL(light);
    TEST_ASSERT_NOT_NULL(prox);
    TEST_ASSERT_NOT_NULL(ir);

    queue_i2c_read(&bus, AP3216C_ADDR_DEFAULT, AP3216C_REG_ALS_DATA_L, NULL, 2U, -1);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, light->ops->read(light, &data));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &data, sizeof(data));

    queue_i2c_read(&bus, AP3216C_ADDR_DEFAULT, AP3216C_REG_PS_DATA_L, overflow, sizeof(overflow), 0);
    TEST_ASSERT_EQUAL_INT(SENSOR_ERROR, prox->ops->read(prox, &data));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &data, sizeof(data));

    queue_i2c_read(&bus, AP3216C_ADDR_DEFAULT, AP3216C_REG_IR_DATA_L, NULL, 2U, -2);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, ir->ops->read(ir, &data));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &data, sizeof(data));
    assert_no_extra_i2c();

    destroy_sensor(light);
    destroy_sensor(prox);
    destroy_sensor(ir);
}

static void test_public_ops_reject_null_and_missing_private_data_without_i2c(void)
{
    int bus;
    sensor_device_t *light = ap3216c_create_light("ap-light-guard", &bus);
    sensor_device_t *prox = ap3216c_create_proximity("ap-prox-guard", &bus);
    sensor_device_t *ir = ap3216c_create_ir("ap-ir-guard", &bus);
    sensor_data_t data = {.type = SENSOR_TYPE_CUSTOM, .value.val_uint32 = 77U, .timestamp = 88U};
    sensor_data_t snapshot = data;

    TEST_ASSERT_NOT_NULL(light);
    TEST_ASSERT_NOT_NULL(prox);
    TEST_ASSERT_NOT_NULL(ir);

    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, light->ops->init(NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, light->ops->deinit(NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, light->ops->read(NULL, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, light->ops->read(light, NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, prox->ops->read(NULL, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, prox->ops->read(prox, NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, ir->ops->read(NULL, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, ir->ops->read(ir, NULL));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &data, sizeof(data));

    SENSOR_FREE(light->priv_data);
    light->priv_data = NULL;
    SENSOR_FREE(prox->priv_data);
    prox->priv_data = NULL;
    SENSOR_FREE(ir->priv_data);
    ir->priv_data = NULL;

    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, light->ops->init(light));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, light->ops->deinit(light));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, light->ops->read(light, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, prox->ops->read(prox, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, ir->ops->read(ir, &data));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &data, sizeof(data));
    TEST_ASSERT_EQUAL_UINT(0U, g_i2c_read_count);
    TEST_ASSERT_EQUAL_UINT(0U, g_i2c_write_count);

    light->priv_data = SENSOR_MALLOC(sizeof(ap3216c_priv_t));
    TEST_ASSERT_NOT_NULL(light->priv_data);
    ((ap3216c_priv_t *)light->priv_data)->i2c_addr = AP3216C_ADDR_DEFAULT;
    ((ap3216c_priv_t *)light->priv_data)->mode = AP3216C_MODE_ALS_PS_IR;
    light->bus = NULL;
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, light->ops->init(light));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, light->ops->deinit(light));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, light->ops->read(light, &data));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &data, sizeof(data));
    TEST_ASSERT_EQUAL_UINT(0U, g_i2c_read_count);
    TEST_ASSERT_EQUAL_UINT(0U, g_i2c_write_count);

    destroy_sensor(light);
    destroy_sensor(prox);
    destroy_sensor(ir);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_create_variants_set_identity_and_reject_null_names);
    RUN_TEST(test_init_deinit_propagate_config_write_failures);
    RUN_TEST(test_light_proximity_and_ir_reads_convert_raw_values);
    RUN_TEST(test_read_failures_and_overflow_preserve_output);
    RUN_TEST(test_public_ops_reject_null_and_missing_private_data_without_i2c);
    return UNITY_END();
}
