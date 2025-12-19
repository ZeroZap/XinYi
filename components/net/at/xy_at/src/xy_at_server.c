/**
 * @file xy_at_server.c
 * @brief AT Command Server Implementation
 * @version 1.0.0
 */

#include "../xy_at_server.h"
#include "../../../osal/xy_os.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#if XY_AT_DEBUG
#define AT_DBG(fmt, ...) printf("[AT_SRV] " fmt "\r\n", ##__VA_ARGS__)
#else
#define AT_DBG(fmt, ...)
#endif

/* ==================== Global Variables ==================== */

static xy_at_server_t *g_at_server = NULL;

/* ==================== Private Functions ==================== */

static void xy_at_server_parser_thread(void *arg);
static int xy_at_server_getline(xy_at_server_t *server, uint32_t timeout);
static xy_at_cmd_t *xy_at_server_find_cmd(xy_at_server_t *server,
                                          const char *name);
static xy_at_cmd_mode_t
xy_at_server_parse_cmd_mode(const char *cmd_line, char *cmd_name, char **args);
static int xy_at_server_execute_cmd(xy_at_server_t *server,
                                    const char *cmd_line);

/* ==================== Server Management ==================== */

xy_at_server_t *xy_at_server_create(const char *name)
{
    xy_at_server_t *server = (xy_at_server_t *)malloc(sizeof(xy_at_server_t));
    if (!server)
        return NULL;

    memset(server, 0, sizeof(xy_at_server_t));
    server->name      = name;
    server->status    = XY_AT_SERVER_STATUS_UNINITIALIZED;
    server->echo_mode = XY_AT_SERVER_ECHO_MODE;

    // Allocate command table
    server->cmd_table =
        (xy_at_cmd_t *)malloc(sizeof(xy_at_cmd_t) * XY_AT_CMD_TABLE_MAX);
    if (!server->cmd_table) {
        free(server);
        return NULL;
    }

    memset(server->cmd_table, 0, sizeof(xy_at_cmd_t) * XY_AT_CMD_TABLE_MAX);
    server->cmd_table_size = XY_AT_CMD_TABLE_MAX;
    server->cmd_count      = 0;

    // Create OSAL primitives
    server->rx_notice = xy_os_semaphore_new(1, 0, NULL);
    if (!server->rx_notice) {
        free(server->cmd_table);
        free(server);
        return NULL;
    }

    server->status = XY_AT_SERVER_STATUS_INITIALIZED;
    g_at_server    = server;

    return server;
}

int xy_at_server_init(xy_at_server_t *server, const char *name)
{
    if (!server)
        return -1;

    memset(server, 0, sizeof(xy_at_server_t));
    server->name      = name;
    server->status    = XY_AT_SERVER_STATUS_INITIALIZED;
    server->echo_mode = XY_AT_SERVER_ECHO_MODE;

    return 0;
}

void xy_at_server_delete(xy_at_server_t *server)
{
    if (!server)
        return;

    // Stop parser thread
    if (server->parser_thread) {
        server->parser_running = false;
        xy_os_delay(100);
        xy_os_thread_terminate((xy_os_thread_id_t)server->parser_thread);
    }

    // Delete OSAL primitives
    if (server->rx_notice) {
        xy_os_semaphore_delete((xy_os_semaphore_id_t)server->rx_notice);
    }

    // Free command table
    free(server->cmd_table);

    if (g_at_server == server) {
        g_at_server = NULL;
    }

    free(server);
}

int xy_at_server_set_hal(xy_at_server_t *server,
                         int (*get_char)(char *ch, uint32_t timeout),
                         size_t (*send)(const char *data, size_t len))
{
    if (!server)
        return -1;

    server->get_char = get_char;
    server->send     = send;

    return 0;
}

int xy_at_server_start(xy_at_server_t *server)
{
    if (!server || !server->get_char || !server->send)
        return -1;

    // Create parser thread
    xy_os_thread_attr_t attr = {
        .name       = "at_srv_parser",
        .stack_size = XY_AT_SERVER_THREAD_STACK_SIZE,
        .priority   = XY_AT_SERVER_THREAD_PRIORITY,
    };

    server->parser_running = true;
    server->parser_thread =
        xy_os_thread_new(xy_at_server_parser_thread, server, &attr);

    if (!server->parser_thread) {
        return -1;
    }

    server->status = XY_AT_SERVER_STATUS_RUNNING;

    AT_DBG("Server started");
    return 0;
}

