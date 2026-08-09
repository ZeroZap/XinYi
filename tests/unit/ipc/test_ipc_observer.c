#include <stdint.h>
#include <string.h>

#include "unity.h"
#include "xy_observer.h"

static xy_subject_t *last_subject;
static const void *last_data;
static void *last_user_data;
static int callback_count;

static xy_observer_t reentrant_observer;
static xy_observer_t *reentrant_detach_observer;
static xy_observer_t *reentrant_attach_observer;
static int reentrant_first_count;
static int reentrant_second_count;

static void capture_callback(xy_subject_t *subject, const void *data, void *user_data)
{
    last_subject = subject;
    last_data = data;
    last_user_data = user_data;
    callback_count++;
}

static void second_callback(xy_subject_t *subject, const void *data, void *user_data)
{
    (void)subject;
    (void)data;
    (void)user_data;
    callback_count += 10;
}

static void reentrant_detach_callback(xy_subject_t *subject, const void *data,
                                      void *user_data)
{
    (void)data;
    (void)user_data;

    reentrant_first_count++;
    if (reentrant_detach_observer) {
        TEST_ASSERT_TRUE(subject->notifying);
        TEST_ASSERT_EQUAL(XY_OBSERVER_OK,
                          xy_subject_detach(subject, reentrant_detach_observer));
        reentrant_detach_observer = NULL;
    }
}

static void reentrant_second_callback(xy_subject_t *subject, const void *data,
                                      void *user_data)
{
    (void)subject;
    (void)data;
    (void)user_data;

    reentrant_second_count++;
}

static void reentrant_attach_callback(xy_subject_t *subject, const void *data,
                                      void *user_data)
{
    (void)data;
    (void)user_data;

    reentrant_first_count++;
    if (reentrant_attach_observer) {
        TEST_ASSERT_TRUE(subject->notifying);
        TEST_ASSERT_EQUAL(XY_OBSERVER_OK,
                          xy_subject_attach(subject, reentrant_attach_observer));
        reentrant_attach_observer = NULL;
    }
}

static void reset_capture(void)
{
    last_subject = NULL;
    last_data = NULL;
    last_user_data = NULL;
    callback_count = 0;
    memset(&reentrant_observer, 0, sizeof(reentrant_observer));
    reentrant_detach_observer = NULL;
    reentrant_attach_observer = NULL;
    reentrant_first_count = 0;
    reentrant_second_count = 0;
}

static void test_observer_and_subject_init_guards(void)
{
    xy_observer_t observer;
    xy_subject_t subject;
    char long_name[64];

    memset(long_name, 'x', sizeof(long_name));
    long_name[sizeof(long_name) - 1U] = '\0';

    TEST_ASSERT_EQUAL(XY_OBSERVER_INVALID_PARAM,
                      xy_observer_init(NULL, "observer", capture_callback, NULL));
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK,
                      xy_observer_init(&observer, long_name, capture_callback,
                                       (void *)0x1234));
    TEST_ASSERT_EQUAL_UINT(31U, strlen(observer.name));
    TEST_ASSERT_EQUAL_PTR(capture_callback, observer.callback);
    TEST_ASSERT_EQUAL_PTR((void *)0x1234, observer.user_data);
    TEST_ASSERT_TRUE(observer.active);

    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_observer_deinit(&observer));
    TEST_ASSERT_NULL(observer.callback);
    TEST_ASSERT_NULL(observer.user_data);
    TEST_ASSERT_FALSE(observer.active);
    TEST_ASSERT_EQUAL(XY_OBSERVER_INVALID_PARAM, xy_observer_deinit(NULL));

    TEST_ASSERT_EQUAL(XY_OBSERVER_INVALID_PARAM, xy_subject_init(NULL, "subject"));
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_subject_init(&subject, long_name));
    TEST_ASSERT_EQUAL_UINT(31U, strlen(subject.name));
    TEST_ASSERT_EQUAL_UINT(0U, xy_subject_observer_count(&subject));
    TEST_ASSERT_FALSE(subject.notifying);
    TEST_ASSERT_EQUAL_UINT(0U, xy_subject_observer_count(NULL));
}

