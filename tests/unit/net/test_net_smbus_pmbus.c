#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "unity.h"
#include "fff.h"

#include "xy_smbus.h"
#include "xy_pmbus.h"

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(smbus_err_t, fake_smbus_init, smbus_device_t *)
FAKE_VALUE_FUNC(smbus_err_t, fake_smbus_deinit, smbus_device_t *)
FAKE_VALUE_FUNC(smbus_err_t, fake_pmbus_init, pmbus_device_t *)
FAKE_VALUE_FUNC(smbus_err_t, fake_pmbus_deinit, pmbus_device_t *)
FAKE_VOID_FUNC(fake_alert_callback, smbus_device_t *, uint8_t)

void xy_log_char(char ch)
{
    (void)ch;
}

static const smbus_ops_t g_fake_smbus_ops = {
    .init = fake_smbus_init,
    .deinit = fake_smbus_deinit,
};

static const pmbus_ops_t g_fake_pmbus_ops = {
    .init = fake_pmbus_init,
    .deinit = fake_pmbus_deinit,
};

void setUp(void)
{
    RESET_FAKE(fake_smbus_init);
    RESET_FAKE(fake_smbus_deinit);
    RESET_FAKE(fake_pmbus_init);
    RESET_FAKE(fake_pmbus_deinit);
    RESET_FAKE(fake_alert_callback);
    FFF_RESET_HISTORY();

    fake_smbus_init_fake.return_val = SMBUS_EOK;
    fake_smbus_deinit_fake.return_val = SMBUS_EOK;
    fake_pmbus_init_fake.return_val = SMBUS_EOK;
    fake_pmbus_deinit_fake.return_val = SMBUS_EOK;
}

void tearDown(void)
{
}

