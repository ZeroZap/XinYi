/**
 * @file test_ipc.c
 * @brief IPC Component Unit Tests using Unity Framework
 * @version 1.0.0
 * @date 2026-02-28
 */

#include <stdint.h>
#include <string.h>
#include "unity.h"

/* IPC headers */
#include "xy_pipe.h"
#include "xy_observer.h"

/* ==================== Test Fixtures ==================== */

static uint8_t test_buffer[256];
static xy_pipe_t test_pipe;

void setUp(void)
{
    /* Initialize test pipe before each test */
    xy_pipe_init(&test_pipe, "test", test_buffer, sizeof(test_buffer));
}

void tearDown(void)
{
    /* Clean up after each test */
    xy_pipe_deinit(&test_pipe);
}

/* ==================== Pipe Tests ==================== */

void test_pipe_init(void)
{
    xy_pipe_t pipe;
    uint8_t buffer[64];
    int result;

    /* Test valid initialization */
    result = xy_pipe_init(&pipe, "test_pipe", buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(XY_PIPE_OK, result);
    TEST_ASSERT_EQUAL_PTR(buffer, pipe.buffer);
    TEST_ASSERT_EQUAL(sizeof(buffer), pipe.size);
    TEST_ASSERT_EQUAL(0, pipe.count);
    TEST_ASSERT_FALSE(pipe.full);

    /* Test invalid parameters */
    TEST_ASSERT_EQUAL(XY_PIPE_INVALID_PARAM, xy_pipe_init(NULL, "test", buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL(XY_PIPE_INVALID_PARAM, xy_pipe_init(&pipe, "test", NULL, sizeof(buffer)));
    TEST_ASSERT_EQUAL(XY_PIPE_INVALID_PARAM, xy_pipe_init(&pipe, "test", buffer, 0));
}

void test_pipe_deinit(void)
{
    xy_pipe_t pipe;
    uint8_t buffer[64];

    xy_pipe_init(&pipe, "test", buffer, sizeof(buffer));
    xy_pipe_write(&pipe, (const uint8_t *)"test", 4);

    int result = xy_pipe_deinit(&pipe);
    TEST_ASSERT_EQUAL(XY_PIPE_OK, result);
    TEST_ASSERT_EQUAL_PTR(NULL, pipe.buffer);
    TEST_ASSERT_EQUAL(0, pipe.size);
}

void test_pipe_write_read(void)
{
    uint8_t write_data[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
    uint8_t read_data[16];
    int result;

    /* Write data */
    result = xy_pipe_write(&test_pipe, write_data, sizeof(write_data));
    TEST_ASSERT_EQUAL(5, result);
    TEST_ASSERT_EQUAL(5, xy_pipe_available(&test_pipe));

    /* Read data */
    result = xy_pipe_read(&test_pipe, read_data, sizeof(read_data));
    TEST_ASSERT_EQUAL(5, result);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(write_data, read_data, 5);
    TEST_ASSERT_EQUAL(0, xy_pipe_available(&test_pipe));
}

void test_pipe_is_empty(void)
{
    TEST_ASSERT_TRUE(xy_pipe_is_empty(&test_pipe));

    xy_pipe_write(&test_pipe, (const uint8_t *)"x", 1);
    TEST_ASSERT_FALSE(xy_pipe_is_empty(&test_pipe));
}

void test_pipe_is_full(void)
{
    TEST_ASSERT_FALSE(xy_pipe_is_full(&test_pipe));

    /* Fill the pipe */
    uint8_t data[256];
    memset(data, 0, sizeof(data));
    xy_pipe_write(&test_pipe, data, sizeof(data));

    TEST_ASSERT_TRUE(xy_pipe_is_full(&test_pipe));
}

void test_pipe_clear(void)
{
    /* Write some data */
    xy_pipe_write(&test_pipe, (const uint8_t *)"test", 4);
    TEST_ASSERT_EQUAL(4, xy_pipe_available(&test_pipe));

    /* Clear */
    int result = xy_pipe_clear(&test_pipe);
    TEST_ASSERT_EQUAL(XY_PIPE_OK, result);
    TEST_ASSERT_EQUAL(0, xy_pipe_available(&test_pipe));
    TEST_ASSERT_TRUE(xy_pipe_is_empty(&test_pipe));
}

void test_pipe_peek(void)
{
    uint8_t write_data[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
    uint8_t peek_data[16];
    uint8_t read_data[16];

    xy_pipe_write(&test_pipe, write_data, sizeof(write_data));

    /* Peek (should not remove data) */
    int result = xy_pipe_peek(&test_pipe, peek_data, 3);
    TEST_ASSERT_EQUAL(3, result);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(write_data, peek_data, 3);
    TEST_ASSERT_EQUAL(5, xy_pipe_available(&test_pipe)); /* Still 5 bytes */

    /* Read (should remove data) */
    result = xy_pipe_read(&test_pipe, read_data, 3);
    TEST_ASSERT_EQUAL(3, result);
    TEST_ASSERT_EQUAL(2, xy_pipe_available(&test_pipe)); /* Now 2 bytes */
}

void test_pipe_buffer_full(void)
{
    xy_pipe_t small_pipe;
    uint8_t small_buffer[8];
    uint8_t large_data[16];

    xy_pipe_init(&small_pipe, "small", small_buffer, sizeof(small_buffer));
    memset(large_data, 0xAA, sizeof(large_data));

    /* Write until full */
    int result = xy_pipe_write(&small_pipe, large_data, sizeof(large_data));
    TEST_ASSERT_EQUAL(8, result); /* Only 8 bytes fit */
    TEST_ASSERT_TRUE(xy_pipe_is_full(&small_pipe));

    xy_pipe_deinit(&small_pipe);
}

void test_pipe_buffer_empty(void)
{
    uint8_t read_data[16];

    /* Try to read from empty pipe */
    int result = xy_pipe_read(&test_pipe, read_data, sizeof(read_data));
    TEST_ASSERT_EQUAL(XY_PIPE_BUFFER_EMPTY, result);
}

/* ==================== Observer Tests ==================== */

static int observer_call_count = 0;
static void *last_notification_data = NULL;

static void test_observer_callback(xy_subject_t *subject, const void *data, void *user_data)
{
    (void)subject;
    (void)user_data;
    observer_call_count++;
    last_notification_data = (void *)data;
}

void test_observer_init(void)
{
    xy_observer_t observer;
    int result;

    result = xy_observer_init(&observer, "test_obs", test_observer_callback, (void *)0x1234);
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, result);
    TEST_ASSERT_EQUAL_STRING("test_obs", observer.name);
    TEST_ASSERT_EQUAL_PTR(test_observer_callback, observer.callback);
    TEST_ASSERT_EQUAL_PTR((void *)0x1234, observer.user_data);
    TEST_ASSERT_TRUE(observer.active);
}

void test_subject_init(void)
{
    xy_subject_t subject;
    int result;

    result = xy_subject_init(&subject, "test_subj");
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, result);
    TEST_ASSERT_EQUAL_STRING("test_subj", subject.name);
    TEST_ASSERT_EQUAL(0, xy_subject_observer_count(&subject));
}

void test_subject_attach_detach(void)
{
    xy_subject_t subject;
    xy_observer_t observer1, observer2;
    int result;

    xy_subject_init(&subject, "test");
    xy_observer_init(&observer1, "obs1", test_observer_callback, NULL);
    xy_observer_init(&observer2, "obs2", test_observer_callback, NULL);

    /* Attach observers */
    result = xy_subject_attach(&subject, &observer1);
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, result);
    TEST_ASSERT_EQUAL(1, xy_subject_observer_count(&subject));

    result = xy_subject_attach(&subject, &observer2);
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, result);
    TEST_ASSERT_EQUAL(2, xy_subject_observer_count(&subject));

    /* Detach one observer */
    result = xy_subject_detach(&subject, &observer1);
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, result);
    TEST_ASSERT_EQUAL(1, xy_subject_observer_count(&subject));

    /* Detach non-existent observer */
    result = xy_subject_detach(&subject, &observer1);
    TEST_ASSERT_EQUAL(XY_OBSERVER_NOT_FOUND, result);
}

