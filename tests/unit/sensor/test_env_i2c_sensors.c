#include "unity.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sensor_aht10.h"
#include "sensor_bmp390.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
    void *bus;
    uint8_t addr;
    uint8_t data[8];
    uint16_t len;
    int ret;
} master_op_t;

typedef struct {
    void *bus;
    uint8_t addr;
    uint8_t reg;
    uint8_t data[8];
    uint16_t len;
    int ret;
} mem_op_t;

static master_op_t g_master_sends[16];
static master_op_t g_master_recvs[16];
static mem_op_t g_mem_reads[32];
static mem_op_t g_mem_writes[16];
static size_t g_master_send_count;
static size_t g_master_send_index;
static size_t g_master_recv_count;
static size_t g_master_recv_index;
static size_t g_mem_read_count;
static size_t g_mem_read_index;
static size_t g_mem_write_count;
static size_t g_mem_write_index;
static uint32_t g_tick;
static uint32_t g_delay_total;
static unsigned int g_delay_count;

uint32_t get_tick_ms(void)
{
    return g_tick;
}

void delay_ms(uint32_t ms)
{
    g_delay_total += ms;
    g_delay_count++;
    g_tick += ms;
}

int hal_i2c_master_send(void *bus, uint8_t addr, uint8_t *data, uint16_t len)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_master_sends), g_master_send_index);
    master_op_t *op = &g_master_sends[g_master_send_index++];
    TEST_ASSERT_EQUAL_PTR(op->bus, bus);
    TEST_ASSERT_EQUAL_UINT8(op->addr, addr);
    TEST_ASSERT_EQUAL_UINT16(op->len, len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(op->data, data, len);
    return op->ret;
}

int hal_i2c_master_recv(void *bus, uint8_t addr, uint8_t *data, uint16_t len)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_master_recvs), g_master_recv_index);
    master_op_t *op = &g_master_recvs[g_master_recv_index++];
    TEST_ASSERT_EQUAL_PTR(op->bus, bus);
    TEST_ASSERT_EQUAL_UINT8(op->addr, addr);
    TEST_ASSERT_EQUAL_UINT16(op->len, len);
    if (op->ret == SENSOR_EOK) {
        memcpy(data, op->data, len);
    }
    return op->ret;
}

int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_mem_reads), g_mem_read_index);
    mem_op_t *op = &g_mem_reads[g_mem_read_index++];
    TEST_ASSERT_EQUAL_PTR(op->bus, bus);
    TEST_ASSERT_EQUAL_UINT8(op->addr, addr);
    TEST_ASSERT_EQUAL_UINT8(op->reg, reg);
    TEST_ASSERT_EQUAL_UINT16(op->len, len);
    if (op->ret == SENSOR_EOK) {
        memcpy(data, op->data, len);
    }
    return op->ret;
}

int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_mem_writes), g_mem_write_index);
    mem_op_t *op = &g_mem_writes[g_mem_write_index++];
    TEST_ASSERT_EQUAL_PTR(op->bus, bus);
    TEST_ASSERT_EQUAL_UINT8(op->addr, addr);
    TEST_ASSERT_EQUAL_UINT8(op->reg, reg);
    TEST_ASSERT_EQUAL_UINT16(op->len, len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(op->data, data, len);
    return op->ret;
}

static void queue_master_send(void *bus, uint8_t addr, const uint8_t *data, uint16_t len, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_master_sends), g_master_send_count);
    master_op_t *op = &g_master_sends[g_master_send_count++];
    op->bus = bus;
    op->addr = addr;
    op->len = len;
    op->ret = ret;
    memcpy(op->data, data, len);
}

static void queue_master_recv(void *bus, uint8_t addr, const uint8_t *data, uint16_t len, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_master_recvs), g_master_recv_count);
    master_op_t *op = &g_master_recvs[g_master_recv_count++];
    op->bus = bus;
    op->addr = addr;
    op->len = len;
    op->ret = ret;
    if (data != NULL) {
        memcpy(op->data, data, len);
    }
}

