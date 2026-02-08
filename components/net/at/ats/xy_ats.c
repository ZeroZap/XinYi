/**
 * @file ats.c
 * @brief AT Command Server Implementation
 * @version 1.0.0
 */

#include "../ats.h"
#include "../../../osal/xy_os.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>


/* ==================== Global Variables ==================== */

static ats_t *g_at_server = NULL;

/* ==================== Private Functions ==================== */

static void ats_parser_thread(void *arg);
static int ats_getline(ats_t *server, uint32_t timeout);
static ats_cmd_t *ats_find_cmd(ats_t *server, const char *name);
static ats_cmd_mode_t ats_parse_cmd_mode(const char *cmd_line, char *cmd_name,
                                         char **args);
static int ats_execute_cmd(ats_t *server, const char *cmd_line);

/* ==================== Server Management ==================== */

ats_t *ats_create(const char *name)
{
    ats_t *server = (ats_t *)malloc(sizeof(ats_t));
    if (!server)
        return NULL;

    memset(server, 0, sizeof(ats_t));
    server->name      = name;
    server->status    = AT_SERVER_STATUS_UNINITIALIZED;
    server->echo_mode = AT_SERVER_ECHO_MODE;

    // Allocate command table
    server->cmd_table =
        (ats_cmd_t *)malloc(sizeof(ats_cmd_t) * AT_CMD_TABLE_MAX);
    if (!server->cmd_table) {
        free(server);
        return NULL;
    }

    memset(server->cmd_table, 0, sizeof(ats_cmd_t) * AT_CMD_TABLE_MAX);
    server->cmd_table_size = AT_CMD_TABLE_MAX;
    server->cmd_count      = 0;

    // Create OSAL primitives
    server->rx_notice = xy_os_semaphore_new(1, 0, NULL);
    if (!server->rx_notice) {
        free(server->cmd_table);
        free(server);
        return NULL;
    }

    server->status = AT_SERVER_STATUS_INITIALIZED;
    g_at_server    = server;

    return server;
}

int ats_init(ats_t *server, const char *name)
{
    if (!server)
        return -1;

    memset(server, 0, sizeof(ats_t));
    server->name      = name;
    server->status    = AT_SERVER_STATUS_INITIALIZED;
    server->echo_mode = AT_SERVER_ECHO_MODE;

    return 0;
}

void ats_delete(ats_t *server)
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

int ats_set_hal(ats_t *server, int (*get_char)(char *ch, uint32_t timeout),
                size_t (*send)(const char *data, size_t len))
{
    if (!server)
        return -1;

    server->get_char = get_char;
    server->send     = send;

    return 0;
}

int ats_start(ats_t *server)
{
    if (!server || !server->get_char || !server->send)
        return -1;

    // Create parser thread
    xy_os_thread_attr_t attr = {
        .name       = "at_srv_parser",
        .stack_size = AT_SERVER_THREAD_STACK_SIZE,
        .priority   = AT_SERVER_THREAD_PRIORITY,
    };

    server->parser_running = true;
    server->parser_thread  = xy_os_thread_new(ats_parser_thread, server, &attr);

    if (!server->parser_thread) {
        return -1;
    }

    server->status = AT_SERVER_STATUS_RUNNING;

    AT_DBG("Server started");
    return 0;
}

int ats_stop(ats_t *server)
{
    if (!server)
        return -1;

    server->parser_running = false;

    if (server->parser_thread) {
        xy_os_delay(100);
        xy_os_thread_terminate((xy_os_thread_id_t)server->parser_thread);
        server->parser_thread = NULL;
    }

    server->status = AT_SERVER_STATUS_INITIALIZED;
    return 0;
}

/* ==================== Command Registration ==================== */

int ats_register_cmd(ats_t *server, const ats_cmd_t *cmd)
{
    if (!server || !cmd || server->cmd_count >= server->cmd_table_size) {
        return -1;
    }

    // Check if command already exists
    if (ats_find_cmd(server, cmd->name)) {
        AT_DBG("Command %s already registered", cmd->name);
        return -1;
    }

    // Copy command to table
    memcpy(&server->cmd_table[server->cmd_count], cmd, sizeof(ats_cmd_t));
    server->cmd_count++;

    AT_DBG("Registered command: %s", cmd->name);
    return 0;
}

int ats_unregister_cmd(ats_t *server, const char *name)
{
    if (!server || !name)
        return -1;

    for (size_t i = 0; i < server->cmd_count; i++) {
        if (strcmp(server->cmd_table[i].name, name) == 0) {
            // Shift remaining commands
            for (size_t j = i; j < server->cmd_count - 1; j++) {
                memcpy(&server->cmd_table[j], &server->cmd_table[j + 1],
                       sizeof(ats_cmd_t));
            }
            server->cmd_count--;
            return 0;
        }
    }

    return -1;
}

/* ==================== Response Functions ==================== */

int ats_printf(ats_t *server, const char *format, ...)
{
    if (!server || !format)
        return -1;

    va_list args;
    va_start(args, format);
    int len =
        vsnprintf(server->send_buf, AT_SERVER_SEND_BUF_SIZE, format, args);
    va_end(args);

    if (len > 0 && len < AT_SERVER_SEND_BUF_SIZE) {
        if (server->send) {
            return server->send(server->send_buf, len);
        }
    }

    return -1;
}

int ats_printfln(ats_t *server, const char *format, ...)
{
    if (!server || !format)
        return -1;

    va_list args;
    va_start(args, format);
    int len =
        vsnprintf(server->send_buf, AT_SERVER_SEND_BUF_SIZE - 2, format, args);
    va_end(args);

    if (len > 0 && len < AT_SERVER_SEND_BUF_SIZE - 2) {
        strcpy(&server->send_buf[len], "\r\n");
        len += 2;

        if (server->send) {
            return server->send(server->send_buf, len);
        }
    }

    return -1;
}

