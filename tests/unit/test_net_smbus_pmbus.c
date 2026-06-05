#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "xy_smbus.h"
#include "xy_pmbus.h"

static int g_smbus_init_count;
static int g_smbus_deinit_count;
static int g_pmbus_init_count;
static int g_pmbus_deinit_count;
static int g_alert_count;
static uint8_t g_alert_mask;

void xy_log_char(char ch)
{
    (void)ch;
}

static smbus_err_t fake_smbus_init(smbus_device_t *dev)
{
    assert(dev != NULL);
    g_smbus_init_count++;
    return SMBUS_EOK;
}

static smbus_err_t fake_smbus_deinit(smbus_device_t *dev)
{
    assert(dev != NULL);
    g_smbus_deinit_count++;
    return SMBUS_EOK;
}

static smbus_err_t fake_pmbus_init(pmbus_device_t *dev)
{
    assert(dev != NULL);
    g_pmbus_init_count++;
    return SMBUS_EOK;
}

static smbus_err_t fake_pmbus_deinit(pmbus_device_t *dev)
{
    assert(dev != NULL);
    g_pmbus_deinit_count++;
    return SMBUS_EOK;
}

static const smbus_ops_t g_fake_smbus_ops = {
    .init = fake_smbus_init,
    .deinit = fake_smbus_deinit,
};

static const pmbus_ops_t g_fake_pmbus_ops = {
    .init = fake_pmbus_init,
    .deinit = fake_pmbus_deinit,
};

static void fake_alert_callback(smbus_device_t *dev, uint8_t alert_mask)
{
    (void)dev;
    g_alert_count++;
    g_alert_mask = alert_mask;
}

static void test_smbus_helpers_and_registry(void)
{
    uint8_t pec_data[] = {0x12, 0x34, 0x56};
    uint8_t pec = smbus_pec_calculate(pec_data, sizeof(pec_data));
    assert(pec == smbus_pec_calculate(pec_data, sizeof(pec_data)));
    assert(smbus_pec_verify(pec_data, sizeof(pec_data), pec));
    assert(!smbus_pec_verify(pec_data, sizeof(pec_data), (uint8_t)(pec ^ 0x01)));

    assert(smbus_addr_7to8(0x5a, false) == 0xb4);
    assert(smbus_addr_7to8(0x5a, true) == 0xb5);
    assert(smbus_addr_8to7(0xb5) == 0x5a);
    assert(smbus_addr_valid(0x08));
    assert(smbus_addr_valid(0x77));
    assert(!smbus_addr_valid(0x07));
    assert(!smbus_addr_valid(0x78));

    smbus_device_t dev = {
        .name = "smbus-test",
        .config = SMBUS_CONFIG_DEFAULT(0x5a),
        .ops = &g_fake_smbus_ops,
    };

    assert(smbus_register(NULL) == SMBUS_EINVAL);
    assert(smbus_register(&dev) == SMBUS_EOK);
    assert(smbus_find("smbus-test") == &dev);
    assert(smbus_find_by_addr(0x5a) == &dev);
    assert(smbus_err_str(SMBUS_EBUSY) != NULL);
    assert(smbus_init(&dev) == SMBUS_EOK);
    assert(smbus_deinit(&dev) == SMBUS_EOK);
    assert(g_smbus_init_count == 1);
    assert(g_smbus_deinit_count == 1);
    assert(smbus_unregister(&dev) == SMBUS_EOK);
    assert(smbus_find("smbus-test") == NULL);
    assert(smbus_unregister(&dev) == SMBUS_ENODEV);
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
    assert(smbus_init(&dev) == SMBUS_EOK);
    assert(smbus_receive_byte(&dev, dev.config.addr, &byte) == SMBUS_EOK);
    assert(byte == 0);
    assert(smbus_read_byte(&dev, dev.config.addr, 0x01, &byte) == SMBUS_EOK);
    assert(byte == 0);
    assert(smbus_read_word(&dev, dev.config.addr, 0x02, &word) == SMBUS_EOK);
    assert(word == 0);
    assert(smbus_read_block(&dev, dev.config.addr, 0x03, block, &len) == SMBUS_EOK);
    assert(len == 0);
    assert(smbus_process_call(&dev, dev.config.addr, 0x05, 0x1234, &process_read) == SMBUS_EOK);
    assert(process_read == 0);
    assert(smbus_block_process_call(&dev, dev.config.addr, 0x06, block, 3, block,
                                    &block_read_len) == SMBUS_EOK);
    assert(block_read_len == 0);
    assert(smbus_scan(&dev, addrs, &count) == SMBUS_EOK);
    assert(count == 0);
    assert(smbus_write_block(&dev, dev.config.addr, 0x04, block, SMBUS_MAX_PAYLOAD + 1) == SMBUS_EINVAL);
    assert(smbus_calculate_pec(&dev, block, 3, &pec) == SMBUS_EOK);
    assert(smbus_verify_pec(&dev, block, 3, pec));
    g_alert_count = 0;
    g_alert_mask = 0;
    assert(smbus_alert_register_callback(fake_alert_callback, NULL) == SMBUS_EOK);
    assert(smbus_alert_response(0x0c) == SMBUS_EOK);
    assert(g_alert_count == 1);
    assert(g_alert_mask == 0x0c);
    assert(smbus_alert_response(0x78) == SMBUS_EINVAL);
}

