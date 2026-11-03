/**
 * @file mb_full.c
 * @brief Nano Modbus Full Implementation - RTU + TCP + ASCII
 * @version 1.0.0
 * @date 2026-03-31
 */

#include "mb_full.h"
#include <string.h>

/* ==================== CRC16 表 ==================== */

static const uint16_t crc16_table[256] = {
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

/* ==================== 全局回调 ==================== */

/*
 * 从站只有发送回调。接收方向由传输层把整帧交给 mb_full_slave_handle*()，
 * 从站不自己拉取数据——旧版在这里存过一个 recv 回调，但从来没有任何地方读过它。
 */
static mb_full_send_cb_t g_slave_send_cb = NULL;
static mb_full_tcp_send_cb_t g_slave_tcp_send_cb = NULL;

static mb_full_connect_cb_t g_master_connect_cb = NULL;
static mb_full_send_cb_t g_master_send_cb = NULL;
static mb_full_recv_cb_t g_master_recv_cb = NULL;
static mb_full_tcp_send_cb_t g_master_tcp_send_cb = NULL;
static mb_full_tcp_recv_cb_t g_master_tcp_recv_cb = NULL;
static mb_full_disconnect_cb_t g_master_disconnect_cb = NULL;

/* ==================== 工具函数 ==================== */

uint16_t mb_full_crc16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc = (crc >> 8) ^ crc16_table[(crc ^ data[i]) & 0xFF];
    }
    return crc;
}

uint8_t mb_full_lrc(const uint8_t *data, uint16_t len)
{
    int8_t lrc = 0;
    for (uint16_t i = 0; i < len; i++) {
        lrc = (int8_t)(lrc + data[i]);
    }
    return (uint8_t)(-(int16_t)lrc);
}

