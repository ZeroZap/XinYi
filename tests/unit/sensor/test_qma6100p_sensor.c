#include "unity.h"
#include "sensor_qma6100p.h"

#include <stdlib.h>
#include <string.h>

static uint8_t regs[256];
static uint32_t tick;

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
    memcpy(data, &regs[reg], length);
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_write_reg(xy_i2c_device_t *dev, uint8_t reg,
                                   const uint8_t *data, size_t length)
{
    TEST_ASSERT_NOT_NULL(dev);
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

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_factory_exposes_accelerometer_contract);
    RUN_TEST(test_wrapper_delegates_lifecycle_and_sample);
    return UNITY_END();
}