static void test_pmbus_conversions_and_status(void)
{
    uint16_t one = pmbus_float_to_linear(1.0f);
    float one_back = pmbus_linear_to_float(one);
    assert(fabsf(one_back - 1.0f) < 0.01f);
    assert(pmbus_linear_to_float(0x0000) == 0.0f);
    assert(fabsf(pmbus_vid_to_voltage(17) - 1.0f) < 0.001f);
    assert(pmbus_voltage_to_vid(1.0f) == 17);

    pmbus_status_t status;
    assert(pmbus_parse_status_word(0xffff, &status) == SMBUS_EOK);
    assert(status.vout_ov_fault);
    assert(status.vout_uv_fault);
    assert(status.iout_oc_fault);
    assert(status.vin_uv_fault);
    assert(status.temp_ot_fault);
    assert(pmbus_parse_status_word(0, NULL) == SMBUS_EINVAL);
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

    assert(pmbus_register(NULL) == SMBUS_EINVAL);
    assert(pmbus_register(&pmbus) == SMBUS_EOK);
    assert(pmbus_find("pmbus-default") == &pmbus);
    assert(pmbus_init(&pmbus) == SMBUS_EOK);
    assert(pmbus_read_vout(&pmbus, &value) == SMBUS_EOK);
    assert(value == 0.0f);
    assert(pmbus_read_iout(&pmbus, &value) == SMBUS_EOK);
    assert(value == 0.0f);
    assert(pmbus_read_temp(&pmbus, 2, &value) == SMBUS_EOK);
    assert(value == 0.0f);
    assert(pmbus_read_temp(&pmbus, 3, &value) == SMBUS_EINVAL);
    assert(pmbus_write_vout_command(&pmbus, 1.2f) == SMBUS_EOK);
    assert(pmbus_set_operation(&pmbus, PMBUS_OP_ON) == SMBUS_EOK);
    assert(pmbus_clear_faults(&pmbus) == SMBUS_EOK);
    assert(pmbus_read_status_word(&pmbus, &status_word) == SMBUS_EOK);
    assert(status_word == 0);
    assert(pmbus_get_status(&pmbus, &status) == SMBUS_EOK);
    assert(!status.vout_ov_fault);
    assert(pmbus_set_page(&pmbus, 1) == SMBUS_EOK);
    assert(pmbus_read_id(&pmbus, id, sizeof(id) - 1) == SMBUS_EOK);
    assert(id[0] == '\0');
    assert(pmbus_deinit(&pmbus) == SMBUS_EOK);
    assert(pmbus_unregister(&pmbus) == SMBUS_EOK);
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

    g_smbus_init_count = 0;
    g_pmbus_init_count = 0;
    assert(pmbus_init(&pmbus) == SMBUS_EOK);
    assert(g_smbus_init_count == 1);
    assert(g_pmbus_init_count == 1);
    assert(pmbus_deinit(&pmbus) == SMBUS_EOK);
    assert(g_pmbus_deinit_count == 1);
}

int main(void)
{
    test_smbus_helpers_and_registry();
    test_smbus_default_io_contracts();
    test_pmbus_conversions_and_status();
    test_pmbus_registry_and_default_fallbacks();
    test_pmbus_custom_ops();
    return 0;
}