void test_subject_notify(void)
{
    xy_subject_t subject;
    xy_observer_t observer;
    int result;
    int test_data = 42;

    xy_subject_init(&subject, "test");
    xy_observer_init(&observer, "obs", test_observer_callback, NULL);

    xy_subject_attach(&subject, &observer);

    observer_call_count = 0;
    last_notification_data = NULL;

    /* Notify observers */
    result = xy_subject_notify(&subject, &test_data);
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, result);
    TEST_ASSERT_EQUAL(1, observer_call_count);
    TEST_ASSERT_EQUAL_PTR(&test_data, last_notification_data);
}

void test_subject_clear(void)
{
    xy_subject_t subject;
    xy_observer_t observer;
    int result;

    xy_subject_init(&subject, "test");
    xy_observer_init(&observer, "obs", test_observer_callback, NULL);

    xy_subject_attach(&subject, &observer);
    TEST_ASSERT_EQUAL(1, xy_subject_observer_count(&subject));

    result = xy_subject_clear(&subject);
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, result);
    TEST_ASSERT_EQUAL(0, xy_subject_observer_count(&subject));
}

void test_observer_invalid_params(void)
{
    /* Test NULL observer */
    TEST_ASSERT_EQUAL(XY_OBSERVER_INVALID_PARAM, xy_observer_init(NULL, "test", test_observer_callback, NULL));
    TEST_ASSERT_EQUAL(XY_OBSERVER_INVALID_PARAM, xy_observer_deinit(NULL));

    /* Test NULL subject */
    TEST_ASSERT_EQUAL(XY_OBSERVER_INVALID_PARAM, xy_subject_init(NULL, "test"));
    TEST_ASSERT_EQUAL(XY_OBSERVER_INVALID_PARAM, xy_subject_deinit(NULL));
    TEST_ASSERT_EQUAL(XY_OBSERVER_INVALID_PARAM, xy_subject_attach(NULL, NULL));
    TEST_ASSERT_EQUAL(XY_OBSERVER_INVALID_PARAM, xy_subject_detach(NULL, NULL));
    TEST_ASSERT_EQUAL(XY_OBSERVER_INVALID_PARAM, xy_subject_notify(NULL, NULL));
    TEST_ASSERT_EQUAL(XY_OBSERVER_INVALID_PARAM, xy_subject_clear(NULL));
}

/* ==================== Main ==================== */

int main(void)
{
    UNITY_BEGIN();

    /* Pipe Tests */
    RUN_TEST(test_pipe_init);
    RUN_TEST(test_pipe_deinit);
    RUN_TEST(test_pipe_write_read);
    RUN_TEST(test_pipe_is_empty);
    RUN_TEST(test_pipe_is_full);
    RUN_TEST(test_pipe_clear);
    RUN_TEST(test_pipe_peek);
    RUN_TEST(test_pipe_buffer_full);
    RUN_TEST(test_pipe_buffer_empty);

    /* Observer Tests */
    RUN_TEST(test_observer_init);
    RUN_TEST(test_subject_init);
    RUN_TEST(test_subject_attach_detach);
    RUN_TEST(test_subject_notify);
    RUN_TEST(test_subject_clear);
    RUN_TEST(test_observer_invalid_params);

    return UNITY_END();
}
