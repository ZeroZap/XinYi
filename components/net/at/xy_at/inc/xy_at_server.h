/**
 * @file xy_at_server.h
 * @brief AT Command Server Implementation for XinYi Framework
 * @version 1.0.0
 * @date 2025-10-27
 *
 * @note This implementation uses XY OSAL for cross-platform compatibility
 */

#ifndef _XY_AT_SERVER_H_
#define _XY_AT_SERVER_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Configuration ==================== */

#ifndef XY_AT_SERVER_RECV_BUF_SIZE
#define XY_AT_SERVER_RECV_BUF_SIZE 256 ///< Receive buffer size
#endif

#ifndef XY_AT_SERVER_SEND_BUF_SIZE
#define XY_AT_SERVER_SEND_BUF_SIZE 512 ///< Send buffer size
#endif

#ifndef XY_AT_CMD_NAME_MAX_LEN
#define XY_AT_CMD_NAME_MAX_LEN 16 ///< Maximum command name length
#endif

#ifndef XY_AT_CMD_TABLE_MAX
#define XY_AT_CMD_TABLE_MAX 32 ///< Maximum command table size
#endif

/* ==================== Result Codes ==================== */

/**
 * @brief AT command result
 */
typedef enum {
    XY_AT_RESULT_OK        = 0,  ///< Command executed successfully
    XY_AT_RESULT_FAIL      = -1, ///< Command execution failed
    XY_AT_RESULT_NULL      = -2, ///< No result to return
    XY_AT_RESULT_CMD_ERR   = -3, ///< Command format error
    XY_AT_RESULT_PARSE_ERR = -4, ///< Parameter parse error
} xy_at_result_t;

/**
 * @brief AT command mode
 */
typedef enum {
    XY_AT_CMD_MODE_TEST,  ///< Test mode (AT+CMD=?)
    XY_AT_CMD_MODE_QUERY, ///< Query mode (AT+CMD?)
    XY_AT_CMD_MODE_SETUP, ///< Setup mode (AT+CMD=<params>)
    XY_AT_CMD_MODE_EXEC,  ///< Execute mode (AT+CMD)
} xy_at_cmd_mode_t;

/* ==================== AT Command Structure ==================== */

/**
 * @brief AT command structure
 */
typedef struct xy_at_cmd {
    char name[XY_AT_CMD_NAME_MAX_LEN]; ///< Command name (e.g., "AT+CMD")
    const char *args_expr;             ///< Argument expression (optional)

    // Command handlers
    xy_at_result_t (*test)(void);  ///< Test mode handler (AT+CMD=?)
    xy_at_result_t (*query)(void); ///< Query mode handler (AT+CMD?)
    xy_at_result_t (*setup)(
        const char *args);        ///< Setup mode handler (AT+CMD=<args>)
    xy_at_result_t (*exec)(void); ///< Execute mode handler (AT+CMD)
} xy_at_cmd_t;

/**
 * @brief AT server status
 */
typedef enum {
    XY_AT_SERVER_STATUS_UNINITIALIZED = 0,
    XY_AT_SERVER_STATUS_INITIALIZED,
    XY_AT_SERVER_STATUS_RUNNING,
} xy_at_server_status_t;

/**
 * @brief AT server structure
 */
typedef struct xy_at_server {
    const char *name;             ///< Server name
    xy_at_server_status_t status; ///< Server status
    bool echo_mode;               ///< Echo mode enabled

    // HAL interface
    int (*get_char)(char *ch, uint32_t timeout);  ///< Get character from device
    size_t (*send)(const char *data, size_t len); ///< Send data to device

    // Buffers
    char send_buf[XY_AT_SERVER_SEND_BUF_SIZE];
    char recv_buf[XY_AT_SERVER_RECV_BUF_SIZE];
    size_t recv_len;

    // OSAL primitives
    void *rx_notice;     ///< Semaphore for RX notification
    void *parser_thread; ///< Parser thread handle
    bool parser_running;

    // Command table
    xy_at_cmd_t *cmd_table;
    size_t cmd_table_size;
    size_t cmd_count;

    // Statistics
    uint32_t cmd_processed;
    uint32_t cmd_ok;
    uint32_t cmd_error;
} xy_at_server_t;

/* ==================== Server Management ==================== */

/**
 * @brief Create AT server
 * @param name Server name
 * @return Pointer to server or NULL on error
 */
xy_at_server_t *xy_at_server_create(const char *name);

/**
 * @brief Initialize AT server
 * @param server Pointer to server structure
 * @param name Server name
 * @return 0 on success, -1 on error
 */
int xy_at_server_init(xy_at_server_t *server, const char *name);

/**
 * @brief Delete AT server
 * @param server Pointer to server
 */
void xy_at_server_delete(xy_at_server_t *server);

/**
 * @brief Set AT server HAL interface
 * @param server Pointer to server
 * @param get_char Get character function
 * @param send Send function
 * @return 0 on success, -1 on error
 */
int xy_at_server_set_hal(xy_at_server_t *server,
                         int (*get_char)(char *ch, uint32_t timeout),
                         size_t (*send)(const char *data, size_t len));

