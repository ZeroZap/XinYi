# SmartCard-USB Bridge System

## 项目概述 / Project Overview

本项目实现了一个基于 CH32X035 MCU 的智能卡-USB桥接系统。MCU通过UART智能卡模式与SIM卡通讯，并通过USB-CDC接口接收PC机的APDU请求并透传给SIM卡。所有通讯使用TLV协议进行封装。

This project implements a SmartCard-USB bridge system based on CH32X035 MCU. The MCU communicates with SIM cards via UART SmartCard mode and receives APDU requests from PC via USB-CDC interface, forwarding them to the SIM card. All communications use TLV protocol encapsulation.

## 系统架构 / System Architecture

```
┌─────────────┐         USB-CDC          ┌──────────────┐
│             │◄───────TLV Protocol──────►│              │
│   PC 主机   │                           │  CH32X035    │
│             │     APDU Request/Resp     │     MCU      │
└─────────────┘                           │              │
                                          └──────┬───────┘
                                                 │
                                          SmartCard UART
                                          (ISO 7816-3)
                                                 │
                                          ┌──────▼───────┐
                                          │   SIM Card   │
                                          │   智能卡     │
                                          └──────────────┘
```

## 主要功能 / Key Features

1. **智能卡接口 / SmartCard Interface**
   - UART SmartCard 模式 (ISO 7816-3)
   - 自动ATR解析
   - T=0 协议支持
   - PPS协议切换支持

2. **USB-CDC 接口 / USB-CDC Interface**
   - 全速USB设备模式
   - 虚拟串口通讯
   - DMA传输优化

3. **TLV 协议封装 / TLV Protocol**
   - 灵活的命令/响应封装
   - 支持APDU透传
   - 错误处理机制
   - 状态查询功能

## 文件结构 / File Structure

```
SmartCard_USB_Bridge/User/
├── main.c                  - 主程序入口
├── smartcard.h/.c          - 智能卡驱动
├── tlv_protocol.h/.c       - TLV协议实现
├── UART.h/.c               - USB-CDC UART实现
├── ch32x035_usbfs_device.h/.c - USB设备驱动
├── usb_desc.h/.c           - USB描述符
├── ch32x035_it.h/.c        - 中断处理
├── ch32x035_conf.h         - 外设配置
└── system_ch32x035.h/.c    - 系统初始化
```

## 硬件连接 / Hardware Connections

### 智能卡接口 / SmartCard Interface
```
CH32X035 Pin    →    SIM Card
─────────────────────────────
PA2 (SC_IO)     →    I/O
PA4 (SC_CK)     →    CLK
PA5 (SC_RST)    →    RST
3.3V            →    VCC
GND             →    GND
```

### USB接口 / USB Interface
```
CH32X035 Pin    →    USB
─────────────────────────
PC16 (USB_DM)   →    D-
PC17 (USB_DP)   →    D+
```

## TLV 协议规范 / TLV Protocol Specification

### 数据包格式 / Packet Format

```
┌──────────┬──────────────┬──────────────────┐
│   Tag    │    Length    │      Value       │
│  (1字节)  │   (2字节BE)  │   (N字节)        │
└──────────┴──────────────┴──────────────────┘
```

### 支持的标签 / Supported Tags

| Tag  | 名称 / Name           | 方向 / Direction | 描述 / Description                    |
|------|-----------------------|------------------|---------------------------------------|
| 0x01 | APDU_REQUEST          | PC → MCU         | APDU命令请求                          |
| 0x02 | APDU_RESPONSE         | MCU → PC         | APDU命令响应                          |
| 0x03 | ATR_DATA              | MCU → PC         | ATR数据                               |
| 0x04 | RESET_SIM             | PC → MCU         | 复位SIM卡                             |
| 0x05 | POWER_ON              | PC → MCU         | 上电SIM卡                             |
| 0x06 | POWER_OFF             | PC → MCU         | 下电SIM卡                             |
| 0x07 | STATUS_QUERY          | PC → MCU         | 查询状态                              |
| 0x08 | STATUS_RESPONSE       | MCU → PC         | 状态响应                              |
| 0x09 | ERROR                 | MCU → PC         | 错误响应                              |
| 0x0A | ACK                   | MCU → PC         | 确认响应                              |
| 0x0B | GET_INFO              | PC → MCU         | 获取卡信息                            |
| 0x0C | INFO_RESPONSE         | MCU → PC         | 卡信息响应                            |

### 错误代码 / Error Codes

| Code | 名称 / Name              | 描述 / Description        |
|------|--------------------------|---------------------------|
| 0x00 | ERR_NONE                 | 无错误                    |
| 0x01 | ERR_INVALID_TAG          | 无效的TLV标签             |
| 0x02 | ERR_INVALID_LENGTH       | 无效的长度                |
| 0x03 | ERR_BUFFER_OVERFLOW      | 缓冲区溢出                |
| 0x04 | ERR_NO_CARD              | 未检测到卡                |
| 0x05 | ERR_CARD_ERROR           | 卡通讯错误                |
| 0x06 | ERR_ATR_PARSE_FAILED     | ATR解析失败               |
| 0x07 | ERR_APDU_FAILED          | APDU命令失败              |
| 0x08 | ERR_TIMEOUT              | 超时错误                  |

## 使用流程 / Usage Flow

