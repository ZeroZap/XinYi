#include "unity.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sensor_ap3216c.h"

typedef struct {
    uint8_t reg;
    uint8_t data[2];
    size_t len;
    xy_error_t result;
} io_step_t;

static io_step_t g_reads[8];
static io_step_t g_writes[8];
static size_t g_read_count;
static size_t g_read_index;
static size_t g_write_count;
static size_t g_write_index;
static uint32_t g_tick;
static uint32_t g_delay_ms;

uint32_t get_tick_ms(void)
{
    return g_tick;
}

void delay_ms(uint32_t ms)
{
    (void)ms;
}

void xy_hal_delay_ms(uint32_t ms)
{
    g_delay_ms += ms;
}

xy_error_t xy_i2c_device_init(xy_i2c_device_t *dev, void *handle, uint16_t address,
                              uint32_t timeout)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_NOT_NULL(handle);
    TEST_ASSERT_EQUAL_UINT16(XY_AP3216C_DEFAULT_ADDRESS, address);
    TEST_ASSERT_EQUAL_UINT32(100U, timeout);
    memset(dev, 0, sizeof(*dev));
    dev->base.initialized = 1U;
    dev->i2c_handle = handle;
    dev->dev_addr = address;
    dev->timeout = timeout;
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_read_reg(xy_i2c_device_t *dev, uint8_t reg, uint8_t *data, size_t len)
{
    io_step_t *step;
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_LESS_THAN_UINT(g_read_count, g_read_index);
    step = &g_reads[g_read_index++];
    TEST_ASSERT_EQUAL_UINT8(step->reg, reg);
    TEST_ASSERT_EQUAL_UINT(step->len, len);
    if (step->result == XY_DEVICE_OK) {
        memcpy(data, step->data, len);
    }
    return step->result;
}

xy_error_t xy_i2c_device_write_reg(xy_i2c_device_t *dev, uint8_t reg, const uint8_t *data,
                                    size_t len)
{
    io_step_t *step;
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_LESS_THAN_UINT(g_write_count, g_write_index);
    step = &g_writes[g_write_index++];
    TEST_ASSERT_EQUAL_UINT8(step->reg, reg);
    TEST_ASSERT_EQUAL_UINT(step->len, len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(step->data, data, len);
    return step->result;
}

static void queue_write(uint8_t value, xy_error_t result)
{
    io_step_t *step = &g_writes[g_write_count++];
    step->reg = XY_AP3216C_REG_SYSTEM_CONFIG;
    step->data[0] = value;
    step->len = 1U;
    step->result = result;
}

static void queue_read(uint8_t reg, const uint8_t *data, xy_error_t result)
{
    io_step_t *step = &g_reads[g_read_count++];
    step->reg = reg;
    step->len = 2U;
    step->result = result;
    if (data != NULL) {
        memcpy(step->data, data, 2U);
    }
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
    g_read_count = g_read_index = 0U;
    g_write_count = g_write_index = 0U;
    g_tick = 424242U;
    g_delay_ms = 0U;
}

void tearDown(void)
{
}

static void test_factories_preserve_legacy_identity(void)
{
    int bus;
    sensor_device_t *light = ap3216c_create_light("light", &bus);
    sensor_device_t *proximity = ap3216c_create_proximity("proximity", &bus);
    sensor_device_t *ir = ap3216c_create_ir("ir", &bus);

    TEST_ASSERT_NULL(ap3216c_create_light(NULL, &bus));
    TEST_ASSERT_NULL(ap3216c_create_light("bad", NULL));
    TEST_ASSERT_NOT_NULL(light);
    TEST_ASSERT_NOT_NULL(proximity);
    TEST_ASSERT_NOT_NULL(ir);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_AMBIENT_LIGHT, light->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_PROXIMITY, proximity->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_IR, ir->info.type);
    TEST_ASSERT_EQUAL_STRING("AP3216C", light->info.model);
    TEST_ASSERT_EQUAL_UINT8(AP3216C_ADDR_DEFAULT,
                            ((ap3216c_priv_t *)light->priv_data)->i2c_addr);

    destroy_sensor(light);
    destroy_sensor(proximity);
    destroy_sensor(ir);
}

