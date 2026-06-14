#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "xy_pipe.h"

static void test_pipe_init_clear_deinit(void)
{
    xy_pipe_t pipe;
    uint8_t buffer[16];

    assert(xy_pipe_init(NULL, "pipe", buffer, sizeof(buffer)) == XY_PIPE_INVALID_PARAM);
    assert(xy_pipe_init(&pipe, "pipe", NULL, sizeof(buffer)) == XY_PIPE_INVALID_PARAM);
    assert(xy_pipe_init(&pipe, "pipe", buffer, 0) == XY_PIPE_INVALID_PARAM);

    assert(xy_pipe_init(&pipe, "pipe", buffer, sizeof(buffer)) == XY_PIPE_OK);
    assert(strcmp(pipe.name, "pipe") == 0);
    assert(pipe.buffer == buffer);
    assert(pipe.size == sizeof(buffer));
    assert(xy_pipe_available(&pipe) == 0);
    assert(xy_pipe_is_empty(&pipe));
    assert(!xy_pipe_is_full(&pipe));

    assert(xy_pipe_write(&pipe, (const uint8_t *)"abc", 3) == 3);
    assert(xy_pipe_available(&pipe) == 3);
    assert(xy_pipe_clear(&pipe) == XY_PIPE_OK);
    assert(xy_pipe_available(&pipe) == 0);
    assert(xy_pipe_is_empty(&pipe));

    assert(xy_pipe_deinit(&pipe) == XY_PIPE_OK);
    assert(pipe.buffer == NULL);
    assert(pipe.size == 0);
}

static void test_pipe_write_read_peek(void)
{
    xy_pipe_t pipe;
    uint8_t buffer[8];
    const uint8_t input[] = {1, 2, 3, 4, 5};
    uint8_t output[8] = {0};

    assert(xy_pipe_init(&pipe, "pipe", buffer, sizeof(buffer)) == XY_PIPE_OK);
    assert(xy_pipe_write(&pipe, input, sizeof(input)) == (int)sizeof(input));
    assert(xy_pipe_available(&pipe) == sizeof(input));

    assert(xy_pipe_peek(&pipe, output, 3) == 3);
    assert(memcmp(output, input, 3) == 0);
    assert(xy_pipe_available(&pipe) == sizeof(input));

    memset(output, 0, sizeof(output));
    assert(xy_pipe_read(&pipe, output, sizeof(output)) == (int)sizeof(input));
    assert(memcmp(output, input, sizeof(input)) == 0);
    assert(xy_pipe_is_empty(&pipe));
}

static void test_pipe_full_empty_and_wraparound(void)
{
    xy_pipe_t pipe;
    uint8_t buffer[4];
    const uint8_t first[] = {1, 2, 3, 4, 5};
    const uint8_t second[] = {6, 7};
    uint8_t output[4] = {0};

    assert(xy_pipe_init(&pipe, "pipe", buffer, sizeof(buffer)) == XY_PIPE_OK);
    assert(xy_pipe_read(&pipe, output, sizeof(output)) == XY_PIPE_BUFFER_EMPTY);
    assert(xy_pipe_write(&pipe, first, sizeof(first)) == 4);
    assert(xy_pipe_is_full(&pipe));

    assert(xy_pipe_read(&pipe, output, 2) == 2);
    assert(output[0] == 1 && output[1] == 2);
    assert(!xy_pipe_is_full(&pipe));

    assert(xy_pipe_write(&pipe, second, sizeof(second)) == 2);
    assert(xy_pipe_is_full(&pipe));

    memset(output, 0, sizeof(output));
    assert(xy_pipe_read(&pipe, output, sizeof(output)) == 4);
    assert(output[0] == 3 && output[1] == 4 && output[2] == 6 && output[3] == 7);
    assert(xy_pipe_is_empty(&pipe));
}

int main(void)
{
    test_pipe_init_clear_deinit();
    test_pipe_write_read_peek();
    test_pipe_full_empty_and_wraparound();
    return 0;
}
