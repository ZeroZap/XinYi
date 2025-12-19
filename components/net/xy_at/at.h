#ifndef AT_HANDLER_H
#define AT_HANDLER_H

#include <stdint.h>
#include <string.h>

// 配置参数
#define AT_MAX_CMD_LEN      128
#define AT_MAX_RESP_LEN     512
#define AT_MAX_RETRY        3
#define AT_DEFAULT_TIMEOUT  5000 // 默认超时5秒
#define AT_CMD_QUEUE_SIZE   10   // 命令队列大小
#define AT_MAX_URC_HANDLERS 5    // 最大URC处理器数量

// 环形缓冲区定义
#define AT_RING_BUFFER_SIZE 1024
typedef struct {
    uint8_t buffer[AT_RING_BUFFER_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
    volatile uint16_t count;
} at_ring_buffer_t;

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
    char expect_resp[32]; // 期望响应前缀
    uint8_t retry_count;
    uint32_t timeout;
    AT_Result_t result;
    void (*callback)(AT_Result_t result, const char *resp);
} at_cmd_t;

// 命令队列结构
typedef struct {
    at_cmd_t queue[AT_CMD_QUEUE_SIZE];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} at_cmd_queue_t;

// URC处理器类型定义
typedef void (*urc_hdlr_t)(const char *urc, const char *params);

// URC处理器结构
typedef struct {
    const char *urc_prefix;
    urc_hdlr_t handler;
} at_urc_hdlr_t;

// AT处理器主结构
typedef struct {
    UART_HandleTypeDef *huart;
    AT_State_t state;
    at_cmd_t current_cmd;
    at_cmd_queue_t cmd_queue;
    at_urc_hdlr_t urc_handlers[AT_MAX_URC_HANDLERS];
    uint8_t urc_handler_count;
    char resp_buffer[AT_MAX_RESP_LEN];
    uint16_t resp_len;
    uint32_t last_send_time;
    uint8_t initialized;
    at_ring_buffer_t rx_ring_buffer;
    uint8_t processing; // 标记是否正在处理数据
} at_hdlr_t;

// 函数声明
void at_init(at_hdlr_t *handler, UART_HandleTypeDef *huart);
AT_Result_t at_send(at_hdlr_t *handler, const char *cmd,
                    const char *expect_resp, uint32_t timeout,
                    void (*callback)(AT_Result_t, const char *));
void at_process(at_hdlr_t *handler);
void at_add_urc_hdlr(at_hdlr_t *handler, const char *urc_prefix,
                     urc_hdlr_t urc_handler);
AT_Result_t at_enter_data_mode(at_hdlr_t *handler, const char *cmd,
                               uint32_t timeout,
                               void (*callback)(AT_Result_t, const char *));
AT_Result_t at_exit_data_mode(at_hdlr_t *handler);
void at_send_data(at_hdlr_t *handler, const uint8_t *data, uint16_t len);
void at_uart_rx_complt_cb(at_hdlr_t *handler);

#endif // AT_HANDLER_H