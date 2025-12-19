/**
 * @file xy_at_client.c
 * @brief AT Command Client Implementation
 * @version 1.0.0
 */

#include "../xy_at_client.h"
#include "../../../osal/xy_os.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#if XY_AT_DEBUG
#define AT_DBG(fmt, ...) printf("[AT_CLI] " fmt "\r\n", ##__VA_ARGS__)
#else
#define AT_DBG(fmt, ...)
#endif

/* ==================== Global Variables ==================== */

static xy_at_client_t *g_at_clients[XY_AT_CLIENT_NUM_MAX] = { NULL };

/* ==================== Private Functions ==================== */

static void xy_at_client_parser_thread(void *arg);
static int xy_at_client_getline(xy_at_client_t *client, uint32_t timeout);
static const xy_at_urc_t *xy_at_get_urc_obj(xy_at_client_t *client);
static int xy_at_client_recv_line(xy_at_client_t *client, uint32_t timeout);

/* ==================== Client Management ==================== */

xy_at_client_t *xy_at_client_create(const char *name, size_t send_buf_size,
                                    size_t recv_buf_size)
{
    xy_at_client_t *client = (xy_at_client_t *)malloc(sizeof(xy_at_client_t));
    if (!client)
        return NULL;

    memset(client, 0, sizeof(xy_at_client_t));
    client->name   = name;
    client->status = XY_AT_STATUS_UNINITIALIZED;

    // Allocate buffers
    client->send_buf      = (char *)malloc(send_buf_size);
    client->recv_line_buf = (char *)malloc(recv_buf_size);

    if (!client->send_buf || !client->recv_line_buf) {
        free(client->send_buf);
        free(client->recv_line_buf);
        free(client);
        return NULL;
    }

    client->send_buf_size  = send_buf_size;
    client->recv_line_size = recv_buf_size;

    // Create OSAL primitives
    client->lock        = xy_os_mutex_new(NULL);
    client->rx_notice   = xy_os_semaphore_new(1, 0, NULL);
    client->resp_notice = xy_os_semaphore_new(1, 0, NULL);

    if (!client->lock || !client->rx_notice || !client->resp_notice) {
        xy_at_client_delete(client);
        return NULL;
    }

    client->status = XY_AT_STATUS_INITIALIZED;

    // Register client
    for (int i = 0; i < XY_AT_CLIENT_NUM_MAX; i++) {
        if (g_at_clients[i] == NULL) {
            g_at_clients[i] = client;
            break;
        }
    }

    return client;
}

int xy_at_client_init(xy_at_client_t *client, const char *name)
{
    if (!client)
        return -1;

    memset(client, 0, sizeof(xy_at_client_t));
    client->name   = name;
    client->status = XY_AT_STATUS_INITIALIZED;

    return 0;
}

void xy_at_client_delete(xy_at_client_t *client)
{
    if (!client)
        return;

    // Stop parser thread
    if (client->parser_thread) {
        client->parser_running = false;
        xy_os_delay(100);
        xy_os_thread_terminate((xy_os_thread_id_t)client->parser_thread);
    }

    // Delete OSAL primitives
    if (client->lock)
        xy_os_mutex_delete((xy_os_mutex_id_t)client->lock);
    if (client->rx_notice)
        xy_os_semaphore_delete((xy_os_semaphore_id_t)client->rx_notice);
    if (client->resp_notice)
        xy_os_semaphore_delete((xy_os_semaphore_id_t)client->resp_notice);

    // Free buffers
    free(client->send_buf);
    free(client->recv_line_buf);

    // Unregister
    for (int i = 0; i < XY_AT_CLIENT_NUM_MAX; i++) {
        if (g_at_clients[i] == client) {
            g_at_clients[i] = NULL;
            break;
        }
    }

    free(client);
}

int xy_at_client_set_hal(xy_at_client_t *client,
                         int (*get_char)(char *ch, uint32_t timeout),
                         size_t (*send)(const char *data, size_t len),
                         size_t (*recv)(char *data, size_t len))
{
    if (!client)
        return -1;

    client->get_char = get_char;
    client->send     = send;
    client->recv     = recv;

    // Start parser thread
    xy_os_thread_attr_t attr = {
        .name       = "at_cli_parser",
        .stack_size = XY_AT_CLIENT_THREAD_STACK_SIZE,
        .priority   = XY_AT_CLIENT_THREAD_PRIORITY,
    };

    client->parser_running = true;
    client->parser_thread =
        xy_os_thread_new(xy_at_client_parser_thread, client, &attr);

    if (!client->parser_thread) {
        return -1;
    }

    client->status = XY_AT_STATUS_IDLE;
    return 0;
}

