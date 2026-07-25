#include "unity.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sensor_sht30.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
    void *bus;
    uint8_t addr;
    uint8_t bytes[8];
    uint16_t len;
    int ret;
} i2c_op_t;

static i2c_op_t g_sends[8];
static i2c_op_t g_recvs[8];
static size_t g_send_count;
static size_t g_send_index;
static size_t g_recv_count;
static size_t g_recv_index;
static uint32_t g_tick;

void delay_ms(uint32_t ms)
{
    g_tick += ms;
}

uint32_t get_tick_ms(void)
{
    return g_tick;
}

int hal_i2c_master_send(void *bus, uint8_t addr, uint8_t *data, uint16_t len)
{
    TEST_ASSERT_LESS_THAN_UINT(g_send_count, g_send_index);
    i2c_op_t *op = &g_sends[g_send_index++];
    TEST_ASSERT_EQUAL_PTR(op->bus, bus);
    TEST_ASSERT_EQUAL_UINT8(op->addr, addr);
    TEST_ASSERT_EQUAL_UINT16(op->len, len);
    TEST_ASSERT_EQUAL_MEMORY(op->bytes, data, len);
    return op->ret;
}

int hal_i2c_master_recv(void *bus, uint8_t addr, uint8_t *data, uint16_t len)
{
    TEST_ASSERT_LESS_THAN_UINT(g_recv_count, g_recv_index);
    i2c_op_t *op = &g_recvs[g_recv_index++];
    TEST_ASSERT_EQUAL_PTR(op->bus, bus);
    TEST_ASSERT_EQUAL_UINT8(op->addr, addr);
    TEST_ASSERT_EQUAL_UINT16(op->len, len);
    if (op->ret == SENSOR_EOK) {
        memcpy(data, op->bytes, len);
    }
    return op->ret;
}

static void queue_send(void *bus, uint8_t addr, const uint8_t *bytes, uint16_t len, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_sends), g_send_count);
    i2c_op_t *op = &g_sends[g_send_count++];
    op->bus = bus;
    op->addr = addr;
    op->len = len;
    op->ret = ret;
    memcpy(op->bytes, bytes, len);
}

static void queue_recv(void *bus, uint8_t addr, const uint8_t *bytes, uint16_t len, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_recvs), g_recv_count);
    i2c_op_t *op = &g_recvs[g_recv_count++];
    op->bus = bus;
    op->addr = addr;
    op->len = len;
    op->ret = ret;
    if (bytes != NULL) {
        memcpy(op->bytes, bytes, len);
    }
}

static void assert_queues_drained(void)
{
    TEST_ASSERT_EQUAL_UINT(g_send_count, g_send_index);
    TEST_ASSERT_EQUAL_UINT(g_recv_count, g_recv_index);
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
    memset(g_sends, 0, sizeof(g_sends));
    memset(g_recvs, 0, sizeof(g_recvs));
    g_send_count = 0;
    g_send_index = 0;
    g_recv_count = 0;
    g_recv_index = 0;
    g_tick = 24680U;
}

void tearDown(void)
{
}

static void test_sht30_create_init_and_read_humidity_contract(void)
{
    int fake_bus;
    const uint8_t reset_cmd[2] = {0x30U, 0xA2U};
    const uint8_t measure_cmd[2] = {0x24U, 0x00U};
    const uint8_t raw[6] = {0x66U, 0x66U, 0x93U, 0x80U, 0x00U, 0xA2U};
    sensor_data_t data = {0};
    sensor_device_t *sensor = sht30_create("sht30-main", &fake_bus, 0U);

    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_STRING("sht30-main", sensor->info.name);
    TEST_ASSERT_EQUAL_STRING("Sensirion", sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING("SHT30", sensor->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_RELATIVE_HUMIDITY, sensor->info.type);
    TEST_ASSERT_EQUAL_UINT(10, sensor->info.max_odr);
    TEST_ASSERT_EQUAL_PTR(&fake_bus, sensor->bus);
    TEST_ASSERT_NOT_NULL(sensor->ops);
    TEST_ASSERT_NOT_NULL(sensor->ops->init);
    TEST_ASSERT_NULL(sensor->ops->deinit);
    TEST_ASSERT_NOT_NULL(sensor->ops->read);
    TEST_ASSERT_EQUAL_UINT8(SHT30_ADDR_DEFAULT, ((sht30_priv_t *)sensor->priv_data)->i2c_addr);

    queue_send(&fake_bus, SHT30_ADDR_DEFAULT, reset_cmd, sizeof(reset_cmd), SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));

    queue_send(&fake_bus, SHT30_ADDR_DEFAULT, measure_cmd, sizeof(measure_cmd), SENSOR_EOK);
    queue_recv(&fake_bus, SHT30_ADDR_DEFAULT, raw, sizeof(raw), SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_RELATIVE_HUMIDITY, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_PERCENT, data.unit);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 50.000763f, data.value.val_float);
    TEST_ASSERT_EQUAL_UINT32(24690U, data.timestamp);
    TEST_ASSERT_EQUAL_UINT8(95, data.accuracy);

    assert_queues_drained();
    destroy_sensor(sensor);
}

static void test_sht30_alt_address_and_failure_guards_preserve_outputs(void)
{
    int fake_bus;
    const uint8_t reset_cmd[2] = {0x30U, 0xA2U};
    const uint8_t measure_cmd[2] = {0x24U, 0x00U};
    sensor_data_t data = {.type = SENSOR_TYPE_TEMPERATURE,
                          .unit = SENSOR_UNIT_CELSIUS,
                          .value.val_float = -12.5f,
                          .timestamp = 123U,
                          .accuracy = 7U};
    sensor_data_t snapshot = data;
    sensor_device_t *sensor = sht30_create("sht30-alt", &fake_bus, SHT30_ADDR_ALT);

    TEST_ASSERT_NULL(sht30_create(NULL, &fake_bus, SHT30_ADDR_DEFAULT));
    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_UINT8(SHT30_ADDR_ALT, ((sht30_priv_t *)sensor->priv_data)->i2c_addr);

    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->init(NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->read(NULL, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->read(sensor, NULL));
    TEST_ASSERT_EQUAL_INT(snapshot.type, data.type);
    TEST_ASSERT_EQUAL_INT(snapshot.unit, data.unit);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, snapshot.value.val_float, data.value.val_float);
    TEST_ASSERT_EQUAL_UINT32(snapshot.timestamp, data.timestamp);
    TEST_ASSERT_EQUAL_UINT8(snapshot.accuracy, data.accuracy);

    queue_send(&fake_bus, SHT30_ADDR_ALT, reset_cmd, sizeof(reset_cmd), SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->init(sensor));

    queue_send(&fake_bus, SHT30_ADDR_ALT, measure_cmd, sizeof(measure_cmd), SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(snapshot.type, data.type);
    TEST_ASSERT_EQUAL_UINT32(snapshot.timestamp, data.timestamp);

    queue_send(&fake_bus, SHT30_ADDR_ALT, measure_cmd, sizeof(measure_cmd), SENSOR_EOK);
    queue_recv(&fake_bus, SHT30_ADDR_ALT, NULL, 6U, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(snapshot.type, data.type);
    TEST_ASSERT_EQUAL_UINT32(snapshot.timestamp, data.timestamp);

    assert_queues_drained();

    SENSOR_FREE(sensor->priv_data);
    sensor->priv_data = NULL;
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->read(sensor, &data));

    destroy_sensor(sensor);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_sht30_create_init_and_read_humidity_contract);
    RUN_TEST(test_sht30_alt_address_and_failure_guards_preserve_outputs);
    return UNITY_END();
}