/**
 * @brief Start AT server
 * @param server Pointer to server
 * @return 0 on success, -1 on error
 */
int xy_at_server_start(xy_at_server_t *server);

/**
 * @brief Stop AT server
 * @param server Pointer to server
 * @return 0 on success, -1 on error
 */
int xy_at_server_stop(xy_at_server_t *server);

/* ==================== Command Registration ==================== */

/**
 * @brief Register AT command
 * @param server Pointer to server
 * @param cmd Pointer to command structure
 * @return 0 on success, -1 on error
 */
int xy_at_server_register_cmd(xy_at_server_t *server, const xy_at_cmd_t *cmd);

/**
 * @brief Unregister AT command
 * @param server Pointer to server
 * @param name Command name
 * @return 0 on success, -1 on error
 */
int xy_at_server_unregister_cmd(xy_at_server_t *server, const char *name);

/**
 * @brief Macro to define and register AT command
 * @param _name Command name
 * @param _args Argument expression
 * @param _test Test mode handler
 * @param _query Query mode handler
 * @param _setup Setup mode handler
 * @param _exec Execute mode handler
 */
#define XY_AT_CMD_EXPORT(_name, _args, _test, _query, _setup, _exec) \
    static const xy_at_cmd_t __xy_at_cmd_##_name = {                 \
        .name      = #_name,                                         \
        .args_expr = _args,                                          \
        .test      = _test,                                          \
        .query     = _query,                                         \
        .setup     = _setup,                                         \
        .exec      = _exec,                                          \
    }

/* ==================== Response Functions ==================== */

/**
 * @brief Send formatted response
 * @param server Pointer to server
 * @param format Format string
 * @param ... Variable arguments
 * @return Number of bytes sent
 */
int xy_at_server_printf(xy_at_server_t *server, const char *format, ...);

/**
 * @brief Send formatted response with newline
 * @param server Pointer to server
 * @param format Format string
 * @param ... Variable arguments
 * @return Number of bytes sent
 */
int xy_at_server_printfln(xy_at_server_t *server, const char *format, ...);

/**
 * @brief Send result code
 * @param server Pointer to server
 * @param result Result code
 * @return Number of bytes sent
 */
int xy_at_server_print_result(xy_at_server_t *server, xy_at_result_t result);

/**
 * @brief Send raw data
 * @param server Pointer to server
 * @param data Data buffer
 * @param len Data length
 * @return Number of bytes sent
 */
size_t xy_at_server_send(xy_at_server_t *server, const char *data, size_t len);

/**
 * @brief Receive data with timeout
 * @param server Pointer to server
 * @param data Data buffer
 * @param len Maximum length
 * @param timeout Timeout in milliseconds
 * @return Number of bytes received
 */
size_t xy_at_server_recv(xy_at_server_t *server, char *data, size_t len,
                         uint32_t timeout);

/* ==================== Parameter Parsing ==================== */

/**
 * @brief Parse command arguments
 * @param args Argument string
 * @param format Format string (e.g., "%d,%s")
 * @param ... Variable arguments
 * @return Number of parsed arguments
 */
int xy_at_parse_args(const char *args, const char *format, ...);

/**
 * @brief Parse integer parameter
 * @param args Argument string
 * @param value Output: parsed value
 * @return 0 on success, -1 on error
 */
int xy_at_parse_int(const char *args, int *value);

/**
 * @brief Parse string parameter
 * @param args Argument string
 * @param value Output: string buffer
 * @param max_len Maximum buffer length
 * @return 0 on success, -1 on error
 */
int xy_at_parse_string(const char *args, char *value, size_t max_len);

/**
 * @brief Parse hexadecimal parameter
 * @param args Argument string
 * @param value Output: parsed value
 * @return 0 on success, -1 on error
 */
int xy_at_parse_hex(const char *args, uint32_t *value);

/* ==================== Echo Mode ==================== */

/**
 * @brief Set echo mode
 * @param server Pointer to server
 * @param enable true to enable, false to disable
 */
void xy_at_server_set_echo(xy_at_server_t *server, bool enable);

/**
 * @brief Get echo mode
 * @param server Pointer to server
 * @return true if enabled, false otherwise
 */
bool xy_at_server_get_echo(xy_at_server_t *server);

/* ==================== Utility Functions ==================== */

/**
 * @brief Get server statistics
 * @param server Pointer to server
 * @param cmd_processed Output: processed count
 * @param cmd_ok Output: OK count
 * @param cmd_error Output: error count
 */
void xy_at_server_get_stats(xy_at_server_t *server, uint32_t *cmd_processed,
                            uint32_t *cmd_ok, uint32_t *cmd_error);

/**
 * @brief Reset server statistics
 * @param server Pointer to server
 */
void xy_at_server_reset_stats(xy_at_server_t *server);

/**
 * @brief Get server by name
 * @param name Server name
 * @return Pointer to server or NULL
 */
xy_at_server_t *xy_at_server_get_by_name(const char *name);

#ifdef __cplusplus
}
#endif

#endif /* _XY_AT_SERVER_H_ */
