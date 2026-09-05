#include "unity.h"

#include <stdint.h>
#include <string.h>

#include "xy_w25q128.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
    uint8_t instruction;
    uint32_t address;
    uint8_t has_address;
    xy_hal_qspi_lines_t data_lines;
    size_t data_len;
    xy_hal_error_t result;
    uint8_t data[256];
} operation_t;

static operation_t operations[32];
static size_t operation_count;
static size_t operation_index;
static uint32_t tick;
static xy_device_t *registered_device;

static void queue_operation(uint8_t instruction, uint32_t address, uint8_t has_address,
                            xy_hal_qspi_lines_t data_lines, const uint8_t *data, size_t data_len,
                            xy_hal_error_t result)
{
    operation_t *operation = &operations[operation_count++];
    operation->instruction = instruction;
    operation->address = address;
    operation->has_address = has_address;
    operation->data_lines = data_lines;
    operation->data_len = data_len;
    operation->result = result;
    if (data != NULL) {
        memcpy(operation->data, data, data_len);
    }
}

xy_hal_error_t xy_hal_qspi_command(void *qspi, const xy_hal_qspi_command_t *command,
                                   uint8_t *data, uint32_t timeout)
{
    operation_t *operation;
    (void)timeout;
    TEST_ASSERT_NOT_NULL(qspi);
    TEST_ASSERT_LESS_THAN_UINT(operation_count, operation_index);
    operation = &operations[operation_index++];
    TEST_ASSERT_EQUAL_HEX8(operation->instruction, command->instruction);
    TEST_ASSERT_EQUAL_UINT8(operation->has_address, command->has_address);
    TEST_ASSERT_EQUAL_HEX32(operation->address, command->address);
    TEST_ASSERT_EQUAL_INT(operation->data_lines, command->data_lines);
    TEST_ASSERT_EQUAL_UINT(operation->data_len, command->data_length);
    if (operation->result == XY_HAL_OK && data != NULL && !command->write) {
        memcpy(data, operation->data, operation->data_len);
    }
    if (operation->result == XY_HAL_OK && data != NULL && command->write) {
        TEST_ASSERT_EQUAL_UINT8_ARRAY(operation->data, data, operation->data_len);
    }
    return operation->result;
}

uint32_t xy_hal_qspi_tick_ms(void)
{
    return tick++;
}

xy_error_t xy_device_register(xy_device_t *device)
{
    registered_device = device;
    return XY_OK;
}

void setUp(void)
{
    memset(operations, 0, sizeof(operations));
    operation_count = 0U;
    operation_index = 0U;
    tick = 0U;
    registered_device = NULL;
}

void tearDown(void)
{
    TEST_ASSERT_EQUAL_UINT(operation_count, operation_index);
}

static void test_probe_identifies_w25q128_and_registers_flash_device(void)
{
    xy_w25q128_t flash = {0};
    uint8_t qspi;
    const uint8_t id[] = {0xEFU, 0x40U, 0x18U};

    queue_operation(0x9FU, 0U, 0U, XY_HAL_QSPI_LINES_1, id, sizeof(id), XY_HAL_OK);
    TEST_ASSERT_EQUAL_INT(XY_W25Q128_OK, xy_w25q128_init(&flash, &qspi, "w25q128"));
    TEST_ASSERT_EQUAL_HEX32(0xEF4018U, flash.jedec_id);
    TEST_ASSERT_EQUAL_UINT32(16U * 1024U * 1024U, flash.capacity);
    TEST_ASSERT_EQUAL_PTR(&flash.device, registered_device);
    TEST_ASSERT_EQUAL_INT(XY_DEV_TYPE_FLASH, flash.device.type);
}

static void test_probe_rejects_wrong_identity(void)
{
    xy_w25q128_t flash = {0};
    uint8_t qspi;
    const uint8_t id[] = {0xEFU, 0x40U, 0x17U};

    queue_operation(0x9FU, 0U, 0U, XY_HAL_QSPI_LINES_1, id, sizeof(id), XY_HAL_OK);
    TEST_ASSERT_EQUAL_INT(XY_W25Q128_NOT_FOUND, xy_w25q128_init(&flash, &qspi, "flash"));
    TEST_ASSERT_NULL(registered_device);
}

