#include "unity.h"
#include "sensor_qma6100p.h"

#include <stdlib.h>
#include <string.h>

static uint8_t regs[256];
static uint32_t tick;
static xy_error_t read_result;
static xy_error_t write_result;
static uint8_t fail_write_reg;
static size_t read_count;

xy_error_t xy_i2c_device_init(xy_i2c_device_t *dev, void *handle, uint16_t address,
                              uint32_t timeout)
{
    memset(dev, 0, sizeof(*dev));
    dev->base.initialized = 1U;
    dev->i2c_handle = handle;
    dev->dev_addr = address;
    dev->timeout = timeout;
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_read_reg(xy_i2c_device_t *dev, uint8_t reg, uint8_t *data,
                                  size_t length)
{
    TEST_ASSERT_NOT_NULL(dev);
    read_count++;
    if (read_result != XY_DEVICE_OK) return read_result;
    memcpy(data, &regs[reg], length);
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_write_reg(xy_i2c_device_t *dev, uint8_t reg,
                                   const uint8_t *data, size_t length)
{
    TEST_ASSERT_NOT_NULL(dev);
    if (write_result != XY_DEVICE_OK && reg == fail_write_reg) return write_result;
    memcpy(&regs[reg], data, length);
    return XY_DEVICE_OK;
}

void xy_hal_delay_ms(uint32_t ms) { tick += ms; }
uint32_t get_tick_ms(void) { return tick; }

void setUp(void)
{
    memset(regs, 0, sizeof(regs));
    regs[XY_QMA6100P_REG_CHIP_ID] = XY_QMA6100P_CHIP_ID;
    tick = 500U;
    read_result = XY_DEVICE_OK;
    write_result = XY_DEVICE_OK;
    fail_write_reg = 0U;
    read_count = 0U;
}
void tearDown(void) {}

static void destroy(sensor_device_t *sensor)
{
    if (sensor != NULL) {
        SENSOR_FREE(sensor->priv_data);
        SENSOR_FREE(sensor);
    }
}

static void test_factory_exposes_accelerometer_contract(void)
{
    int bus;
    sensor_device_t *sensor = qma6100p_create_accel("qma", &bus, XY_QMA6100P_ADDR_LOW);

    TEST_ASSERT_NULL(qma6100p_create_accel(NULL, &bus, XY_QMA6100P_ADDR_LOW));
    TEST_ASSERT_NULL(qma6100p_create_accel("qma", NULL, XY_QMA6100P_ADDR_LOW));
    TEST_ASSERT_NULL(qma6100p_create_accel("qma", &bus, 0x14U));
    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_STRING("QST", sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING("QMA6100P", sensor->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_ACCELEROMETER, sensor->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_MILLI_G, sensor->info.unit);
    TEST_ASSERT_EQUAL_INT32(-2000, sensor->info.range_min);
    TEST_ASSERT_EQUAL_INT32(2000, sensor->info.range_max);
    TEST_ASSERT_EQUAL_UINT8(14U, sensor->info.resolution);
    TEST_ASSERT_EQUAL_PTR(&bus, sensor->bus);
    destroy(sensor);
}

static void test_wrapper_delegates_lifecycle_and_sample(void)
{
    int bus;
    sensor_data_t data = {0};
    sensor_device_t *sensor = qma6100p_create_accel("qma", &bus, XY_QMA6100P_ADDR_LOW);

    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    regs[1] = 0x00U; regs[2] = 0x10U;
    regs[3] = 0x00U; regs[4] = 0xF0U;
    regs[5] = 0x00U; regs[6] = 0x40U;
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_ACCELEROMETER, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_MILLI_G, data.unit);
    TEST_ASSERT_EQUAL_INT32(250, data.value.val_3axis.x);
    TEST_ASSERT_EQUAL_INT32(-250, data.value.val_3axis.y);
    TEST_ASSERT_EQUAL_INT32(1000, data.value.val_3axis.z);
    TEST_ASSERT_EQUAL_UINT32(tick, data.timestamp);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->deinit(sensor));
    destroy(sensor);
}

static void test_wrapper_preserves_output_on_read_timeout(void)
{
    int bus;
    sensor_data_t data;
    sensor_data_t before;
    sensor_device_t *sensor = qma6100p_create_accel("qma", &bus, XY_QMA6100P_ADDR_LOW);

    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    memset(&data, 0xA5, sizeof(data));
    before = data;
    read_result = XY_DEVICE_TIMEOUT;
    read_count = 0U;

    TEST_ASSERT_EQUAL_INT(SENSOR_ETIMEOUT, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_MEMORY(&before, &data, sizeof(data));
    TEST_ASSERT_EQUAL_UINT(2U, read_count);
    destroy(sensor);
}

static void test_wrapper_rejects_missing_bus_before_deinit(void)
{
    int bus;
    sensor_device_t *sensor = qma6100p_create_accel("qma", &bus, XY_QMA6100P_ADDR_LOW);

    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    sensor->bus = NULL;
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->deinit(sensor));
    TEST_ASSERT_TRUE(((qma6100p_priv_t *)sensor->priv_data)->device.initialized);
    destroy(sensor);
}

static void test_wrapper_range_updates_metadata_and_sample_scaling(void)
{
    int bus;
    sensor_data_t data = {0};
    sensor_device_t *sensor = qma6100p_create_accel("qma", &bus, XY_QMA6100P_ADDR_LOW);

    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    regs[1] = 0x00U;
    regs[2] = 0x10U;

    TEST_ASSERT_EQUAL_INT(SENSOR_EOK,
                          qma6100p_set_range(sensor, XY_QMA6100P_RANGE_8G));
    TEST_ASSERT_EQUAL_INT32(-8000, sensor->info.range_min);
    TEST_ASSERT_EQUAL_INT32(8000, sensor->info.range_max);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT32(1000, data.value.val_3axis.x);
    destroy(sensor);
}

static void test_wrapper_range_rejection_preserves_metadata(void)
{
    int bus;
    sensor_device_t *sensor = qma6100p_create_accel("qma", &bus, XY_QMA6100P_ADDR_LOW);

    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, qma6100p_set_range(sensor, 0x03U));
    TEST_ASSERT_EQUAL_INT32(-2000, sensor->info.range_min);
    TEST_ASSERT_EQUAL_INT32(2000, sensor->info.range_max);
    write_result = XY_DEVICE_TIMEOUT;
    fail_write_reg = XY_QMA6100P_REG_RANGE;
    TEST_ASSERT_EQUAL_INT(SENSOR_ETIMEOUT,
                          qma6100p_set_range(sensor, XY_QMA6100P_RANGE_8G));
    TEST_ASSERT_EQUAL_INT32(-2000, sensor->info.range_min);
    TEST_ASSERT_EQUAL_INT32(2000, sensor->info.range_max);
    TEST_ASSERT_EQUAL_HEX8(XY_QMA6100P_RANGE_2G,
                           ((qma6100p_priv_t *)sensor->priv_data)->device.range);
    destroy(sensor);
}

