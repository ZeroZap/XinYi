#include "unity.h"
#include "xy_aht30.h"
#include "xy_bme680.h"
#include "xy_l3g4200d.h"
#include "xy_sc7a22h.h"

#include <string.h>

typedef enum { OP_READ, OP_WRITE, OP_READ_REG, OP_WRITE_REG } op_kind_t;
typedef struct {
    op_kind_t kind;
    uint8_t reg;
    uint8_t data[8];
    size_t length;
    xy_error_t result;
} op_t;

static op_t ops[16];
static size_t op_count;
static size_t op_index;
static uint32_t delay_total;
static xy_error_t i2c_init_result;

static void queue(op_kind_t kind, uint8_t reg, const uint8_t *data, size_t length,
                  xy_error_t result)
{
    op_t *op = &ops[op_count++];
    op->kind = kind;
    op->reg = reg;
    op->length = length;
    op->result = result;
    if (data != NULL) {
        memcpy(op->data, data, length);
    }
}

static op_t *next_op(op_kind_t kind, size_t length)
{
    TEST_ASSERT_LESS_THAN_UINT(op_count, op_index);
    op_t *op = &ops[op_index++];
    TEST_ASSERT_EQUAL_INT(kind, op->kind);
    TEST_ASSERT_EQUAL_UINT(length, op->length);
    return op;
}

xy_error_t xy_i2c_device_init(xy_i2c_device_t *dev, void *handle, uint16_t address,
                              uint32_t timeout)
{
    memset(dev, 0, sizeof(*dev));
    dev->i2c_handle = handle;
    dev->dev_addr = address;
    dev->timeout = timeout;
    dev->base.initialized = 1U;
    return i2c_init_result;
}

xy_error_t xy_i2c_device_write(xy_i2c_device_t *dev, const uint8_t *data, size_t length)
{
    (void)dev;
    op_t *op = next_op(OP_WRITE, length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(op->data, data, length);
    return op->result;
}

xy_error_t xy_i2c_device_read(xy_i2c_device_t *dev, uint8_t *data, size_t length)
{
    (void)dev;
    op_t *op = next_op(OP_READ, length);
    if (op->result == XY_DEVICE_OK) {
        memcpy(data, op->data, length);
    }
    return op->result;
}

xy_error_t xy_i2c_device_read_reg(xy_i2c_device_t *dev, uint8_t reg, uint8_t *data,
                                  size_t length)
{
    (void)dev;
    op_t *op = next_op(OP_READ_REG, length);
    TEST_ASSERT_EQUAL_HEX8(op->reg, reg);
    if (op->result == XY_DEVICE_OK) {
        memcpy(data, op->data, length);
    }
    return op->result;
}

xy_error_t xy_i2c_device_write_reg(xy_i2c_device_t *dev, uint8_t reg, const uint8_t *data,
                                   size_t length)
{
    (void)dev;
    op_t *op = next_op(OP_WRITE_REG, length);
    TEST_ASSERT_EQUAL_HEX8(op->reg, reg);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(op->data, data, length);
    return op->result;
}

void xy_hal_delay_ms(uint32_t milliseconds) { delay_total += milliseconds; }

static uint8_t fixture_crc(const uint8_t *data, size_t length)
{
    uint8_t crc = 0xFFU;
    while (length-- != 0U) {
        crc ^= *data++;
        for (uint8_t bit = 0U; bit < 8U; ++bit) {
            crc = (crc & 0x80U) != 0U ? (uint8_t)((crc << 1) ^ 0x31U)
                                      : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

void setUp(void)
{
    memset(ops, 0, sizeof(ops));
    op_count = 0U;
    op_index = 0U;
    delay_total = 0U;
    i2c_init_result = XY_DEVICE_OK;
}
void tearDown(void) {}

static void test_aht30_valid_frame_and_crc_atomicity(void)
{
    int bus;
    xy_aht30_t dev;
    xy_aht30_data_t data = {11, 22};
    const uint8_t trigger[3] = {0xACU, 0x33U, 0x00U};
    uint8_t frame[7] = {0x18U, 0x80U, 0x00U, 0x04U, 0x00U, 0x00U, 0U};
    frame[6] = fixture_crc(frame, 6U);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_aht30_init(&dev, &bus));
    TEST_ASSERT_EQUAL_HEX8(0x38U, dev.i2c_dev.dev_addr);
    queue(OP_WRITE, 0U, trigger, sizeof(trigger), XY_DEVICE_OK);
    queue(OP_READ, 0U, frame, sizeof(frame), XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_aht30_read(&dev, &data));
    TEST_ASSERT_EQUAL_INT32(0, data.temperature_centi_c);
    TEST_ASSERT_EQUAL_UINT32(5000U, data.humidity_centi_pct);
    TEST_ASSERT_EQUAL_UINT32(85U, delay_total);

    xy_aht30_data_t snapshot = data;
    frame[6] ^= 1U;
    queue(OP_WRITE, 0U, trigger, sizeof(trigger), XY_DEVICE_OK);
    queue(OP_READ, 0U, frame, sizeof(frame), XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_HAL_ERROR_CRC, xy_aht30_read(&dev, &data));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &data, sizeof(data));
}

static void test_aht30_init_clears_handle_when_i2c_helper_fails(void)
{
    int bus;
    xy_aht30_t dev;

    memset(&dev, 0xA5, sizeof(dev));
    i2c_init_result = XY_DEVICE_BUSY;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_BUSY, xy_aht30_init(&dev, &bus));
    TEST_ASSERT_EQUAL_UINT8(0U, dev.initialized);
    TEST_ASSERT_EQUAL_UINT8(0U, dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_UINT32(0U, delay_total);
    TEST_ASSERT_EQUAL_UINT(0U, op_index);
}

static void test_aht30_public_ops_reject_invalid_nested_bus_lifecycle(void)
{
    xy_aht30_t dev;
    xy_aht30_data_t output = {11, 22};
    xy_aht30_data_t snapshot = output;

    memset(&dev, 0, sizeof(dev));
    dev.initialized = 1U;
    dev.i2c_dev.base.initialized = 0U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_aht30_read(&dev, &output));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_aht30_deinit(&dev));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &output, sizeof(output));
    TEST_ASSERT_EQUAL_UINT8(1U, dev.initialized);
    TEST_ASSERT_EQUAL_UINT(0U, op_index);
}

