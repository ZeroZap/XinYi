#include "at_handler.h"
#include <stdio.h>

// 初始化AT处理器
void at_init(at_hdlr_t *handler, UART_HandleTypeDef *huart)
{
    handler->huart             = huart;
    handler->state             = AT_STATE_IDLE;
    handler->resp_len          = 0;
    handler->initialized       = 1;
    handler->urc_handler_count = 0;

    // 初始化队列
    handler->cmd_queue.head  = 0;
    handler->cmd_queue.tail  = 0;
    handler->cmd_queue.count = 0;

    // 清空缓冲区
    memset(handler->resp_buffer, 0, AT_MAX_RESP_LEN);
    memset(&handler->current_cmd, 0, sizeof(at_cmd_t));

    // 初始化环形缓冲区
    handler->rx_ring_buffer.head  = 0;
    handler->rx_ring_buffer.tail  = 0;
    handler->rx_ring_buffer.count = 0;
    handler->processing           = 0;
}

// 发送AT命令
AT_Result_t at_send(at_hdlr_t *handler, const char *cmd,
                    const char *expect_resp, uint32_t timeout,
                    void (*callback)(AT_Result_t, const char *))
{
    if (!handler->initialized) {
        return AT_RESULT_ERROR;
    }

    at_cmd_t new_cmd;
    memset(&new_cmd, 0, sizeof(at_cmd_t));
    snprintf(new_cmd.cmd, AT_MAX_CMD_LEN, "%s\r\n", cmd);
    strncpy(new_cmd.expect_resp, expect_resp, 31);
    new_cmd.timeout  = timeout ? timeout : AT_DEFAULT_TIMEOUT;
    new_cmd.callback = callback;

    // 如果当前有命令在执行或队列不为空，加入队列
    if (handler->state != AT_STATE_IDLE || handler->cmd_queue.count > 0) {
        if (handler->cmd_queue.count >= AT_CMD_QUEUE_SIZE) {
            return AT_RESULT_ERROR;
        }

        handler->cmd_queue.queue[handler->cmd_queue.tail] = new_cmd;
        handler->cmd_queue.tail =
            (handler->cmd_queue.tail + 1) % AT_CMD_QUEUE_SIZE;
        handler->cmd_queue.count++;
        return AT_RESULT_OK;
    }

    // 直接发送命令
    memcpy(&handler->current_cmd, &new_cmd, sizeof(at_cmd_t));
    HAL_UART_Transmit(handler->huart, (uint8_t *)handler->current_cmd.cmd,
                      strlen(handler->current_cmd.cmd), HAL_MAX_DELAY);

    handler->state          = AT_STATE_SENDING;
    handler->last_send_time = HAL_GetTick();
    handler->resp_len       = 0;

    return AT_RESULT_OK;
}