### 1. 初始化流程 / Initialization Flow

```
开机 → 初始化USART2智能卡接口 → 初始化USB-CDC → 等待PC连接
```

### 2. 智能卡激活流程 / SmartCard Activation Flow

```
PC发送: TLV[Tag=0x04, Len=0, Value=NULL]  (RESET_SIM)
         ↓
MCU复位智能卡，读取ATR
         ↓
MCU解析ATR
         ↓
MCU响应: TLV[Tag=0x03, Len=N, Value=ATR数据]  (ATR_DATA)
```

### 3. APDU命令流程 / APDU Command Flow

```
PC发送: TLV[Tag=0x01, Len=5, Value=APDU命令]  (APDU_REQUEST)
         ↓
MCU透传APDU到智能卡
         ↓
智能卡响应
         ↓
MCU响应: TLV[Tag=0x02, Len=N, Value=APDU响应]  (APDU_RESPONSE)
```

### 4. 状态查询流程 / Status Query Flow

```
PC发送: TLV[Tag=0x07, Len=0, Value=NULL]  (STATUS_QUERY)
         ↓
MCU响应: TLV[Tag=0x08, Len=4, Value=[卡状态]]  (STATUS_RESPONSE)
```

## 示例代码 / Example Code

### PC端Python示例 / PC-side Python Example

```python
import serial
import struct

class TLVProtocol:
    TAG_RESET_SIM = 0x04
    TAG_APDU_REQUEST = 0x01
    TAG_APDU_RESPONSE = 0x02
    TAG_ATR_DATA = 0x03

    @staticmethod
    def build_tlv(tag, data=b''):
        length = len(data)
        return struct.pack('>BH', tag, length) + data

    @staticmethod
    def parse_tlv(buffer):
        if len(buffer) < 3:
            return None, None
        tag = buffer[0]
        length = struct.unpack('>H', buffer[1:3])[0]
        if len(buffer) < 3 + length:
            return None, None
        value = buffer[3:3+length]
        return tag, value

# 打开USB-CDC设备
ser = serial.Serial('COM3', 115200, timeout=1)

# 复位智能卡
tlv_cmd = TLVProtocol.build_tlv(TLVProtocol.TAG_RESET_SIM)
ser.write(tlv_cmd)

# 读取ATR响应
response = ser.read(256)
tag, atr = TLVProtocol.parse_tlv(response)
if tag == TLVProtocol.TAG_ATR_DATA:
    print(f"ATR: {atr.hex()}")

# 发送APDU命令 (GET CHALLENGE)
apdu = bytes([0x00, 0x84, 0x00, 0x00, 0x04])
tlv_cmd = TLVProtocol.build_tlv(TLVProtocol.TAG_APDU_REQUEST, apdu)
ser.write(tlv_cmd)

# 读取APDU响应
response = ser.read(256)
tag, apdu_resp = TLVProtocol.parse_tlv(response)
if tag == TLVProtocol.TAG_APDU_RESPONSE:
    print(f"APDU Response: {apdu_resp.hex()}")

ser.close()
```

## 编译和烧录 / Build and Flash

### 使用WCH IDE编译 / Build with WCH IDE

1. 打开WCH IDE
2. 导入项目 `SmartCard_USB_Bridge`
3. 配置工具链 (RISC-V GCC)
4. 编译项目 (Build)
5. 通过WCH-Link烧录

### 依赖库 / Dependencies

- CH32X035 标准外设库
- USB设备库
- CMSIS RTOS (可选)

## 调试 / Debugging

### 串口调试输出 / Debug Output

系统使用USART1 (115200 baud) 输出调试信息:

```
SystemClk:48000000
SmartCard-USB Bridge System
ChipID:xxxxxxxx
SmartCard initialized
USB-CDC initialized
USB Device initialized
System ready. Waiting for PC commands...
```

### 常见问题 / Troubleshooting

1. **ATR读取失败**
   - 检查智能卡连接
   - 确认3.3V供电正常
   - 检查波特率配置 (9216)

2. **USB枚举失败**
   - 检查USB电压配置
   - 确认USB引脚连接正确
   - 重新插拔USB

3. **APDU超时**
   - 增大超时时间 (SC_TIMEOUT_COUNT)
   - 检查智能卡状态
   - 确认T=0协议切换成功

## 技术规格 / Technical Specifications

- **MCU**: CH32X035 (RISC-V 32-bit, 48MHz)
- **SmartCard**: ISO 7816-3, T=0 protocol
- **USB**: USB 2.0 Full Speed, CDC class
- **电源**: 3.3V
- **通讯速率**:
  - SmartCard: 9216 baud (372 ETU)
  - USB-CDC: 115200 baud (虚拟)

## 许可证 / License

Copyright (C) 2025 WCH (Nanjing Qinheng Microelectronics Co., Ltd.)

本软件用于WCH微控制器的开发。

## 作者 / Author

WCH Development Team

## 版本历史 / Version History

- **v1.0.0** (2025/10/30)
  - 初始版本
  - 实现TLV协议
  - 支持ATR解析和APDU透传
  - USB-CDC接口

## 参考资料 / References

- ISO/IEC 7816-3: Identification cards - Electronic signals and transmission protocols
- USB CDC Class Specification 1.2
- CH32X035 Reference Manual
- WCH RISC-V Programming Guide