static void test_aht30_transport_failures_preserve_output_and_cache(void)
{
    int bus;
    xy_aht30_t dev;
    xy_aht30_data_t output = {11, 22};
    xy_aht30_data_t output_snapshot = output;
    xy_aht30_data_t cache_snapshot;
    const uint8_t trigger[3] = {0xACU, 0x33U, 0x00U};

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_aht30_init(&dev, &bus));
    dev.data = (xy_aht30_data_t){33, 44};
    cache_snapshot = dev.data;
    queue(OP_WRITE, 0U, trigger, sizeof(trigger), XY_DEVICE_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_aht30_read(&dev, &output));
    TEST_ASSERT_EQUAL_MEMORY(&output_snapshot, &output, sizeof(output));
    TEST_ASSERT_EQUAL_MEMORY(&cache_snapshot, &dev.data, sizeof(dev.data));
    TEST_ASSERT_EQUAL_UINT32(5U, delay_total);

    queue(OP_WRITE, 0U, trigger, sizeof(trigger), XY_DEVICE_OK);
    queue(OP_READ, 0U, NULL, 7U, XY_DEVICE_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_aht30_read(&dev, &output));
    TEST_ASSERT_EQUAL_MEMORY(&output_snapshot, &output, sizeof(output));
    TEST_ASSERT_EQUAL_MEMORY(&cache_snapshot, &dev.data, sizeof(dev.data));
    TEST_ASSERT_EQUAL_UINT32(85U, delay_total);
}

static void test_l3g4200d_identity_axis_order_and_range(void)
{
    int bus;
    xy_l3g4200d_t dev;
    xy_l3g4200d_data_t data;
    const uint8_t id = 0xD3U;
    const uint8_t ctrl4 = 0x80U;
    const uint8_t ctrl1 = 0x1FU;
    const uint8_t raw[6] = {0x64U, 0x00U, 0x9CU, 0xFFU, 0xC8U, 0x00U};

    queue(OP_READ_REG, 0x0FU, &id, 1U, XY_DEVICE_OK);
    queue(OP_WRITE_REG, 0x23U, &ctrl4, 1U, XY_DEVICE_OK);
    queue(OP_WRITE_REG, 0x20U, &ctrl1, 1U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_l3g4200d_init(&dev, &bus, 0x69U));
    queue(OP_READ_REG, 0xA8U, raw, sizeof(raw), XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_l3g4200d_read(&dev, &data));
    TEST_ASSERT_EQUAL_INT32(875, data.x_mdps);
    TEST_ASSERT_EQUAL_INT32(-875, data.y_mdps);
    TEST_ASSERT_EQUAL_INT32(1750, data.z_mdps);

    const uint8_t range = 0xA0U;
    queue(OP_WRITE_REG, 0x23U, &range, 1U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_l3g4200d_set_range(&dev, 2000U));
    TEST_ASSERT_EQUAL_UINT16(2000U, dev.range_dps);
}

static void test_l3g4200d_rejects_wrong_identity(void)
{
    int bus;
    xy_l3g4200d_t dev;
    const uint8_t id = 0x00U;
    queue(OP_READ_REG, 0x0FU, &id, 1U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_NOT_FOUND, xy_l3g4200d_init(&dev, &bus, 0x69U));
    TEST_ASSERT_EQUAL_UINT8(0U, dev.initialized);
}

static void test_l3g4200d_init_clears_handle_when_i2c_helper_fails(void)
{
    int bus;
    xy_l3g4200d_t dev;

    memset(&dev, 0xA5, sizeof(dev));
    i2c_init_result = XY_DEVICE_BUSY;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_BUSY, xy_l3g4200d_init(&dev, &bus, 0x69U));
    TEST_ASSERT_EQUAL_UINT8(0U, dev.initialized);
    TEST_ASSERT_EQUAL_UINT8(0U, dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_UINT(0U, op_index);
}

