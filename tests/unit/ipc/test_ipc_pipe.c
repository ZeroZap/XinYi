#include <stdint.h>
#include <string.h>

#include "unity.h"
#include "xy_pipe.h"

static void test_pipe_init_clear_deinit(void) {
    xy_pipe_t pipe = {0};
    uint8_t buffer[16];

    TEST_ASSERT_EQUAL(XY_PIPE_INVALID_PARAM, xy_pipe_init(NULL, "pipe", buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL(XY_PIPE_INVALID_PARAM, xy_pipe_init(&pipe, "pipe", NULL, sizeof(buffer)));
    TEST_ASSERT_EQUAL(XY_PIPE_INVALID_PARAM, xy_pipe_init(&pipe, "pipe", buffer, 0));

    TEST_ASSERT_EQUAL(XY_PIPE_OK, xy_pipe_init(&pipe, "pipe", buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_STRING("pipe", pipe.name);
    TEST_ASSERT_EQUAL_PTR(buffer, pipe.buffer);
    TEST_ASSERT_EQUAL_UINT32(sizeof(buffer), pipe.size);
    TEST_ASSERT_EQUAL_INT(0, xy_pipe_available(&pipe));
    TEST_ASSERT_TRUE(xy_pipe_is_empty(&pipe));
    TEST_ASSERT_FALSE(xy_pipe_is_full(&pipe));

    TEST_ASSERT_EQUAL_INT(3, xy_pipe_write(&pipe, (const uint8_t*)"abc", 3));
    TEST_ASSERT_EQUAL_INT(3, xy_pipe_available(&pipe));
    TEST_ASSERT_EQUAL(XY_PIPE_OK, xy_pipe_clear(&pipe));
    TEST_ASSERT_EQUAL_INT(0, xy_pipe_available(&pipe));
    TEST_ASSERT_TRUE(xy_pipe_is_empty(&pipe));

    TEST_ASSERT_EQUAL(XY_PIPE_OK, xy_pipe_deinit(&pipe));
    TEST_ASSERT_NULL(pipe.buffer);
    TEST_ASSERT_EQUAL_UINT32(0U, pipe.size);
}

static void test_pipe_write_read_peek(void) {
    xy_pipe_t pipe = {0};
    uint8_t buffer[8];
    const uint8_t input[] = {1, 2, 3, 4, 5};
    uint8_t output[8] = {0};

    TEST_ASSERT_EQUAL(XY_PIPE_OK, xy_pipe_init(&pipe, "pipe", buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_INT((int)sizeof(input), xy_pipe_write(&pipe, input, sizeof(input)));
    TEST_ASSERT_EQUAL_INT((int)sizeof(input), xy_pipe_available(&pipe));

    TEST_ASSERT_EQUAL_INT(3, xy_pipe_peek(&pipe, output, 3));
    TEST_ASSERT_EQUAL_MEMORY(input, output, 3);
    TEST_ASSERT_EQUAL_INT((int)sizeof(input), xy_pipe_available(&pipe));

    memset(output, 0, sizeof(output));
    TEST_ASSERT_EQUAL_INT((int)sizeof(input), xy_pipe_read(&pipe, output, sizeof(output)));
    TEST_ASSERT_EQUAL_MEMORY(input, output, sizeof(input));
    TEST_ASSERT_TRUE(xy_pipe_is_empty(&pipe));
}

static void test_pipe_full_empty_and_wraparound(void) {
    xy_pipe_t pipe = {0};
    uint8_t buffer[4];
    const uint8_t first[] = {1, 2, 3, 4, 5};
    const uint8_t second[] = {6, 7};
    uint8_t output[4] = {0};

    TEST_ASSERT_EQUAL(XY_PIPE_OK, xy_pipe_init(&pipe, "pipe", buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL(XY_PIPE_BUFFER_EMPTY, xy_pipe_read(&pipe, output, sizeof(output)));
    TEST_ASSERT_EQUAL_INT(4, xy_pipe_write(&pipe, first, sizeof(first)));
    TEST_ASSERT_TRUE(xy_pipe_is_full(&pipe));

    TEST_ASSERT_EQUAL_INT(2, xy_pipe_read(&pipe, output, 2));
    TEST_ASSERT_EQUAL_UINT8(1U, output[0]);
    TEST_ASSERT_EQUAL_UINT8(2U, output[1]);
    TEST_ASSERT_FALSE(xy_pipe_is_full(&pipe));

    TEST_ASSERT_EQUAL_INT(2, xy_pipe_write(&pipe, second, sizeof(second)));
    TEST_ASSERT_TRUE(xy_pipe_is_full(&pipe));

    memset(output, 0, sizeof(output));
    TEST_ASSERT_EQUAL_INT(4, xy_pipe_read(&pipe, output, sizeof(output)));
    TEST_ASSERT_EQUAL_UINT8(3U, output[0]);
    TEST_ASSERT_EQUAL_UINT8(4U, output[1]);
    TEST_ASSERT_EQUAL_UINT8(6U, output[2]);
    TEST_ASSERT_EQUAL_UINT8(7U, output[3]);
    TEST_ASSERT_TRUE(xy_pipe_is_empty(&pipe));
}

static void test_pipe_lifecycle_owner_and_reinit_fail_closed(void) {
    xy_pipe_t pipe = {0};
    xy_pipe_t before;
    xy_pipe_t alias;
    uint8_t buffer[8] = {0};
    uint8_t output = 0xA5U;

    TEST_ASSERT_EQUAL(XY_PIPE_OK, xy_pipe_init(&pipe, "pipe", buffer, sizeof(buffer)));
    before = pipe;
    TEST_ASSERT_EQUAL(XY_PIPE_ERROR, xy_pipe_init(&pipe, "replacement", buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_MEMORY(&before, &pipe, sizeof(pipe));

    alias = pipe;
    TEST_ASSERT_EQUAL(XY_PIPE_ERROR, xy_pipe_write(&alias, (const uint8_t*)"x", 1U));
    TEST_ASSERT_EQUAL(XY_PIPE_ERROR, xy_pipe_read(&alias, &output, 1U));
    TEST_ASSERT_EQUAL_UINT8(0xA5U, output);
    TEST_ASSERT_EQUAL(XY_PIPE_ERROR, xy_pipe_deinit(&alias));

    TEST_ASSERT_EQUAL_INT(1, xy_pipe_write(&pipe, (const uint8_t*)"x", 1U));
    TEST_ASSERT_EQUAL(XY_PIPE_OK, xy_pipe_deinit(&pipe));
}

static void test_pipe_public_geometry_corruption_preserves_owned_teardown(void) {
    xy_pipe_t pipe = {0};
    uint8_t buffer[4] = {0};
    uint8_t before[sizeof(buffer)];
    uint8_t output = 0xA5U;

    TEST_ASSERT_EQUAL(XY_PIPE_OK, xy_pipe_init(&pipe, "pipe", buffer, sizeof(buffer)));
    memcpy(before, buffer, sizeof(buffer));
    pipe.size++;

    TEST_ASSERT_EQUAL(XY_PIPE_ERROR, xy_pipe_write(&pipe, (const uint8_t*)"x", 1U));
    TEST_ASSERT_EQUAL_MEMORY(before, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(XY_PIPE_ERROR, xy_pipe_read(&pipe, &output, 1U));
    TEST_ASSERT_EQUAL_UINT8(0xA5U, output);
    TEST_ASSERT_EQUAL(XY_PIPE_ERROR, xy_pipe_peek(&pipe, &output, 1U));
    TEST_ASSERT_EQUAL_UINT8(0xA5U, output);
    TEST_ASSERT_EQUAL_UINT(0U, xy_pipe_available(&pipe));
    TEST_ASSERT_TRUE(xy_pipe_is_empty(&pipe));
    TEST_ASSERT_FALSE(xy_pipe_is_full(&pipe));
    TEST_ASSERT_EQUAL(XY_PIPE_ERROR, xy_pipe_clear(&pipe));

    TEST_ASSERT_EQUAL(XY_PIPE_OK, xy_pipe_deinit(&pipe));
    TEST_ASSERT_NULL(pipe.buffer);
    TEST_ASSERT_EQUAL_UINT(0U, pipe.size);
}

static void test_pipe_ring_state_corruption_fails_closed(void) {
    xy_pipe_t pipe = {0};
    uint8_t buffer[4] = {0};
    uint8_t before[sizeof(buffer)];
    uint8_t output = 0xA5U;

    TEST_ASSERT_EQUAL(XY_PIPE_OK, xy_pipe_init(&pipe, "pipe", buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_INT(2, xy_pipe_write(&pipe, (const uint8_t*)"ab", 2U));
    memcpy(before, buffer, sizeof(buffer));
    pipe.count = pipe.size + 1U;

    TEST_ASSERT_EQUAL(XY_PIPE_ERROR, xy_pipe_write(&pipe, (const uint8_t*)"x", 1U));
    TEST_ASSERT_EQUAL_MEMORY(before, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(XY_PIPE_ERROR, xy_pipe_read(&pipe, &output, 1U));
    TEST_ASSERT_EQUAL_UINT8(0xA5U, output);
    TEST_ASSERT_EQUAL_UINT(0U, xy_pipe_available(&pipe));
    TEST_ASSERT_TRUE(xy_pipe_is_empty(&pipe));
    TEST_ASSERT_FALSE(xy_pipe_is_full(&pipe));
    TEST_ASSERT_EQUAL(XY_PIPE_ERROR, xy_pipe_clear(&pipe));

    TEST_ASSERT_EQUAL(XY_PIPE_OK, xy_pipe_deinit(&pipe));
}

void setUp(void) {}

void tearDown(void) {}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_pipe_init_clear_deinit);
    RUN_TEST(test_pipe_write_read_peek);
    RUN_TEST(test_pipe_full_empty_and_wraparound);
    RUN_TEST(test_pipe_lifecycle_owner_and_reinit_fail_closed);
    RUN_TEST(test_pipe_public_geometry_corruption_preserves_owned_teardown);
    RUN_TEST(test_pipe_ring_state_corruption_fails_closed);
    return UNITY_END();
}