static void test_smbus_helpers_and_registry(void)
{
    uint8_t pec_data[] = {0x12, 0x34, 0x56};
    uint8_t pec = smbus_pec_calculate(pec_data, sizeof(pec_data));
    TEST_ASSERT_EQUAL_HEX8(pec, smbus_pec_calculate(pec_data, sizeof(pec_data)));
    TEST_ASSERT_TRUE(smbus_pec_verify(pec_data, sizeof(pec_data), pec));
    TEST_ASSERT_FALSE(smbus_pec_verify(pec_data, sizeof(pec_data), (uint8_t)(pec ^ 0x01)));

    TEST_ASSERT_EQUAL_HEX8(0xb4, smbus_addr_7to8(0x5a, false));
    TEST_ASSERT_EQUAL_HEX8(0xb5, smbus_addr_7to8(0x5a, true));
    TEST_ASSERT_EQUAL_HEX8(0x5a, smbus_addr_8to7(0xb5));
    TEST_ASSERT_TRUE(smbus_addr_valid(0x08));
    TEST_ASSERT_TRUE(smbus_addr_valid(0x77));
    TEST_ASSERT_FALSE(smbus_addr_valid(0x07));
    TEST_ASSERT_FALSE(smbus_addr_valid(0x78));

    smbus_device_t dev = {
        .name = "smbus-test",
        .config = SMBUS_CONFIG_DEFAULT(0x5a),
        .ops = &g_fake_smbus_ops,
    };

    TEST_ASSERT_EQUAL_INT(SMBUS_EINVAL, smbus_register(NULL));
    TEST_ASSERT_EQUAL_INT(SMBUS_EOK, smbus_register(&dev));
    TEST_ASSERT_EQUAL_PTR(&dev, smbus_find("smbus-test"));
    TEST_ASSERT_EQUAL_PTR(&dev, smbus_find_by_addr(0x5a));
    TEST_ASSERT_NOT_NULL(smbus_err_str(SMBUS_EBUSY));
    TEST_ASSERT_EQUAL_INT(SMBUS_EOK, smbus_init(&dev));
    TEST_ASSERT_EQUAL_INT(SMBUS_EOK, smbus_deinit(&dev));
    TEST_ASSERT_EQUAL_UINT(1U, fake_smbus_init_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&dev, fake_smbus_init_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT(1U, fake_smbus_deinit_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&dev, fake_smbus_deinit_fake.arg0_val);
    TEST_ASSERT_EQUAL_INT(SMBUS_EOK, smbus_unregister(&dev));
    TEST_ASSERT_NULL(smbus_find("smbus-test"));
    TEST_ASSERT_NULL(smbus_find_by_addr(0x5a));
    TEST_ASSERT_EQUAL_INT(SMBUS_ENODEV, smbus_unregister(&dev));
}

static void test_smbus_default_io_contracts(void)
{
    smbus_device_t dev = {
        .name = "smbus-default",
        .config = SMBUS_CONFIG_DEFAULT(0x40),
        .ops = &smbus_default_ops,
    };
    uint8_t byte = 0xff;
    uint16_t word = 0xffff;
    uint8_t block[SMBUS_MAX_PAYLOAD];
    uint8_t len = sizeof(block);
    uint8_t addrs[8];
    uint8_t count = 0xff;
    uint8_t pec = 0;
    uint16_t process_read = 0xffff;
    uint8_t block_read_len = sizeof(block);

    memset(block, 0xaa, sizeof(block));
    TEST_ASSERT_EQUAL_INT(SMBUS_EOK, smbus_init(&dev));
    TEST_ASSERT_EQUAL_INT(SMBUS_EOK, smbus_receive_byte(&dev, dev.config.addr, &byte));
    TEST_ASSERT_EQUAL_HEX8(0, byte);
    TEST_ASSERT_EQUAL_INT(SMBUS_EOK, smbus_read_byte(&dev, dev.config.addr, 0x01, &byte));
    TEST_ASSERT_EQUAL_HEX8(0, byte);
    TEST_ASSERT_EQUAL_INT(SMBUS_EOK, smbus_read_word(&dev, dev.config.addr, 0x02, &word));
    TEST_ASSERT_EQUAL_HEX16(0, word);
    TEST_ASSERT_EQUAL_INT(SMBUS_EOK, smbus_read_block(&dev, dev.config.addr, 0x03, block, &len));
    TEST_ASSERT_EQUAL_UINT8(0, len);
    TEST_ASSERT_EQUAL_INT(SMBUS_EOK, smbus_process_call(&dev, dev.config.addr, 0x05, 0x1234, &process_read));
    TEST_ASSERT_EQUAL_HEX16(0, process_read);
    TEST_ASSERT_EQUAL_INT(SMBUS_EOK, smbus_block_process_call(&dev, dev.config.addr, 0x06, block, 3, block,
                                                          &block_read_len));
    TEST_ASSERT_EQUAL_UINT8(0, block_read_len);
    TEST_ASSERT_EQUAL_INT(SMBUS_EOK, smbus_scan(&dev, addrs, &count));
    TEST_ASSERT_EQUAL_UINT8(0, count);
    TEST_ASSERT_EQUAL_INT(SMBUS_EINVAL, smbus_write_block(&dev, dev.config.addr, 0x04, block, SMBUS_MAX_PAYLOAD + 1));
    TEST_ASSERT_EQUAL_INT(SMBUS_EOK, smbus_calculate_pec(&dev, block, 3, &pec));
    TEST_ASSERT_TRUE(smbus_verify_pec(&dev, block, 3, pec));
    TEST_ASSERT_EQUAL_INT(SMBUS_EOK, smbus_alert_register_callback(fake_alert_callback, NULL));
    TEST_ASSERT_EQUAL_INT(SMBUS_EOK, smbus_alert_response(0x0c));
    TEST_ASSERT_EQUAL_UINT(1U, fake_alert_callback_fake.call_count);
    TEST_ASSERT_NULL(fake_alert_callback_fake.arg0_val);
    TEST_ASSERT_EQUAL_HEX8(0x0c, fake_alert_callback_fake.arg1_val);
    TEST_ASSERT_EQUAL_INT(SMBUS_EINVAL, smbus_alert_response(0x78));
}

static void test_pmbus_conversions_and_status(void)
{
    uint16_t one = pmbus_float_to_linear(1.0f);
    float one_back = pmbus_linear_to_float(one);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, one_back);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, pmbus_linear_to_float(0x0000));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, pmbus_vid_to_voltage(17));
    TEST_ASSERT_EQUAL_UINT16(17, pmbus_voltage_to_vid(1.0f));

    pmbus_status_t status;
    TEST_ASSERT_EQUAL_INT(SMBUS_EOK, pmbus_parse_status_word(0xffff, &status));
    TEST_ASSERT_TRUE(status.vout_ov_fault);
    TEST_ASSERT_TRUE(status.vout_uv_fault);
    TEST_ASSERT_TRUE(status.iout_oc_fault);
    TEST_ASSERT_TRUE(status.vin_uv_fault);
    TEST_ASSERT_TRUE(status.temp_ot_fault);
    TEST_ASSERT_EQUAL_INT(SMBUS_EINVAL, pmbus_parse_status_word(0, NULL));
}

