#ifndef AT_HANDLER_H
#define AT_HANDLER_H

#include <stdint.h>
#include <string.h>

// 配置参数
#define AT_MAX_CMD_LEN         128
#define AT_MAX_RESP_LEN        512
#define AT_MAX_RETRY           3
#define AT_DEFAULT_TIMEOUT     5000    // 默认超时5秒
#define AT_CMD_QUEUE_SIZE      10      // 命令队列大小
#define AT_MAX_URC_HANDLERS    5       // 最大URC处理器数量

// 状态枚举
typedef enum {
    AT_STATE_IDLE,
    AT_STATE_SENDING,
    AT_STATE_WAITING_RESP,
    AT_STATE_RESP_RECEIVED,
    AT_STATE_TIMEOUT,
    AT_STATE_ERROR,
    AT_STATE_DATA_MODE,
    AT_STATE_EXITING_DATA,
    AT_STATE_WAITING_DATA_MODE
} AT_State_t;

// 结果枚举
typedef enum {
    AT_RESULT_OK,
    AT_RESULT_ERROR,
    AT_RESULT_TIMEOUT,
    AT_RESULT_UNKNOWN
} AT_Result_t;

// AT命令结构
typedef struct {
    char cmd[AT_MAX_CMD_LEN];
    char expect_resp[32];      // 期望响应前缀
    uint8_t retry_count;
    uint32_t timeout;
    AT_Result_t result;
    void (*callback)(AT_Result_t result, const char* resp);
} AT_Command_t;

// 命令队列结构
typedef struct {
    AT_Command_t queue[AT_CMD_QUEUE_SIZE];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} AT_CommandQueue_t;

// URC处理器类型定义
typedef void (*URC_Handler_t)(const char* urc, const char* params);

// URC处理器结构
typedef struct {
    const char* urc_prefix;
    URC_Handler_t handler;
} AT_URC_Handler_t;

// AT处理器主结构
typedef struct {
    UART_HandleTypeDef* huart;
    AT_State_t state;
    AT_Command_t current_cmd;
    AT_CommandQueue_t cmd_queue;
    AT_URC_Handler_t urc_handlers[AT_MAX_URC_HANDLERS];
    uint8_t urc_handler_count;
    char resp_buffer[AT_MAX_RESP_LEN];
    uint16_t resp_len;
    uint32_t last_send_time;
    uint8_t initialized;
} AT_Handler_t;

// 函数声明
void AT_Init(AT_Handler_t* handler, UART_HandleTypeDef* huart);
AT_Result_t AT_SendCommand(AT_Handler_t* handler, const char* cmd,
                         const char* expect_resp, uint32_t timeout,
                         void (*callback)(AT_Result_t, const char*));
void AT_Process(AT_Handler_t* handler);
void AT_AddURCHandler(AT_Handler_t* handler, const char* urc_prefix, URC_Handler_t urc_handler);
AT_Result_t AT_EnterDataMode(AT_Handler_t* handler, const char* cmd,
                           uint32_t timeout, void (*callback)(AT_Result_t, const char*));
AT_Result_t AT_ExitDataMode(AT_Handler_t* handler);
void AT_SendData(AT_Handler_t* handler, const uint8_t* data, uint16_t len);
void AT_UART_RxCpltCallback(AT_Handler_t* handler);

#endif // AT_HANDLER_H