/**
 * @file xy_pipe.c
 * @brief IPC Pipe Implementation
 * @version 1.0.0
 * @date 2026-02-28
 */

#include "xy_pipe.h"
#include <string.h>

#define XY_PIPE_LIFECYCLE_COOKIE 0x58595032U

static bool pipe_is_owned(const xy_pipe_t* pipe) {
    return pipe && pipe->lifecycle_cookie == XY_PIPE_LIFECYCLE_COOKIE && pipe->owner == pipe &&
           pipe->owned_buffer && pipe->owned_size > 0U;
}

static bool pipe_is_valid(const xy_pipe_t* pipe) {
    size_t expected_head;

    if (!pipe_is_owned(pipe) || pipe->buffer != pipe->owned_buffer ||
        pipe->size != pipe->owned_size || pipe->head >= pipe->size || pipe->tail >= pipe->size ||
        pipe->count > pipe->size || pipe->full != (pipe->count == pipe->size)) {
        return false;
    }

    expected_head = pipe->count >= pipe->size - pipe->tail ? pipe->count - (pipe->size - pipe->tail)
                                                           : pipe->tail + pipe->count;
    return pipe->head == expected_head;
}

/* ==================== Helper Macros ==================== */

#define INCREMENT_HEAD(pipe)                                                                       \
    do {                                                                                           \
        (pipe)->head++;                                                                            \
        if ((pipe)->head >= (pipe)->size) {                                                        \
            (pipe)->head = 0;                                                                      \
        }                                                                                          \
    } while (0)

#define INCREMENT_TAIL(pipe)                                                                       \
    do {                                                                                           \
        (pipe)->tail++;                                                                            \
        if ((pipe)->tail >= (pipe)->size) {                                                        \
            (pipe)->tail = 0;                                                                      \
        }                                                                                          \
    } while (0)

/* ==================== Implementation ==================== */

int xy_pipe_init(xy_pipe_t* pipe, const char* name, uint8_t* buffer, size_t size) {
    if (!pipe || !buffer || size == 0) {
        return XY_PIPE_INVALID_PARAM;
    }
    if (pipe_is_owned(pipe)) {
        return XY_PIPE_ERROR;
    }

    memset(pipe, 0, sizeof(*pipe));

    if (name) {
        strncpy(pipe->name, name, sizeof(pipe->name) - 1);
        pipe->name[sizeof(pipe->name) - 1] = '\0';
    }

    pipe->buffer = buffer;
    pipe->owned_buffer = buffer;
    pipe->size = size;
    pipe->owned_size = size;
    pipe->head = 0;
    pipe->tail = 0;
    pipe->count = 0;
    pipe->full = false;
    pipe->lifecycle_cookie = XY_PIPE_LIFECYCLE_COOKIE;
    pipe->owner = pipe;

    return XY_PIPE_OK;
}

int xy_pipe_deinit(xy_pipe_t* pipe) {
    if (!pipe) {
        return XY_PIPE_INVALID_PARAM;
    }
    if (!pipe_is_owned(pipe)) {
        return XY_PIPE_ERROR;
    }

    pipe->buffer = NULL;
    pipe->owned_buffer = NULL;
    pipe->size = 0;
    pipe->owned_size = 0;
    pipe->head = 0;
    pipe->tail = 0;
    pipe->count = 0;
    pipe->full = false;
    pipe->lifecycle_cookie = 0U;
    pipe->owner = NULL;

    return XY_PIPE_OK;
}

int xy_pipe_write(xy_pipe_t* pipe, const uint8_t* data, size_t len) {
    size_t written = 0;

    if (!pipe || !data || len == 0) {
        return XY_PIPE_INVALID_PARAM;
    }
    if (!pipe_is_valid(pipe)) {
        return XY_PIPE_ERROR;
    }

    while (written < len) {
        if (pipe->full) {
            /* Buffer is full, return bytes written so far */
            return written > 0 ? (int)written : XY_PIPE_BUFFER_FULL;
        }

        pipe->buffer[pipe->head] = data[written];
        INCREMENT_HEAD(pipe);
        written++;
        pipe->count++;
        pipe->full = pipe->count == pipe->size;
    }

    return (int)written;
}

int xy_pipe_read(xy_pipe_t* pipe, uint8_t* data, size_t len) {
    size_t read_count = 0;

    if (!pipe || !data || len == 0) {
        return XY_PIPE_INVALID_PARAM;
    }
    if (!pipe_is_valid(pipe)) {
        return XY_PIPE_ERROR;
    }

    while (read_count < len) {
        if (pipe->count == 0) {
            /* Buffer is empty, return bytes read so far */
            return read_count > 0 ? (int)read_count : XY_PIPE_BUFFER_EMPTY;
        }

        data[read_count] = pipe->buffer[pipe->tail];
        INCREMENT_TAIL(pipe);
        read_count++;
        pipe->count--;
        pipe->full = false;
    }

    return (int)read_count;
}

int xy_pipe_peek(xy_pipe_t* pipe, uint8_t* data, size_t len) {
    size_t peek_count = 0;
    size_t temp_tail;

    if (!pipe || !data || len == 0) {
        return XY_PIPE_INVALID_PARAM;
    }
    if (!pipe_is_valid(pipe)) {
        return XY_PIPE_ERROR;
    }

    temp_tail = pipe->tail;

    while (peek_count < len && peek_count < pipe->count) {
        data[peek_count] = pipe->buffer[temp_tail];
        temp_tail++;
        if (temp_tail >= pipe->size) {
            temp_tail = 0;
        }
        peek_count++;
    }

    return (int)peek_count;
}

size_t xy_pipe_available(xy_pipe_t* pipe) {
    if (!pipe_is_valid(pipe)) {
        return 0;
    }

    return pipe->count;
}

bool xy_pipe_is_empty(xy_pipe_t* pipe) {
    if (!pipe_is_valid(pipe)) {
        return true;
    }

    return pipe->count == 0;
}

bool xy_pipe_is_full(xy_pipe_t* pipe) {
    if (!pipe_is_valid(pipe)) {
        return false;
    }

    return pipe->full;
}

int xy_pipe_clear(xy_pipe_t* pipe) {
    if (!pipe) {
        return XY_PIPE_INVALID_PARAM;
    }
    if (!pipe_is_valid(pipe)) {
        return XY_PIPE_ERROR;
    }

    pipe->head = 0;
    pipe->tail = 0;
    pipe->count = 0;
    pipe->full = false;

    return XY_PIPE_OK;
}
