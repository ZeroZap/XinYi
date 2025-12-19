# SmartCard-USB Bridge Implementation Guide

## 实现概述 / Implementation Overview

本文档提供了基于CH32X035微控制器的智能卡-USB桥接系统的完整实现指南。

This document provides a complete implementation guide for the SmartCard-USB Bridge system based on CH32X035 microcontroller.

## 系统组成 / System Components

### 1. 硬件组件 / Hardware Components

```
┌─────────────────────────────────────────────────────────┐
│                    CH32X035 MCU                         │
│                                                         │
│  USART2 SmartCard    │    USBFS Device                 │
│  ─────────────────   │    ─────────────                │
│  • PA2 (SC_IO)       │    • PC16 (D-)                  │
│  • PA4 (SC_CK)       │    • PC17 (D+)                  │
│  • PA5 (SC_RST)      │                                 │
└─────────────────────────────────────────────────────────┘
         │                           │
         ▼                           ▼
    ┌────────┐                  ┌─────────┐
    │ SIM卡  │                  │  USB PC │
    └────────┘                  └─────────┘
```

### 2. 软件组件 / Software Components

#### 2.1 核心模块 / Core Modules

| 模块 | 文件 | 功能 |
|------|------|------|
| 主程序 | main.c | 系统初始化和主循环 |
| 智能卡驱动 | smartcard.c/h | UART SmartCard接口 |
| TLV协议 | tlv_protocol.c/h | TLV协议封装/解析 |
| USB-CDC | UART.c/h, ch32x035_usbfs_device.c/h | USB虚拟串口 |
| USB描述符 | usb_desc.c/h | USB设备描述符 |

#### 2.2 依赖库 / Dependencies

- CH32X035标准外设库 (位于 `SRC/Peripheral/`)
- USB全速设备库
- CMSIS核心库

## 详细实现说明 / Detailed Implementation

### 1. TLV协议层 / TLV Protocol Layer

#### 核心数据结构

```c
/* TLV包头 */
typedef struct __attribute__((packed)) {
    uint8_t  tag;           // 标签
    uint16_t length;        // 长度(大端序)
} TLV_Header;

/* TLV数据包 */
typedef struct {
    TLV_Header header;
    uint8_t    value[TLV_MAX_PAYLOAD_SIZE];
} TLV_Packet;
```

#### 关键函数

1. **TLV_Build()** - 构建TLV包
2. **TLV_Parse()** - 解析TLV包
3. **TLV_Serialize()** - 序列化TLV包到缓冲区

### 2. 智能卡驱动层 / SmartCard Driver Layer

#### UART SmartCard配置

```c
void SC_Init(void) {
    // 配置USART2为SmartCard模式
    USART_InitStructure.USART_BaudRate = 9216;  // ISO 7816-3标准
    USART_InitStructure.USART_WordLength = USART_WordLength_9b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1_5;
    USART_InitStructure.USART_Parity = USART_Parity_Even;

    USART_SmartCardCmd(USART2, ENABLE);
    USART_SmartCardNACKCmd(USART2, ENABLE);
    USART_SetPrescaler(USART2, 7);  // 时钟分频
}
```

#### ATR解析状态机

ATR解析使用状态机实现,支持:
- TS (初始字符) 检测
- T0 (格式字符) 解析
- 接口字节 (TA, TB, TC, TD) 解析
- 历史字节提取
- TCK (校验字节) 验证

#### APDU命令处理

```c
uint8_t SC_SendAPDU(uint8_t *apdu, uint16_t apdu_len,
                    uint8_t *response, uint16_t *response_len) {
    // 1. 检查卡状态
    // 2. 发送APDU命令
    // 3. 接收响应
    // 4. 返回结果
}
```

### 3. USB-CDC层 / USB-CDC Layer

#### USB端点配置

```
端点0 (EP0): 控制端点
端点1 (EP1): 中断上传 (CDC控制)
端点2 (EP2): 批量下载 (PC → MCU数据)
端点3 (EP3): 批量上传 (MCU → PC数据)
```

#### 数据流处理

