#include "unity.h"
#include "xy_aht30.h"
#include "xy_bme680.h"
#include "xy_l3g4200d.h"

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
    return XY_DEVICE_OK;
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
    queue(OP_READ_REG, 0xD0U, NULL, 1U, XY_DEVICE_IO_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR, xy_bme680_init(&dev, &bus, 0x77U));
    TEST_ASSERT_EQUAL_UINT8(0U, dev.initialized);
    TEST_ASSERT_EQUAL_UINT8(0U, dev.i2c_dev.base.initialized);
    TEST_ASSERT_EQUAL_UINT(2U, op_index);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_aht30_valid_frame_and_crc_atomicity);
    RUN_TEST(test_l3g4200d_identity_axis_order_and_range);
    RUN_TEST(test_l3g4200d_rejects_wrong_identity);
    RUN_TEST(test_bme680_rejects_invalid_public_inputs_without_bus_access);
    RUN_TEST(test_bme680_init_propagates_bus_failure_and_preserves_no_ready_state);
    return UNITY_END();
}