/* ==================== Response Management ==================== */

xy_at_response_t *xy_at_create_resp(size_t buf_size, size_t line_num,
                                    uint32_t timeout)
{
    xy_at_response_t *resp =
        (xy_at_response_t *)malloc(sizeof(xy_at_response_t));
    if (!resp)
        return NULL;

    resp->buf = (char *)malloc(buf_size);
    if (!resp->buf) {
        free(resp);
        return NULL;
    }

    memset(resp->buf, 0, buf_size);
    resp->buf_size    = buf_size;
    resp->buf_len     = 0;
    resp->line_num    = line_num;
    resp->line_counts = 0;
    resp->timeout     = timeout;

    return resp;
}

void xy_at_delete_resp(xy_at_response_t *resp)
{
    if (!resp)
        return;

    free(resp->buf);
    free(resp);
}

const char *xy_at_resp_get_line(xy_at_response_t *resp, size_t line_num)
{
    if (!resp || line_num >= resp->line_counts)
        return NULL;

    size_t line = 0;
    char *p     = resp->buf;

    while (line < line_num && *p) {
        if (*p == '\n')
            line++;
        p++;
    }

    return (*p) ? p : NULL;
}

const char *xy_at_resp_get_line_by_prefix(xy_at_response_t *resp,
                                          const char *prefix)
{
    if (!resp || !prefix)
        return NULL;

    char *p           = resp->buf;
    size_t prefix_len = strlen(prefix);

    while (*p) {
        if (strncmp(p, prefix, prefix_len) == 0) {
            return p;
        }
        // Move to next line
        while (*p && *p != '\n')
            p++;
        if (*p == '\n')
            p++;
    }

    return NULL;
}

int xy_at_resp_parse_line_args(const char *line, const char *format, ...)
{
    if (!line || !format)
        return -1;

    va_list args;
    va_start(args, format);
    int result = vsscanf(line, format, args);
    va_end(args);

    return result;
}

int xy_at_resp_parse_line_args_by_kw(const char *line, const char *keyword,
                                     const char *format, ...)
{
    if (!line || !keyword || !format)
        return -1;

    const char *p = strstr(line, keyword);
    if (!p)
        return -1;

    p += strlen(keyword);

    va_list args;
    va_start(args, format);
    int result = vsscanf(p, format, args);
    va_end(args);

    return result;
}

/* ==================== Command Execution ==================== */

xy_at_resp_status_t xy_at_exec_cmd(xy_at_client_t *client,
                                   xy_at_response_t *resp, const char *cmd_expr,
                                   ...)
{
    if (!client || !cmd_expr)
        return XY_AT_RESP_ERROR;

    // Lock
    xy_os_mutex_acquire((xy_os_mutex_id_t)client->lock, XY_OS_WAIT_FOREVER);

    // Format command
    va_list args;
    va_start(args, cmd_expr);
    int len =
        vsnprintf(client->send_buf, client->send_buf_size, cmd_expr, args);
    va_end(args);

    if (len < 0 || len >= client->send_buf_size) {
        xy_os_mutex_release((xy_os_mutex_id_t)client->lock);
        return XY_AT_RESP_ERROR;
    }

    // Add line ending if not present
    if (len >= 2 && strcmp(&client->send_buf[len - 2], "\r\n") != 0) {
        if (len + 2 < client->send_buf_size) {
            strcpy(&client->send_buf[len], "\r\n");
            len += 2;
        }
    }

    client->last_cmd_len = len;

#if XY_AT_PRINT_RAW_CMD
    AT_DBG("Send: %s", client->send_buf);
#endif

    // Set response
    client->resp        = resp;
    client->resp_status = XY_AT_RESP_OK;

    if (resp) {
        resp->buf_len     = 0;
        resp->line_counts = 0;
        memset(resp->buf, 0, resp->buf_size);
    }

    // Clear response notice
    xy_os_semaphore_acquire((xy_os_semaphore_id_t)client->resp_notice, 0);

    // Send command
    client->status = XY_AT_STATUS_BUSY;
    if (client->send) {
        client->send(client->send_buf, len);
        client->tx_count++;
    }

    // Wait for response
    uint32_t timeout      = resp ? resp->timeout : XY_AT_DEFAULT_TIMEOUT;
    xy_os_status_t result = xy_os_semaphore_acquire(
        (xy_os_semaphore_id_t)client->resp_notice, timeout);

    xy_at_resp_status_t status = client->resp_status;

    if (result == XY_OS_ERROR_TIMEOUT) {
        status = XY_AT_RESP_TIMEOUT;
        client->timeout_count++;
    }

    // Clear response
    client->resp   = NULL;
    client->status = XY_AT_STATUS_IDLE;

    // Unlock
    xy_os_mutex_release((xy_os_mutex_id_t)client->lock);

    return status;
}

