#include <stdint.h>
#include <string.h>

#include "unity.h"
#include "xy_event_group.h"
#include "xy_timer_sw.h"

static uint32_t fake_tick;

uint32_t xy_tick_get(void)
{
    return fake_tick;
}

void xy_timer_sw_init(void)
{
}

xy_timer_sw_id_t xy_timer_sw_create(uint32_t interval, xy_timer_sw_callback_t callback,
                                    void *arg, uint8_t periodic)
{
    (void)interval;
    (void)callback;
    (void)arg;
    (void)periodic;
    return XY_TIMER_SW_INVALID_ID;
}

xy_timer_sw_error_t xy_timer_sw_stop(xy_timer_sw_id_t id)
{
    (void)id;
    return XY_TIMER_SW_ERR_INVALID;
}

xy_timer_sw_error_t xy_timer_sw_delete(xy_timer_sw_id_t id)
{
    (void)id;
    return XY_TIMER_SW_ERR_INVALID;
}

static void test_event_group_init_guards_and_name(void)
{
    xy_ipc_event_group_t group;
    char long_name[64];

    memset(long_name, 'e', sizeof(long_name));
    long_name[sizeof(long_name) - 1U] = '\0';

    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_INVALID_PARAM, xy_ipc_event_group_init(NULL, "evt"));
    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_OK, xy_ipc_event_group_init(&group, long_name));
    TEST_ASSERT_TRUE(group.initialized);
    TEST_ASSERT_NOT_NULL(group.os_flags);
    TEST_ASSERT_EQUAL_UINT(31U, strlen(group.name));

    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_OK, xy_ipc_event_group_deinit(&group));
    TEST_ASSERT_FALSE(group.initialized);
    TEST_ASSERT_NULL(group.os_flags);

    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_OK, xy_ipc_event_group_init(&group, NULL));
    TEST_ASSERT_EQUAL_UINT(0U, strlen(group.name));
    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_OK, xy_ipc_event_group_deinit(&group));
}

static void test_event_group_set_get_and_clear_contracts(void)
{
    xy_ipc_event_group_t group = {0};
    xy_ipc_event_bits_t bits = 0xDEADBEEFU;

    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_NOT_INITIALIZED,
                          xy_ipc_event_group_set(&group, 0x1U, &bits));
    TEST_ASSERT_EQUAL_UINT32(0xDEADBEEFU, bits);
    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_OK, xy_ipc_event_group_init(&group, "evt"));

    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_INVALID_PARAM, xy_ipc_event_group_set(&group, 0U, &bits));
    TEST_ASSERT_EQUAL_UINT32(0xDEADBEEFU, bits);
    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_INVALID_PARAM,
                          xy_ipc_event_group_set(&group, ~XY_IPC_EVENT_USER_BITS_MASK,
                                                 &bits));
    TEST_ASSERT_EQUAL_UINT32(0xDEADBEEFU, bits);
    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_OK, xy_ipc_event_group_set(&group, 0x05U, &bits));
    TEST_ASSERT_EQUAL_UINT32(0x05U, bits);
    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_OK, xy_ipc_event_group_set(&group, 0x02U, NULL));

    bits = 0U;
    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_OK, xy_ipc_event_group_get(&group, &bits));
    TEST_ASSERT_EQUAL_UINT32(0x07U, bits);
    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_INVALID_PARAM, xy_ipc_event_group_get(&group, NULL));

    bits = 0xCAFEU;
    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_INVALID_PARAM, xy_ipc_event_group_clear(&group, 0U, &bits));
    TEST_ASSERT_EQUAL_UINT32(0xCAFEU, bits);
    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_INVALID_PARAM,
                          xy_ipc_event_group_clear(&group, ~XY_IPC_EVENT_USER_BITS_MASK,
                                                   &bits));
    TEST_ASSERT_EQUAL_UINT32(0xCAFEU, bits);
    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_OK, xy_ipc_event_group_clear(&group, 0x03U, &bits));
    TEST_ASSERT_EQUAL_UINT32(0x07U, bits);

    bits = 0U;
    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_OK, xy_ipc_event_group_get(&group, &bits));
    TEST_ASSERT_EQUAL_UINT32(0x04U, bits);

    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_OK, xy_ipc_event_group_deinit(&group));
    bits = 0x55U;
    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_NOT_INITIALIZED, xy_ipc_event_group_get(&group, &bits));
    TEST_ASSERT_EQUAL_UINT32(0x55U, bits);
}

