#ifndef _SD_H_
#define _SD_H_

// ref: https://blog.csdn.net/LH_SMD/article/details/121605139

typedef struct {
    uint8_t manufacturer_id;
    uint16_t application_id;
    uint8_t name[5];
    uint8_t revision;
    uint32_t serial_number;
    uint8_t reserved;
    uint16_t manufacture_date;
} sd_reg_cid;

typedef struct {

} sd_reg_csd;

typedef struct {
} sd_reg_rca;

typedef struct {
} sd_reg_scr;

typedef struct {
    uint32_t reserved2 : 7;
    uint32_t voltage_16_17 : 1;
    uint32_t voltage_18_19 : 1;
    uint32_t voltage_20_21 : 1;
    uint32_t voltage_21_22 : 1;
    uint32_t voltage_22_23 : 1;
    uint32_t voltage_23_24 : 1;
    uint32_t voltage_24_25 : 1;
    uint32_t voltage_25_26 : 1;
    uint32_t voltage_27_28 : 1;
    uint32_t voltage_28_29 : 1;
    uint32_t voltage_29_30 : 1;
    uint32_t voltage_30_31 : 1;
    uint32_t voltage_31_32 : 1;
    uint32_t voltage_32_33 : 1;
    uint32_t voltage_33_34 : 1;
    uint32_t voltage_34_35 : 1;
    uint32_t voltage_35_36 : 1;
    uint32_t reserved2 : 6;
    uint32_t card_capacity_status : 1;
    uint32_t card_power_status : 1;
} sd_reg_ocr;

typedef struct {
} sd_reg_status;

typedef struct {

} sd_card_info_t;

// R1 Response (表示命令状态)
#define SD_R1_IDLE_STATE           0x01
#define SD_R1_ERASE_RESET          0x02
#define SD_R1_ILLEGAL_COMMAND      0x04
#define SD_R1_COM_CRC_ERROR        0x08
#define SD_R1_ERASE_SEQUENCE_ERROR 0x10
#define SD_R1_ADDRESS_ERROR        0x20
#define SD_R1_PARAMETER_ERROR      0x40
#define SD_R1_RESPONSE_BUSY        0x80

// R1 Response (表示命令状态)
typedef struct {
    uint8_t reserved : 1;             // Reserved bit 保留位
    uint8_t param_error : 1;          // Parameter error 参数错误
    uint8_t address_error : 1;        // Address error 地址错误
    uint8_t erase_sequence_error : 1; // Erase sequence error 擦除序列错误
    uint8_t com_crc_error : 1;        // Command CRC error 命令CRC错误
    uint8_t illegal_command : 1;      // Illegal command flag 非法命令
    uint8_t erase_reset : 1;          // Erase reset flag 擦除序列被清除
    uint8_t in_idle_state : 1; // Card is in idle state 卡处于空闲状态
} sd_resp1;

// R1 Response (表示命令状态)
typedef struct {
    uint8_t busy : 1;                 // Card is busy 卡忙
    uint8_t param_error : 1;          // Parameter error 参数错误
    uint8_t address_error : 1;        // Address error 地址错误
    uint8_t erase_sequence_error : 1; // Erase sequence error 擦除序列错误
    uint8_t com_crc_error : 1;        // Command CRC error 命令CRC错误
    uint8_t illegal_command : 1;      // Illegal command flag 非法命令
    uint8_t erase_reset : 1;          // Erase reset flag 擦除序列被清除
    uint8_t in_idle_state : 1; // Card is in idle state 卡处于空闲状态
} sd_resp1b;

