# TLV Protocol Specification for SmartCard-USB Bridge

## 版本信息 / Version Information

- **版本 / Version**: 1.0.0
- **日期 / Date**: 2025/10/30
- **作者 / Author**: WCH Development Team

## 1. 概述 / Overview

本TLV (Tag-Length-Value) 协议用于PC与CH32X035智能卡桥接设备之间的通讯。协议设计简单、高效，支持APDU命令透传、智能卡控制和状态查询。

This TLV (Tag-Length-Value) protocol is used for communication between PC and CH32X035 SmartCard bridge device. The protocol is designed to be simple and efficient, supporting APDU command passthrough, smart card control, and status query.

## 2. 协议格式 / Protocol Format

### 2.1 基本格式 / Basic Format

每个TLV数据包由三部分组成:

```
┌─────────────────────────────────────────────────┐
│  Tag (1 byte)  │  Length (2 bytes)  │  Value   │
│     标签        │      长度           │   数据   │
└─────────────────────────────────────────────────┘
```

- **Tag**: 1字节，标识数据类型
- **Length**: 2字节，大端序(Big-Endian)，表示Value字段的长度
- **Value**: 可变长度，最大512字节

### 2.2 字节序 / Byte Order

- **Length字段**: 大端序(Big-Endian) - 高位字节在前
- **Value字段**: 根据具体数据类型而定

示例:
```
Length = 0x0104 (260字节)
在数据包中表示为: 0x01 0x04
```

## 3. 标签定义 / Tag Definitions

### 3.1 命令标签 (PC → MCU) / Command Tags

| Tag值 | 标签名称          | Value内容      | 说明                     |
|-------|------------------|----------------|--------------------------|
| 0x01  | APDU_REQUEST     | APDU命令       | 发送APDU到智能卡         |
| 0x04  | RESET_SIM        | 空             | 复位智能卡并获取ATR      |
| 0x05  | POWER_ON         | 空             | 给智能卡上电             |
| 0x06  | POWER_OFF        | 空             | 给智能卡断电             |
| 0x07  | STATUS_QUERY     | 空             | 查询当前状态             |
| 0x0B  | GET_INFO         | 空             | 获取智能卡信息           |

### 3.2 响应标签 (MCU → PC) / Response Tags

| Tag值 | 标签名称          | Value内容      | 说明                     |
|-------|------------------|----------------|--------------------------|
| 0x02  | APDU_RESPONSE    | APDU响应       | 智能卡APDU响应           |
| 0x03  | ATR_DATA         | ATR数据        | 智能卡ATR数据            |
| 0x08  | STATUS_RESPONSE  | 状态数据       | 当前状态信息             |
| 0x09  | ERROR            | 错误码(1字节)  | 错误响应                 |
| 0x0A  | ACK              | 空             | 确认响应                 |
| 0x0C  | INFO_RESPONSE    | 卡信息         | 智能卡详细信息           |

## 4. 数据格式定义 / Data Format Definitions

### 4.1 APDU_REQUEST (0x01)

发送APDU命令到智能卡。

**格式**:
```
Tag: 0x01
Length: APDU命令长度 (通常5-261字节)
Value: 完整的APDU命令
```

**APDU格式** (ISO 7816-4):
```
CLA INS P1 P2 [Lc Data] [Le]
```

**示例 - GET CHALLENGE**:
```
Tag:    0x01
Length: 0x00 0x05
Value:  0x00 0x84 0x00 0x00 0x04
        │    │    │    │    │
        CLA  INS  P1   P2   Le
```

### 4.2 APDU_RESPONSE (0x02)

智能卡的APDU响应数据。

**格式**:
```
Tag: 0x02
Length: 响应数据长度
Value: [响应数据] SW1 SW2
```

**示例**:
```
Tag:    0x02
Length: 0x00 0x06
Value:  0x12 0x34 0x56 0x78 0x90 0x00
        │─────数据──────────│  │─SW─│
```

### 4.3 ATR_DATA (0x03)

智能卡的ATR (Answer To Reset) 数据。

**格式**:
```
Tag: 0x03
Length: ATR长度+1
Value: [Protocol] [ATR数据]
       │         │
       1字节     原始ATR数据
```

**Protocol字节**:
- 0x00: T=0 协议
- 0x01: T=1 协议

**示例**:
```
Tag:    0x03
Length: 0x00 0x0F
Value:  0x00 0x3B 0x9F 0x95 0x80 0x1F 0xC7 0x80
        │    │─────────ATR数据──────────────────
        T=0
        0x31 0xE0 0x73 0xFE 0x21 0x1B 0x66 0xD0
        ───────────────────────────────────────│
```

### 4.4 RESET_SIM (0x04)

复位智能卡命令。

**格式**:
```
Tag: 0x04
Length: 0x00 0x00
Value: (空)
```

**响应**: ATR_DATA (0x03) 或 ERROR (0x09)

### 4.5 POWER_ON (0x05)

智能卡上电命令。

**格式**:
```
Tag: 0x05
Length: 0x00 0x00
Value: (空)
```

**响应**: ACK (0x0A) 或 ERROR (0x09)

### 4.6 POWER_OFF (0x06)

智能卡断电命令。

**格式**:
```
Tag: 0x06
Length: 0x00 0x00
Value: (空)
```

**响应**: ACK (0x0A)

### 4.7 STATUS_QUERY (0x07)

查询当前状态。

