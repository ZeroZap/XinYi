#include "at_handler.h"
#include <stdio.h>

// 初始化AT处理器
void AT_Init(AT_Handler_t* handler, UART_HandleTypeDef* huart) {
    handler->huart = huart;
    handler->state = AT_STATE_IDLE;
    handler->resp_len = 0;
    handler->initialized = 1;
    handler->urc_handler_count = 0;

    // 初始化队列
    handler->cmd_queue.head = 0;
    handler->cmd_queue.tail = 0;
    handler->cmd_queue.count = 0;

    // 清空缓冲区
    memset(handler->resp_buffer, 0, AT_MAX_RESP_LEN);
    memset(&handler->current_cmd, 0, sizeof(AT_Command_t));
}

// 发送AT命令
AT_Result_t AT_SendCommand(AT_Handler_t* handler, const char* cmd,
                         const char* expect_resp, uint32_t timeout,
                         void (*callback)(AT_Result_t, const char*)) {
    if (!handler->initialized) {
        return AT_RESULT_ERROR;
    }

    AT_Command_t new_cmd;
    memset(&new_cmd, 0, sizeof(AT_Command_t));
    snprintf(new_cmd.cmd, AT_MAX_CMD_LEN, "%s\r\n", cmd);
    strncpy(new_cmd.expect_resp, expect_resp, 31);
    new_cmd.timeout = timeout ? timeout : AT_DEFAULT_TIMEOUT;
    new_cmd.callback = callback;

    // 如果当前有命令在执行或队列不为空，加入队列
    if (handler->state != AT_STATE_IDLE || handler->cmd_queue.count > 0) {
        if (handler->cmd_queue.count >= AT_CMD_QUEUE_SIZE) {
            return AT_RESULT_ERROR;
        }

        handler->cmd_queue.queue[handler->cmd_queue.tail] = new_cmd;
        handler->cmd_queue.tail = (handler->cmd_queue.tail + 1) % AT_CMD_QUEUE_SIZE;
        handler->cmd_queue.count++;
        return AT_RESULT_OK;
    }

    // 直接发送命令
    memcpy(&handler->current_cmd, &new_cmd, sizeof(AT_Command_t));
    HAL_UART_Transmit(handler->huart, (uint8_t*)handler->current_cmd.cmd,
                     strlen(handler->current_cmd.cmd), HAL_MAX_DELAY);

    handler->state = AT_STATE_SENDING;
    handler->last_send_time = HAL_GetTick();
    handler->resp_len = 0;

    return AT_RESULT_OK;
}

