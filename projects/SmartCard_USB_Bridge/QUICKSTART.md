# Quick Start Guide - 快速开始指南

## 5分钟快速上手 / 5-Minute Quick Start

### 第一步: 硬件连接 / Step 1: Hardware Connection

#### 连接SIM卡 / Connect SIM Card
```
CH32X035 Pin    →    SIM Card Pin
────────────────────────────────────
PA2             →    I/O (数据线)
PA4             →    CLK (时钟)
PA5             →    RST (复位)
3.3V            →    VCC (电源)
GND             →    GND (地)
```

#### 连接USB / Connect USB
```
将CH32X035的USB引脚连接到PC:
- PC16 (D-) → USB D-
- PC17 (D+) → USB D+
```

### 第二步: 编译固件 / Step 2: Build Firmware

#### 方法A: 使用WCH IDE
```
1. 打开WCH MounRiver Studio IDE
2. 导入SmartCard_USB_Bridge项目
3. 选择编译目标: CH32X035
4. 点击Build (Ctrl+B)
5. 确认编译成功
```

#### 方法B: 使用命令行
```bash
# 进入项目目录
cd SmartCard_USB_Bridge

# 编译
make clean
make all

# 检查生成的文件
ls -l build/SmartCard_USB_Bridge.hex
```

### 第三步: 烧录固件 / Step 3: Flash Firmware

```
1. 连接WCH-Link到CH32X035的调试接口
2. 打开WCH烧录工具
3. 选择芯片型号: CH32X035
4. 加载hex文件
5. 点击"下载"按钮
6. 等待烧录完成
```

### 第四步: 测试运行 / Step 4: Test Run

#### 1. 连接USB到PC

插入USB线,PC应该识别到新的USB CDC设备。

**Windows检查**:
```
设备管理器 → 端口(COM和LPT) → 查看新增的COM口
例如: USB Serial Port (COM3)
```

**Linux检查**:
```bash
dmesg | grep tty
# 应该看到: ttyUSB0 或 ttyACM0
ls -l /dev/ttyUSB*
```

#### 2. 安装Python依赖

```bash
pip install pyserial
```

#### 3. 运行测试客户端

**Windows**:
```bash
python test_client.py COM3
```

**Linux**:
```bash
python test_client.py /dev/ttyUSB0
```

#### 4. 查看测试结果

成功的输出应该类似:
```
============================================================
SmartCard-USB Bridge Test Client
Version 1.0.0
============================================================
[+] Connected to COM3 at 115200 baud

=== Querying Status ===
[→] Sent: Tag=0x07, Len=0, Data=
[←] Received: Tag=0x08, Len=4, Data=00 00 00 00
[+] Status:
    Card State: 0 (Absent)
    Active State: 0 (Inactive)
    ATR Valid: No
    Protocol: T=0

=== Resetting SIM Card ===
[→] Sent: Tag=0x04, Len=0, Data=
[←] Received: Tag=0x03, Len=14, Data=00 3B 9F 95 80 ...
[+] ATR received:
    Protocol: T=0
    ATR Data: 3B 9F 95 80 1F C7 80 31 E0 73 FE 21 1B 66
...
```

### 第五步: 串口调试输出 / Step 5: Debug Output

连接USART1 (115200波特率) 查看调试信息:

```
SystemClk:48000000
SmartCard-USB Bridge System
ChipID:03500520
SmartCard initialized
USB-CDC initialized
USB Device initialized
System ready. Waiting for PC commands...
```

## 常见问题快速解决 / Quick Troubleshooting

### ❌ USB设备无法识别

**解决方法**:
1. 检查USB线是否连接正确
2. 确认USB电压配置正确 (3.3V或5V)
3. 重新插拔USB线
4. 检查设备管理器的错误信息

### ❌ ATR读取失败

**解决方法**:
1. 确认SIM卡插入正确
2. 检查电源3.3V稳定
3. 检查PA2/PA4/PA5引脚连接
4. 确认SIM卡未损坏

### ❌ Python脚本连接失败

**解决方法**:
```bash
# 检查串口是否存在
# Windows
mode COM3

# Linux
ls -l /dev/ttyUSB0

# 检查串口权限(Linux)
sudo chmod 666 /dev/ttyUSB0
# 或
sudo usermod -a -G dialout $USER
# (需要重新登录)

# 确认pyserial已安装
pip list | grep serial
```

## 简单命令行测试 / Simple CLI Test

不使用Python,也可以用串口工具测试:

### 使用PuTTY (Windows)
```
1. 打开PuTTY
2. 选择Serial
3. Serial line: COM3
4. Speed: 115200
5. 连接
```

### 手动发送TLV命令

**复位SIM卡**:
```
发送十六进制: 04 00 00
(Tag=0x04, Length=0x0000)

期待响应: 03 00 0E 00 3B 9F ...
(Tag=0x03 ATR_DATA)
```

**查询状态**:
```
发送十六进制: 07 00 00
(Tag=0x07, Length=0x0000)

期待响应: 08 00 04 01 02 01 00
(Tag=0x08 STATUS_RESPONSE)
```

**发送APDU (GET CHALLENGE)**:
```
发送十六进制: 01 00 05 00 84 00 00 08
(Tag=0x01, Length=0x0005, APDU=00 84 00 00 08)

期待响应: 02 00 0A [8字节随机数] 90 00
(Tag=0x02 APDU_RESPONSE)
```

## 自定义开发快速模板 / Quick Development Template

### 发送自定义APDU

```python
from test_client import SmartCardBridge

bridge = SmartCardBridge('COM3')
bridge.connect()

# 复位卡片
bridge.reset_sim()

# 自定义APDU
my_apdu = [0x00, 0xA4, 0x04, 0x00, 0x10,
           0xA0, 0x00, 0x00, 0x00, 0x03, 0x10, 0x10, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]
response = bridge.send_apdu(my_apdu)

bridge.disconnect()
```

### C语言MCU端添加新命令

```c
// 在tlv_protocol.h添加新标签
#define TLV_TAG_MY_COMMAND    0x0D

// 在ProcessTLVCommand()添加处理
case TLV_TAG_MY_COMMAND:
    // 处理你的命令
    uint8_t result = MyCustomFunction(tlv_in->value);
    TLV_Build(&tlv_out, TLV_TAG_ACK, &result, 1);
    SendTLVResponse(&tlv_out);
    break;
```

## 性能基准 / Performance Benchmarks

典型操作的响应时间:

| 操作 | 响应时间 |
|------|----------|
| 复位SIM卡 | ~200ms |
| ATR解析 | ~10ms |
| APDU命令(简单) | ~100ms |
| APDU命令(复杂) | ~500ms |
| 状态查询 | ~5ms |

## 下一步学习 / Next Steps

1. **阅读详细文档**
   - README.md - 系统概述
   - TLV_Protocol_Specification.md - 协议详情
   - IMPLEMENTATION_GUIDE.md - 实现细节

2. **研究示例代码**
   - main.c - 主程序流程
   - smartcard.c - 智能卡通讯
   - tlv_protocol.c - 协议实现

3. **自定义开发**
   - 添加新的TLV命令
   - 支持T=1协议
   - 实现PIN码验证

## 技术支持 / Support

遇到问题?

1. 查看IMPLEMENTATION_GUIDE.md的"常见问题解决"章节
2. 启用调试输出检查详细日志
3. 使用逻辑分析仪检查硬件信号
4. 参考ISO 7816标准文档

---

**祝您使用愉快!** / **Happy Coding!**

如有问题,欢迎反馈!