// 主处理函数
void at_process(at_hdlr_t *handler)
{
    if (!handler->initialized)
        return;

    // 处理接收到的数据
    at_process_rx_data(handler);

    // 原有的状态机处理
    switch (handler->state) {
    case AT_STATE_SENDING:
        // 检查是否发送完成
        if (HAL_UART_GetState(handler->huart) == HAL_UART_STATE_READY) {
            handler->state = AT_STATE_WAITING_RESP;
        }
        break;

    case AT_STATE_WAITING_RESP:
        // 检查超时
        if (HAL_GetTick() - handler->last_send_time
            > handler->current_cmd.timeout) {
            handler->state              = AT_STATE_TIMEOUT;
            handler->current_cmd.result = AT_RESULT_TIMEOUT;

            // 重试逻辑
            if (handler->current_cmd.retry_count < AT_MAX_RETRY) {
                handler->current_cmd.retry_count++;
                HAL_UART_Transmit(
                    handler->huart, (uint8_t *)handler->current_cmd.cmd,
                    strlen(handler->current_cmd.cmd), HAL_MAX_DELAY);
                handler->last_send_time = HAL_GetTick();
                handler->state          = AT_STATE_SENDING;
            } else {
                // 重试次数用完，触发回调
                if (handler->current_cmd.callback) {
                    handler->current_cmd.callback(AT_RESULT_TIMEOUT, "Timeout");
                }
                handler->state = AT_STATE_IDLE;
            }
        }
        break;

    case AT_STATE_RESP_RECEIVED:
        // 处理响应
        if (strstr(handler->resp_buffer, handler->current_cmd.expect_resp)) {
            handler->current_cmd.result = AT_RESULT_OK;
        } else if (strstr(handler->resp_buffer, "ERROR")) {
            handler->current_cmd.result = AT_RESULT_ERROR;
        } else {
            handler->current_cmd.result = AT_RESULT_UNKNOWN;
        }

        // 触发回调
        if (handler->current_cmd.callback) {
            handler->current_cmd.callback(
                handler->current_cmd.result, handler->resp_buffer);
        }

        // 回到空闲状态
        handler->state    = AT_STATE_IDLE;
        handler->resp_len = 0;
        break;

    case AT_STATE_WAITING_DATA_MODE:
        // 检查是否收到"CONNECT"响应
        if (strstr(handler->resp_buffer, "CONNECT")) {
            handler->state = AT_STATE_DATA_MODE;
            if (handler->current_cmd.callback) {
                handler->current_cmd.callback(
                    AT_RESULT_OK, "Entered data mode");
            }
        }
        // 检查超时
        else if (HAL_GetTick() - handler->last_send_time
                 > handler->current_cmd.timeout) {
            handler->state = AT_STATE_IDLE;
            if (handler->current_cmd.callback) {
                handler->current_cmd.callback(
                    AT_RESULT_TIMEOUT, "Data mode timeout");
            }
        }
        break;

    case AT_STATE_EXITING_DATA:
        // 等待足够时间确保模块退出数据模式
        if (HAL_GetTick() - handler->last_send_time > 1000) {
            handler->state = AT_STATE_IDLE;
        }
        break;

    case AT_STATE_IDLE:
        // 检查队列中是否有待发送命令
        if (handler->cmd_queue.count > 0) {
            memcpy(&handler->current_cmd,
                   &handler->cmd_queue.queue[handler->cmd_queue.head],
                   sizeof(at_cmd_t));

            handler->cmd_queue.head =
                (handler->cmd_queue.head + 1) % AT_CMD_QUEUE_SIZE;
            handler->cmd_queue.count--;

            HAL_UART_Transmit(handler->huart,
                              (uint8_t *)handler->current_cmd.cmd,
                              strlen(handler->current_cmd.cmd), HAL_MAX_DELAY);

            handler->state          = AT_STATE_SENDING;
            handler->last_send_time = HAL_GetTick();
            handler->resp_len       = 0;
        }
        break;

    default:
        break;
    }
}

// 处理接收到的数据
void at_process_rx_data(at_hdlr_t *handler)
{
    // 如果当前正在处理数据，则跳过
    if (handler->processing)
        return;

    handler->processing = 1;

    // 处理环形缓冲区中的数据
    while (handler->rx_ring_buffer.count > 0) {
        uint8_t data =
            handler->rx_ring_buffer.buffer[handler->rx_ring_buffer.tail];
        handler->rx_ring_buffer.tail =
            (handler->rx_ring_buffer.tail + 1) % AT_RING_BUFFER_SIZE;
        handler->rx_ring_buffer.count--;

        // 在数据模式下，所有接收的数据都应该直接传递给应用层
        if (handler->state == AT_STATE_DATA_MODE) {
            // 这里可以添加回调函数将数据传递给应用层
            continue;
        }

        // 添加到响应缓冲区
        if (handler->resp_len < AT_MAX_RESP_LEN - 1) {
            handler->resp_buffer[handler->resp_len++] = data;
            handler->resp_buffer[handler->resp_len] = '\0'; // 确保以null结尾
        }

        // 检测是否收到完整的行（以\r\n结尾）
        if (handler->resp_len >= 2
            && handler->resp_buffer[handler->resp_len - 2] == '\r'
            && handler->resp_buffer[handler->resp_len - 1] == '\n') {

            // 检查是否为URC消息
            char *line_start = handler->resp_buffer;
            char *line_end   = strstr(line_start, "\r\n");

            while (line_end) {
                // 临时终止字符串以处理当前行
                *line_end = '\0';

                // 处理URC
                if (line_start[0] == '+') {
                    at_process_urc(handler, line_start);
                }

                // 恢复\r字符
                *line_end = '\r';

                // 移动到下一行
                line_start = line_end + 2;
                if (*line_start) {
                    line_end = strstr(line_start, "\r\n");
                } else {
                    line_end = NULL;
                }
            }

            // 检查是否收到了预期的响应
            if (handler->state == AT_STATE_WAITING_RESP) {
                if (strstr(
                        handler->resp_buffer, handler->current_cmd.expect_resp)
                    || strstr(handler->resp_buffer, "ERROR")
                    || strstr(handler->resp_buffer, "OK")) {
                    handler->state = AT_STATE_RESP_RECEIVED;
                }
            }
        }
    }

    handler->processing = 0;
}