static void test_pmbus_registry_and_default_fallbacks(void)
{
    smbus_device_t smbus = {
        .name = "pmbus-smbus",
        .config = SMBUS_CONFIG_DEFAULT(0x40),
        .ops = &smbus_default_ops,
    };
    pmbus_device_t pmbus = {
        .name = "pmbus-default",
        .config = {
            .smbus = SMBUS_CONFIG_DEFAULT(0x40),
            .vout_mode = PMBUS_VOUT_MODE_LINEAR,
            .spec_version = PMBUS_SPEC_VERSION_1_2,
        },
        .smbus = &smbus,
        .ops = &pmbus_default_ops,
    };
    float value = -1.0f;
    uint16_t status_word = 0xffff;
    pmbus_status_t status;
    char id[8];

    TEST_ASSERT_EQUAL_INT(SMBUS_EINVAL, pmbus_register(NULL));
    TEST_ASSERT_EQUAL_INT(SMBUS_EOK, pmbus_register(&pmbus));
    TEST_ASSERT_EQUAL_PTR(&pmbus, pmbus_find("pmbus-default"));
    TEST_ASSERT_EQUAL_INT(SMBUS_EOK, pmbus_init(&pmbus));
    TEST_ASSERT_EQUAL_INT(SMBUS_EOK, pmbus_read_vout(&pmbus, &value));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, value);
    TEST_ASSERT_EQUAL_INT(SMBUS_EOK, pmbus_read_iout(&pmbus, &value));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, value);
    TEST_ASSERT_EQUAL_INT(SMBUS_EOK, pmbus_read_temp(&pmbus, 2, &value));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, value);
    TEST_ASSERT_EQUAL_INT(SMBUS_EINVAL, pmbus_read_temp(&pmbus, 3, &value));
    TEST_ASSERT_EQUAL_INT(SMBUS_EOK, pmbus_write_vout_command(&pmbus, 1.2f));
    TEST_ASSERT_EQUAL_INT(SMBUS_EOK, pmbus_set_operation(&pmbus, PMBUS_OP_ON));
    TEST_ASSERT_EQUAL_INT(SMBUS_EOK, pmbus_clear_faults(&pmbus));
    TEST_ASSERT_EQUAL_INT(SMBUS_EOK, pmbus_read_status_word(&pmbus, &status_word));
    TEST_ASSERT_EQUAL_HEX16(0, status_word);
    TEST_ASSERT_EQUAL_INT(SMBUS_EOK, pmbus_get_status(&pmbus, &status));
    TEST_ASSERT_FALSE(status.vout_ov_fault);
    TEST_ASSERT_EQUAL_INT(SMBUS_EOK, pmbus_set_page(&pmbus, 1));
    TEST_ASSERT_EQUAL_INT(SMBUS_EOK, pmbus_read_id(&pmbus, id, sizeof(id) - 1));
    TEST_ASSERT_EQUAL_CHAR('\0', id[0]);
    TEST_ASSERT_EQUAL_INT(SMBUS_EOK, pmbus_deinit(&pmbus));
    TEST_ASSERT_EQUAL_INT(SMBUS_EOK, pmbus_unregister(&pmbus));
    TEST_ASSERT_NULL(pmbus_find("pmbus-default"));
    TEST_ASSERT_EQUAL_INT(SMBUS_ENODEV, pmbus_unregister(&pmbus));
}

static void test_pmbus_custom_ops(void)
{
    smbus_device_t smbus = {
        .name = "pmbus-smbus-custom",
        .config = SMBUS_CONFIG_DEFAULT(0x41),
        .ops = &g_fake_smbus_ops,
    };
    pmbus_device_t pmbus = {
        .name = "pmbus-custom",
        .config = { .smbus = SMBUS_CONFIG_DEFAULT(0x41) },
        .smbus = &smbus,
        .ops = &g_fake_pmbus_ops,
    };

    TEST_ASSERT_EQUAL_INT(SMBUS_EOK, pmbus_init(&pmbus));
    TEST_ASSERT_EQUAL_UINT(1U, fake_smbus_init_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&smbus, fake_smbus_init_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT(1U, fake_pmbus_init_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&pmbus, fake_pmbus_init_fake.arg0_val);
    TEST_ASSERT_EQUAL_INT(SMBUS_EOK, pmbus_deinit(&pmbus));
    TEST_ASSERT_EQUAL_UINT(1U, fake_pmbus_deinit_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&pmbus, fake_pmbus_deinit_fake.arg0_val);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_smbus_helpers_and_registry);
    RUN_TEST(test_smbus_default_io_contracts);
    RUN_TEST(test_pmbus_conversions_and_status);
    RUN_TEST(test_pmbus_registry_and_default_fallbacks);
    RUN_TEST(test_pmbus_custom_ops);
    return UNITY_END();
}