// R2 Response (Response of CID/CSD Register)
typedef struct {
    // First byte (MSB)
    uint8_t reserved : 1;             // Reserved bit (MSB)
    uint8_t param_error : 1;          // Parameter error
    uint8_t address_error : 1;        // Address error
    uint8_t erase_sequence_error : 1; // Erase sequence error
    uint8_t com_crc_error : 1;        // Command CRC error
    uint8_t illegal_command : 1;      // Illegal command flag
    uint8_t erase_reset : 1;          // Erase reset flag
    uint8_t in_idle_state : 1;        // Card is in idle state

    // Second byte (LSB)
    uint8_t otf_csd_ovr : 1;     // Out of range or CSD overwrite
    uint8_t erase_param : 1;     // Erase parameter error
    uint8_t wp_violation : 1;    // Write protect violation
    uint8_t card_ecc_failed : 1; // Card ECC failed
    uint8_t cc_error : 1;        // Card controller error
    uint8_t error : 1;           // General error
    uint8_t wp_ers_l_ul : 1; // Write protect erase skip, lock/unlock cmd failed
    uint8_t card_locked : 1; // Card is locked
} sd_resp2;

// R3 Response (OCR Register Format)
typedef struct {
    sd_reg_ocr ocr;      // OCR寄存器内容（4字节）
    uint8_t r1_response; // R1响应（1字节）
} sd_resp3;

// R4 / R5 Unused for SPI mode

// R7 Response (Interface Condition Response Format)
typedef struct {
    uint8_t r1_response;       // R1响应（1字节）
    uint8_t command_version;   // 命令版本（1字节）
    uint16_t voltage_accepted; // 接受的电压范围（2字节）
    uint8_t check_pattern;     // 检查模式（1字节）
} sd_resp7;

typedef enum {
    sd_error_unknown = -1,
    sd_error_ok      = 0,
} sd_error_t;

// Write data block written Data Response Token
#define SD_DATA_RESPONSE_ACCEPTED    0x06
#define SD_DATA_RESPONSE_CRC_ERROR   0x0B
#define SD_DATA_RESPONSE_WRITE_ERROR 0x0D
#define SD_DATA_RESPONSE_OTHER_ERROR 0x0E

// Start/Stop Token
#define SD_TOKEN_START_BLOCK 0xFE

// Data error token
#define SD_DATA_ERROR_HASH_ERROR      0x01
#define SD_DATA_ERROR_CC_ERROR        0x02
#define SD_DATA_ERROR_CARD_ECC_FAILED 0x08
#define SD_DATA_ERROR_OUT_OF_RANGE    0x10


#define SD_BLOCK_SIZE 512

#define SD_RESPONSE_NO_ERROR 0x00
#define SD_RESPONSE_ERROR    0x01
#define SD_RESPONSE_IDLE     0x02
#define SD_RESPONSE_FAILURE  0x03


#define SD_TYPE_NONE_SD
#define SD_TYPE_SDSC 0x01
#define SD_TYPE_SDHC 0x02

#define SD_CMD0_GO_IDLE_STATE         0x40
#define SD_CMD1_SEND_OP_COND          0x41
#define SD_CMD2_ALL_SEND_CID          0x42
#define SD_CMD3_SEND_RELATIVE_ADDR    0x43
#define SD_CMD4_SET_DSR               0x44
#define SD_CMD5_IO_SEND_OP_COND       0x45
#define SD_CMD6_SWITCH_FUNC           0x46
#define SD_CMD7_SELECT_CARD           0x47
#define SD_CMD8_SEND_IF_COND          0x49
#define SD_CMD9_SEND_CSD              0x49
#define SD_CMD10_SEND_CID             0x4A
#define SD_CMD12_STOP_TRANSMISSION    0x4C
#define SD_CMD13_SEND_STATUS          0x4D
#define SD_CMD16_SET_BLOCKLEN         0x50
#define SD_CMD17_READ_BLOCK           0x51
#define SD_CMD18_READ_MULTIPLE        0x52
#define SD_CMD23_SET_BLOCK_COUNT      0x57
#define SD_CMD24_WRITE_BLOCK          0x58
#define SD_CMD25_WRITE_MULTIPLE       0x59
#define SD_CMD27_PROGRAM_CSD          0x5B
#define SD_CMD28_SET_WRITE_PROT       0x5C
#define SD_CMD29_CLR_WRITE_PROT       0x5D
#define SD_CMD30_SEND_WRITE_PROT      0x5E
#define SD_CMD32_ERASE_WR_BLK_START   0x60
#define SD_CMD33_ERASE_WR_BLK_END     0x61
#define SD_CMD38_ERASE                0x66
#define SD_CMD40_CRC_ON_OFF           0x68
#define SD_CMD41_SD_SEND_OP_COND      0x69
#define SD_CMD42_LOCK_UNLOCK          0x6A
#define SD_CMD55_APP_CMD              0x77
#define SD_CMD58_READ_OCR             0x7A
#define SD_ACMD41_SD_SEND_OP_COND     0x69
#define SD_ACMD42_SET_CLR_CARD_DETECT 0x6A
#define SD_ACMD51_SEND_SCR            0x73