**下行数据流** (PC → SIM卡):
```
USB EP2 → UART2_Tx_Buf → TLV解析 → ProcessTLVCommand() → SC_SendAPDU()
```

**上行数据流** (SIM卡 → PC):
```
SC_ReceiveData() → TLV_Build() → SendTLVResponse() → USB EP3
```

### 4. 主程序流程 / Main Program Flow

```c
int main(void) {
    // 1. 系统初始化
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init(115200);

    // 2. 外设初始化
    RCC_Configuration();
    TIM3_Init();
    SC_Init();           // SmartCard初始化
    UART2_Init();        // USB-CDC初始化
    USBFS_Device_Init(); // USB设备初始化

    // 3. 主循环
    while (1) {
        UART2_DataTx_Deal();  // USB数据发送
        UART2_DataRx_Deal();  // USB数据接收
        ProcessUSBData();      // 处理TLV命令
    }
}
```

## 内存使用 / Memory Usage

### 缓冲区分配

| 缓冲区 | 大小 | 用途 |
|--------|------|------|
| UART2_Tx_Buf | 1024 bytes | USB→SmartCard发送缓冲 |
| UART2_Rx_Buf | 2048 bytes | SmartCard→USB接收缓冲 |
| TLV_RxBuffer | 515 bytes | TLV包解析缓冲 |
| USBFS_EP0_Buf | 64 bytes | USB端点0缓冲 |
| USBFS_EP1_Buf | 64 bytes | USB端点1缓冲 |
| USBFS_EP3_Buf | 64 bytes | USB端点3缓冲 |

### 总内存占用估算

- **RAM**: ~4KB (缓冲区 + 栈 + 全局变量)
- **Flash**: ~20KB (代码 + 常量)

## 时序要求 / Timing Requirements

### SmartCard通讯时序

```
ETU (Elementary Time Unit) = 372 clock cycles
Baud Rate = F / (D × ETU)
Default: F=372, D=1
Clock Frequency = 3.5712 MHz (48MHz / 7 / 2)
Baud Rate = 9600 / 1.04 ≈ 9216 baud
```

### USB时序

- **全速模式**: 12 Mbps
- **包间隔**: 1ms (全速批量端点)
- **超时设置**: 100ms (数据上传)

## 错误处理策略 / Error Handling Strategy

### 1. 智能卡错误

```c
if (SC_ResetAndGetATR(&atr) != 0) {
    // ATR接收失败
    TLV_BuildErrorResponse(&tlv_out, TLV_ERR_ATR_PARSE_FAILED);
    SendTLVResponse(&tlv_out);
}
```

### 2. TLV协议错误

```c
parsed_len = TLV_Parse(&tlv_cmd, TLV_RxBuffer, TLV_RxIndex);
if (parsed_len == 0 && TLV_RxIndex >= sizeof(TLV_RxBuffer)) {
    // 缓冲区溢出 - 重置
    TLV_RxIndex = 0;
}
```

### 3. USB传输错误

```c
if (Uart.USB_Up_TimeOut >= DEF_UARTx_USB_UP_TIMEOUT) {
    // 上传超时 - 清除忙标志
    Uart.USB_Up_IngFlag = 0x00;
    USBFS_Endp_Busy[DEF_UEP3] = 0;
}
```

## 性能优化 / Performance Optimization

### 1. DMA使用

- **UART2 RX**: DMA循环模式接收
- **UART2 TX**: DMA正常模式发送
- 减少CPU干预,提高吞吐量

### 2. 中断优先级

```c
NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
// USB中断: 较高优先级
// TIM3中断: 中等优先级
// UART中断: 较低优先级
```

### 3. 缓冲区管理

- 循环缓冲区避免频繁内存分配
- 双缓冲机制减少数据拷贝
- 预分配固定大小缓冲区

## 调试技巧 / Debugging Tips

### 1. 使用USART1调试输出

```c
printf("SystemClk:%d\r\n", SystemCoreClock);
printf("SmartCard initialized\r\n");
printf("ATR: ");
for (int i = 0; i < atr_len; i++) {
    printf("%02x ", atr_buf[i]);
}
printf("\r\n");
```

### 2. GPIO测试点