int xy_at_server_stop(xy_at_server_t *server)
{
    if (!server)
        return -1;

    server->parser_running = false;

    if (server->parser_thread) {
        xy_os_delay(100);
        xy_os_thread_terminate((xy_os_thread_id_t)server->parser_thread);
        server->parser_thread = NULL;
    }

    server->status = XY_AT_SERVER_STATUS_INITIALIZED;
    return 0;
}

/* ==================== Command Registration ==================== */

int xy_at_server_register_cmd(xy_at_server_t *server, const xy_at_cmd_t *cmd)
{
    if (!server || !cmd || server->cmd_count >= server->cmd_table_size) {
        return -1;
    }

    // Check if command already exists
    if (xy_at_server_find_cmd(server, cmd->name)) {
        AT_DBG("Command %s already registered", cmd->name);
        return -1;
    }

    // Copy command to table
    memcpy(&server->cmd_table[server->cmd_count], cmd, sizeof(xy_at_cmd_t));
    server->cmd_count++;

    AT_DBG("Registered command: %s", cmd->name);
    return 0;
}

int xy_at_server_unregister_cmd(xy_at_server_t *server, const char *name)
{
    if (!server || !name)
        return -1;

    for (size_t i = 0; i < server->cmd_count; i++) {
        if (strcmp(server->cmd_table[i].name, name) == 0) {
            // Shift remaining commands
            for (size_t j = i; j < server->cmd_count - 1; j++) {
                memcpy(&server->cmd_table[j], &server->cmd_table[j + 1],
                       sizeof(xy_at_cmd_t));
            }
            server->cmd_count--;
            return 0;
        }
    }

    return -1;
}

/* ==================== Response Functions ==================== */

int xy_at_server_printf(xy_at_server_t *server, const char *format, ...)
{
    if (!server || !format)
        return -1;

    va_list args;
    va_start(args, format);
    int len =
        vsnprintf(server->send_buf, XY_AT_SERVER_SEND_BUF_SIZE, format, args);
    va_end(args);

    if (len > 0 && len < XY_AT_SERVER_SEND_BUF_SIZE) {
        if (server->send) {
            return server->send(server->send_buf, len);
        }
    }

    return -1;
}

int xy_at_server_printfln(xy_at_server_t *server, const char *format, ...)
{
    if (!server || !format)
        return -1;

    va_list args;
    va_start(args, format);
    int len = vsnprintf(
        server->send_buf, XY_AT_SERVER_SEND_BUF_SIZE - 2, format, args);
    va_end(args);

    if (len > 0 && len < XY_AT_SERVER_SEND_BUF_SIZE - 2) {
        strcpy(&server->send_buf[len], "\r\n");
        len += 2;

        if (server->send) {
            return server->send(server->send_buf, len);
        }
    }

    return -1;
}

int xy_at_server_print_result(xy_at_server_t *server, xy_at_result_t result)
{
    if (!server)
        return -1;

    const char *result_str = NULL;

    switch (result) {
    case XY_AT_RESULT_OK:
        result_str = "\r\nOK\r\n";
        break;
    case XY_AT_RESULT_FAIL:
    case XY_AT_RESULT_CMD_ERR:
    case XY_AT_RESULT_PARSE_ERR:
        result_str = "\r\nERROR\r\n";
        break;
    case XY_AT_RESULT_NULL:
        return 0; // No output
    default:
        result_str = "\r\nERROR\r\n";
        break;
    }

    if (server->send && result_str) {
        return server->send(result_str, strlen(result_str));
    }

    return -1;
}

size_t xy_at_server_send(xy_at_server_t *server, const char *data, size_t len)
{
    if (!server || !data || !server->send)
        return 0;

    return server->send(data, len);
}