static void queue_mem_read(void *bus, uint8_t addr, uint8_t reg, const uint8_t *data, uint16_t len, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_mem_reads), g_mem_read_count);
    mem_op_t *op = &g_mem_reads[g_mem_read_count++];
    op->bus = bus;
    op->addr = addr;
    op->reg = reg;
    op->len = len;
    op->ret = ret;
    if (data != NULL) {
        memcpy(op->data, data, len);
    }
}

static void queue_mem_read8(void *bus, uint8_t addr, uint8_t reg, uint8_t value, int ret)
{
    queue_mem_read(bus, addr, reg, &value, 1U, ret);
}

static void queue_mem_write8(void *bus, uint8_t addr, uint8_t reg, uint8_t value, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_mem_writes), g_mem_write_count);
    mem_op_t *op = &g_mem_writes[g_mem_write_count++];
    op->bus = bus;
    op->addr = addr;
    op->reg = reg;
    op->len = 1U;
    op->ret = ret;
    op->data[0] = value;
}

void setUp(void)
{
    memset(g_master_sends, 0, sizeof(g_master_sends));
    memset(g_master_recvs, 0, sizeof(g_master_recvs));
    memset(g_mem_reads, 0, sizeof(g_mem_reads));
    memset(g_mem_writes, 0, sizeof(g_mem_writes));
    g_master_send_count = 0;
    g_master_send_index = 0;
    g_master_recv_count = 0;
    g_master_recv_index = 0;
    g_mem_read_count = 0;
    g_mem_read_index = 0;
    g_mem_write_count = 0;
    g_mem_write_index = 0;
    g_tick = 1000U;
    g_delay_total = 0;
    g_delay_count = 0;
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

static void test_aht10_create_defaults_and_reads_humidity(void)
{
    int fake_bus;
    uint8_t init_cmd[3] = {0xE1, 0x08, 0x00};
    uint8_t measure_cmd[3] = {0xAC, 0x33, 0x00};
    uint8_t sample[6] = {0x00, 0x80, 0x00, 0x00, 0x12, 0x34};
    sensor_data_t data = {0};

    sensor_device_t *sensor = aht10_create("aht10-main", &fake_bus, 0U);
    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_STRING("aht10-main", sensor->info.name);
    TEST_ASSERT_EQUAL_STRING("Aosong", sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING("AHT10", sensor->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_RELATIVE_HUMIDITY, sensor->info.type);
    TEST_ASSERT_EQUAL_UINT32(10U, sensor->info.max_odr);
    TEST_ASSERT_EQUAL_UINT32(10U, sensor->odr);
    TEST_ASSERT_EQUAL_PTR(&fake_bus, sensor->bus);
    TEST_ASSERT_EQUAL_UINT8(AHT10_ADDR_DEFAULT, ((aht10_priv_t *)sensor->priv_data)->i2c_addr);

    queue_master_send(&fake_bus, AHT10_ADDR_DEFAULT, init_cmd, sizeof(init_cmd), SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_UINT32(10U, g_delay_total);

    queue_master_send(&fake_bus, AHT10_ADDR_DEFAULT, measure_cmd, sizeof(measure_cmd), SENSOR_EOK);
    queue_master_recv(&fake_bus, AHT10_ADDR_DEFAULT, sample, sizeof(sample), SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_RELATIVE_HUMIDITY, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_PERCENT, data.unit);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 50.0f, data.value.val_float);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);
    TEST_ASSERT_EQUAL_UINT8(90U, data.accuracy);
    TEST_ASSERT_EQUAL_UINT32(90U, g_delay_total);
    TEST_ASSERT_EQUAL_UINT(2U, g_delay_count);

    destroy_sensor(sensor);
}

static void test_aht10_read_maps_receive_failure(void)
{
    int fake_bus;
    uint8_t measure_cmd[3] = {0xAC, 0x33, 0x00};
    sensor_data_t data = {0};
    sensor_device_t *sensor = aht10_create("aht10-alt", &fake_bus, 0x39U);

    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_UINT8(0x39U, ((aht10_priv_t *)sensor->priv_data)->i2c_addr);
    queue_master_send(&fake_bus, 0x39U, measure_cmd, sizeof(measure_cmd), SENSOR_EOK);
    queue_master_recv(&fake_bus, 0x39U, NULL, 6U, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->read(sensor, &data));

    destroy_sensor(sensor);
}

static void test_bmp390_create_init_and_read_pressure(void)
{
    int fake_bus;
    sensor_data_t data = {0};
    sensor_device_t *sensor = bmp390_create("bmp390-main", &fake_bus, 0U);

    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_STRING("bmp390-main", sensor->info.name);
    TEST_ASSERT_EQUAL_STRING("Bosch", sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING("BMP390", sensor->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_PRESSURE, sensor->info.type);
    TEST_ASSERT_EQUAL_UINT32(200U, sensor->info.max_odr);
    TEST_ASSERT_EQUAL_UINT32(100U, sensor->odr);
    TEST_ASSERT_EQUAL_PTR(&fake_bus, sensor->bus);
    TEST_ASSERT_EQUAL_UINT8(BMP390_ADDR_DEFAULT, ((bmp390_priv_t *)sensor->priv_data)->i2c_addr);

    queue_mem_read8(&fake_bus, BMP390_ADDR_DEFAULT, BMP390_REG_CHIP_ID, BMP390_CHIP_ID, SENSOR_EOK);
    queue_mem_write8(&fake_bus, BMP390_ADDR_DEFAULT, BMP390_REG_PWR_CTRL, 0x33U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));

    queue_mem_read8(&fake_bus, BMP390_ADDR_DEFAULT, BMP390_REG_PRESS_XLSB, 0x00U, SENSOR_EOK);
    queue_mem_read8(&fake_bus, BMP390_ADDR_DEFAULT, BMP390_REG_PRESS_LSB, 0x20U, SENSOR_EOK);
    queue_mem_read8(&fake_bus, BMP390_ADDR_DEFAULT, BMP390_REG_PRESS_MSB, 0x03U, SENSOR_EOK);
    queue_mem_read8(&fake_bus, BMP390_ADDR_DEFAULT, BMP390_REG_TEMP_XLSB, 0xAAU, SENSOR_EOK);
    queue_mem_read8(&fake_bus, BMP390_ADDR_DEFAULT, BMP390_REG_TEMP_LSB, 0xBBU, SENSOR_EOK);
    queue_mem_read8(&fake_bus, BMP390_ADDR_DEFAULT, BMP390_REG_TEMP_MSB, 0xCCU, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_PRESSURE, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_HECTOPASCAL, data.unit);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 800.0f, data.value.val_float);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);
    TEST_ASSERT_EQUAL_UINT8(95U, data.accuracy);

    destroy_sensor(sensor);
}

static void test_bmp390_init_rejects_bad_chip_id_and_read_maps_i2c_error(void)
{
    int fake_bus;
    sensor_data_t data = {0};
    sensor_device_t *sensor = bmp390_create("bmp390-alt", &fake_bus, BMP390_ADDR_ALT);

    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_UINT8(BMP390_ADDR_ALT, ((bmp390_priv_t *)sensor->priv_data)->i2c_addr);
    queue_mem_read8(&fake_bus, BMP390_ADDR_ALT, BMP390_REG_CHIP_ID, 0x00U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_ERROR, sensor->ops->init(sensor));

    queue_mem_read8(&fake_bus, BMP390_ADDR_ALT, BMP390_REG_PRESS_XLSB, 0x00U, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->read(sensor, &data));

    destroy_sensor(sensor);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_aht10_create_defaults_and_reads_humidity);
    RUN_TEST(test_aht10_read_maps_receive_failure);
    RUN_TEST(test_bmp390_create_init_and_read_pressure);
    RUN_TEST(test_bmp390_init_rejects_bad_chip_id_and_read_maps_i2c_error);
    return UNITY_END();
}