static void prepare_initialized_l3g4200d(xy_l3g4200d_t *dev)
{
    memset(dev, 0, sizeof(*dev));
    dev->initialized = 1U;
    dev->i2c_dev.base.initialized = 1U;
    dev->i2c_dev.i2c_handle = (void *)0x1;
    dev->range_dps = 500U;
    dev->data = (xy_l3g4200d_data_t){111, 222, 333};
}

static void test_l3g4200d_public_ops_reject_invalid_nested_bus_lifecycle(void)
{
    xy_l3g4200d_t dev;
    xy_l3g4200d_data_t output = {11, 22, 33};
    xy_l3g4200d_data_t output_snapshot = output;
    uint8_t ready = 0xA5U;

    prepare_initialized_l3g4200d(&dev);
    dev.i2c_dev.base.initialized = 0U;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_l3g4200d_deinit(&dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_l3g4200d_data_ready(&dev, &ready));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_l3g4200d_set_range(&dev, 2000U));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_l3g4200d_read(&dev, &output));
    TEST_ASSERT_EQUAL_UINT8(1U, dev.initialized);
    TEST_ASSERT_EQUAL_UINT16(500U, dev.range_dps);
    TEST_ASSERT_EQUAL_UINT8(0xA5U, ready);
    TEST_ASSERT_EQUAL_MEMORY(&output_snapshot, &output, sizeof(output));
    TEST_ASSERT_EQUAL_UINT(0U, op_index);
}

static void test_l3g4200d_transport_failures_preserve_state_and_outputs(void)
{
    xy_l3g4200d_t dev;
    xy_l3g4200d_data_t output = {11, 22, 33};
    xy_l3g4200d_data_t output_snapshot = output;
    xy_l3g4200d_data_t cache_snapshot;
    uint8_t ready = 0xA5U;
    const uint8_t power_down = 0x07U;
    const uint8_t range = 0xA0U;

    prepare_initialized_l3g4200d(&dev);
    cache_snapshot = dev.data;
    queue(OP_READ_REG, 0x27U, NULL, 1U, XY_DEVICE_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_l3g4200d_data_ready(&dev, &ready));
    TEST_ASSERT_EQUAL_UINT8(0xA5U, ready);

    queue(OP_READ_REG, 0xA8U, NULL, 6U, XY_DEVICE_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_l3g4200d_read(&dev, &output));
    TEST_ASSERT_EQUAL_MEMORY(&output_snapshot, &output, sizeof(output));
    TEST_ASSERT_EQUAL_MEMORY(&cache_snapshot, &dev.data, sizeof(dev.data));

    queue(OP_WRITE_REG, 0x23U, &range, 1U, XY_DEVICE_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_l3g4200d_set_range(&dev, 2000U));
    TEST_ASSERT_EQUAL_UINT16(500U, dev.range_dps);

    queue(OP_WRITE_REG, 0x20U, &power_down, 1U, XY_DEVICE_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_l3g4200d_deinit(&dev));
    TEST_ASSERT_EQUAL_UINT8(1U, dev.initialized);
    TEST_ASSERT_EQUAL_UINT8(1U, dev.i2c_dev.base.initialized);
    TEST_ASSERT_EQUAL_UINT(4U, op_index);
}

static void test_bme680_rejects_invalid_public_inputs_without_bus_access(void)
{
    int bus;
    xy_bme680_t dev;
    xy_bme680_data_t data = {123, 456U, 789U, 321U, 0xA5U};
    xy_bme680_data_t snapshot = data;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bme680_init(NULL, &bus, 0x77U));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bme680_init(&dev, NULL, 0x77U));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bme680_init(&dev, &bus, 0x68U));

    memset(&dev, 0, sizeof(dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bme680_read(&dev, &data));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &data, sizeof(data));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bme680_read(&dev, NULL));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bme680_deinit(&dev));
    TEST_ASSERT_EQUAL_UINT(0U, op_index);
}

static void test_bme680_init_propagates_bus_failure_and_preserves_no_ready_state(void)
{
    int bus;
    xy_bme680_t dev;
    const uint8_t reset = 0xB6U;
    memset(&dev, 0xA5, sizeof(dev));

    queue(OP_WRITE_REG, 0xE0U, &reset, 1U, XY_DEVICE_OK);
    queue(OP_READ_REG, 0xD0U, NULL, 1U, XY_DEVICE_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_bme680_init(&dev, &bus, 0x77U));
    TEST_ASSERT_EQUAL_UINT8(0U, dev.initialized);
    TEST_ASSERT_EQUAL_UINT8(0U, dev.i2c_dev.base.initialized);
    TEST_ASSERT_EQUAL_UINT(2U, op_index);
}

static void test_bme680_init_clears_handle_when_i2c_helper_fails(void)
{
    int bus;
    xy_bme680_t dev;

    memset(&dev, 0xA5, sizeof(dev));
    i2c_init_result = XY_DEVICE_BUSY;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_BUSY, xy_bme680_init(&dev, &bus, 0x77U));
    TEST_ASSERT_EQUAL_UINT8(0U, dev.initialized);
    TEST_ASSERT_EQUAL_UINT8(0U, dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_UINT(0U, op_index);
}