// 主处理函数
void AT_Process(AT_Handler_t* handler) {
    if (!handler->initialized) return;

    switch (handler->state) {
        case AT_STAThandler->stateE_SENDING:
            // 检查是否发送完成
            if (HAL_UART_GetState(handler->huart) == HAL_UART_STATE_READY) {
                handler->state = AT_STATE_WAITING_RESP;
            }
            break;

        case AT_STATE_WAITING_RESP:
            // 检查超时
            if (HAL_GetTick() - handler->last_send_time > handler->current_cmd.timeout) {
                handler->state = AT_STATE_TIMEOUT;
                handler->current_cmd.result = AT_RESULT_TIMEOUT;

                // 重试逻辑
                if (handler->current_cmd.retry_count < AT_MAX_RETRY) {
                    handler->current_cmd.retry_count++;
                    HAL_UART_Transmit(handler->huart, (uint8_t*)handler->current_cmd.cmd,
                                     strlen(handler->current_cmd.cmd), HAL_MAX_DELAY);
                    handler->last_send_time = HAL_GetTick();
                    handler->state = AT_STATE_SENDING;
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
                handler->current_cmd.callback(handler->current_cmd.result, handler->resp_buffer);
            }

            // 回到空闲状态
            handler->state = AT_STATE_IDLE;
            handler->resp_len = 0;
            break;

        case AT_STATE_WAITING_DATA_MODE:
            // 检查是否收到"CONNECT"响应
            if (strstr(handler->resp_buffer, "CONNECT")) {
                handler->state = AT_STATE_DATA_MODE;
                if (handler->current_cmd.callback) {
                    handler->current_cmd.callback(AT_RESULT_OK, "Entered data mode");
                }
            }
            // 检查超时
            else if (HAL_GetTick() - handler->last_send_time > handler->current_cmd.timeout) {
                handler->state = AT_STATE_IDLE;
                if (handler->current_cmd.callback) {
                    handler->current_cmd.callback(AT_RESULT_TIMEOUT, "Data mode timeout");
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
                      sizeof(AT_Command_t));

                handler->cmd_queue.head = (handler->cmd_queue.head + 1) % AT_CMD_QUEUE_SIZE;
                handler->cmd_queue.count--;

                HAL_UART_Transmit(handler->huart, (uint8_t*)handler->current_cmd.cmd,
                                 strlen(handler->current_cmd.cmd), HAL_MAX_DELAY);

                handler->state = AT_STATE_SENDING;
                handler->last_send_time = HAL_GetTick();
                handler->resp_len = 0;
            }
            break;

        default:
            break;
    }
}

// 添加URC处理器
void AT_AddURCHandler(AT_Handler_t* handler, const char* urc_prefix, URC_Handler_t urc_handler) {
    if (handler->urc_handler_count >= AT_MAX_URC_HANDLERS) return;

    handler->urc_handlers[handler->urc_handler_count].urc_prefix = urc_prefix;
    handler->urc_handlers[handler->urc_handler_count].handler = urc_handler;
    handler->urc_handler_count++;
}

// 处理URC
void AT_ProcessURC(AT_Handler_t* handler, const char* line) {
    // 跳过空白字符
    while (*line == ' ' || *line == '\r' || *line == '\n') line++;

    // 检查是否为URC（以+开头）
    if (*line != '+') return;

    // 查找分隔符
    const char* params = strchr(line, ':');
    if (!params) params = strchr(line, ',');
    if (!params) return;

    // 提取URC前缀
    char urc_prefix[32];
    size_t prefix_len = params - line;
    if (prefix_len >= sizeof(urc_prefix)) prefix_len = sizeof(urc_prefix) - 1;
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
AT_Result_t AT_EnterDataMode(AT_Handler_t* handler, const char* cmd,
                           uint32_t timeout, void (*callback)(AT_Result_t, const char*)) {
    if (handler->state != AT_STATE_IDLE) {
        return AT_RESULT_ERROR;
    }

    // 发送进入数据模式命令
    AT_SendCommand(handler, cmd, "CONNECT", timeout, callback);
    handler->state = AT_STATE_WAITING_DATA_MODE;

    return AT_RESULT_OK;
}

// 退出数据模式
AT_Result_t AT_ExitDataMode(AT_Handler_t* handler) {
    if (handler->state != AT_STATE_DATA_MODE) {
        return AT_RESULT_ERROR;
    }

    // 发送退出序列
    HAL_UART_Transmit(handler->huart, (uint8_t*)"+++", 3, HAL_MAX_DELAY);
    handler->state = AT_STATE_EXITING_DATA;
    handler->last_send_time = HAL_GetTick();

    return AT_RESULT_OK;
}

// 在数据模式下发送数据
void AT_SendData(AT_Handler_t* handler, const uint8_t* data, uint16_t len) {
    if (handler->state == AT_STATE_DATA_MODE) {
        HAL_UART_Transmit(handler->huart, data, len, HAL_MAX_DELAY);
    }
}

// UART接收完成回调
void AT_UART_RxCpltCallback(AT_Handler_t* handler) {
    if (handler->state == AT_STATE_WAITING_RESP ||
        handler->state == AT_STATE_DATA_MODE ||
        handler->state == AT_STATE_WAITING_DATA_MODE) {

        // 检查是否收到完整行（以\r\n结尾）
        if (handler->resp_len >= 2 &&
            handler->resp_buffer[handler->resp_len-2] == '\r' &&
            handler->resp_buffer[handler->resp_len-1] == '\n') {

            // 处理URC（数据模式或普通模式下的+开头响应）
            if (handler->state == AT_STATE_DATA_MODE ||
                handler->resp_buffer[0] == '+') {
                AT_ProcessURC(handler, handler->resp_buffer);
            }
            // 正常响应处理
            else {
                handler->state = AT_STATE_RESP_RECEIVED;
            }

            // 清空缓冲区准备接收下一行
            handler->resp_len = 0;
        }
    }
}