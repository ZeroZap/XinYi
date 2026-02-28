# NET 组件 - 网络协议

**状态**: ✅ 完善 | **测试**: 22 用例 | **版本**: 1.0

---

## 📖 简介

XinYi 网络协议（NET）组件提供 MQTT、Modbus、AT 命令、ISO7816 等通信协议。

### 核心特性

- ✅ **MQTT** - IoT 消息协议
- ✅ **Modbus** - 工业协议 (RTU/TCP)
- ✅ **AT 命令** - 蜂窝调制解调器
- ✅ **ISO7816** - SIM 卡通信

### 子模块

| 子模块 | 说明 | 状态 |
|--------|------|------|
| MQTT | MQTT 协议 | ✅ 完善 |
| Modbus | Modbus RTU | ✅ 完善 |
| AT | AT 命令 | ✅ 完善 |
| ISO7816 | 智能卡协议 | ✅ 完善 |

---

## 🚀 快速开始

### ISO7816 示例

```c
#include "xy_iso7816.h"

int main(void) {
    xy_iso7816_handle_t handle;
    xy_iso7816_config_t config = {
        .uart = UART1,
        .timeout = 1000,
    };
    
    xy_iso7816_init(&handle, &config);
    
    // 发送 APDU 命令
    xy_iso7816_apdu_cmd_t cmd;
    cmd.cla = 0x00;
    cmd.ins = 0xA4;
    cmd.p1 = 0x00;
    cmd.p2 = 0x00;
    cmd.lc = 2;
    cmd.data[0] = 0x3F;
    cmd.data[1] = 0x00;
    
    xy_iso7816_apdu_resp_t resp;
    xy_iso7816_apdu_send(&handle, &cmd, &resp);
    
    return 0;
}
```

### Modbus 示例

```c
#include "mb_slave.h"

int main(void) {
    mb_slave_t slave;
    
    mb_slave_init(&slave);
    
    // 写保持寄存器
    mb_slave_write_holding_register(&slave, 0, 100);
    
    // 读保持寄存器
    uint16_t value = mb_slave_read_holding_register(&slave, 0);
    
    return 0;
}
```

---

## 📋 API 参考

### ISO7816

| 函数 | 说明 |
|------|------|
| `xy_iso7816_init()` | 初始化 |
| `xy_iso7816_apdu_send()` | 发送 APDU |
| `xy_iso7816_select_file()` | 选择文件 |
| `xy_iso7816_read_binary()` | 读二进制 |

### Modbus

| 函数 | 说明 |
|------|------|
| `mb_slave_init()` | 初始化从站 |
| `mb_slave_process()` | 处理请求 |
| `mb_slave_read_coil()` | 读线圈 |
| `mb_slave_write_holding_register()` | 写保持寄存器 |

---

## 🧪 测试用例

NET 组件包含 22 个测试用例：

| 测试类别 | 用例数 |
|----------|--------|
| ISO7816 常量 | 5 |
| ISO7816 结构体 | 4 |
| Modbus 功能码 | 3 |
| Modbus CRC | 3 |
| Modbus 地址验证 | 2 |
| Modbus 寄存器访问 | 2 |

运行测试：
```bash
ctest -R test_net --output-on-failure
```

---

*最后更新：2026-02-28 | 维护者：XinYi Team*
