# DM 组件 - 数据管理

**状态**: ✅ 完善 | **测试**: 24 用例 | **版本**: 1.0

---

## 📖 简介

XinYi 数据管理（DM）组件提供 EEPROM、Flash、TLV 编码等数据存储和管理功能。

### 核心特性

- ✅ **EEPROM 管理** - 模拟 EEPROM
- ✅ **Flash 管理** - NOR/NAND Flash
- ✅ **TLV 编码** - 标签 - 长度 - 值协议
- ✅ **NVM 存储** - 非易失性内存
- ✅ **磨损均衡** - Flash 寿命优化

### 子模块

| 子模块 | 说明 | 状态 |
|--------|------|------|
| TLV | TLV 编码/解码 | ✅ 完善 |
| EEPROM | EEPROM 模拟 | ✅ 完善 |
| Flash | Flash 管理 | ✅ 完善 |
| NVM | 非易失性存储 | ✅ 完善 |
| JSON | JSON 解析 | 📋 基础 |
| YAML | YAML 解析 | 📋 基础 |

---

## 🚀 快速开始

### TLV 编码示例

```c
#include "xy_tlv.h"

int main(void) {
    uint8_t buffer[256];
    xy_tlv_buffer_t tlv_buf;
    
    // 初始化 Buffer
    xy_tlv_buffer_init(&tlv_buf, buffer, sizeof(buffer));
    
    // 编码数据
    xy_tlv_buffer_append_uint8(&tlv_buf, 0x0001, 42);
    xy_tlv_buffer_append_uint16(&tlv_buf, 0x0002, 1000);
    xy_tlv_buffer_append_string(&tlv_buf, 0x0101, "Hello");
    
    return 0;
}
```

### TLV 解码示例

```c
#include "xy_tlv.h"

int main(void) {
    xy_tlv_iterator_t iter;
    xy_tlv_t tlv;
    
    // 初始化迭代器
    xy_tlv_iterator_init(&iter, buffer, buffer_len);
    
    // 遍历 TLV
    while (xy_tlv_iterator_next(&iter, &tlv) == XY_TLV_OK) {
        switch (tlv.type) {
            case 0x0001:
                // 处理 uint8
                break;
            case 0x0002:
                // 处理 uint16
                break;
        }
    }
    
    return 0;
}
```

---

## 📋 API 参考

### TLV 编码

| 函数 | 说明 |
|------|------|
| `xy_tlv_encode_uint8()` | 编码 uint8 |
| `xy_tlv_encode_uint16()` | 编码 uint16 |
| `xy_tlv_encode_uint32()` | 编码 uint32 |
| `xy_tlv_encode_string()` | 编码字符串 |
| `xy_tlv_encode_bytes()` | 编码字节数组 |

### TLV 解码

| 函数 | 说明 |
|------|------|
| `xy_tlv_decode_uint8()` | 解码 uint8 |
| `xy_tlv_decode_uint16()` | 解码 uint16 |
| `xy_tlv_decode_string()` | 解码字符串 |

### TLV 迭代器

| 函数 | 说明 |
|------|------|
| `xy_tlv_iterator_init()` | 初始化迭代器 |
| `xy_tlv_iterator_next()` | 获取下一个 TLV |

### TLV 查找

| 函数 | 说明 |
|------|------|
| `xy_tlv_find_by_type()` | 按类型查找 |

---

## 🧪 测试用例

DM 组件包含 24 个测试用例：

| 测试类别 | 用例数 |
|----------|--------|
| TLV 常量 | 6 |
| TLV 编码 | 5 |
| TLV 解码 | 3 |
| TLV 迭代器 | 2 |
| TLV 查找 | 1 |
| TLV 验证 | 2 |
| TLV Buffer | 3 |
| TLV 嵌套 | 2 |

运行测试：
```bash
ctest -R test_dm --output-on-failure
```

---

*最后更新：2026-02-28 | 维护者：XinYi Team*