在关键点切换GPIO状态,使用逻辑分析仪观察:
```c
GPIO_SetBits(GPIOA, GPIO_Pin_15);   // 标记开始
// 关键代码段
GPIO_ResetBits(GPIOA, GPIO_Pin_15); // 标记结束
```

### 3. USB抓包分析

使用Wireshark + USBPcap捕获USB通讯:
- 查看CDC数据包
- 验证TLV格式
- 检测丢包/重传

## 常见问题解决 / Troubleshooting

### Q1: ATR读取失败

**症状**: `SC_ReceiveData()` 返回长度为0

**可能原因**:
1. 智能卡未正确插入
2. RST引脚时序错误
3. 波特率不匹配

**解决方案**:
```c
// 增加复位延迟
GPIO_ResetBits(GPIOA, SC_RST_PIN);
Delay_Ms(10);  // 增加到10ms
GPIO_SetBits(GPIOA, SC_RST_PIN);

// 调整超时时间
#define SC_TIMEOUT_COUNT 1000  // 增大超时
```

### Q2: USB枚举失败

**症状**: PC无法识别设备

**可能原因**:
1. USB电压配置错误
2. 描述符错误
3. 上拉电阻配置错误

**解决方案**:
```c
// 检查电压配置
if (VDD_Voltage == PWR_VDD_5V) {
    AFIO->CTLR |= UDP_PUE_10K | USB_IOEN;
} else {
    AFIO->CTLR |= USB_PHY_V33 | UDP_PUE_1K5 | USB_IOEN;
}
```

### Q3: APDU响应数据损坏

**症状**: 响应数据不完整或错误

**可能原因**:
1. 缓冲区溢出
2. DMA配置错误
3. 时序问题

**解决方案**:
```c
// 确保缓冲区足够大
#define DEF_UARTx_RX_BUF_LEN (4 * 512)

// 检查DMA计数器
if (Uart.Rx_RemainLen + temp16 > DEF_UARTx_RX_BUF_LEN) {
    printf("Buffer overflow: %d\n", Uart.Rx_RemainLen);
}
```

## 扩展建议 / Extension Suggestions

### 1. 支持T=1协议

当前实现仅支持T=0,可扩展支持T=1:
- 实现块传输机制
- 添加CRC/LRC校验
- 支持链式传输

### 2. 多卡槽支持

扩展硬件支持多个智能卡槽:
- 使用GPIO复用不同RST引脚
- TLV协议添加卡槽选择命令
- 状态机支持卡槽切换

### 3. 安全增强

添加安全特性:
- PIN码保护
- 命令白名单
- 访问日志记录

### 4. 性能监控

添加性能监控功能:
- 命令执行时间统计
- 错误率统计
- 通讯流量统计

## 测试用例 / Test Cases

### 测试1: 基本连接测试
```python
# 使用test_client.py
python test_client.py COM3
# 预期: 设备枚举成功,连接建立
```

### 测试2: ATR读取测试
```python
bridge.reset_sim()
# 预期: 收到ATR数据,解析成功
```

### 测试3: APDU命令测试
```python
# SELECT MF
bridge.send_apdu([0x00, 0xA4, 0x00, 0x00, 0x02, 0x3F, 0x00])
# 预期: 收到SW=9000
```

### 测试4: 错误处理测试
```python
# 未复位时发送APDU
bridge.send_apdu([0x00, 0x84, 0x00, 0x00, 0x08])
# 预期: 收到ERROR响应,错误码=ERR_NO_CARD
```

## 参考资源 / References

### 标准文档
- ISO/IEC 7816-3: Smart cards - Electronic signals
- ISO/IEC 7816-4: Smart cards - Commands
- USB CDC Class Specification 1.2

### WCH技术文档
- CH32X035 Reference Manual
- CH32X035 Datasheet
- WCH USB Device Programming Guide

### 开发工具
- WCH MounRiver Studio IDE
- WCH-Link调试器
- USB协议分析工具

## 技术支持 / Technical Support

如有问题,请联系:
- WCH官方技术支持
- GitHub Issues
- 技术论坛

---

**文档版本**: 1.0.0
**最后更新**: 2025/10/30
**作者**: WCH Development Team