size_t xy_at_server_recv(xy_at_server_t *server, char *data, size_t len,
                         uint32_t timeout)
{
    if (!server || !data)
        return 0;

    uint32_t start = xy_os_kernel_get_tick_count();
    size_t count   = 0;
    char ch;

    while (count < len && (xy_os_kernel_get_tick_count() - start) < timeout) {
        if (server->get_char && server->get_char(&ch, 10) == 0) {
            data[count++] = ch;
        } else {
            xy_os_delay(1);
        }
    }

    return count;
}

/* ==================== Parameter Parsing ==================== */

int xy_at_parse_args(const char *args, const char *format, ...)
{
    if (!args || !format)
        return -1;

    va_list ap;
    va_start(ap, format);
    int result = vsscanf(args, format, ap);
    va_end(ap);

    return result;
}

int xy_at_parse_int(const char *args, int *value)
{
    if (!args || !value)
        return -1;

    // Skip whitespace
    while (*args && isspace(*args))
        args++;

    // Parse integer
    char *endptr;
    long val = strtol(args, &endptr, 10);

    if (endptr == args)
        return -1; // No conversion

    *value = (int)val;
    return 0;
}

int xy_at_parse_string(const char *args, char *value, size_t max_len)
{
    if (!args || !value || max_len == 0)
        return -1;

    // Skip whitespace
    while (*args && isspace(*args))
        args++;

    // Check for quotes
    bool quoted = (*args == '"');
    if (quoted)
        args++;

    size_t len = 0;
    while (*args && len < max_len - 1) {
        if (quoted && *args == '"')
            break;
        if (!quoted && (*args == ',' || isspace(*args)))
            break;

        value[len++] = *args++;
    }

    value[len] = '\0';
    return 0;
}

int xy_at_parse_hex(const char *args, uint32_t *value)
{
    if (!args || !value)
        return -1;

    // Skip whitespace
    while (*args && isspace(*args))
        args++;

    // Skip 0x prefix if present
    if (args[0] == '0' && (args[1] == 'x' || args[1] == 'X')) {
        args += 2;
    }

    char *endptr;
    unsigned long val = strtoul(args, &endptr, 16);

    if (endptr == args)
        return -1;

    *value = (uint32_t)val;
    return 0;
}

/* ==================== Echo Mode ==================== */

void xy_at_server_set_echo(xy_at_server_t *server, bool enable)
{
    if (server) {
        server->echo_mode = enable;
    }
}

bool xy_at_server_get_echo(xy_at_server_t *server)
{
    return server ? server->echo_mode : false;
}

/* ==================== Utility Functions ==================== */

void xy_at_server_get_stats(xy_at_server_t *server, uint32_t *cmd_processed,
                            uint32_t *cmd_ok, uint32_t *cmd_error)
{
    if (!server)
        return;

    if (cmd_processed)
        *cmd_processed = server->cmd_processed;
    if (cmd_ok)
        *cmd_ok = server->cmd_ok;
    if (cmd_error)
        *cmd_error = server->cmd_error;
}

void xy_at_server_reset_stats(xy_at_server_t *server)
{
    if (!server)
        return;

    server->cmd_processed = 0;
    server->cmd_ok        = 0;
    server->cmd_error     = 0;
}

xy_at_server_t *xy_at_server_get_by_name(const char *name)
{
    if (g_at_server && name && strcmp(g_at_server->name, name) == 0) {
        return g_at_server;
    }
    return NULL;
}

/* ==================== Parser Thread ==================== */

static void xy_at_server_parser_thread(void *arg)
{
    xy_at_server_t *server = (xy_at_server_t *)arg;

    while (server->parser_running) {
        // Get command line
        int ret = xy_at_server_getline(server, 500);

        if (ret > 0) {
            // Echo if enabled
            if (server->echo_mode && server->send) {
                server->send(server->recv_buf, server->recv_len);
            }

            // Execute command
            xy_at_server_execute_cmd(server, server->recv_buf);
        }

        xy_os_delay(1);
    }
}

