#ifndef _MB_PROTOCOL_H_
#define _MB_PROTOCOL_H_

// 宏定义
#define MB_FC_NONE                     (0x00)
#define MB_FC_READ_COILS               (0x01)
#define MB_FC_READ_DISCRETE_INPUTS     (0x02)
#define MB_FC_READ_HOLDING_REGISTERS   (0x03)
#define MB_FC_READ_INPUT_REGISTERS     (0x04)
#define MB_FC_WRITE_SINGLE_COIL        (0x05)
#define MB_FC_WRITE_SINGLE_REGISTER    (0x06)
#define MB_FC_READ_EXCEPTION_STATUS    (0x07)
#define MB_FC_WRITE_MULTIPLE_COILS     (0x0F)
#define MB_FC_WRITE_MULTIPLE_REGISTERS (0x10)
#define MB_FC_REPORT_SLAVE_ID          (0x11)
#define MB_FC_MASK_WRITE_REGISTER      (0x16)
#define MB_FC_WRITE_AND_READ_REGISTERS (0x17)
#define MB_FC_ERROR                    (0x7f)


typedef enum {
    MB_EX_NONE = 0x00,
    MB_EX_ILLEGAL_FUNCTION = 0x01,
    MB_EX_ILLEGAL_DATA_ADDRESS = 0x02,
    MB_EX_ILLEGAL_DATA_VALUE = 0x03,
    MB_EX_SLAVE_DEVICE_FAILURE = 0x04,
    MB_EX_ACKNOWLEDGE = 0x05,
    MB_EX_SLAVE_BUSY = 0x06,
    MB_EX_MEMORY_PARITY_ERROR = 0x08,
    MB_EX_GATEWAY_PATH_FAILED = 0x0A,
    MB_EX_GATEWAY_TGT_FAILED = 0x0B
} MB_EXCEPTION;


/** 数据类型
 * DISCRETE: RO
 * COILS: RW
 * INPUT: RO
 * HOLDING: RW
 * 这几个数据结构，可以在同一片内存，也可以在不同内存区域
*/

// (0x01) Read Coils: Quantity of Coils to read (2 bytes): 1 to 2000 (0x7D0)
#define MB_MAX_READ_COILS   0x7D0 // 1~2000
// (0x0f) Write Multiple Coils: Quantity of Coils to read (2 bytes): 1 to 1968 (0x7B0)
#define MB_MAX_WRITE_COILS  0x7B0
//  (0x02) Read Discrete Inputs: Quantity of Inputs to read (2 bytes): 1 to 2000 (0x7D0)
#define MB_MAX_READ_DISCRETE 0x7D0 // 1~2000
// (0x03) Read Holding Registers: Quantity of Holding Registers: (2 bytes): 1 to 125 (0x7D)
#define MB_MAX_READ_INPUT_REG_NUM   0X7D // 1~125
// (0x04) Read Input Registers: Quantity of Input Registers: (2 bytes): 1 to 125 (0x7D)
#define MB_MAX_READ_HOLD_REG_NUM   0X7D // 1~125
// (0x10) Write Multiple registers: Quantity of Write Registers: (2 bytes): 1 to 125 (0x7B)
#define MB_MAX_WRITE_REG_NUM   0X7B // 1~123
// (0x17) Read/Write Multiple registers
#define MB_MAX_RW_REG_NUM   0x79 // 1~121

// length_ptr will changed dynamically，注册时不同的功能处理，固定长度不一
typedef MB_EXCEPTION (*mb_function_handler)(uint8_t *frame_data, uint16_t *length_ptr);

typedef struct {
    uint8_t function_code;
    mb_function_handler handler;
}mb_function;

#endif