static void test_bme680_deinit_rejects_invalid_nested_bus_lifecycle(void)
{
    int bus;
    xy_bme680_t dev;

    memset(&dev, 0, sizeof(dev));
    dev.initialized = 1U;
    dev.i2c_dev.base.initialized = 0U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bme680_deinit(&dev));
    TEST_ASSERT_EQUAL_UINT8(1U, dev.initialized);
    TEST_ASSERT_EQUAL_UINT(0U, op_index);
    (void)bus;
}

static void test_bme680_read_rejects_invalid_nested_bus_lifecycle(void)
{
    xy_bme680_t dev;
    xy_bme680_data_t data = {123, 456U, 789U, 321U, 0xA5U};
    xy_bme680_data_t snapshot = data;

    memset(&dev, 0, sizeof(dev));
    dev.initialized = 1U;
    dev.i2c_dev.base.initialized = 0U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bme680_read(&dev, &data));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &data, sizeof(data));
    TEST_ASSERT_EQUAL_UINT(0U, op_index);
}

static BME68X_INTF_RET_TYPE bme680_transport_failure_read(uint8_t reg, uint8_t *data,
                                                          uint32_t length, void *context)
{
    xy_bme680_t *dev = context;

    (void)reg;
    (void)data;
    (void)length;
    dev->transport_error = XY_DEVICE_TIMEOUT;
    return BME68X_E_COM_FAIL;
}

static BME68X_INTF_RET_TYPE bme680_unexpected_write(uint8_t reg, const uint8_t *data,
                                                    uint32_t length, void *context)
{
    (void)reg;
    (void)data;
    (void)length;
    (void)context;
    TEST_FAIL_MESSAGE("BME680 must stop after the first transport failure");
    return BME68X_E_COM_FAIL;
}

static void bme680_test_delay(uint32_t us, void *context)
{
    (void)context;
    delay_total += (us + 999U) / 1000U;
}

static void prepare_initialized_bme680(xy_bme680_t *dev)
{
    memset(dev, 0, sizeof(*dev));
    dev->initialized = 1U;
    dev->i2c_dev.base.initialized = 1U;
    dev->bosch.intf = BME68X_I2C_INTF;
    dev->bosch.intf_ptr = dev;
    dev->bosch.read = bme680_transport_failure_read;
    dev->bosch.write = bme680_unexpected_write;
    dev->bosch.delay_us = bme680_test_delay;
}

static void test_bme680_deinit_preserves_lifecycle_on_transport_failure(void)
{
    xy_bme680_t dev;

    prepare_initialized_bme680(&dev);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_bme680_deinit(&dev));
    TEST_ASSERT_EQUAL_UINT8(1U, dev.initialized);
    TEST_ASSERT_EQUAL_UINT8(1U, dev.i2c_dev.base.initialized);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, dev.transport_error);
    TEST_ASSERT_EQUAL_UINT(0U, op_index);
}

static void test_bme680_forced_read_preserves_data_on_transport_failure(void)
{
    xy_bme680_t dev;
    xy_bme680_data_t output = {123, 456U, 789U, 321U, 0xA5U};
    xy_bme680_data_t output_snapshot = output;
    xy_bme680_data_t cache_snapshot = {321, 654U, 987U, 123U, 0x5AU};

    prepare_initialized_bme680(&dev);
    dev.data = cache_snapshot;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_bme680_read(&dev, &output));
    TEST_ASSERT_EQUAL_MEMORY(&output_snapshot, &output, sizeof(output));
    TEST_ASSERT_EQUAL_MEMORY(&cache_snapshot, &dev.data, sizeof(dev.data));
    TEST_ASSERT_EQUAL_UINT8(1U, dev.initialized);
    TEST_ASSERT_EQUAL_UINT8(1U, dev.i2c_dev.base.initialized);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, dev.transport_error);
    TEST_ASSERT_EQUAL_UINT32(0U, delay_total);
    TEST_ASSERT_EQUAL_UINT(0U, op_index);
}

static BME68X_INTF_RET_TYPE bme680_generic_failure_read(uint8_t reg, uint8_t *data,
                                                        uint32_t length, void *context)
{
    (void)reg;
    (void)data;
    (void)length;
    (void)context;
    return BME68X_E_COM_FAIL;
}

static void test_bme680_stale_transport_error_is_not_reused(void)
{
    xy_bme680_t dev;
    xy_bme680_data_t output = {123, 456U, 789U, 321U, 0xA5U};
    xy_bme680_data_t snapshot = output;

    prepare_initialized_bme680(&dev);
    dev.transport_error = XY_DEVICE_TIMEOUT;
    dev.bosch.read = bme680_generic_failure_read;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR, xy_bme680_read(&dev, &output));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &output, sizeof(output));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, dev.transport_error);
    TEST_ASSERT_EQUAL_UINT32(0U, delay_total);
}