static void test_subject_attach_notify_detach_and_clear(void)
{
    xy_subject_t subject;
    xy_observer_t observer;
    xy_observer_t observer_copy;
    xy_observer_t other;
    const uint32_t payload = 0xA5A55A5AU;
    int user_context = 7;

    reset_capture();
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_subject_init(&subject, "subject"));
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK,
                      xy_observer_init(&observer, "observer", capture_callback,
                                       &user_context));
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK,
                      xy_observer_init(&other, "other", second_callback, NULL));

    observer_copy = observer;
    TEST_ASSERT_EQUAL(XY_OBSERVER_INVALID_PARAM, xy_subject_attach(NULL, &observer));
    TEST_ASSERT_EQUAL(XY_OBSERVER_INVALID_PARAM, xy_subject_attach(&subject, NULL));
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_subject_attach(&subject, &observer));
    TEST_ASSERT_EQUAL_UINT(1U, xy_subject_observer_count(&subject));
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_subject_attach(&subject, &observer_copy));
    TEST_ASSERT_EQUAL_UINT(1U, xy_subject_observer_count(&subject));
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_subject_attach(&subject, &other));
    TEST_ASSERT_EQUAL_UINT(2U, xy_subject_observer_count(&subject));

    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_subject_notify(&subject, &payload));
    TEST_ASSERT_EQUAL_INT(11, callback_count);
    TEST_ASSERT_EQUAL_PTR(&subject, last_subject);
    TEST_ASSERT_EQUAL_PTR(&payload, last_data);
    TEST_ASSERT_EQUAL_PTR(&user_context, last_user_data);
    TEST_ASSERT_FALSE(subject.notifying);

    TEST_ASSERT_EQUAL(XY_OBSERVER_INVALID_PARAM, xy_subject_detach(NULL, &observer));
    TEST_ASSERT_EQUAL(XY_OBSERVER_INVALID_PARAM, xy_subject_detach(&subject, NULL));
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_subject_detach(&subject, &observer));
    TEST_ASSERT_EQUAL_UINT(1U, xy_subject_observer_count(&subject));
    TEST_ASSERT_EQUAL(XY_OBSERVER_NOT_FOUND, xy_subject_detach(&subject, &observer));

    reset_capture();
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_subject_notify(&subject, &payload));
    TEST_ASSERT_EQUAL_INT(10, callback_count);
    TEST_ASSERT_NULL(last_subject);

    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_subject_clear(&subject));
    TEST_ASSERT_EQUAL_UINT(0U, xy_subject_observer_count(&subject));
    TEST_ASSERT_EQUAL(XY_OBSERVER_INVALID_PARAM, xy_subject_clear(NULL));
    TEST_ASSERT_EQUAL(XY_OBSERVER_INVALID_PARAM, xy_subject_notify(NULL, &payload));
}

static void test_subject_clear_resets_notifying_state(void)
{
    xy_subject_t subject;

    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_subject_init(&subject, "subject"));

    subject.notifying = true;
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_subject_clear(&subject));
    TEST_ASSERT_EQUAL_UINT(0U, xy_subject_observer_count(&subject));
    TEST_ASSERT_FALSE(subject.notifying);

    subject.notifying = true;
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_subject_deinit(&subject));
    TEST_ASSERT_EQUAL_UINT(0U, xy_subject_observer_count(&subject));
    TEST_ASSERT_FALSE(subject.notifying);
}

static void test_subject_allows_same_callback_with_distinct_user_data(void)
{
    xy_subject_t subject;
    xy_observer_t first;
    xy_observer_t second;
    xy_observer_t same_as_first;
    const uint32_t payload = 0x11223344U;
    int first_context = 1;
    int second_context = 2;

    reset_capture();
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_subject_init(&subject, "subject"));
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK,
                      xy_observer_init(&first, "first", capture_callback,
                                       &first_context));
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK,
                      xy_observer_init(&second, "second", capture_callback,
                                       &second_context));

    same_as_first = first;
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_subject_attach(&subject, &first));
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_subject_attach(&subject, &same_as_first));
    TEST_ASSERT_EQUAL_UINT(1U, xy_subject_observer_count(&subject));

    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_subject_attach(&subject, &second));
    TEST_ASSERT_EQUAL_UINT(2U, xy_subject_observer_count(&subject));

    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_subject_notify(&subject, &payload));
    TEST_ASSERT_EQUAL_INT(2, callback_count);
    TEST_ASSERT_EQUAL_PTR(&subject, last_subject);
    TEST_ASSERT_EQUAL_PTR(&payload, last_data);
    TEST_ASSERT_EQUAL_PTR(&second_context, last_user_data);
}

static void test_subject_rejects_inactive_or_callbackless_observers(void)
{
    xy_subject_t subject;
    xy_observer_t observer;

    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_subject_init(&subject, "subject"));
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_observer_init(&observer, "inactive", NULL, NULL));
    TEST_ASSERT_EQUAL(XY_OBSERVER_INVALID_PARAM, xy_subject_attach(&subject, &observer));
    TEST_ASSERT_EQUAL_UINT(0U, xy_subject_observer_count(&subject));

    TEST_ASSERT_EQUAL(XY_OBSERVER_OK,
                      xy_observer_init(&observer, "observer", capture_callback, NULL));
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_observer_deinit(&observer));
    TEST_ASSERT_EQUAL(XY_OBSERVER_INVALID_PARAM, xy_subject_attach(&subject, &observer));
    TEST_ASSERT_EQUAL_UINT(0U, xy_subject_observer_count(&subject));
}

static void test_subject_notify_recovers_notifying_state_after_callback_mutates_active_flag(void)
{
    xy_subject_t subject;
    xy_observer_t observer;
    const uint32_t payload = 0xCAFEBABEU;

    reset_capture();
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_subject_init(&subject, "subject"));
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK,
                      xy_observer_init(&observer, "observer", capture_callback, NULL));
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_subject_attach(&subject, &observer));

    subject.observers[0].active = false;
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_subject_notify(&subject, &payload));
    TEST_ASSERT_FALSE(subject.notifying);
    TEST_ASSERT_EQUAL_INT(0, callback_count);
}