#define SD_CRC_CMD0_GO_IDLE_STATE         0x95
#define SD_CRC_CMD1_SEND_OP_COND          0xF9
#define SD_CRC_CMD2_ALL_SEND_CID          0xFF
#define SD_CRC_CMD3_SEND_RELATIVE_ADDR    0x27
#define SD_CRC_CMD4_SET_DSR               0x2F
#define SD_CRC_CMD5_IO_SEND_OP_COND       0x2F
#define SD_CRC_CMD6_SWITCH_FUNC           0x2F
#define SD_CRC_CMD7_SELECT_CARD           0x2F
#define SD_CRC_CMD8_SEND_IF_COND          0x87
#define SD_CRC_CMD9_SEND_CSD              0xAF
#define SD_CRC_CMD10_SEND_CID             0x1B
#define SD_CRC12_STOP_TRANSMISSION        0x3F
#define SD_CRC13_SEND_STATUS              0x2F
#define SD_CRC16_SET_BLOCKLEN             0xFF
#define SD_CRC17_READ_BLOCK               0xFF
#define SD_CRC18_READ_MULTIPLE            0xFF
#define SD_CRC23_SET_BLOCK_COUNT          0xFF
#define SD_CRC24_WRITE_BLOCK              0xFF
#define SD_CRC25_WRITE_MULTIPLE           0xFF
#define SD_CRC27_PROGRAM_CSD              0xFF
#define SD_CRC28_SET_WRITE_PROT           0xFF
#define SD_CRC29_CLR_WRITE_PROT           0xFF
#define SD_CRC30_SEND_WRITE_PROT          0xFF
#define SD_CRC32_ERASE_WR_BLK_START       0x3F
#define SD_CRC33_ERASE_WR_BLK_END         0x3F
#define SD_CRC38_ERASE                    0x3F
#define SD_CRC40_CRC_ON_OFF               0x3F
#define SD_CRC41_SD_SEND_OP_COND          0x3F
#define SD_CRC42_LOCK_UNLOCK              0x3F
#define SD_CRC55_APP_CMD                  0x65
#define SD_CRC58_READ_OCR                 0x3F
#define SD_CRC_ACMD41_SD_SEND_OP_COND     0x3F
#define SD_CRC_ACMD42_SET_CLR_CARD_DETECT 0x3F
#define SD_CRC_ACMD51_SEND_SCR            0x3F

#define SD_RESP_IDLE 0x01


typedef enum { SD_SPEED_LOW = 0, SD_SPEED_HIGH = 1 } sd_speed_t;

typedef enum { SD_PLUG_IN = 0, SD_PLUG_OUT = 1 } sd_plug_status_t;

typedef void (*sd_detect_callback)(sd_plug_status_t status);

extern int32_t sd_init(void);
extern int32_t sd_read_block(uint32_t block, uint8_t *data);
extern int32_t sd_write_block(uint32_t block, uint8_t *data);
extern int32_t sd_earase_block(uint32_t start_block, uint32_t end_block);
#endif