static void test_sc7a22h_init_config_and_accel_conversion(void)
{
    int bus;
    xy_sc7a22h_t dev;
    xy_sc7a22h_data_t raw;
    xy_sc7a22h_accel_t accel;
    const uint8_t id = XY_SC7A22H_WHO_AM_I_VALUE;
    const uint8_t com_cfg = XY_SC7A22H_DEMO_COM_CFG;
    const uint8_t acc_conf = XY_SC7A22H_DEMO_ACC_CONF;
    const uint8_t acc_range = XY_SC7A22H_DEMO_ACC_RANGE;
    const uint8_t int_cfg1 = XY_SC7A22H_DEMO_INT_CFG1;
    const uint8_t filter_cfg = XY_SC7A22H_DEMO_FILTER_CFG;
    const uint8_t raw_bytes[6] = {0x10U, 0x00U, 0xF0U, 0x00U, 0x08U, 0x00U};

    queue(OP_READ_REG, XY_SC7A22H_REG_WHO_AM_I, &id, 1U, XY_DEVICE_OK);
    queue(OP_WRITE_REG, XY_SC7A22H_REG_PWR_CTRL, (uint8_t[]){XY_SC7A22H_ACC_ENABLE}, 1U,
          XY_DEVICE_OK);
    queue(OP_WRITE_REG, XY_SC7A22H_REG_ACC_CONF, &acc_conf, 1U, XY_DEVICE_OK);
    queue(OP_WRITE_REG, XY_SC7A22H_REG_ACC_RANGE, &acc_range, 1U, XY_DEVICE_OK);
    queue(OP_WRITE_REG, XY_SC7A22H_REG_COM_CFG, &com_cfg, 1U, XY_DEVICE_OK);
    queue(OP_WRITE_REG, XY_SC7A22H_REG_INT_CFG1, &int_cfg1, 1U, XY_DEVICE_OK);
    queue(OP_WRITE_REG, XY_SC7A22H_REG_HPF_LPF_CFG, &filter_cfg, 1U, XY_DEVICE_OK);
    queue(OP_READ_REG, XY_SC7A22H_REG_COM_CFG, &com_cfg, 1U, XY_DEVICE_OK);
    queue(OP_READ_REG, XY_SC7A22H_REG_ACC_CONF, &acc_conf, 1U, XY_DEVICE_OK);
    queue(OP_READ_REG, XY_SC7A22H_REG_ACC_RANGE, &acc_range, 1U, XY_DEVICE_OK);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sc7a22h_init(&dev, &bus));
    TEST_ASSERT_EQUAL_UINT8(1U, dev.initialized);
    TEST_ASSERT_EQUAL_UINT32(12U, delay_total);
    TEST_ASSERT_EQUAL_UINT8(acc_range, dev.acc_range);

    queue(OP_READ_REG, XY_SC7A22H_REG_OUT_X_H, raw_bytes, sizeof(raw_bytes), XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sc7a22h_read(&dev, &raw));
    TEST_ASSERT_EQUAL_INT16(0x1000, raw.x);
    TEST_ASSERT_EQUAL_INT16((int16_t)0xF000, raw.y);
    TEST_ASSERT_EQUAL_INT16(0x0800, raw.z);

    queue(OP_READ_REG, XY_SC7A22H_REG_OUT_X_H, raw_bytes, sizeof(raw_bytes), XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sc7a22h_read_accel(&dev, &accel));
    TEST_ASSERT_EQUAL_INT32(499, accel.x_mg);
    TEST_ASSERT_EQUAL_INT32(-499, accel.y_mg);
    TEST_ASSERT_EQUAL_INT32(249, accel.z_mg);
}

static void test_sc7a22h_rejects_bad_identity_and_invalid_state(void)
{
    int bus;
    xy_sc7a22h_t dev;
    xy_sc7a22h_data_t data = {11, 22, 33};
    const uint8_t bad_id = 0x00U;

    queue(OP_READ_REG, XY_SC7A22H_REG_WHO_AM_I, &bad_id, 1U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_NOT_FOUND, xy_sc7a22h_init(&dev, &bus));
    TEST_ASSERT_EQUAL_UINT8(0U, dev.initialized);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sc7a22h_read(NULL, &data));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sc7a22h_read(&dev, &data));
    TEST_ASSERT_EQUAL_UINT(1U, op_index);
}

static void test_sc7a22h_init_clears_handle_when_i2c_helper_fails(void)
{
    int bus;
    xy_sc7a22h_t dev;

    memset(&dev, 0xA5, sizeof(dev));
    i2c_init_result = XY_DEVICE_BUSY;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_BUSY, xy_sc7a22h_init(&dev, &bus));
    TEST_ASSERT_EQUAL_UINT8(0U, dev.initialized);
    TEST_ASSERT_EQUAL_UINT8(0U, dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_UINT(0U, op_index);
}