/* ==================== Parser Thread ==================== */

static void xy_at_client_parser_thread(void *arg)
{
    xy_at_client_t *client = (xy_at_client_t *)arg;

    while (client->parser_running) {
        // Get line with timeout
        int ret = xy_at_client_getline(client, 500);

        if (ret > 0) {
            client->rx_count++;

            // Check for URC
            const xy_at_urc_t *urc = xy_at_get_urc_obj(client);

            if (urc && urc->func) {
                // Handle URC
                urc->func(client, client->recv_line_buf, client->recv_line_len);
            } else if (client->resp) {
                // Handle response
                xy_at_response_t *resp = client->resp;

                // Replace \r\n with \0
                if (client->recv_line_len >= 2
                    && client->recv_line_buf[client->recv_line_len - 2]
                           == '\r') {
                    client->recv_line_buf[client->recv_line_len - 2] = '\0';
                    client->recv_line_len -= 2;
                }

                // Check if buffer has space
                if (resp->buf_len + client->recv_line_len + 1
                    < resp->buf_size) {
                    // Copy line to response buffer
                    memcpy(resp->buf + resp->buf_len, client->recv_line_buf,
                           client->recv_line_len);
                    resp->buf_len += client->recv_line_len;
                    resp->buf[resp->buf_len++] = '\n';
                    resp->line_counts++;

                    // Check for end conditions
                    if (strcmp(client->recv_line_buf, XY_AT_RESP_OK_STR) == 0) {
                        client->resp_status = XY_AT_RESP_OK;
                        xy_os_semaphore_release(
                            (xy_os_semaphore_id_t)client->resp_notice);
                    } else if (strcmp(
                                   client->recv_line_buf, XY_AT_RESP_ERROR_STR)
                               == 0) {
                        client->resp_status = XY_AT_RESP_ERROR;
                        xy_os_semaphore_release(
                            (xy_os_semaphore_id_t)client->resp_notice);
                    } else if (resp->line_num > 0
                               && resp->line_counts >= resp->line_num) {
                        client->resp_status = XY_AT_RESP_OK;
                        xy_os_semaphore_release(
                            (xy_os_semaphore_id_t)client->resp_notice);
                    }
                } else {
                    // Buffer full
                    client->resp_status = XY_AT_RESP_BUFF_FULL;
                    xy_os_semaphore_release(
                        (xy_os_semaphore_id_t)client->resp_notice);
                }
            }
        }

        xy_os_delay(1);
    }
}

static int xy_at_client_getline(xy_at_client_t *client, uint32_t timeout)
{
    if (!client || !client->get_char)
        return -1;

    char ch;
    client->recv_line_len = 0;
    memset(client->recv_line_buf, 0, client->recv_line_size);

    uint32_t start = xy_os_kernel_get_tick_count();

    while ((xy_os_kernel_get_tick_count() - start) < timeout) {
        if (client->get_char(&ch, 10) == 0) {
            if (client->recv_line_len < client->recv_line_size - 1) {
                client->recv_line_buf[client->recv_line_len++] = ch;

                // Check for line end (\r\n)
                if (client->recv_line_len >= 2
                    && client->recv_line_buf[client->recv_line_len - 2] == '\r'
                    && client->recv_line_buf[client->recv_line_len - 1]
                           == '\n') {
                    client->recv_line_buf[client->recv_line_len] = '\0';
                    return client->recv_line_len;
                }

                // Check for end sign
                if (client->end_sign != 0 && ch == client->end_sign) {
                    client->recv_line_buf[client->recv_line_len] = '\0';
                    return client->recv_line_len;
                }
            } else {
                // Line buffer full
                return -1;
            }
        }
    }

    return 0; // Timeout
}