static void test_erase_program_read_uses_qspi_bus_and_propagates_errors(void)
{
    xy_w25q128_t flash = {.initialized = 1U};
    uint8_t qspi;
    const uint8_t payload[] = {1U, 2U, 3U, 4U};
    const uint8_t ready[] = {0U};
    uint8_t actual[sizeof(payload)] = {0};
    flash.qspi = &qspi;
    flash.capacity = 16U * 1024U * 1024U;

    queue_operation(0x06U, 0U, 0U, XY_HAL_QSPI_LINES_NONE, NULL, 0U, XY_HAL_OK);
    queue_operation(0x20U, 0x1000U, 1U, XY_HAL_QSPI_LINES_NONE, NULL, 0U, XY_HAL_OK);
    queue_operation(0x05U, 0U, 0U, XY_HAL_QSPI_LINES_1, ready, 1U, XY_HAL_OK);
    TEST_ASSERT_EQUAL_INT(XY_W25Q128_OK, xy_w25q128_sector_erase(&flash, 0x1000U, 10U));

    queue_operation(0x06U, 0U, 0U, XY_HAL_QSPI_LINES_NONE, NULL, 0U, XY_HAL_OK);
    queue_operation(0x02U, 0x1000U, 1U, XY_HAL_QSPI_LINES_1, payload, sizeof(payload), XY_HAL_OK);
    queue_operation(0x05U, 0U, 0U, XY_HAL_QSPI_LINES_1, ready, 1U, XY_HAL_OK);
    TEST_ASSERT_EQUAL_INT(XY_W25Q128_OK,
                          xy_w25q128_page_program(&flash, 0x1000U, payload, sizeof(payload), 10U));

    queue_operation(0x03U, 0x1000U, 1U, XY_HAL_QSPI_LINES_1, payload, sizeof(payload), XY_HAL_OK);
    TEST_ASSERT_EQUAL_INT(XY_W25Q128_OK,
                          xy_w25q128_read(&flash, 0x1000U, actual, sizeof(actual)));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(payload, actual, sizeof(payload));

    queue_operation(0x03U, 0x1000U, 1U, XY_HAL_QSPI_LINES_1, NULL, sizeof(payload),
                    XY_HAL_ERROR_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(XY_W25Q128_TIMEOUT,
                          xy_w25q128_read(&flash, 0x1000U, actual, sizeof(actual)));
}

static void test_quad_program_uses_four_data_lines_and_checks_page_bounds(void)
{
    xy_w25q128_t flash = {.initialized = 1U};
    uint8_t qspi;
    const uint8_t payload[] = {0xA5U, 0x5AU, 0x3CU, 0xC3U};
    const uint8_t ready[] = {0U};
    flash.qspi = &qspi;
    flash.capacity = 16U * 1024U * 1024U;

    queue_operation(0x06U, 0U, 0U, XY_HAL_QSPI_LINES_NONE, NULL, 0U, XY_HAL_OK);
    queue_operation(0x32U, 0x1100U, 1U, XY_HAL_QSPI_LINES_4, payload, sizeof(payload),
                    XY_HAL_OK);
    queue_operation(0x05U, 0U, 0U, XY_HAL_QSPI_LINES_1, ready, 1U, XY_HAL_OK);
    TEST_ASSERT_EQUAL_INT(
        XY_W25Q128_OK,
        xy_w25q128_quad_page_program(&flash, 0x1100U, payload, sizeof(payload), 10U));

    TEST_ASSERT_EQUAL_INT(
        XY_W25Q128_INVALID_PARAM,
        xy_w25q128_quad_page_program(&flash, 0x11FFU, payload, sizeof(payload), 10U));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_probe_identifies_w25q128_and_registers_flash_device);
    RUN_TEST(test_probe_rejects_wrong_identity);
    RUN_TEST(test_erase_program_read_uses_qspi_bus_and_propagates_errors);
    RUN_TEST(test_quad_program_uses_four_data_lines_and_checks_page_bounds);
    return UNITY_END();
}