static void test_wrapper_delegates_lifecycle_and_error_mapping(void)
{
    int bus;
    sensor_device_t *sensor = ap3216c_create_light("light", &bus);

    queue_write(XY_AP3216C_MODE_RESET, XY_DEVICE_OK);
    queue_write(XY_AP3216C_MODE_ALS_PS, XY_DEVICE_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(SENSOR_ETIMEOUT, sensor->ops->init(sensor));
    TEST_ASSERT_FALSE(((ap3216c_priv_t *)sensor->priv_data)->device.initialized);

    queue_write(XY_AP3216C_MODE_RESET, XY_DEVICE_OK);
    queue_write(XY_AP3216C_MODE_ALS_PS, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_TRUE(((ap3216c_priv_t *)sensor->priv_data)->device.initialized);
    TEST_ASSERT_EQUAL_UINT32(150U, g_delay_ms);

    queue_write(XY_AP3216C_MODE_POWER_DOWN, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->deinit(sensor));
    destroy_sensor(sensor);
}

static void test_wrapper_converts_channels_and_preserves_output_on_failure(void)
{
    static const uint8_t als[] = {0x34U, 0x12U};
    static const uint8_t ps[] = {0x8AU, 0x21U};
    static const uint8_t ir_raw[] = {0xAAU, 0x03U};
    int bus;
    sensor_device_t *light = ap3216c_create_light("light", &bus);
    sensor_device_t *proximity = ap3216c_create_proximity("proximity", &bus);
    sensor_device_t *ir = ap3216c_create_ir("ir", &bus);
    sensor_data_t data = {.type = SENSOR_TYPE_CUSTOM, .value.val_uint32 = 0xA5A5U};
    sensor_data_t snapshot;

    queue_write(XY_AP3216C_MODE_RESET, XY_DEVICE_OK);
    queue_write(XY_AP3216C_MODE_ALS_PS, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, light->ops->init(light));
    queue_write(XY_AP3216C_MODE_RESET, XY_DEVICE_OK);
    queue_write(XY_AP3216C_MODE_ALS_PS, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, proximity->ops->init(proximity));
    queue_write(XY_AP3216C_MODE_RESET, XY_DEVICE_OK);
    queue_write(XY_AP3216C_MODE_ALS_PS, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, ir->ops->init(ir));

    queue_read(XY_AP3216C_REG_ALS_DATA_L, als, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, light->ops->read(light, &data));
    TEST_ASSERT_EQUAL_UINT32(1631U, data.value.val_uint32);
    queue_read(XY_AP3216C_REG_PS_DATA_L, ps, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, proximity->ops->read(proximity, &data));
    TEST_ASSERT_EQUAL_INT32(0x21A, data.value.val_int32);
    queue_read(XY_AP3216C_REG_IR_DATA_L, ir_raw, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, ir->ops->read(ir, &data));
    TEST_ASSERT_EQUAL_UINT32(0x3AAU, data.value.val_uint32);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);

    snapshot = data;
    queue_read(XY_AP3216C_REG_IR_DATA_L, NULL, XY_DEVICE_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(SENSOR_ETIMEOUT, ir->ops->read(ir, &data));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &data, sizeof(data));

    destroy_sensor(light);
    destroy_sensor(proximity);
    destroy_sensor(ir);
}

static void test_wrapper_rejects_missing_context_without_io(void)
{
    int bus;
    sensor_device_t *sensor = ap3216c_create_light("light", &bus);
    sensor_data_t data = {0};

    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->init(NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->read(sensor, NULL));
    sensor->bus = NULL;
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_UINT(0U, g_write_index);
    TEST_ASSERT_EQUAL_UINT(0U, g_read_index);
    destroy_sensor(sensor);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_factories_preserve_legacy_identity);
    RUN_TEST(test_wrapper_delegates_lifecycle_and_error_mapping);
    RUN_TEST(test_wrapper_converts_channels_and_preserves_output_on_failure);
    RUN_TEST(test_wrapper_rejects_missing_context_without_io);
    return UNITY_END();
}