static void test_wrapper_enable_controls_power_and_status(void)
{
    int bus;
    sensor_data_t data = {0};
    sensor_device_t *sensor = qma6100p_create_accel("qma", &bus, XY_QMA6100P_ADDR_LOW);

    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_NOT_NULL(sensor->ops->enable);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->enable(sensor, false));
    TEST_ASSERT_EQUAL_INT(SENSOR_STATUS_IDLE, sensor->status);
    TEST_ASSERT_EQUAL_HEX8(0U, regs[XY_QMA6100P_REG_POWER]);
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->enable(sensor, true));
    TEST_ASSERT_EQUAL_INT(SENSOR_STATUS_READY, sensor->status);
    TEST_ASSERT_EQUAL_HEX8(XY_QMA6100P_POWER_ACTIVE, regs[XY_QMA6100P_REG_POWER]);

    write_result = XY_DEVICE_TIMEOUT;
    fail_write_reg = XY_QMA6100P_REG_POWER;
    TEST_ASSERT_EQUAL_INT(SENSOR_ETIMEOUT, sensor->ops->enable(sensor, false));
    TEST_ASSERT_EQUAL_INT(SENSOR_STATUS_READY, sensor->status);
    destroy(sensor);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_factory_exposes_accelerometer_contract);
    RUN_TEST(test_wrapper_delegates_lifecycle_and_sample);
    RUN_TEST(test_wrapper_preserves_output_on_read_timeout);
    RUN_TEST(test_wrapper_rejects_missing_bus_before_deinit);
    RUN_TEST(test_wrapper_range_updates_metadata_and_sample_scaling);
    RUN_TEST(test_wrapper_range_rejection_preserves_metadata);
    RUN_TEST(test_wrapper_enable_controls_power_and_status);
    return UNITY_END();
}
