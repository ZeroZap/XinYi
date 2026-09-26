#include "unity.h"
#include "xy_qma6100p.h"

#include <string.h>

static xy_i2c_device_t *active;
static uint8_t regs[256];
static size_t reads;
static size_t writes;
static uint32_t delayed;

xy_error_t xy_i2c_device_init(xy_i2c_device_t *dev, void *handle, uint16_t address,
                              uint32_t timeout_ms)
{
    memset(dev, 0, sizeof(*dev));
    dev->base.initialized = 1U;
    dev->i2c_handle = handle;
    dev->dev_addr = address;
    dev->timeout = timeout_ms;
    active = dev;
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_read_reg(xy_i2c_device_t *dev, uint8_t reg, uint8_t *data,
                                  size_t length)
{
    TEST_ASSERT_EQUAL_PTR(active, dev);
    memcpy(data, &regs[reg], length);
    reads++;
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_write_reg(xy_i2c_device_t *dev, uint8_t reg,
                                   const uint8_t *data, size_t length)
{
    TEST_ASSERT_EQUAL_PTR(active, dev);
    memcpy(&regs[reg], data, length);
    writes++;
    return XY_DEVICE_OK;
}

void xy_hal_delay_ms(uint32_t ms) { delayed += ms; }

void setUp(void)
{
    memset(regs, 0, sizeof(regs));
    regs[XY_QMA6100P_REG_CHIP_ID] = XY_QMA6100P_CHIP_ID;
    active = NULL;
    reads = writes = delayed = 0U;
}
void tearDown(void) {}

static void test_init_configures_documented_profile(void)
{
    xy_qma6100p_t dev;
    int bus;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_qma6100p_init(&dev, &bus, XY_QMA6100P_ADDR_LOW));
    TEST_ASSERT_TRUE(dev.initialized);
    TEST_ASSERT_EQUAL_HEX8(XY_QMA6100P_RANGE_2G, regs[XY_QMA6100P_REG_RANGE]);
    TEST_ASSERT_EQUAL_HEX8(XY_QMA6100P_BW_100HZ, regs[XY_QMA6100P_REG_BW]);
    TEST_ASSERT_EQUAL_HEX8(XY_QMA6100P_POWER_ACTIVE, regs[XY_QMA6100P_REG_POWER]);
    TEST_ASSERT_EQUAL_UINT32(2U, delayed);
}

static void test_init_rejects_wrong_identity(void)
{
    xy_qma6100p_t dev;
    int bus;
    regs[XY_QMA6100P_REG_CHIP_ID] = 0xFFU;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_NOT_FOUND,
                          xy_qma6100p_init(&dev, &bus, XY_QMA6100P_ADDR_HIGH));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_EQUAL_UINT(0U, writes);
}

static void test_read_decodes_signed_14_bit_data(void)
{
    xy_qma6100p_t dev;
    xy_qma6100p_raw_t raw;
    xy_qma6100p_accel_t accel;
    int bus;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_qma6100p_init(&dev, &bus, XY_QMA6100P_ADDR_LOW));
    regs[1] = 0x00U; regs[2] = 0x10U;
    regs[3] = 0x00U; regs[4] = 0xF0U;
    regs[5] = 0x00U; regs[6] = 0x40U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_qma6100p_read_raw(&dev, &raw));
    TEST_ASSERT_EQUAL_INT16(1024, raw.x);
    TEST_ASSERT_EQUAL_INT16(-1024, raw.y);
    TEST_ASSERT_EQUAL_INT16(4096, raw.z);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_qma6100p_read_accel(&dev, &accel));
    TEST_ASSERT_EQUAL_INT32(250, accel.x_mg);
    TEST_ASSERT_EQUAL_INT32(-250, accel.y_mg);
    TEST_ASSERT_EQUAL_INT32(1000, accel.z_mg);
}

static void test_interrupt_profile_maps_both_pins(void)
{
    xy_qma6100p_t dev;
    int bus;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_qma6100p_init(&dev, &bus, XY_QMA6100P_ADDR_LOW));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_qma6100p_configure_data_ready_interrupts(&dev, 1U, 1U));
    TEST_ASSERT_EQUAL_HEX8(0x05U, regs[XY_QMA6100P_REG_INT_PIN_CONFIG]);
    TEST_ASSERT_EQUAL_HEX8(XY_QMA6100P_DATA_READY_BIT, regs[XY_QMA6100P_REG_INT_MAP1]);
    TEST_ASSERT_EQUAL_HEX8(XY_QMA6100P_DATA_READY_BIT, regs[XY_QMA6100P_REG_INT_MAP3]);
    TEST_ASSERT_EQUAL_HEX8(XY_QMA6100P_DATA_READY_BIT, regs[XY_QMA6100P_REG_INT_ENABLE1]);
}

static void test_interrupt_config_readback_is_staged(void)
{
    xy_qma6100p_t dev;
    xy_qma6100p_interrupt_config_t config;
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_qma6100p_init(&dev, &bus, XY_QMA6100P_ADDR_LOW));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_qma6100p_configure_data_ready_interrupts(&dev, 1U, 1U));
    memset(&config, 0xA5, sizeof(config));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_qma6100p_read_interrupt_config(&dev, &config));
    TEST_ASSERT_EQUAL_HEX8(XY_QMA6100P_DATA_READY_BIT, config.enable1);
    TEST_ASSERT_EQUAL_HEX8(XY_QMA6100P_DATA_READY_BIT, config.map_int1);
    TEST_ASSERT_EQUAL_HEX8(XY_QMA6100P_DATA_READY_BIT, config.map_int2);
    TEST_ASSERT_EQUAL_HEX8(0x05U, config.pin_config);
    TEST_ASSERT_EQUAL_HEX8(0x0CU, config.interrupt_config);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_configures_documented_profile);
    RUN_TEST(test_init_rejects_wrong_identity);
    RUN_TEST(test_read_decodes_signed_14_bit_data);
    RUN_TEST(test_interrupt_profile_maps_both_pins);
    RUN_TEST(test_interrupt_config_readback_is_staged);
    return UNITY_END();
}