// 添加URC处理器
void at_add_urc_hdlr(at_hdlr_t *handler, const char *urc_prefix,
                     urc_hdlr_t urc_handler)
{
    if (handler->urc_handler_count >= AT_MAX_URC_HANDLERS)
        return;

    handler->urc_handlers[handler->urc_handler_count].urc_prefix = urc_prefix;
    handler->urc_handlers[handler->urc_handler_count].handler    = urc_handler;
    handler->urc_handler_count++;
}

// 处理URC
void at_process_urc(at_hdlr_t *handler, const char *line)
{
    // 跳过空白字符
    while (*line == ' ' || *line == '\r' || *line == '\n')
        line++;

    // 检查是否为URC（以+开头）
    if (*line != '+')
        return;

    // 查找分隔符
    const char *params = strchr(line, ':');
    if (!params)
        params = strchr(line, ',');
    if (!params)
        return;

    // 提取URC前缀
    char urc_prefix[32];
    size_t prefix_len = params - line;
    if (prefix_len >= sizeof(urc_prefix))
        prefix_len = sizeof(urc_prefix) - 1;
    strncpy(urc_prefix, line, prefix_len);
    urc_prefix[prefix_len] = '\0';

    // 跳过分隔符
    params++;

    // 查找匹配的URC处理器
    for (int i = 0; i < handler->urc_handler_count; i++) {
        if (strcmp(handler->urc_handlers[i].urc_prefix, urc_prefix) == 0) {
            handler->urc_handlers[i].handler(urc_prefix, params);
            break;
        }
    }
}

// 进入数据模式
AT_Result_t at_enter_data_mode(at_hdlr_t *handler, const char *cmd,
                               uint32_t timeout,
                               void (*callback)(AT_Result_t, const char *))
{
    if (handler->state != AT_STATE_IDLE) {
        return AT_RESULT_ERROR;
    }

    // 发送进入数据模式命令
    at_send(handler, cmd, "CONNECT", timeout, callback);
    handler->state = AT_STATE_WAITING_DATA_MODE;

    return AT_RESULT_OK;
}

// 退出数据模式
AT_Result_t at_exit_data_mode(at_hdlr_t *handler)
{
    if (handler->state != AT_STATE_DATA_MODE) {
        return AT_RESULT_ERROR;
    }

    // 发送退出序列
    HAL_UART_Transmit(handler->huart, (uint8_t *)"+++", 3, HAL_MAX_DELAY);
    handler->state          = AT_STATE_EXITING_DATA;
    handler->last_send_time = HAL_GetTick();

    return AT_RESULT_OK;
}

// 在数据模式下发送数据
void at_send_data(at_hdlr_t *handler, const uint8_t *data, uint16_t len)
{
    if (handler->state == AT_STATE_DATA_MODE) {
        HAL_UART_Transmit(handler->huart, data, len, HAL_MAX_DELAY);
    }
}

// UART接收中断回调函数
void at_uart_rx_complt_cb(at_hdlr_t *handler, uint8_t data)
{
    uint16_t next = (handler->rx_ring_buffer.head + 1) % AT_RING_BUFFER_SIZE;

    // 缓冲区未满则存入数据
    if (handler->rx_ring_buffer.count < AT_RING_BUFFER_SIZE) {
        handler->rx_ring_buffer.buffer[handler->rx_ring_buffer.head] = data;
        handler->rx_ring_buffer.head                                 = next;
        handler->rx_ring_buffer.count++;
    }
}