static void test_sc7a22h_public_config_status_and_deinit_contracts(void)
{
    int bus;
    xy_sc7a22h_t dev;
    uint8_t status = 0xA5U;
    const uint8_t id = XY_SC7A22H_WHO_AM_I_VALUE;
    const uint8_t com_cfg = XY_SC7A22H_DEMO_COM_CFG;
    const uint8_t acc_conf = XY_SC7A22H_DEMO_ACC_CONF;
    const uint8_t acc_range = XY_SC7A22H_DEMO_ACC_RANGE;
    const uint8_t int_cfg1 = XY_SC7A22H_DEMO_INT_CFG1;
    const uint8_t filter_cfg = XY_SC7A22H_DEMO_FILTER_CFG;
    const uint8_t status_value = 0x03U;
    const uint8_t new_acc_conf = 0xB0U;
    const uint8_t new_acc_range = 0x02U;

    queue(OP_READ_REG, XY_SC7A22H_REG_WHO_AM_I, &id, 1U, XY_DEVICE_OK);
    queue(OP_WRITE_REG, XY_SC7A22H_REG_PWR_CTRL, (uint8_t[]){XY_SC7A22H_ACC_ENABLE}, 1U,
          XY_DEVICE_OK);
    queue(OP_WRITE_REG, XY_SC7A22H_REG_ACC_CONF, &acc_conf, 1U, XY_DEVICE_OK);
    queue(OP_WRITE_REG, XY_SC7A22H_REG_ACC_RANGE, &acc_range, 1U, XY_DEVICE_OK);
    queue(OP_WRITE_REG, XY_SC7A22H_REG_COM_CFG, &com_cfg, 1U, XY_DEVICE_OK);
    queue(OP_WRITE_REG, XY_SC7A22H_REG_INT_CFG1, &int_cfg1, 1U, XY_DEVICE_OK);
    queue(OP_WRITE_REG, XY_SC7A22H_REG_HPF_LPF_CFG, &filter_cfg, 1U, XY_DEVICE_OK);
    queue(OP_READ_REG, XY_SC7A22H_REG_COM_CFG, &com_cfg, 1U, XY_DEVICE_OK);
    queue(OP_READ_REG, XY_SC7A22H_REG_ACC_CONF, &acc_conf, 1U, XY_DEVICE_OK);
    queue(OP_READ_REG, XY_SC7A22H_REG_ACC_RANGE, &acc_range, 1U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sc7a22h_init(&dev, &bus));

    queue(OP_READ_REG, XY_SC7A22H_REG_DATA_STAT, &status_value, 1U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sc7a22h_read_status(&dev, &status));
    TEST_ASSERT_EQUAL_UINT8(status_value, status);
    TEST_ASSERT_EQUAL_UINT8(status_value, dev.data_status);

    queue(OP_READ_REG, XY_SC7A22H_REG_DATA_STAT, NULL, 1U, XY_DEVICE_IO_ERROR);
    status = 0xA5U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR, xy_sc7a22h_read_status(&dev, &status));
    TEST_ASSERT_EQUAL_UINT8(0xA5U, status);
    TEST_ASSERT_EQUAL_UINT8(status_value, dev.data_status);

    queue(OP_WRITE_REG, XY_SC7A22H_REG_ACC_CONF, &new_acc_conf, 1U, XY_DEVICE_IO_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR, xy_sc7a22h_set_acc_config(&dev, new_acc_conf));
    TEST_ASSERT_EQUAL_UINT8(acc_conf, dev.acc_conf);

    queue(OP_WRITE_REG, XY_SC7A22H_REG_ACC_RANGE, &new_acc_range, 1U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sc7a22h_set_acc_range(&dev, new_acc_range));
    TEST_ASSERT_EQUAL_UINT8(new_acc_range, dev.acc_range);

    queue(OP_WRITE_REG, XY_SC7A22H_REG_PWR_CTRL,
          (uint8_t[]){XY_SC7A22H_ACC_DISABLE}, 1U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sc7a22h_deinit(&dev));
    TEST_ASSERT_EQUAL_UINT8(0U, dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sc7a22h_read_status(&dev, &status));
    TEST_ASSERT_EQUAL_UINT(15U, op_index);
}

static void test_sc7a22h_init_stops_on_configuration_failure(void)
{
    int bus;
    xy_sc7a22h_t dev;
    const uint8_t id = XY_SC7A22H_WHO_AM_I_VALUE;
    const uint8_t acc_conf = XY_SC7A22H_DEMO_ACC_CONF;
    const uint8_t acc_range = XY_SC7A22H_DEMO_ACC_RANGE;

    queue(OP_READ_REG, XY_SC7A22H_REG_WHO_AM_I, &id, 1U, XY_DEVICE_OK);
    queue(OP_WRITE_REG, XY_SC7A22H_REG_PWR_CTRL, (uint8_t[]){XY_SC7A22H_ACC_ENABLE}, 1U,
          XY_DEVICE_OK);
    queue(OP_WRITE_REG, XY_SC7A22H_REG_ACC_CONF, &acc_conf, 1U, XY_DEVICE_OK);
    queue(OP_WRITE_REG, XY_SC7A22H_REG_ACC_RANGE, &acc_range, 1U, XY_DEVICE_IO_ERROR);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR, xy_sc7a22h_init(&dev, &bus));
    TEST_ASSERT_EQUAL_UINT8(0U, dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_EQUAL_UINT(4U, op_index);
    TEST_ASSERT_EQUAL_UINT32(10U, delay_total);
}

