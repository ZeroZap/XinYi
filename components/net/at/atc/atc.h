#ifndef _AT_H_
#define _ATC_H_

#define ATC_PRINT_RAW_CMD
#define ATC_MAX_RESP_LEN 1024
#define ATC_MAX_CMD_LEN 1024
#define ATC_MAX_RETRY 5
#define ATC_MAX_DEFAULT_TIMEOUT 5000

#define ATC_MAX_CLIENT 1

#define ATC_END_SIGN

struct atc_client;

typedef void* atc_mutex_t;
typedef void* atc_semaphore_t;

enum atc_error
{
    atc_error_ok = 0,
    atc_error = -1,
    atc_error_invalid_cmd
};
typedef enum  atc_error atc_error_t;

enum atc_status{
    atc_status_uninitialized = 0,
    atc_status_initialized,
    atc_status_cli,
    atc_status_idle,
    atc_status_busy
};
typedef enum atc_status atc_status_t

enum {
    atc_resp_status_ok = 0,
    atc_resp_status_error = -1,
    atc_resp_status_timeout = -2,
    atc_resp_status_buff_full = -3,
}atc_resp_status;
typedef enum atc_resp_status atc_resp_status_t;

struct atc_response{
    char *buf; /** response buff */
    size_t buf_size; /** max buffer size */
    size_t buf_len;  /** current buffer len */
    // 0: wait for OK or ERROR response , >0  wait for target line
    uint8_t target_line; // !0 wait target line
    uint8_t line_counts;
    uint16_t resp_len;
    uint32_t timeout; /** the maximum reponse time. */
};

typedef struct atc_response *atc_response_t;

struct atc_urc{
    const char *cmd_prefix;
    const char *cmd_suffix;
    void (*func)(struct atc_client *client, const char *data, size_t size);
};
typedef struct atc_urc *atc_urc_t;

struct atc_urc_table{
    size_t urc_size;
    const struct atc_urc *urc;
};
typedef struct atc_urc_table atc_urc_table_t;

struct atc_client{

    at_status_t status;
    char end_sign;

    int (*get_char)(&ch, timeout);
    size_t (*send)(char *data, size_t len);

    // 用于 URC 后面的数据读取
    size_t (*recv)(char *data, size_t len);

    char *send_buf;
    size_t send_bufsz;
    /** 最后命令的长度 */
    size_last_cmd_len;

    /**当前获取行buf*/
    char *recv_line_buf;
    /** 当前行数 */
    size_t recv_line_len;
    /** 当前行最大长度 */
    size_t recv_bufsz;
    int rx_notice;

    // send cmd lock 住， send cmd 释放
    uint32_t mutex_lock;

    // send cmd 时清除，parse 到加操作
    uint32_t sem_notice;
    atc_resp_status_t resp_status;
   // 当前的 resp
    atc_response_t resp;

    struct atc_urc_table *urc_table
    size_t urc_table_size;
    const struct atc_urc *urc;
};
typedef struct atc_client *atc_client_t


// note uart 搞个 ringbuffer??? 或者不需要了,直接挂到 uart 的处理? 不能，只能 ring buffer
char at_cmd[AT_MAX_CMD_LEN] = {0};
char at_resp[AT_MAX_RESP_LEN] = {0};


/**
= 0 返回 OK，小于其他，需要几个 bit 去表示 client 有些地方并未初始化
*/
int atc_register(atc_client_t client);
int atc_init(atc_client_t client);
int atc_add_urc_hlr(uint8_t client_index, struct atc_urc_table *urc_table, size_t size);

// int (*get_char)(&ch, timeout);
// size_t (*send)(char *data, size_t len);
// // 用于 URC 后面的数据读取
// size_t (*recv)(char *data, size_t len);

atc_client_t atc_get_client(uint8_t client_index);

int atc_exec_cmd(atc_client_t client, atc_response_t resp, const char *cmd_expr, ...);


// 换个思路,是否有 换行可以直接记录该值位置呢,一般 AT 也不会有很多行的吧
// 如果有 10行,那就 uint16_t line_index[10] = 20个字节也好

// at_procee 切分成几个状态

/** atc_process
* get char -> get line
* send buff ->
* get buff ->
*/

/**
atc_idle 状态
get_char
urc
get buff
*/

/**
* send cmd 状态
* send buff
* get_char
*/

#endif