static void test_event_group_wait_any_all_clear_and_timeout(void)
{
    xy_ipc_event_group_t group;
    xy_ipc_event_bits_t matched = 0xA5A5U;
    xy_ipc_event_bits_t current = 0U;

    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_OK, xy_ipc_event_group_init(&group, "evt"));
    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_INVALID_PARAM,
                          xy_ipc_event_group_wait(&group, 0U, XY_IPC_EVENT_WAIT_ANY, 0U,
                                                  &matched));
    TEST_ASSERT_EQUAL_UINT32(0xA5A5U, matched);
    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_INVALID_PARAM,
                          xy_ipc_event_group_wait(&group, ~XY_IPC_EVENT_USER_BITS_MASK,
                                                  XY_IPC_EVENT_WAIT_ANY, 0U, &matched));
    TEST_ASSERT_EQUAL_UINT32(0xA5A5U, matched);
    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_INVALID_PARAM,
                          xy_ipc_event_group_wait(&group, 0x01U, XY_IPC_EVENT_WAIT_ANY, 0U,
                                                  NULL));
    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_INVALID_PARAM,
                          xy_ipc_event_group_wait(&group, 0x01U, 0x8000U, 0U, &matched));
    TEST_ASSERT_EQUAL_UINT32(0xA5A5U, matched);

    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_OK, xy_ipc_event_group_set(&group, 0x0FU, NULL));
    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_OK,
                          xy_ipc_event_group_wait(&group, 0x03U, XY_IPC_EVENT_WAIT_ANY, 0U,
                                                  &matched));
    TEST_ASSERT_EQUAL_UINT32(0x03U, matched);
    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_OK, xy_ipc_event_group_get(&group, &current));
    TEST_ASSERT_EQUAL_UINT32(0x0CU, current);

    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_TIMEOUT,
                          xy_ipc_event_group_wait(&group, 0x03U, XY_IPC_EVENT_WAIT_ALL, 0U,
                                                  &matched));
    TEST_ASSERT_EQUAL_UINT32(0x03U, matched);
    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_OK, xy_ipc_event_group_set(&group, 0x03U, NULL));
    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_OK,
                          xy_ipc_event_group_wait(&group, 0x03U, XY_IPC_EVENT_WAIT_ALL, 0U,
                                                  &matched));
    TEST_ASSERT_EQUAL_UINT32(0x03U, matched);

    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_OK, xy_ipc_event_group_set(&group, 0x80U, NULL));
    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_OK,
                          xy_ipc_event_group_wait(&group, 0x80U,
                                                  XY_IPC_EVENT_WAIT_ANY | XY_IPC_EVENT_NO_CLEAR,
                                                  0U, &matched));
    TEST_ASSERT_EQUAL_UINT32(0x80U, matched);
    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_OK, xy_ipc_event_group_get(&group, &current));
    TEST_ASSERT_EQUAL_UINT32(0x8CU, current);

    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_TIMEOUT,
                          xy_ipc_event_group_wait(&group, 0x40U, XY_IPC_EVENT_WAIT_ANY, 0U,
                                                  &matched));
    TEST_ASSERT_EQUAL_UINT32(0x80U, matched);

    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_OK, xy_ipc_event_group_deinit(&group));
    TEST_ASSERT_EQUAL_UINT32(0x80U, matched);
    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_NOT_INITIALIZED,
                          xy_ipc_event_group_set(&group, 0x01U, &matched));
    TEST_ASSERT_EQUAL_UINT32(0x80U, matched);
    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_NOT_INITIALIZED,
                          xy_ipc_event_group_clear(&group, 0x01U, &matched));
    TEST_ASSERT_EQUAL_UINT32(0x80U, matched);
    TEST_ASSERT_EQUAL_INT(XY_IPC_EVENT_NOT_INITIALIZED,
                          xy_ipc_event_group_wait(&group, 0x01U, XY_IPC_EVENT_WAIT_ANY, 0U,
                                                  &matched));
    TEST_ASSERT_EQUAL_UINT32(0x80U, matched);
}

void setUp(void)
{
    fake_tick = 0U;
}

void tearDown(void)
{
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_event_group_init_guards_and_name);
    RUN_TEST(test_event_group_set_get_and_clear_contracts);
    RUN_TEST(test_event_group_wait_any_all_clear_and_timeout);
    return UNITY_END();
}