static int mb_hex_nibble(uint8_t c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

/**
 * @brief ASCII 帧解码
 *
 * 与 mb_full_ascii_encode() 对称：跳过起始符 ':'，遇到 CR/LF 或非十六进制字符
 * 即结束。LRC 属于被解码的内容（接收方要靠它校验），因此一并返回。
 *
 * @return 解码后的字节数；参数为空时返回 MB_FULL_INVALID_PARAM
 */
int mb_full_ascii_decode(const uint8_t *src, uint16_t src_len, uint8_t *dst)
{
    uint16_t i = 0;
    uint16_t j = 0;

    if (!src || !dst) {
        return MB_FULL_INVALID_PARAM;
    }

    if (src_len > 0U && src[0] == ':') {
        i = 1;
    }

    while (i + 1U < src_len) {
        int hi = mb_hex_nibble(src[i]);
        int lo = mb_hex_nibble(src[i + 1U]);
        if (hi < 0 || lo < 0) {
            break;
        }
        dst[j++] = (uint8_t)((hi << 4) | lo);
        i = (uint16_t)(i + 2U);
    }

    return (int)j;
}

int mb_full_ascii_encode(const uint8_t *src, uint16_t src_len, uint8_t *dst)
{
    static const char hex[] = "0123456789ABCDEF";
    uint16_t j = 0;

    if (!src || !dst) {
        return MB_FULL_INVALID_PARAM;
    }

    dst[j++] = ':';  /* 起始符 */

    for (uint16_t i = 0; i < src_len; i++) {
        dst[j++] = (uint8_t)hex[(src[i] >> 4) & 0x0F];
        dst[j++] = (uint8_t)hex[src[i] & 0x0F];
    }

    /* LRC */
    uint8_t lrc = mb_full_lrc(src, src_len);
    dst[j++] = (uint8_t)hex[(lrc >> 4) & 0x0F];
    dst[j++] = (uint8_t)hex[lrc & 0x0F];

    /* 结束符 */
    dst[j++] = '\r';
    dst[j++] = '\n';

    return (int)j;
}

const char* mb_full_version(void)
{
    return "1.0.0";
}

const char* mb_full_error_string(int error)
{
    switch (error) {
        case MB_FULL_OK: return "OK";
        case MB_FULL_ERROR: return "Error";
        case MB_FULL_INVALID_PARAM: return "Invalid Parameter";
        case MB_FULL_TIMEOUT: return "Timeout";
        case MB_FULL_CRC_ERROR: return "CRC Error";
        case MB_FULL_LRC_ERROR: return "LRC Error";
        case MB_FULL_NOT_INITIALIZED: return "Not Initialized";
        case MB_FULL_NO_MEMORY: return "No Memory";
        default: return "Unknown";
    }
}

/* ==================== 地址检查 ==================== */

/*
 * 位区（线圈/离散输入）按位打包：区域起始地址对应 bit 0，存 count 位需要
 * (count + 7) / 8 字节。读写与响应帧的字节数规则因此是同一套。
 */
static bool mb_bit_area_get(const mb_bit_area_t *area, uint16_t addr)
{
    uint16_t offset = (uint16_t)(addr - area->start_addr);

    return (area->data[offset / 8U] & (uint8_t)(1U << (offset % 8U))) != 0;
}

static void mb_bit_area_set(const mb_bit_area_t *area, uint16_t addr, bool level)
{
    uint16_t offset = (uint16_t)(addr - area->start_addr);
    uint8_t mask = (uint8_t)(1U << (offset % 8U));

    if (level) {
        area->data[offset / 8U] |= mask;
    } else {
        area->data[offset / 8U] &= (uint8_t)~mask;
    }
}

/* 请求是否完整落在已配置区域内；未配置的区域（count == 0）一律不覆盖。 */
static bool mb_area_covers(uint16_t start, uint16_t count, uint16_t addr, uint16_t n)
{
    if (count == 0U) {
        return false;
    }
    if (addr < start) {
        return false;
    }
    return (uint32_t)addr + n <= (uint32_t)start + count;
}

/* 把寄存器区的一段按大端写入响应，返回写入的字节数。 */
static uint16_t mb_put_regs(uint8_t *dst, const mb_reg_area_t *area,
                            uint16_t addr, uint16_t count)
{
    const uint16_t *reg = &area->data[addr - area->start_addr];

    for (uint16_t i = 0; i < count; i++) {
        dst[i * 2U] = (uint8_t)(reg[i] >> 8);
        dst[i * 2U + 1U] = (uint8_t)(reg[i] & 0xFFU);
    }
    return (uint16_t)(count * 2U);
}

/* 把位区的一段按 Modbus 位序（请求首地址落到 bit 0）写入响应，返回字节数。 */
static uint16_t mb_put_bits(uint8_t *dst, const mb_bit_area_t *area,
                            uint16_t addr, uint16_t count)
{
    uint16_t byte_count = (uint16_t)((count + 7U) / 8U);

    memset(dst, 0, byte_count);
    for (uint16_t i = 0; i < count; i++) {
        if (mb_bit_area_get(area, (uint16_t)(addr + i))) {
            dst[i / 8U] |= (uint8_t)(1U << (i % 8U));
        }
    }
    return byte_count;
}

/* 追加 RTU CRC 并返回整帧长度（含 CRC）。 */
static uint16_t mb_rtu_finish(uint8_t *tx, uint16_t len)
{
    uint16_t crc = mb_full_crc16(tx, len);

    tx[len] = (uint8_t)(crc & 0xFFU);
    tx[len + 1U] = (uint8_t)(crc >> 8);
    return (uint16_t)(len + 2U);
}

/* ==================== 从站实现 ==================== */

int mb_full_slave_init(mb_full_slave_t *slave, uint8_t slave_id, mb_mode_t mode)
{
    if (!slave) {
        return MB_FULL_INVALID_PARAM;
    }
    
    memset(slave, 0, sizeof(*slave));
    slave->slave_id = slave_id;
    slave->mode = mode;
    slave->initialized = true;
    
    return MB_FULL_OK;
}

int mb_full_slave_config_coils(mb_full_slave_t *slave, uint8_t *data,
                                uint16_t start_addr, uint16_t count)
{
    if (!slave || !data) {
        return MB_FULL_INVALID_PARAM;
    }
    slave->coils.data = data;
    slave->coils.start_addr = start_addr;
    slave->coils.count = count;
    return MB_FULL_OK;
}

int mb_full_slave_config_discrete(mb_full_slave_t *slave, uint8_t *data,
                                    uint16_t start_addr, uint16_t count)
{
    if (!slave || !data) {
        return MB_FULL_INVALID_PARAM;
    }
    slave->discrete.data = data;
    slave->discrete.start_addr = start_addr;
    slave->discrete.count = count;
    return MB_FULL_OK;
}

int mb_full_slave_config_holding(mb_full_slave_t *slave, uint16_t *data,
                                   uint16_t start_addr, uint16_t count)
{
    if (!slave || !data) {
        return MB_FULL_INVALID_PARAM;
    }
    slave->holding.data = data;
    slave->holding.start_addr = start_addr;
    slave->holding.count = count;
    return MB_FULL_OK;
}

int mb_full_slave_config_input(mb_full_slave_t *slave, uint16_t *data,
                                uint16_t start_addr, uint16_t count)
{
    if (!slave || !data) {
        return MB_FULL_INVALID_PARAM;
    }
    slave->input.data = data;
    slave->input.start_addr = start_addr;
    slave->input.count = count;
    return MB_FULL_OK;
}

void mb_full_slave_set_uart(mb_full_slave_t *slave, mb_full_send_cb_t send_cb)
{
    (void)slave;
    g_slave_send_cb = send_cb;
}

void mb_full_slave_set_tcp(mb_full_slave_t *slave, mb_full_tcp_send_cb_t send_cb)
{
    (void)slave;
    g_slave_tcp_send_cb = send_cb;
}

static void mb_full_slave_send(mb_full_slave_t *slave, const uint8_t *data, uint16_t len)
{
    if (slave->mode == MB_MODE_TCP) {
        if (g_slave_tcp_send_cb) g_slave_tcp_send_cb(data, len);
    } else {
        if (g_slave_send_cb) g_slave_send_cb(data, len);
    }
}

/**
 * @brief 处理一帧请求的 PDU
 *
 * @param func   功能码（等于 req[1]）
 * @param req    指向单元 ID 的第一个字节；req[2..] 是 PDU 数据
 * @param req_len req 中不含 RTU CRC 的字节数
 * @param rsp_len 成功时写入整帧长度（含 RTU CRC）
 *
 * @return 0 成功；否则返回 Modbus 异常码（MB_ERR_*）
 */
static int mb_full_slave_process(mb_full_slave_t *slave, uint8_t func,
                                  const uint8_t *req, uint16_t req_len,
                                  uint16_t *rsp_len)
{
    uint8_t *tx = slave->tx_buf;
    uint16_t addr;
    uint16_t count;
    int error = 0;

    /* 位区响应靠逐位 |= 组装，必须从干净缓冲区开始，否则上一帧的位会留下来。 */
    memset(tx, 0, sizeof(slave->tx_buf));

    switch (func) {
        /* 0x01: 读线圈 / 0x02: 读离散输入 */
        case MB_FUNC_READ_COILS:
        case MB_FUNC_READ_DISCRETE_INPUTS: {
            const mb_bit_area_t *area;

            if (req_len < 6U) {
                error = MB_ERR_ILLEGAL_DATA_VALUE;
                break;
            }

            addr = ((uint16_t)req[2] << 8) | req[3];
            count = ((uint16_t)req[4] << 8) | req[5];
            if (count == 0U || count > 2000U) {
                error = MB_ERR_ILLEGAL_DATA_VALUE;
                break;
            }

            area = (func == MB_FUNC_READ_COILS) ? &slave->coils : &slave->discrete;
            if (!mb_area_covers(area->start_addr, area->count, addr, count)) {
                error = MB_ERR_ILLEGAL_DATA_ADDRESS;
                break;
            }

            tx[0] = slave->slave_id;
            tx[1] = func;
            tx[2] = (uint8_t)mb_put_bits(&tx[3], area, addr, count);
            *rsp_len = mb_rtu_finish(tx, (uint16_t)(3U + tx[2]));
            return 0;
        }

        /* 0x03: 读保持寄存器 / 0x04: 读输入寄存器 */
        case MB_FUNC_READ_HOLDING_REGISTERS:
        case MB_FUNC_READ_INPUT_REGISTERS: {
            const mb_reg_area_t *area;

            if (req_len < 6U) {
                error = MB_ERR_ILLEGAL_DATA_VALUE;
                break;
            }

            addr = ((uint16_t)req[2] << 8) | req[3];
            count = ((uint16_t)req[4] << 8) | req[5];
            if (count == 0U || count > 125U) {
                error = MB_ERR_ILLEGAL_DATA_VALUE;
                break;
            }

            area = (func == MB_FUNC_READ_HOLDING_REGISTERS) ? &slave->holding : &slave->input;
            if (!mb_area_covers(area->start_addr, area->count, addr, count)) {
                error = MB_ERR_ILLEGAL_DATA_ADDRESS;
                break;
            }

            tx[0] = slave->slave_id;
            tx[1] = func;
            tx[2] = (uint8_t)mb_put_regs(&tx[3], area, addr, count);
            *rsp_len = mb_rtu_finish(tx, (uint16_t)(3U + tx[2]));
            return 0;
        }

        /* 0x05: 写单个线圈 */
        case MB_FUNC_WRITE_SINGLE_COIL: {
            uint16_t value;

            if (req_len < 6U) {
                error = MB_ERR_ILLEGAL_DATA_VALUE;
                break;
            }

            addr = ((uint16_t)req[2] << 8) | req[3];
            value = ((uint16_t)req[4] << 8) | req[5];
            if (value != 0xFF00U && value != 0x0000U) {
                error = MB_ERR_ILLEGAL_DATA_VALUE;
                break;
            }
            if (!mb_area_covers(slave->coils.start_addr, slave->coils.count, addr, 1U)) {
                error = MB_ERR_ILLEGAL_DATA_ADDRESS;
                break;
            }

            mb_bit_area_set(&slave->coils, addr, value == 0xFF00U);

            /* 响应 (回显请求前 6 字节) */
            memcpy(tx, req, 6);
            *rsp_len = mb_rtu_finish(tx, 6);
            return 0;
        }

        /* 0x06: 写单个寄存器 */
        case MB_FUNC_WRITE_SINGLE_REGISTER: {
            uint16_t value;

            if (req_len < 6U) {
                error = MB_ERR_ILLEGAL_DATA_VALUE;
                break;
            }

            addr = ((uint16_t)req[2] << 8) | req[3];
            value = ((uint16_t)req[4] << 8) | req[5];
            if (!mb_area_covers(slave->holding.start_addr, slave->holding.count, addr, 1U)) {
                error = MB_ERR_ILLEGAL_DATA_ADDRESS;
                break;
            }

            slave->holding.data[addr - slave->holding.start_addr] = value;

            memcpy(tx, req, 6);
            *rsp_len = mb_rtu_finish(tx, 6);
            return 0;
        }

        /* 0x0F: 写多个线圈 */
        case MB_FUNC_WRITE_MULTIPLE_COILS: {
            uint8_t byte_count;

            if (req_len < 7U) {
                error = MB_ERR_ILLEGAL_DATA_VALUE;
                break;
            }

            addr = ((uint16_t)req[2] << 8) | req[3];
            count = ((uint16_t)req[4] << 8) | req[5];
            byte_count = req[6];
            if (count == 0U || count > 1968U) {
                error = MB_ERR_ILLEGAL_DATA_VALUE;
                break;
            }
            if (byte_count != (uint8_t)((count + 7U) / 8U)) {
                error = MB_ERR_ILLEGAL_DATA_VALUE;
                break;
            }
            if ((uint32_t)7U + byte_count > req_len) {
                error = MB_ERR_ILLEGAL_DATA_VALUE;
                break;
            }
            if (!mb_area_covers(slave->coils.start_addr, slave->coils.count, addr, count)) {
                error = MB_ERR_ILLEGAL_DATA_ADDRESS;
                break;
            }

            for (uint16_t i = 0; i < count; i++) {
                bool level = (req[7U + i / 8U] & (uint8_t)(1U << (i % 8U))) != 0;
                mb_bit_area_set(&slave->coils, (uint16_t)(addr + i), level);
            }

            /* 响应: 单元 ID、功能码、起始地址、数量 */
            tx[0] = slave->slave_id;
            tx[1] = func;
            memcpy(&tx[2], &req[2], 4);
            *rsp_len = mb_rtu_finish(tx, 6);
            return 0;
        }

        /* 0x10: 写多个寄存器 */
        case MB_FUNC_WRITE_MULTIPLE_REGISTERS: {
            uint8_t byte_count;

            if (req_len < 7U) {
                error = MB_ERR_ILLEGAL_DATA_VALUE;
                break;
            }

            addr = ((uint16_t)req[2] << 8) | req[3];
            count = ((uint16_t)req[4] << 8) | req[5];
            byte_count = req[6];
            if (count == 0U || count > 123U) {
                error = MB_ERR_ILLEGAL_DATA_VALUE;
                break;
            }
            if (byte_count != (uint8_t)(count * 2U)) {
                error = MB_ERR_ILLEGAL_DATA_VALUE;
                break;
            }
            if ((uint32_t)7U + byte_count > req_len) {
                error = MB_ERR_ILLEGAL_DATA_VALUE;
                break;
            }
            if (!mb_area_covers(slave->holding.start_addr, slave->holding.count, addr, count)) {
                error = MB_ERR_ILLEGAL_DATA_ADDRESS;
                break;
            }

            for (uint16_t i = 0; i < count; i++) {
                slave->holding.data[addr - slave->holding.start_addr + i] =
                    ((uint16_t)req[7U + i * 2U] << 8) | req[8U + i * 2U];
            }

            tx[0] = slave->slave_id;
            tx[1] = func;
            memcpy(&tx[2], &req[2], 4);
            *rsp_len = mb_rtu_finish(tx, 6);
            return 0;
        }

        /* 0x17: 读写多个寄存器 */
        case MB_FUNC_READ_WRITE_MULTIPLE: {
            uint16_t read_addr;
            uint16_t read_count;
            uint16_t write_addr;
            uint16_t write_count;
            uint8_t write_byte_count;

            if (req_len < 11U) {
                error = MB_ERR_ILLEGAL_DATA_VALUE;
                break;
            }

            read_addr = ((uint16_t)req[2] << 8) | req[3];
            read_count = ((uint16_t)req[4] << 8) | req[5];
            write_addr = ((uint16_t)req[6] << 8) | req[7];
            write_count = ((uint16_t)req[8] << 8) | req[9];
            write_byte_count = req[10];
            if (read_count == 0U || read_count > 125U ||
                write_count == 0U || write_count > 121U) {
                error = MB_ERR_ILLEGAL_DATA_VALUE;
                break;
            }
            if (write_byte_count != (uint8_t)(write_count * 2U)) {
                error = MB_ERR_ILLEGAL_DATA_VALUE;
                break;
            }
            if ((uint32_t)11U + write_byte_count > req_len) {
                error = MB_ERR_ILLEGAL_DATA_VALUE;
                break;
            }
            if (!mb_area_covers(slave->holding.start_addr, slave->holding.count,
                                read_addr, read_count) ||
                !mb_area_covers(slave->holding.start_addr, slave->holding.count,
                                write_addr, write_count)) {
                error = MB_ERR_ILLEGAL_DATA_ADDRESS;
                break;
            }

            /* 规范要求先执行写、再执行读，因此响应里能看到刚写入的值。 */
            for (uint16_t i = 0; i < write_count; i++) {
                slave->holding.data[write_addr - slave->holding.start_addr + i] =
                    ((uint16_t)req[11U + i * 2U] << 8) | req[12U + i * 2U];
            }

            tx[0] = slave->slave_id;
            tx[1] = func;
            tx[2] = (uint8_t)mb_put_regs(&tx[3], &slave->holding, read_addr, read_count);
            *rsp_len = mb_rtu_finish(tx, (uint16_t)(3U + tx[2]));
            return 0;
        }

        /* 0x2E: 读设备标识（MEI 类型 0x0E，仅支持"基本"读取码） */
        case MB_FUNC_READ_DEVICE_ID: {
            if (req_len < 5U) {
                error = MB_ERR_ILLEGAL_DATA_VALUE;
                break;
            }
            if (req[2] != 0x0E || req[3] != 0x01U) {
                error = MB_ERR_ILLEGAL_DATA_VALUE;
                break;
            }
            if (req[4] != (uint8_t)MB_OBJ_BASIC) {
                error = MB_ERR_ILLEGAL_DATA_ADDRESS;
                break;
            }

            tx[0] = slave->slave_id;
            tx[1] = func;
            tx[2] = 0x0E;   /* MEI 类型 */
            tx[3] = 0x01;   /* 基本读取码 */
            tx[4] = 0x01;   /* 一致性级别: 基本 */
            tx[5] = 0x00;   /* 后续对象: 无 */
            tx[6] = 0x00;   /* 下一个对象 ID */
            tx[7] = 0x01;   /* 对象数量 */
            tx[8] = 0x00;   /* 对象 ID: 基本 */
            tx[9] = 0x01;   /* 对象长度 */
            tx[10] = slave->device_id.device_id;
            *rsp_len = mb_rtu_finish(tx, 11);
            return 0;
        }

        default:
            error = MB_ERR_ILLEGAL_FUNCTION;
            break;
    }

    return error;
}

/**
 * @brief 从站处理 RTU 帧
 */
int mb_full_slave_handle(mb_full_slave_t *slave, const uint8_t *data, uint16_t len)
{
    uint16_t crc_rx;
    uint16_t crc_calc;
    uint16_t rsp_len = 0;
    int error;

    if (!slave || !data || len < 4U) {
        return MB_FULL_INVALID_PARAM;
    }

    /* RTU: 验证 CRC */
    crc_rx = ((uint16_t)data[len - 1U] << 8) | data[len - 2U];
    crc_calc = mb_full_crc16(data, (uint16_t)(len - 2U));
    if (crc_rx != crc_calc) {
        slave->crc_error_count++;
        slave->error_count++;
        return MB_FULL_CRC_ERROR;
    }

    /* 检查从站 ID；不是发给自己的帧既不应答也不计数 */
    if (data[0] != slave->slave_id) {
        return MB_FULL_OK;
    }

    error = mb_full_slave_process(slave, data[1], data, (uint16_t)(len - 2U), &rsp_len);
    if (error != 0) {
        /* 异常响应: 单元 ID、功能码 | 0x80、异常码、CRC */
        slave->tx_buf[0] = slave->slave_id;
        slave->tx_buf[1] = (uint8_t)(data[1] | 0x80U);
        slave->tx_buf[2] = (uint8_t)error;
        rsp_len = mb_rtu_finish(slave->tx_buf, 3);
        slave->error_count++;
    } else {
        slave->request_count++;
    }

    slave->tx_len = rsp_len;
    mb_full_slave_send(slave, slave->tx_buf, rsp_len);
    return MB_FULL_OK;
}

/**
 * @brief 从站处理 Modbus TCP 帧
 *
 * MBAP 头（事务 ID、协议 ID、长度）在线上是大端，这里逐字节解析/组装，
 * 不把结构体直接盖在缓冲区上——那样在小端机器上事务 ID 会整体翻转。
 */
int mb_full_slave_handle_tcp(mb_full_slave_t *slave, const uint8_t *data, uint16_t len)
{
    uint8_t rsp[MB_FULL_MAX_TCP_ADU_SIZE];
    uint16_t transaction_id;
    uint16_t protocol_id;
    uint16_t length;
    uint16_t rsp_len = 0;
    uint16_t pdu_len;
    int error;

    if (!slave || !data || len < 9U) {
        return MB_FULL_INVALID_PARAM;
    }

    transaction_id = ((uint16_t)data[0] << 8) | data[1];
    protocol_id = ((uint16_t)data[2] << 8) | data[3];
    length = ((uint16_t)data[4] << 8) | data[5];

    if (protocol_id != 0U) {
        return MB_FULL_INVALID_PARAM;
    }
    /* 长度覆盖单元 ID + PDU，必须与实际到达的字节数一致 */
    if (length < 3U || (uint16_t)(length + 6U) != len) {
        return MB_FULL_INVALID_PARAM;
    }

    /* 检查从站 ID；不是发给自己的帧既不应答也不计数 */
    if (data[6] != slave->slave_id) {
        return MB_FULL_OK;
    }

    error = mb_full_slave_process(slave, data[7], data + 6, length, &rsp_len);

    rsp[0] = (uint8_t)(transaction_id >> 8);
    rsp[1] = (uint8_t)(transaction_id & 0xFFU);
    rsp[2] = 0x00;
    rsp[3] = 0x00;
    rsp[6] = slave->slave_id;

    if (error != 0) {
        rsp[4] = 0x00;
        rsp[5] = 0x03;  /* 异常响应固定 3 字节: 单元 ID + 功能码 + 异常码 */
        rsp[7] = (uint8_t)(data[7] | 0x80U);
        rsp[8] = (uint8_t)error;
        rsp_len = 9;
        slave->error_count++;
    } else {
        /* tx_buf 是整条 RTU 帧（单元 ID + PDU + CRC）。TCP 载荷 = PDU（去掉单元 ID 与 CRC），
           长度字段再算上 MBAP 尾部的单元 ID。 */
        pdu_len = (uint16_t)(rsp_len - 3U);
        rsp[4] = (uint8_t)((pdu_len + 1U) >> 8);
        rsp[5] = (uint8_t)((pdu_len + 1U) & 0xFFU);
        memcpy(&rsp[7], &slave->tx_buf[1], pdu_len);
        rsp_len = (uint16_t)(7U + pdu_len);
        slave->request_count++;
    }

    slave->tx_len = rsp_len;
    mb_full_slave_send(slave, rsp, rsp_len);
    return MB_FULL_OK;
}

/* ==================== 主站实现 ==================== */

int mb_full_master_init(mb_full_master_t *master, mb_mode_t mode)
{
    if (!master) {
        return MB_FULL_INVALID_PARAM;
    }
    
    memset(master, 0, sizeof(*master));
    master->mode = mode;
    master->timeout_ms = MB_FULL_TIMEOUT_MS;
    master->initialized = true;
    
    return MB_FULL_OK;
}

void mb_full_master_set_timeout(mb_full_master_t *master, uint32_t timeout_ms)
{
    if (master) {
        master->timeout_ms = timeout_ms;
    }
}

void mb_full_master_set_uart(mb_full_master_t *master, 
                              mb_full_send_cb_t send_cb,
                              mb_full_recv_cb_t recv_cb)
{
    (void)master;
    g_master_send_cb = send_cb;
    g_master_recv_cb = recv_cb;
}

void mb_full_master_set_tcp(mb_full_master_t *master,
                             mb_full_connect_cb_t connect_cb,
                             mb_full_tcp_send_cb_t send_cb,
                             mb_full_tcp_recv_cb_t recv_cb,
                             mb_full_disconnect_cb_t disconnect_cb)
{
    (void)master;
    g_master_connect_cb = connect_cb;
    g_master_tcp_send_cb = send_cb;
    g_master_tcp_recv_cb = recv_cb;
    g_master_disconnect_cb = disconnect_cb;
}

int mb_full_master_connect(mb_full_master_t *master, const char *host, uint16_t port)
{
    (void)master;
    (void)host;
    (void)port;
    
    if (g_master_connect_cb) {
        return g_master_connect_cb(host, port);
    }
    return MB_FULL_ERROR;
}

int mb_full_master_disconnect(mb_full_master_t *master)
{
    (void)master;
    
    if (g_master_disconnect_cb) {
        return g_master_disconnect_cb();
    }
    return MB_FULL_ERROR;
}

static int mb_full_master_tx(mb_full_master_t *master, uint8_t *data, uint16_t len)
{
    (void)master;
    if (master->mode == MB_MODE_TCP) {
        if (g_master_tcp_send_cb) return g_master_tcp_send_cb(data, len);
    } else {
        if (g_master_send_cb) return g_master_send_cb(data, len);
    }
    return MB_FULL_ERROR;
}

static int mb_full_master_rx(mb_full_master_t *master, uint8_t *data, uint16_t len)
{
    (void)master;
    if (master->mode == MB_MODE_TCP) {
        if (g_master_tcp_recv_cb) return g_master_tcp_recv_cb(data, len, master->timeout_ms);
    } else {
        if (g_master_recv_cb) return g_master_recv_cb(data, len, master->timeout_ms);
    }
    return MB_FULL_ERROR;
}

/**
 * @brief 主站发送请求并接收响应
 */
static int mb_full_master_request(mb_full_master_t *master, uint8_t slave_id,
                                   const uint8_t *req, uint16_t req_len,
                                   uint8_t *rsp, uint16_t *rsp_len)
{
    uint8_t tx[MB_FULL_MAX_TCP_ADU_SIZE];
    uint16_t tx_len = 0;
    int ret;
    
    /* 构建 TCP 头 (如果是 TCP 模式) */
    if (master->mode == MB_MODE_TCP) {
        mb_tcp_header_t *hdr = (mb_tcp_header_t *)tx;
        hdr->transaction_id = master->conn.transaction_id++;
        hdr->protocol_id = 0;
        hdr->length = req_len + 1;  /* + unit_id */
        hdr->unit_id = slave_id;
        
        memcpy(tx + 7, req, req_len);
        tx_len = 7 + req_len;
    } else {
        tx[0] = slave_id;
        memcpy(tx + 1, req, req_len);
        tx_len = 1 + req_len;
        
        /* 添加 CRC */
        uint16_t crc = mb_full_crc16(tx, tx_len);
        tx[tx_len++] = crc & 0xFF;
        tx[tx_len++] = (crc >> 8) & 0xFF;
    }
    
    /* 发送 */
    ret = mb_full_master_tx(master, tx, tx_len);
    if (ret < 0) {
        master->error_count++;
        return MB_FULL_ERROR;
    }
    
    /* 接收 */
    ret = mb_full_master_rx(master, rsp, MB_FULL_MAX_TCP_ADU_SIZE);
    if (ret < 5) {
        master->error_count++;
        return MB_FULL_TIMEOUT;
    }
    
    *rsp_len = ret;
    master->request_count++;
    return MB_FULL_OK;
}

int mb_full_master_read_coils(mb_full_master_t *master, uint8_t slave_id,
                               uint16_t addr, uint16_t count, uint8_t *data)
{
    uint8_t req[6], rsp[MB_FULL_MAX_TCP_ADU_SIZE];
    uint16_t rsp_len;
    int ret;
    
    if (!master || !data || count == 0 || count > 2000) {
        return MB_FULL_INVALID_PARAM;
    }
    
    req[0] = MB_FUNC_READ_COILS;
    req[1] = (addr >> 8) & 0xFF;
    req[2] = addr & 0xFF;
    req[3] = (count >> 8) & 0xFF;
    req[4] = count & 0xFF;
    
    ret = mb_full_master_request(master, slave_id, req, 5, rsp, &rsp_len);
    if (ret != MB_FULL_OK) return ret;
    
    /* 检查错误 */
    if (rsp[1] & 0x80) {
        master->error_count++;
        return MB_FULL_ERROR;
    }
    
    /* 复制数据 */
    uint8_t byte_count = rsp[2];
    memcpy(data, &rsp[3], byte_count);
    
    return MB_FULL_OK;
}

int mb_full_master_read_discrete(mb_full_master_t *master, uint8_t slave_id,
                                  uint16_t addr, uint16_t count, uint8_t *data)
{
    uint8_t req[6], rsp[MB_FULL_MAX_TCP_ADU_SIZE];
    uint16_t rsp_len;
    int ret;
    
    if (!master || !data || count == 0 || count > 2000) {
        return MB_FULL_INVALID_PARAM;
    }
    
    req[0] = MB_FUNC_READ_DISCRETE_INPUTS;
    req[1] = (addr >> 8) & 0xFF;
    req[2] = addr & 0xFF;
    req[3] = (count >> 8) & 0xFF;
    req[4] = count & 0xFF;
    
    ret = mb_full_master_request(master, slave_id, req, 5, rsp, &rsp_len);
    if (ret != MB_FULL_OK) return ret;
    
    if (rsp[1] & 0x80) {
        master->error_count++;
        return MB_FULL_ERROR;
    }
    
    uint8_t byte_count = rsp[2];
    memcpy(data, &rsp[3], byte_count);
    
    return MB_FULL_OK;
}

int mb_full_master_read_holding(mb_full_master_t *master, uint8_t slave_id,
                                 uint16_t addr, uint16_t count, uint16_t *data)
{
    uint8_t req[6], rsp[MB_FULL_MAX_TCP_ADU_SIZE];
    uint16_t rsp_len;
    int ret;
    
    if (!master || !data || count == 0 || count > 125) {
        return MB_FULL_INVALID_PARAM;
    }
    
    req[0] = MB_FUNC_READ_HOLDING_REGISTERS;
    req[1] = (addr >> 8) & 0xFF;
    req[2] = addr & 0xFF;
    req[3] = (count >> 8) & 0xFF;
    req[4] = count & 0xFF;
    
    ret = mb_full_master_request(master, slave_id, req, 5, rsp, &rsp_len);
    if (ret != MB_FULL_OK) return ret;
    
    if (rsp[1] & 0x80) {
        master->error_count++;
        return MB_FULL_ERROR;
    }
    
    uint8_t byte_count = rsp[2];
    for (uint8_t i = 0; i < byte_count / 2; i++) {
        data[i] = ((uint16_t)rsp[3 + i * 2] << 8) | rsp[4 + i * 2];
    }
    
    return MB_FULL_OK;
}

int mb_full_master_read_input(mb_full_master_t *master, uint8_t slave_id,
                               uint16_t addr, uint16_t count, uint16_t *data)
{
    uint8_t req[6], rsp[MB_FULL_MAX_TCP_ADU_SIZE];
    uint16_t rsp_len;
    int ret;
    
    if (!master || !data || count == 0 || count > 125) {
        return MB_FULL_INVALID_PARAM;
    }
    
    req[0] = MB_FUNC_READ_INPUT_REGISTERS;
    req[1] = (addr >> 8) & 0xFF;
    req[2] = addr & 0xFF;
    req[3] = (count >> 8) & 0xFF;
    req[4] = count & 0xFF;
    
    ret = mb_full_master_request(master, slave_id, req, 5, rsp, &rsp_len);
    if (ret != MB_FULL_OK) return ret;
    
    if (rsp[1] & 0x80) {
        master->error_count++;
        return MB_FULL_ERROR;
    }
    
    uint8_t byte_count = rsp[2];
    for (uint8_t i = 0; i < byte_count / 2; i++) {
        data[i] = ((uint16_t)rsp[3 + i * 2] << 8) | rsp[4 + i * 2];
    }
    
    return MB_FULL_OK;
}

int mb_full_master_write_coil(mb_full_master_t *master, uint8_t slave_id,
                               uint16_t addr, uint16_t value)
{
    uint8_t req[6], rsp[8];
    uint16_t rsp_len;
    int ret;
    
    if (!master) {
        return MB_FULL_INVALID_PARAM;
    }
    
    req[0] = MB_FUNC_WRITE_SINGLE_COIL;
    req[1] = (addr >> 8) & 0xFF;
    req[2] = addr & 0xFF;
    req[3] = (value != 0) ? 0xFF : 0x00;
    req[4] = 0x00;
    
    ret = mb_full_master_request(master, slave_id, req, 5, rsp, &rsp_len);
    if (ret != MB_FULL_OK) return ret;
    
    if (rsp[1] & 0x80) {
        master->error_count++;
        return MB_FULL_ERROR;
    }
    
    return MB_FULL_OK;
}

int mb_full_master_write_register(mb_full_master_t *master, uint8_t slave_id,
                                   uint16_t addr, uint16_t value)
{
    uint8_t req[6], rsp[8];
    uint16_t rsp_len;
    int ret;
    
    if (!master) {
        return MB_FULL_INVALID_PARAM;
    }
    
    req[0] = MB_FUNC_WRITE_SINGLE_REGISTER;
    req[1] = (addr >> 8) & 0xFF;
    req[2] = addr & 0xFF;
    req[3] = (value >> 8) & 0xFF;
    req[4] = value & 0xFF;
    
    ret = mb_full_master_request(master, slave_id, req, 5, rsp, &rsp_len);
    if (ret != MB_FULL_OK) return ret;
    
    if (rsp[1] & 0x80) {
        master->error_count++;
        return MB_FULL_ERROR;
    }
    
    return MB_FULL_OK;
}

int mb_full_master_write_coils(mb_full_master_t *master, uint8_t slave_id,
                                uint16_t addr, uint16_t count, const uint8_t *data)
{
    uint8_t req[MB_FULL_MAX_TCP_ADU_SIZE];
    uint8_t rsp[8];
    uint16_t rsp_len;
    uint16_t byte_count = (count + 7) / 8;
    int ret;
    
    if (!master || !data || count == 0 || count > 2000) {
        return MB_FULL_INVALID_PARAM;
    }
    
    req[0] = MB_FUNC_WRITE_MULTIPLE_COILS;
    req[1] = (addr >> 8) & 0xFF;
    req[2] = addr & 0xFF;
    req[3] = (count >> 8) & 0xFF;
    req[4] = count & 0xFF;
    req[5] = byte_count;
    memcpy(&req[6], data, byte_count);
    
    ret = mb_full_master_request(master, slave_id, req, 6 + byte_count, rsp, &rsp_len);
    if (ret != MB_FULL_OK) return ret;
    
    if (rsp[1] & 0x80) {
        master->error_count++;
        return MB_FULL_ERROR;
    }
    
    return MB_FULL_OK;
}

int mb_full_master_write_registers(mb_full_master_t *master, uint8_t slave_id,
                                    uint16_t addr, uint16_t count, const uint16_t *data)
{
    uint8_t req[MB_FULL_MAX_TCP_ADU_SIZE];
    uint8_t rsp[8];
    uint16_t rsp_len;
    int ret;
    
    if (!master || !data || count == 0 || count > 123) {
        return MB_FULL_INVALID_PARAM;
    }
    
    req[0] = MB_FUNC_WRITE_MULTIPLE_REGISTERS;
    req[1] = (addr >> 8) & 0xFF;
    req[2] = addr & 0xFF;
    req[3] = (count >> 8) & 0xFF;
    req[4] = count & 0xFF;
    req[5] = count * 2;
    
    for (uint16_t i = 0; i < count; i++) {
        req[6 + i * 2] = (data[i] >> 8) & 0xFF;
        req[7 + i * 2] = data[i] & 0xFF;
    }
    
    ret = mb_full_master_request(master, slave_id, req, 6 + count * 2, rsp, &rsp_len);
    if (ret != MB_FULL_OK) return ret;
    
    if (rsp[1] & 0x80) {
        master->error_count++;
        return MB_FULL_ERROR;
    }
    
    return MB_FULL_OK;
}

int mb_full_master_read_write_registers(mb_full_master_t *master, uint8_t slave_id,
                                         uint16_t read_addr, uint16_t read_count,
                                         uint16_t *read_data,
                                         uint16_t write_addr, uint16_t write_count,
                                         const uint16_t *write_data)
{
    uint8_t req[MB_FULL_MAX_TCP_ADU_SIZE];
    uint8_t rsp[MB_FULL_MAX_TCP_ADU_SIZE];
    uint16_t rsp_len;
    int ret;
    
    if (!master || !read_data || !write_data) {
        return MB_FULL_INVALID_PARAM;
    }
    
    req[0] = MB_FUNC_READ_WRITE_MULTIPLE;
    req[1] = (read_addr >> 8) & 0xFF;
    req[2] = read_addr & 0xFF;
    req[3] = (read_count >> 8) & 0xFF;
    req[4] = read_count & 0xFF;
    req[5] = (write_addr >> 8) & 0xFF;
    req[6] = write_addr & 0xFF;
    req[7] = (write_count >> 8) & 0xFF;
    req[8] = write_count & 0xFF;
    req[9] = write_count * 2;
    
    for (uint16_t i = 0; i < write_count; i++) {
        req[10 + i * 2] = (write_data[i] >> 8) & 0xFF;
        req[11 + i * 2] = write_data[i] & 0xFF;
    }
    
    ret = mb_full_master_request(master, slave_id, req, 10 + write_count * 2, rsp, &rsp_len);
    if (ret != MB_FULL_OK) return ret;
    
    if (rsp[1] & 0x80) {
        master->error_count++;
        return MB_FULL_ERROR;
    }
    
    /* 复制读回的数据 */
    uint8_t byte_count = rsp[2];
    for (uint8_t i = 0; i < byte_count / 2; i++) {
        read_data[i] = ((uint16_t)rsp[3 + i * 2] << 8) | rsp[4 + i * 2];
    }
    
    return MB_FULL_OK;
}

int mb_full_master_read_device_id(mb_full_master_t *master, uint8_t slave_id,
                                    mb_object_id_t object_id, uint8_t *data)
{
    uint8_t req[3], rsp[MB_FULL_MAX_TCP_ADU_SIZE];
    uint16_t rsp_len;
    int ret;
    
    if (!master || !data) {
        return MB_FULL_INVALID_PARAM;
    }
    
    req[0] = MB_FUNC_READ_DEVICE_ID;
    req[1] = 0x0E;  /* 读设备 ID */
    req[2] = (uint8_t)object_id;
    
    ret = mb_full_master_request(master, slave_id, req, 3, rsp, &rsp_len);
    if (ret != MB_FULL_OK) return ret;
    
    if (rsp[1] & 0x80) {
        master->error_count++;
        return MB_FULL_ERROR;
    }
    
    memcpy(data, &rsp[2], rsp_len - 2);
    
    return MB_FULL_OK;
}