int ats_print_result(ats_t *server, ats_result_t result)
{
    if (!server)
        return -1;

    const char *result_str = NULL;

    switch (result) {
    case AT_RESULT_OK:
        result_str = "\r\nOK\r\n";
        break;
    case AT_RESULT_FAIL:
    case AT_RESULT_CMD_ERR:
    case AT_RESULT_PARSE_ERR:
        result_str = "\r\nERROR\r\n";
        break;
    case AT_RESULT_NULL:
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

size_t ats_send(ats_t *server, const char *data, size_t len)
{
    if (!server || !data || !server->send)
        return 0;

    return server->send(data, len);
}

size_t ats_recv(ats_t *server, char *data, size_t len, uint32_t timeout)
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

int ats_parse_args(const char *args, const char *format, ...)
{
    if (!args || !format)
        return -1;

    va_list ap;
    va_start(ap, format);
    int result = vsscanf(args, format, ap);
    va_end(ap);

    return result;
}

int ats_parse_int(const char *args, int *value)
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

int ats_parse_string(const char *args, char *value, size_t max_len)
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

int ats_parse_hex(const char *args, uint32_t *value)
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

void ats_set_echo(ats_t *server, bool enable)
{
    if (server) {
        server->echo_mode = enable;
    }
}

bool ats_get_echo(ats_t *server)
{
    return server ? server->echo_mode : false;
}

/* ==================== Utility Functions ==================== */

void ats_get_stats(ats_t *server, uint32_t *cmd_processed, uint32_t *cmd_ok,
                   uint32_t *cmd_error)
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

void ats_reset_stats(ats_t *server)
{
    if (!server)
        return;

    server->cmd_processed = 0;
    server->cmd_ok        = 0;
    server->cmd_error     = 0;
}

ats_t *ats_get_by_name(const char *name)
{
    if (g_at_server && name && strcmp(g_at_server->name, name) == 0) {
        return g_at_server;
    }
    return NULL;
}

/* ==================== Parser Thread ==================== */

static void ats_parser_thread(void *arg)
{
    ats_t *server = (ats_t *)arg;

    while (server->parser_running) {
        // Get command line
        int ret = ats_getline(server, 500);

        if (ret > 0) {
            // Echo if enabled
            if (server->echo_mode && server->send) {
                server->send(server->recv_buf, server->recv_len);
            }

            // Execute command
            ats_execute_cmd(server, server->recv_buf);
        }

        xy_os_delay(1);
    }
}

static int ats_getline(ats_t *server, uint32_t timeout)
{
    if (!server || !server->get_char)
        return -1;

    char ch;
    server->recv_len = 0;
    memset(server->recv_buf, 0, AT_SERVER_RECV_BUF_SIZE);

    uint32_t start = xy_os_kernel_get_tick_count();

    while ((xy_os_kernel_get_tick_count() - start) < timeout) {
        if (server->get_char(&ch, 10) == 0) {
            if (server->recv_len < AT_SERVER_RECV_BUF_SIZE - 1) {
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

static ats_cmd_t *ats_find_cmd(ats_t *server, const char *name)
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

static ats_cmd_mode_t ats_parse_cmd_mode(const char *cmd_line, char *cmd_name,
                                         char **args)
{
    if (!cmd_line || !cmd_name)
        return AT_CMD_MODE_EXEC;

    const char *p = cmd_line;

    // Skip leading whitespace
    while (*p && isspace(*p))
        p++;

    // Extract command name (until '=', '?', or end)
    size_t len = 0;
    while (*p && !isspace(*p) && *p != '=' && *p != '?' && *p != '\r'
           && *p != '\n') {
        if (len < AT_CMD_NAME_MAX_LEN - 1) {
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
            return AT_CMD_MODE_TEST;
        } else {
            // Setup mode: AT+CMD=<args>
            if (args)
                *args = (char *)p;
            return AT_CMD_MODE_SETUP;
        }
    } else if (*p == '?') {
        // Query mode: AT+CMD?
        return AT_CMD_MODE_QUERY;
    } else {
        // Execute mode: AT+CMD
        return AT_CMD_MODE_EXEC;
    }
}

static int ats_execute_cmd(ats_t *server, const char *cmd_line)
{
    if (!server || !cmd_line)
        return -1;

    char cmd_name[AT_CMD_NAME_MAX_LEN];
    char *args = NULL;

    // Parse command mode and extract name
    ats_cmd_mode_t mode = ats_parse_cmd_mode(cmd_line, cmd_name, &args);

    AT_DBG("Cmd: %s, Mode: %d", cmd_name, mode);

    // Find command
    ats_cmd_t *cmd = ats_find_cmd(server, cmd_name);

    if (!cmd) {
        server->cmd_error++;
        ats_print_result(server, AT_RESULT_CMD_ERR);
        return -1;
    }

    server->cmd_processed++;

    // Execute based on mode
    ats_result_t result = AT_RESULT_CMD_ERR;

    switch (mode) {
    case AT_CMD_MODE_TEST:
        if (cmd->test) {
            result = cmd->test();
        }
        break;

    case AT_CMD_MODE_QUERY:
        if (cmd->query) {
            result = cmd->query();
        }
        break;

    case AT_CMD_MODE_SETUP:
        if (cmd->setup && args) {
            result = cmd->setup(args);
        }
        break;

    case AT_CMD_MODE_EXEC:
        if (cmd->exec) {
            result = cmd->exec();
        }
        break;
    }

    // Send result
    if (result == AT_RESULT_OK) {
        server->cmd_ok++;
    } else {
        server->cmd_error++;
    }

    ats_print_result(server, result);

    return 0;
}