static const xy_at_urc_t *xy_at_get_urc_obj(xy_at_client_t *client)
{
    if (!client || !client->urc_table || client->urc_table_size == 0) {
        return NULL;
    }

    for (size_t i = 0; i < client->urc_table_size; i++) {
        const xy_at_urc_t *urc = &client->urc_table->urc[i];

        if (urc->prefix
            && strncmp(client->recv_line_buf, urc->prefix, strlen(urc->prefix))
                   == 0) {
            if (!urc->suffix) {
                return urc;
            }

            // Check suffix
            size_t line_len   = strlen(client->recv_line_buf);
            size_t suffix_len = strlen(urc->suffix);

            if (line_len >= suffix_len
                && strcmp(&client->recv_line_buf[line_len - suffix_len],
                          urc->suffix)
                       == 0) {
                return urc;
            }
        }
    }

    return NULL;
}

/* ==================== URC Management ==================== */

int xy_at_set_urc_table(xy_at_client_t *client, const xy_at_urc_t *urc_table,
                        size_t table_size)
{
    if (!client || !urc_table)
        return -1;

    client->urc_table = (xy_at_urc_table_t *)malloc(sizeof(xy_at_urc_table_t));
    if (!client->urc_table)
        return -1;

    client->urc_table->urc       = urc_table;
    client->urc_table->urc_count = table_size;
    client->urc_table_size       = table_size;

    return 0;
}

/* ==================== Data Mode ==================== */

int xy_at_client_enter_data_mode(xy_at_client_t *client)
{
    if (!client)
        return -1;

    client->status = XY_AT_STATUS_DATA_MODE;
    return 0;
}

int xy_at_client_exit_data_mode(xy_at_client_t *client)
{
    if (!client)
        return -1;

    // Send +++
    if (client->send) {
        client->send("+++", 3);
        xy_os_delay(1000);
    }

    client->status = XY_AT_STATUS_IDLE;
    return 0;
}

int xy_at_client_send_data(xy_at_client_t *client, const uint8_t *data,
                           size_t len)
{
    if (!client || !data || client->status != XY_AT_STATUS_DATA_MODE)
        return -1;

    if (client->send) {
        return client->send((const char *)data, len);
    }

    return -1;
}

int xy_at_client_recv_data(xy_at_client_t *client, uint8_t *data, size_t len,
                           uint32_t timeout)
{
    if (!client || !data || client->status != XY_AT_STATUS_DATA_MODE)
        return -1;

    if (client->recv) {
        uint32_t start = xy_os_kernel_get_tick_count();
        size_t total   = 0;

        while (total < len
               && (xy_os_kernel_get_tick_count() - start) < timeout) {
            size_t ret = client->recv((char *)&data[total], len - total);
            total += ret;

            if (ret == 0) {
                xy_os_delay(10);
            }
        }

        return total;
    }

    return -1;
}

/* ==================== Utility Functions ==================== */

void xy_at_client_get_stats(xy_at_client_t *client, uint32_t *tx_count,
                            uint32_t *rx_count, uint32_t *error_count,
                            uint32_t *timeout_count)
{
    if (!client)
        return;

    if (tx_count)
        *tx_count = client->tx_count;
    if (rx_count)
        *rx_count = client->rx_count;
    if (error_count)
        *error_count = client->error_count;
    if (timeout_count)
        *timeout_count = client->timeout_count;
}

void xy_at_client_reset_stats(xy_at_client_t *client)
{
    if (!client)
        return;

    client->tx_count      = 0;
    client->rx_count      = 0;
    client->error_count   = 0;
    client->timeout_count = 0;
}

int xy_at_client_wait_idle(xy_at_client_t *client, uint32_t timeout)
{
    if (!client)
        return -1;

    uint32_t start = xy_os_kernel_get_tick_count();

    while (client->status != XY_AT_STATUS_IDLE) {
        if ((xy_os_kernel_get_tick_count() - start) >= timeout) {
            return -1;
        }
        xy_os_delay(10);
    }

    return 0;
}

xy_at_client_t *xy_at_client_get_by_name(const char *name)
{
    if (!name)
        return NULL;

    for (int i = 0; i < XY_AT_CLIENT_NUM_MAX; i++) {
        if (g_at_clients[i] && strcmp(g_at_clients[i]->name, name) == 0) {
            return g_at_clients[i];
        }
    }

    return NULL;
}

xy_at_client_t *xy_at_client_get_first(void)
{
    return g_at_clients[0];
}