static void test_sc7a22h_read_preserves_output_on_transport_failure(void)
{
    int bus;
    xy_sc7a22h_t dev;
    xy_sc7a22h_data_t output = {101, 202, 303};
    xy_sc7a22h_data_t snapshot = output;
    const uint8_t id = XY_SC7A22H_WHO_AM_I_VALUE;
    const uint8_t com_cfg = XY_SC7A22H_DEMO_COM_CFG;
    const uint8_t acc_conf = XY_SC7A22H_DEMO_ACC_CONF;
    const uint8_t acc_range = XY_SC7A22H_DEMO_ACC_RANGE;
    const uint8_t int_cfg1 = XY_SC7A22H_DEMO_INT_CFG1;
    const uint8_t filter_cfg = XY_SC7A22H_DEMO_FILTER_CFG;

    queue(OP_READ_REG, XY_SC7A22H_REG_WHO_AM_I, &id, 1U, XY_DEVICE_OK);
    queue(OP_WRITE_REG, XY_SC7A22H_REG_PWR_CTRL, (uint8_t[]){XY_SC7A22H_ACC_ENABLE}, 1U,
          XY_DEVICE_OK);
    queue(OP_WRITE_REG, XY_SC7A22H_REG_ACC_CONF, &acc_conf, 1U, XY_DEVICE_OK);
    queue(OP_WRITE_REG, XY_SC7A22H_REG_ACC_RANGE, &acc_range, 1U, XY_DEVICE_OK);
    queue(OP_WRITE_REG, XY_SC7A22H_REG_COM_CFG, &com_cfg, 1U, XY_DEVICE_OK);
    queue(OP_WRITE_REG, XY_SC7A22H_REG_INT_CFG1, &int_cfg1, 1U, XY_DEVICE_OK);
    queue(OP_WRITE_REG, XY_SC7A22H_REG_HPF_LPF_CFG, &filter_cfg, 1U, XY_DEVICE_OK);
    queue(OP_READ_REG, XY_SC7A22H_REG_COM_CFG, &com_cfg, 1U, XY_DEVICE_OK);
    queue(OP_READ_REG, XY_SC7A22H_REG_ACC_CONF, &acc_conf, 1U, XY_DEVICE_OK);
    queue(OP_READ_REG, XY_SC7A22H_REG_ACC_RANGE, &acc_range, 1U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sc7a22h_init(&dev, &bus));

    queue(OP_READ_REG, XY_SC7A22H_REG_OUT_X_H, NULL, 6U, XY_DEVICE_IO_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR, xy_sc7a22h_read(&dev, &output));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &output, sizeof(output));
    TEST_ASSERT_EQUAL_INT16(0, dev.data.x);
    TEST_ASSERT_EQUAL_INT16(0, dev.data.y);
    TEST_ASSERT_EQUAL_INT16(0, dev.data.z);
}

static void test_sc7a22h_rejects_invalid_range_without_bus_access(void)
{
    int bus;
    xy_sc7a22h_t dev;
    const uint8_t id = XY_SC7A22H_WHO_AM_I_VALUE;
    const uint8_t com_cfg = XY_SC7A22H_DEMO_COM_CFG;
    const uint8_t acc_conf = XY_SC7A22H_DEMO_ACC_CONF;
    const uint8_t acc_range = XY_SC7A22H_DEMO_ACC_RANGE;
    const uint8_t int_cfg1 = XY_SC7A22H_DEMO_INT_CFG1;
    const uint8_t filter_cfg = XY_SC7A22H_DEMO_FILTER_CFG;

    queue(OP_READ_REG, XY_SC7A22H_REG_WHO_AM_I, &id, 1U, XY_DEVICE_OK);
    queue(OP_WRITE_REG, XY_SC7A22H_REG_PWR_CTRL, (uint8_t[]){XY_SC7A22H_ACC_ENABLE}, 1U,
          XY_DEVICE_OK);
    queue(OP_WRITE_REG, XY_SC7A22H_REG_ACC_CONF, &acc_conf, 1U, XY_DEVICE_OK);
    queue(OP_WRITE_REG, XY_SC7A22H_REG_ACC_RANGE, &acc_range, 1U, XY_DEVICE_OK);
    queue(OP_WRITE_REG, XY_SC7A22H_REG_COM_CFG, &com_cfg, 1U, XY_DEVICE_OK);
    queue(OP_WRITE_REG, XY_SC7A22H_REG_INT_CFG1, &int_cfg1, 1U, XY_DEVICE_OK);
    queue(OP_WRITE_REG, XY_SC7A22H_REG_HPF_LPF_CFG, &filter_cfg, 1U, XY_DEVICE_OK);
    queue(OP_READ_REG, XY_SC7A22H_REG_COM_CFG, &com_cfg, 1U, XY_DEVICE_OK);
    queue(OP_READ_REG, XY_SC7A22H_REG_ACC_CONF, &acc_conf, 1U, XY_DEVICE_OK);
    queue(OP_READ_REG, XY_SC7A22H_REG_ACC_RANGE, &acc_range, 1U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sc7a22h_init(&dev, &bus));

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sc7a22h_set_acc_range(&dev, 0x04U));
    TEST_ASSERT_EQUAL_UINT8(acc_range, dev.acc_range);
    TEST_ASSERT_EQUAL_UINT(10U, op_index);
}

