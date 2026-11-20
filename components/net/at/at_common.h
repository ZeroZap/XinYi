#ifndef XY_AT_COMMON_H_
#define XY_AT_COMMON_H_

/**
 * @file at_common.h
 * @brief The vocabulary the AT client and the AT server both speak.
 *
 * An AT client sends commands to a modem and waits for answers. An AT server makes this board answer
 * commands. They are opposite ends of the same protocol, and both are used here - the client for talking
 * to a modem, the server for making a XinYi board look like a modem to something else.
 *
 * What they share is the vocabulary: the same response strings, the same line ending, the same command
 * prefix, and the same description of the transport underneath. That vocabulary used to be written out
 * twice, once per side, and the two copies were byte-for-byte identical. Two identical definitions are
 * not a redundancy to leave in place: a translation unit that includes both headers sees the same
 * `struct at_dev_ops` twice, and the guard that watches for duplicate definitions then has to carry an
 * exemption saying the duplication is intentional. One definition, included twice, is the same
 * behaviour with none of that.
 *
 * What is *not* here is anything specific to one side: the client's state machine, the server's
 * command table, their thread priorities. Those belong to their own headers.
 *
 * The definitions below are moved verbatim. Nothing about the wire format, the state names or the
 * transport contract changes - only where the text lives.
 */

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Response End Markers ==================== */

/**
 * @brief OK response string
 */
#ifndef AT_RESP_OK_STR
#define AT_RESP_OK_STR "OK"
#endif

/**
 * @brief ERROR response string
 */
#ifndef AT_RESP_ERROR_STR
#define AT_RESP_ERROR_STR "ERROR"
#endif

/**
 * @brief Line ending for commands
 */
#ifndef AT_CMD_LINE_END
#define AT_CMD_LINE_END "\r\n"
#endif

/**
 * @brief Command prefix
 */
#ifndef AT_CMD_PREFIX
#define AT_CMD_PREFIX "AT"
#endif

/* ==================== Transport ==================== */

/**
 * @brief The states a side of an AT link can be in.
 *
 * A client walks these while a command is in flight; a server walks them while a command is being
 * served. The names are shared because the states are shared - they are the same conversation seen
 * from the two ends.
 */
typedef enum {
    at_state_init,
    at_state_idle,
    at_state_sending,
    at_state_receiving,
    at_state_error,
    at_state_data_mode,
} at_state_t;

/**
 * @brief Buffer sizes and owner context for one side of an AT link.
 */
struct at_dev_attr {
    uint16_t recv_size;
    uint16_t send_size;
    void *user_data;
};
typedef struct at_dev_attr *at_dev_attr_t;

/**
 * @brief The transport an AT link runs over.
 *
 * Both sides talk to their transport the same way, which is why one description serves both.
 */
struct at_dev_ops {
    /** read from device ring buffer. */
    int (*read)(void *buffer, int len);
    /** write to device */
    int (*write)(const void *buffer, int len);
    /** cmd: power on, power off, enter sleep. */
    int (*control)(int cmd, void *args);
    /** dev ops attr */
    at_dev_attr_t attr;
};
typedef struct at_dev_ops *at_dev_ops_t;

#ifdef __cplusplus
}
#endif

#endif /* XY_AT_COMMON_H_ */