static int xy_at_server_getline(xy_at_server_t *server, uint32_t timeout)
{
    if (!server || !server->get_char)
        return -1;

    char ch;
    server->recv_len = 0;
    memset(server->recv_buf, 0, XY_AT_SERVER_RECV_BUF_SIZE);

    uint32_t start = xy_os_kernel_get_tick_count();

    while ((xy_os_kernel_get_tick_count() - start) < timeout) {
        if (server->get_char(&ch, 10) == 0) {
            if (server->recv_len < XY_AT_SERVER_RECV_BUF_SIZE - 1) {
                server->recv_buf[server->recv_len++] = ch;

                // Check for line end (\r or \n)
                if (ch == '\r' || ch == '\n') {
                    if (server->recv_len > 1) {
                        server->recv_buf[server->recv_len] = '\0';
                        return server->recv_len;
                    } else {
                        // Empty line, reset
                        server->recv_len = 0;
                    }
                }
            }
        }
    }

    return 0;
}

static xy_at_cmd_t *xy_at_server_find_cmd(xy_at_server_t *server,
                                          const char *name)
{
    if (!server || !name)
        return NULL;

    for (size_t i = 0; i < server->cmd_count; i++) {
        if (strncmp(server->cmd_table[i].name, name,
                    strlen(server->cmd_table[i].name))
            == 0) {
            return &server->cmd_table[i];
        }
    }

    return NULL;
}

static xy_at_cmd_mode_t xy_at_server_parse_cmd_mode(const char *cmd_line,
                                                    char *cmd_name, char **args)
{
    if (!cmd_line || !cmd_name)
        return XY_AT_CMD_MODE_EXEC;

    const char *p = cmd_line;

    // Skip leading whitespace
    while (*p && isspace(*p))
        p++;

    // Extract command name (until '=', '?', or end)
    size_t len = 0;
    while (*p && !isspace(*p) && *p != '=' && *p != '?' && *p != '\r'
           && *p != '\n') {
        if (len < XY_AT_CMD_NAME_MAX_LEN - 1) {
            cmd_name[len++] = toupper(*p);
        }
        p++;
    }
    cmd_name[len] = '\0';

    // Skip whitespace after command name
    while (*p && isspace(*p))
        p++;

    // Determine mode
    if (*p == '=') {
        p++;
        if (*p == '?') {
            // Test mode: AT+CMD=?
            return XY_AT_CMD_MODE_TEST;
        } else {
            // Setup mode: AT+CMD=<args>
            if (args)
                *args = (char *)p;
            return XY_AT_CMD_MODE_SETUP;
        }
    } else if (*p == '?') {
        // Query mode: AT+CMD?
        return XY_AT_CMD_MODE_QUERY;
    } else {
        // Execute mode: AT+CMD
        return XY_AT_CMD_MODE_EXEC;
    }
}

static int xy_at_server_execute_cmd(xy_at_server_t *server,
                                    const char *cmd_line)
{
    if (!server || !cmd_line)
        return -1;

    char cmd_name[XY_AT_CMD_NAME_MAX_LEN];
    char *args = NULL;

    // Parse command mode and extract name
    xy_at_cmd_mode_t mode =
        xy_at_server_parse_cmd_mode(cmd_line, cmd_name, &args);

    AT_DBG("Cmd: %s, Mode: %d", cmd_name, mode);

    // Find command
    xy_at_cmd_t *cmd = xy_at_server_find_cmd(server, cmd_name);

    if (!cmd) {
        server->cmd_error++;
        xy_at_server_print_result(server, XY_AT_RESULT_CMD_ERR);
        return -1;
    }

    server->cmd_processed++;

    // Execute based on mode
    xy_at_result_t result = XY_AT_RESULT_CMD_ERR;

    switch (mode) {
    case XY_AT_CMD_MODE_TEST:
        if (cmd->test) {
            result = cmd->test();
        }
        break;

    case XY_AT_CMD_MODE_QUERY:
        if (cmd->query) {
            result = cmd->query();
        }
        break;

    case XY_AT_CMD_MODE_SETUP:
        if (cmd->setup && args) {
            result = cmd->setup(args);
        }
        break;

    case XY_AT_CMD_MODE_EXEC:
        if (cmd->exec) {
            result = cmd->exec();
        }
        break;
    }

    // Send result
    if (result == XY_AT_RESULT_OK) {
        server->cmd_ok++;
    } else {
        server->cmd_error++;
    }

    xy_at_server_print_result(server, result);

    return 0;
}