static void test_sc7a22h_read_config_is_atomic_on_transport_failure(void)
{
    int bus;
    xy_sc7a22h_t dev;
    const uint8_t id = XY_SC7A22H_WHO_AM_I_VALUE;
    const uint8_t com_cfg = XY_SC7A22H_DEMO_COM_CFG;
    const uint8_t acc_conf = XY_SC7A22H_DEMO_ACC_CONF;
    const uint8_t acc_range = XY_SC7A22H_DEMO_ACC_RANGE;
    const uint8_t int_cfg1 = XY_SC7A22H_DEMO_INT_CFG1;
    const uint8_t filter_cfg = XY_SC7A22H_DEMO_FILTER_CFG;
    const uint8_t new_com_cfg = 0xA0U;
    const uint8_t new_acc_conf = 0xB0U;

    queue(OP_READ_REG, XY_SC7A22H_REG_WHO_AM_I, &id, 1U, XY_DEVICE_OK);
    queue(OP_WRITE_REG, XY_SC7A22H_REG_PWR_CTRL, (uint8_t[]){XY_SC7A22H_ACC_ENABLE}, 1U,
          XY_DEVICE_OK);
    queue(OP_WRITE_REG, XY_SC7A22H_REG_ACC_CONF, &acc_conf, 1U, XY_DEVICE_OK);
    queue(OP_WRITE_REG, XY_SC7A22H_REG_ACC_RANGE, &acc_range, 1U, XY_DEVICE_OK);
    queue(OP_WRITE_REG, XY_SC7A22H_REG_COM_CFG, &com_cfg, 1U, XY_DEVICE_OK);
    queue(OP_WRITE_REG, XY_SC7A22H_REG_INT_CFG1, &int_cfg1, 1U, XY_DEVICE_OK);
    queue(OP_WRITE_REG, XY_SC7A22H_REG_HPF_LPF_CFG, &filter_cfg, 1U, XY_DEVICE_OK);
    queue(OP_READ_REG, XY_SC7A22H_REG_COM_CFG, &com_cfg, 1U, XY_DEVICE_OK);
    queue(OP_READ_REG, XY_SC7A22H_REG_ACC_CONF, &acc_conf, 1U, XY_DEVICE_OK);
    queue(OP_READ_REG, XY_SC7A22H_REG_ACC_RANGE, &acc_range, 1U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sc7a22h_init(&dev, &bus));

    queue(OP_READ_REG, XY_SC7A22H_REG_COM_CFG, &new_com_cfg, 1U, XY_DEVICE_OK);
    queue(OP_READ_REG, XY_SC7A22H_REG_ACC_CONF, &new_acc_conf, 1U, XY_DEVICE_IO_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR, xy_sc7a22h_read_config(&dev));
    TEST_ASSERT_EQUAL_UINT8(com_cfg, dev.com_cfg);
    TEST_ASSERT_EQUAL_UINT8(acc_conf, dev.acc_conf);
    TEST_ASSERT_EQUAL_UINT8(acc_range, dev.acc_range);
    TEST_ASSERT_EQUAL_UINT(12U, op_index);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_aht30_valid_frame_and_crc_atomicity);
    RUN_TEST(test_aht30_init_clears_handle_when_i2c_helper_fails);
    RUN_TEST(test_aht30_public_ops_reject_invalid_nested_bus_lifecycle);
    RUN_TEST(test_aht30_transport_failures_preserve_output_and_cache);
    RUN_TEST(test_l3g4200d_identity_axis_order_and_range);
    RUN_TEST(test_l3g4200d_rejects_wrong_identity);
    RUN_TEST(test_l3g4200d_init_clears_handle_when_i2c_helper_fails);
    RUN_TEST(test_l3g4200d_public_ops_reject_invalid_nested_bus_lifecycle);
    RUN_TEST(test_l3g4200d_transport_failures_preserve_state_and_outputs);
    RUN_TEST(test_bme680_rejects_invalid_public_inputs_without_bus_access);
    RUN_TEST(test_bme680_init_propagates_bus_failure_and_preserves_no_ready_state);
    RUN_TEST(test_bme680_init_clears_handle_when_i2c_helper_fails);
    RUN_TEST(test_bme680_deinit_rejects_invalid_nested_bus_lifecycle);
    RUN_TEST(test_bme680_read_rejects_invalid_nested_bus_lifecycle);
    RUN_TEST(test_bme680_deinit_preserves_lifecycle_on_transport_failure);
    RUN_TEST(test_bme680_forced_read_preserves_data_on_transport_failure);
    RUN_TEST(test_bme680_stale_transport_error_is_not_reused);
    RUN_TEST(test_sc7a22h_init_config_and_accel_conversion);
    RUN_TEST(test_sc7a22h_rejects_bad_identity_and_invalid_state);
    RUN_TEST(test_sc7a22h_init_clears_handle_when_i2c_helper_fails);
    RUN_TEST(test_sc7a22h_public_config_status_and_deinit_contracts);
    RUN_TEST(test_sc7a22h_init_stops_on_configuration_failure);
    RUN_TEST(test_sc7a22h_read_preserves_output_on_transport_failure);
    RUN_TEST(test_sc7a22h_rejects_invalid_range_without_bus_access);
    RUN_TEST(test_sc7a22h_read_config_is_atomic_on_transport_failure);
    return UNITY_END();
}