**格式**:
```
Tag: 0x07
Length: 0x00 0x00
Value: (空)
```

**响应**: STATUS_RESPONSE (0x08)

### 4.8 STATUS_RESPONSE (0x08)

状态响应数据。

**格式**:
```
Tag: 0x08
Length: 0x00 0x04
Value: [卡状态] [激活状态] [ATR有效] [协议类型]
       │        │          │          │
       1字节    1字节      1字节      1字节
```

**字节定义**:
- Byte 0 - 卡状态:
  - 0x00: 空闲
  - 0x01: 卡存在
  - 0x02: 卡激活
- Byte 1 - 激活状态:
  - 0x00: 未激活
  - 0x02: 已激活
- Byte 2 - ATR有效性:
  - 0x00: ATR无效
  - 0x01: ATR有效
- Byte 3 - 协议类型:
  - 0x00: T=0
  - 0x01: T=1

### 4.9 ERROR (0x09)

错误响应。

**格式**:
```
Tag: 0x09
Length: 0x00 0x01
Value: [错误码]
       │
       1字节
```

**错误码定义**:

| 错误码 | 名称                 | 说明                     |
|--------|---------------------|--------------------------|
| 0x00   | ERR_NONE            | 无错误                   |
| 0x01   | ERR_INVALID_TAG     | 无效的TLV标签            |
| 0x02   | ERR_INVALID_LENGTH  | 无效的长度               |
| 0x03   | ERR_BUFFER_OVERFLOW | 缓冲区溢出               |
| 0x04   | ERR_NO_CARD         | 未检测到卡               |
| 0x05   | ERR_CARD_ERROR      | 卡通讯错误               |
| 0x06   | ERR_ATR_PARSE_FAILED| ATR解析失败              |
| 0x07   | ERR_APDU_FAILED     | APDU命令执行失败         |
| 0x08   | ERR_TIMEOUT         | 超时错误                 |

### 4.10 ACK (0x0A)

确认响应。

**格式**:
```
Tag: 0x0A
Length: 0x00 0x00
Value: (空)
```

### 4.11 GET_INFO (0x0B)

获取智能卡详细信息。

**格式**:
```
Tag: 0x0B
Length: 0x00 0x00
Value: (空)
```

**响应**: INFO_RESPONSE (0x0C) 或 ERROR (0x09)

### 4.12 INFO_RESPONSE (0x0C)

智能卡信息响应。

**格式**:
```
Tag: 0x0C
Length: 信息长度
Value: [协议] [历史字节数] [可选:更多信息]
       │      │            │
       1字节  1字节        可变
```

## 5. 通讯流程示例 / Communication Flow Examples

### 5.1 完整的智能卡初始化和APDU流程

```
Step 1: PC 复位智能卡
PC  → MCU: [0x04][0x00][0x00]
           RESET_SIM

Step 2: MCU 返回ATR
MCU → PC:  [0x03][0x00][0x0E][0x00][0x3B][0x9F]...[ATR数据]
           ATR_DATA

Step 3: PC 发送APDU (SELECT)
PC  → MCU: [0x01][0x00][0x0F][0x00][0xA4][0x04][0x00][0x0A]...
           APDU_REQUEST

Step 4: MCU 返回APDU响应
MCU → PC:  [0x02][0x00][0x12][数据...][0x90][0x00]
           APDU_RESPONSE

Step 5: PC 查询状态
PC  → MCU: [0x07][0x00][0x00]
           STATUS_QUERY

Step 6: MCU 返回状态
MCU → PC:  [0x08][0x00][0x04][0x01][0x02][0x01][0x00]
           STATUS_RESPONSE
```

### 5.2 错误处理流程

```
Step 1: PC 发送APDU (无卡时)
PC  → MCU: [0x01][0x00][0x05][0x00][0x84][0x00][0x00][0x04]
           APDU_REQUEST

Step 2: MCU 返回错误
MCU → PC:  [0x09][0x00][0x01][0x04]
           ERROR (ERR_NO_CARD)
```

## 6. 实现建议 / Implementation Recommendations

### 6.1 超时设置

- **APDU超时**: 建议5秒
- **ATR接收超时**: 建议2秒
- **USB传输超时**: 建议1秒

### 6.2 缓冲区大小

- **接收缓冲区**: 最小1024字节
- **TLV最大包长**: 515字节 (3字节头 + 512字节数据)
- **USB包大小**: 64字节 (全速模式)

### 6.3 错误恢复

1. TLV解析错误时，丢弃当前缓冲区
2. APDU超时后，发送ERROR响应
3. 卡移除后，自动发送POWER_OFF

## 7. 安全考虑 / Security Considerations

1. **长度校验**: 严格校验Length字段，防止缓冲区溢出
2. **标签验证**: 拒绝未定义的Tag值
3. **状态检查**: APDU命令前确认卡已激活
4. **超时保护**: 所有操作设置合理超时

## 8. 扩展性 / Extensibility

协议保留Tag值范围:
- 0x00: 保留
- 0x0D - 0x7F: 未来扩展使用
- 0x80 - 0xFF: 厂商自定义

## 9. 版本历史 / Version History

- **v1.0.0** (2025/10/30): 初始版本

## 10. 参考文档 / References

- ISO/IEC 7816-3: Smart cards - Electronic signals
- ISO/IEC 7816-4: Smart cards - Organization, security and commands
- USB CDC Class Specification 1.2