static void test_subject_notify_tolerates_observer_detach_during_callback(void)
{
    xy_subject_t subject;
    xy_observer_t first;
    xy_observer_t second;
    const uint32_t payload = 0x5A5AA5A5U;

    reset_capture();
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_subject_init(&subject, "subject"));
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK,
                      xy_observer_init(&first, "first", reentrant_detach_callback, NULL));
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK,
                      xy_observer_init(&second, "second", reentrant_second_callback, NULL));

    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_subject_attach(&subject, &first));
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_subject_attach(&subject, &second));
    reentrant_observer = second;
    reentrant_detach_observer = &reentrant_observer;

    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_subject_notify(&subject, &payload));
    TEST_ASSERT_FALSE(subject.notifying);
    TEST_ASSERT_EQUAL_UINT(1U, xy_subject_observer_count(&subject));
    TEST_ASSERT_EQUAL_INT(1, reentrant_first_count);
    TEST_ASSERT_EQUAL_INT(0, reentrant_second_count);

    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_subject_notify(&subject, &payload));
    TEST_ASSERT_FALSE(subject.notifying);
    TEST_ASSERT_EQUAL_UINT(1U, xy_subject_observer_count(&subject));
    TEST_ASSERT_EQUAL_INT(2, reentrant_first_count);
    TEST_ASSERT_EQUAL_INT(0, reentrant_second_count);
}

static void test_subject_notify_defers_reentrant_attach_until_next_cycle(void)
{
    xy_subject_t subject;
    xy_observer_t first;
    xy_observer_t second;
    const uint32_t payload = 0xA55A5AA5U;

    reset_capture();
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_subject_init(&subject, "subject"));
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK,
                      xy_observer_init(&first, "first", reentrant_attach_callback, NULL));
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK,
                      xy_observer_init(&second, "second", reentrant_second_callback, NULL));

    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_subject_attach(&subject, &first));
    reentrant_observer = second;
    reentrant_attach_observer = &reentrant_observer;

    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_subject_notify(&subject, &payload));
    TEST_ASSERT_FALSE(subject.notifying);
    TEST_ASSERT_EQUAL_UINT(2U, xy_subject_observer_count(&subject));
    TEST_ASSERT_EQUAL_INT(1, reentrant_first_count);
    TEST_ASSERT_EQUAL_INT(0, reentrant_second_count);

    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_subject_notify(&subject, &payload));
    TEST_ASSERT_FALSE(subject.notifying);
    TEST_ASSERT_EQUAL_UINT(2U, xy_subject_observer_count(&subject));
    TEST_ASSERT_EQUAL_INT(2, reentrant_first_count);
    TEST_ASSERT_EQUAL_INT(1, reentrant_second_count);
}

static void test_subject_capacity_and_deinit(void)
{
    xy_subject_t subject;
    xy_observer_t observer;

    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_subject_init(&subject, "subject"));

    for (size_t i = 0; i < XY_OBSERVER_MAX_OBSERVERS; i++) {
        TEST_ASSERT_EQUAL(XY_OBSERVER_OK,
                          xy_observer_init(&observer, "slot", capture_callback,
                                           (void *)(uintptr_t)i));
        observer.callback = (i == 0U) ? capture_callback : second_callback;
        if (i > 1U) {
            observer.callback = (observer_callback_t)(uintptr_t)(0x1000U + i);
        }
        TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_subject_attach(&subject, &observer));
    }

    TEST_ASSERT_EQUAL_UINT(XY_OBSERVER_MAX_OBSERVERS, xy_subject_observer_count(&subject));
    TEST_ASSERT_EQUAL(XY_OBSERVER_FULL, xy_subject_attach(&subject, &observer));
    TEST_ASSERT_EQUAL(XY_OBSERVER_OK, xy_subject_deinit(&subject));
    TEST_ASSERT_EQUAL_UINT(0U, xy_subject_observer_count(&subject));
    TEST_ASSERT_EQUAL(XY_OBSERVER_INVALID_PARAM, xy_subject_deinit(NULL));
}

void setUp(void)
{
}

void tearDown(void)
{
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_observer_and_subject_init_guards);
    RUN_TEST(test_subject_attach_notify_detach_and_clear);
    RUN_TEST(test_subject_clear_resets_notifying_state);
    RUN_TEST(test_subject_allows_same_callback_with_distinct_user_data);
    RUN_TEST(test_subject_rejects_inactive_or_callbackless_observers);
    RUN_TEST(test_subject_notify_recovers_notifying_state_after_callback_mutates_active_flag);
    RUN_TEST(test_subject_notify_tolerates_observer_detach_during_callback);
    RUN_TEST(test_subject_notify_defers_reentrant_attach_until_next_cycle);
    RUN_TEST(test_subject_capacity_and_deinit);
    return UNITY_END();
}
