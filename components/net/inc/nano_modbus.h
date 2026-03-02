/**
 * @file nano_modbus.h
 * @brief Nano Modbus RTU Protocol Stack (TCP-free)
 * @version 1.0.0
 * @date 2026-03-01 下午
 */

#ifndef NANO_MODBUS_H
#define NANO_MODBUS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Modbus 功能码
 */
typedef enum {
    MB_FUNC_READ_COILS          = 0x01, /**< 读线圈 */
    MB_FUNC_READ_DISCRETE       = 0x02, /**< 读离散输入 */
    MB_FUNC_READ_HOLDING        = 0x03, /**< 读保持寄存器 */
    MB_FUNC_READ_INPUT          = 0x04, /**< 读输入寄存器 */
    MB_FUNC_WRITE_SINGLE_COIL   = 0x05, /**< 写单个线圈 */
    MB_FUNC_WRITE_SINGLE_REG    = 0x06, /**< 写单个寄存器 */
    MB_FUNC_WRITE_MULTIPLE_COILS= 0x0F, /**< 写多个线圈 */
    MB_FUNC_WRITE_MULTIPLE_REGS = 0x10, /**< 写多个寄存器 */
} mb_function_code_t;

/**
 * @brief Modbus 错误码
 */
typedef enum {
    MB_ERROR_NONE               = 0x00, /**< 无错误 */
    MB_ERROR_ILLEGAL_FUNC       = 0x01, /**< 非法功能码 */
    MB_ERROR_ILLEGAL_DATA_ADDR  = 0x02, /**< 非法数据地址 */
    MB_ERROR_ILLEGAL_DATA_VALUE = 0x03, /**< 非法数据值 */
    MB_ERROR_SLAVE_DEVICE_FAIL  = 0x04, /**< 从站设备故障 */
    MB_ERROR_ACKNOWLEDGE        = 0x05, /**< 确认 */
    MB_ERROR_SLAVE_DEVICE_BUSY  = 0x06, /**< 从站设备忙 */
    MB_ERROR_MEMORY_PARITY      = 0x08, /**< 内存奇偶错误 */
    MB_ERROR_GATEWAY_PATH       = 0x0A, /**< 网关路径不可用 */
    MB_ERROR_GATEWAY_TARGET     = 0x0B, /**< 网关目标设备响应失败 */
} mb_error_code_t;

/**
 * @brief Nano Modbus 错误码 (API 返回)
 */
#define NANO_MB_OK              0
#define NANO_MB_ERROR           (-1)
#define NANO_MB_INVALID_PARAM   (-2)
#define NANO_MB_TIMEOUT         (-3)
#define NANO_MB_CRC_ERROR       (-4)
#define NANO_MB_BUSY            (-5)

/**
 * @brief Modbus 最大 PDU 大小
 */
#define MB_MAX_PDU_SIZE         256

/**
 * @brief Modbus ADU 缓冲区大小 (地址 + PDU + CRC)
 */
#define MB_MAX_ADU_SIZE         (MB_MAX_PDU_SIZE + 4)

/**
 * @brief Modbus 超时时间 (ms)
 */
#define MB_DEFAULT_TIMEOUT_MS   1000

/**
 * @brief Modbus 帧间隔 (us) - 3.5 字符时间
 */
#define MB_FRAME_INTERVAL_US    3500

/**
 * @brief Modbus 从站配置
 */
typedef struct {
    uint8_t slave_id;               /**< 从站 ID (1-247) */
    uint16_t coil_start;            /**< 线圈起始地址 */
    uint16_t coil_count;            /**< 线圈数量 */
    uint16_t discrete_start;        /**< 离散输入起始地址 */
    uint16_t discrete_count;        /**< 离散输入数量 */
    uint16_t holding_start;         /**< 保持寄存器起始地址 */
    uint16_t holding_count;         /**< 保持寄存器数量 */
    uint16_t input_start;           /**< 输入寄存器起始地址 */
    uint16_t input_count;           /**< 输入寄存器数量 */
} mb_slave_config_t;

/**
 * @brief Modbus 回调函数
 */
