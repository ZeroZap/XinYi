#include "fff.h"
#include "unity.h"

DEFINE_FFF_GLOBALS;

#include <stdint.h>

FAKE_VALUE_FUNC(int, fake_bus_read, uint8_t, uint8_t *, uint16_t)
FAKE_VOID_FUNC(fake_delay_ms, uint32_t)

static int custom_bus_read(uint8_t reg, uint8_t *buffer, uint16_t len)
{
    if (buffer == 0 || len == 0U) {
        return -1;
    }

    buffer[0] = (uint8_t)(reg + len);
    return (int)len;
}

void setUp(void)
{
    RESET_FAKE(fake_bus_read);
    RESET_FAKE(fake_delay_ms);
    FFF_RESET_HISTORY();
}

void tearDown(void)
{
}

static void test_fff_tracks_call_count_arguments_and_return_value(void)
{
    uint8_t value = 0;

    fake_bus_read_fake.return_val = 3;

    TEST_ASSERT_EQUAL_INT(3, fake_bus_read(0x10, &value, 2));
    TEST_ASSERT_EQUAL_UINT(1U, fake_bus_read_fake.call_count);
    TEST_ASSERT_EQUAL_HEX8(0x10, fake_bus_read_fake.arg0_val);
    TEST_ASSERT_EQUAL_PTR(&value, fake_bus_read_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT16(2U, fake_bus_read_fake.arg2_val);
}

static void test_fff_supports_custom_fake_behavior(void)
{
    uint8_t value = 0;

    fake_bus_read_fake.custom_fake = custom_bus_read;

    TEST_ASSERT_EQUAL_INT(4, fake_bus_read(0x20, &value, 4));
    TEST_ASSERT_EQUAL_HEX8(0x24, value);
    TEST_ASSERT_EQUAL_UINT(1U, fake_bus_read_fake.call_count);
}

static void test_fff_resets_fake_state_between_tests(void)
{
    TEST_ASSERT_EQUAL_UINT(0U, fake_bus_read_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(0U, fake_delay_ms_fake.call_count);

    fake_delay_ms(5);

    TEST_ASSERT_EQUAL_UINT(1U, fake_delay_ms_fake.call_count);
    TEST_ASSERT_EQUAL_UINT32(5U, fake_delay_ms_fake.arg0_val);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_fff_tracks_call_count_arguments_and_return_value);
    RUN_TEST(test_fff_supports_custom_fake_behavior);
    RUN_TEST(test_fff_resets_fake_state_between_tests);
    return UNITY_END();
}