typedef int (*mb_read_coils_cb_t)(uint16_t addr, uint16_t count, uint8_t *data);
typedef int (*mb_read_discrete_cb_t)(uint16_t addr, uint16_t count, uint8_t *data);
typedef int (*mb_read_holding_cb_t)(uint16_t addr, uint16_t count, uint16_t *data);
typedef int (*mb_read_input_cb_t)(uint16_t addr, uint16_t count, uint16_t *data);
typedef int (*mb_write_coil_cb_t)(uint16_t addr, uint16_t value);
typedef int (*mb_write_reg_cb_t)(uint16_t addr, uint16_t value);
typedef int (*mb_write_coils_cb_t)(uint16_t addr, uint16_t count, const uint8_t *data);
typedef int (*mb_write_regs_cb_t)(uint16_t addr, uint16_t count, const uint16_t *data);

/**
 * @brief Modbus 从站设备
 */
typedef struct {
    mb_slave_config_t config;       /**< 配置 */
    
    /* 回调函数 */
    mb_read_coils_cb_t read_coils_cb;
    mb_read_discrete_cb_t read_discrete_cb;
    mb_read_holding_cb_t read_holding_cb;
    mb_read_input_cb_t read_input_cb;
    mb_write_coil_cb_t write_coil_cb;
    mb_write_reg_cb_t write_reg_cb;
    mb_write_coils_cb_t write_coils_cb;
    mb_write_regs_cb_t write_regs_cb;
    
    uint8_t rx_buf[MB_MAX_ADU_SIZE];/**< 接收缓冲区 */
    uint8_t tx_buf[MB_MAX_ADU_SIZE];/**< 发送缓冲区 */
    uint16_t rx_count;              /**< 接收计数 */
    
    uint32_t last_rx_time;          /**< 上次接收时间 */
    uint32_t request_count;         /**< 请求计数 */
    uint32_t error_count;           /**< 错误计数 */
    
    bool initialized;               /**< 初始化标志 */
} mb_slave_t;

/**
 * @brief Modbus 主站设备
 */
typedef struct {
    uint8_t current_slave;          /**< 当前从站 ID */
    uint32_t timeout_ms;            /**< 超时时间 */
    
    uint8_t tx_buf[MB_MAX_ADU_SIZE];/**< 发送缓冲区 */
    uint8_t rx_buf[MB_MAX_ADU_SIZE];/**< 接收缓冲区 */
    uint16_t tx_count;              /**< 发送计数 */
    uint16_t rx_count;              /**< 接收计数 */
    
    uint32_t request_count;         /**< 请求计数 */
    uint32_t error_count;           /**< 错误计数 */
    
    bool initialized;               /**< 初始化标志 */
} mb_master_t;

/**
 * @brief Modbus 串口操作接口
 */
typedef struct {
    int (*init)(uint32_t baudrate);
    int (*deinit)(void);
    int (*send)(const uint8_t *data, uint16_t len, uint32_t timeout);
    int (*receive)(uint8_t *data, uint16_t len, uint32_t timeout);
    void (*set_timeout)(uint32_t timeout_ms);
} mb_uart_ops_t;

/* ==================== 从站 API ==================== */

/**
 * @brief 初始化 Modbus 从站
 * @param slave 从站设备句柄
 * @param config 从站配置
 * @return NANO_MB_OK 成功，其他值失败
 */
int nano_mb_slave_init(mb_slave_t *slave, const mb_slave_config_t *config);

/**
 * @brief 反初始化从站
 * @param slave 从站设备句柄
 * @return NANO_MB_OK 成功，其他值失败
 */
int nano_mb_slave_deinit(mb_slave_t *slave);

/**
 * @brief 注册读线圈回调
 * @param slave 从站设备句柄
 * @param callback 回调函数
 * @return NANO_MB_OK 成功，其他值失败
 */
int nano_mb_slave_register_read_coils(mb_slave_t *slave, mb_read_coils_cb_t callback);

/**
 * @brief 注册读保持寄存器回调
 * @param slave 从站设备句柄
 * @param callback 回调函数
 * @return NANO_MB_OK 成功，其他值失败
 */
int nano_mb_slave_register_read_holding(mb_slave_t *slave, mb_read_holding_cb_t callback);

/**
 * @brief 注册写单个寄存器回调
 * @param slave 从站设备句柄
 * @param callback 回调函数
 * @return NANO_MB_OK 成功，其他值失败
 */
int nano_mb_slave_register_write_reg(mb_slave_t *slave, mb_write_reg_cb_t callback);

/**
 * @brief 从站轮询处理 (在串口接收中断或任务中调用)
 * @param slave 从站设备句柄
 * @param data 接收到的数据
 * @param len 数据长度
 * @return NANO_MB_OK 成功，其他值失败
 */
int nano_mb_slave_poll(mb_slave_t *slave, const uint8_t *data, uint16_t len);

/* ==================== 主站 API ==================== */

/**
 * @brief 初始化 Modbus 主站
 * @param master 主站设备句柄
 * @return NANO_MB_OK 成功，其他值失败
 */
int nano_mb_master_init(mb_master_t *master);

/**
 * @brief 反初始化主站
 * @param master 主站设备句柄
 * @return NANO_MB_OK 成功，其他值失败
 */
int nano_mb_master_deinit(mb_master_t *master);

/**
 * @brief 设置超时时间
 * @param master 主站设备句柄
 * @param timeout_ms 超时时间 (ms)
 * @return NANO_MB_OK 成功，其他值失败
 */
int nano_mb_master_set_timeout(mb_master_t *master, uint32_t timeout_ms);

/**
 * @brief 读多个保持寄存器
 * @param master 主站设备句柄
 * @param slave_id 从站 ID
 * @param addr 起始地址
 * @param count 寄存器数量
 * @param data 数据缓冲区
 * @param timeout 超时时间 (ms)
 * @return NANO_MB_OK 成功，其他值失败
 */
int nano_mb_master_read_holding(mb_master_t *master, uint8_t slave_id, 
                                uint16_t addr, uint16_t count, uint16_t *data,
                                uint32_t timeout);

/**
 * @brief 写单个寄存器
 * @param master 主站设备句柄
 * @param slave_id 从站 ID
 * @param addr 寄存器地址
 * @param value 寄存器值
 * @param timeout 超时时间 (ms)
 * @return NANO_MB_OK 成功，其他值失败
 */
int nano_mb_master_write_reg(mb_master_t *master, uint8_t slave_id,
                             uint16_t addr, uint16_t value, uint32_t timeout);

/**
 * @brief 写多个寄存器
 * @param master 主站设备句柄
 * @param slave_id 从站 ID
 * @param addr 起始地址
 * @param count 寄存器数量
 * @param data 数据缓冲区
 * @param timeout 超时时间 (ms)
 * @return NANO_MB_OK 成功，其他值失败
 */
int nano_mb_master_write_multi_regs(mb_master_t *master, uint8_t slave_id,
                                    uint16_t addr, uint16_t count, const uint16_t *data,
                                    uint32_t timeout);

/**
 * @brief 读多个线圈
 * @param master 主站设备句柄
 * @param slave_id 从站 ID
 * @param addr 起始地址
 * @param count 线圈数量
 * @param data 数据缓冲区
 * @param timeout 超时时间 (ms)
 * @return NANO_MB_OK 成功，其他值失败
 */
int nano_mb_master_read_coils(mb_master_t *master, uint8_t slave_id,
                              uint16_t addr, uint16_t count, uint8_t *data,
                              uint32_t timeout);

/* ==================== 工具函数 ==================== */

/**
 * @brief 计算 CRC16 (Modbus)
 * @param data 数据指针
 * @param len 数据长度
 * @return CRC16 值
 */
uint16_t nano_mb_crc16(const uint8_t *data, uint16_t len);

/**
 * @brief 获取错误字符串
 * @param error 错误码
 * @return 错误字符串
 */
const char* nano_mb_error_string(int error);

#ifdef __cplusplus
}
#endif

#endif /* NANO_MODBUS_H */
