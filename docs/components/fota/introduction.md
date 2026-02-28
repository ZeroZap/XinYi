# FOTA 组件 - 固件无线升级

**状态**: 📋 基础 | **测试**: 0 用例 | **版本**: 1.0

---

## 📖 简介

XinYi FOTA 组件提供固件 Over-The-Air 升级框架。

### 核心特性

- 📋 **双 Bank 更新** - 安全升级
- 📋 **CRC32 校验** - 完整性验证
- 📋 **进度回调** - 升级进度跟踪
- 📋 **安全启动** - 可选安全验证

---

## 🚀 快速开始

```c
#include "xy_fota.h"

int main(void) {
    xy_fota_t fota;
    xy_fota_config_t config = {
        .flash_base_addr = 0x08010000,
        .slot_size = 128 * 1024,
        .slot_count = 2,
    };
    
    xy_fota_init(&fota, &config);
    
    // 开始下载
    xy_fota_start_download(&fota, version, size);
    
    // 下载数据块
    xy_fota_download_chunk(&fota, data, chunk_size);
    
    // 完成下载
    xy_fota_finish_download(&fota);
    
    // 开始更新
    xy_fota_start_update(&fota);
    
    return 0;
}
```

---

## 📋 API 参考

| 函数 | 说明 |
|------|------|
| `xy_fota_init()` | 初始化 FOTA |
| `xy_fota_start_download()` | 开始下载 |
| `xy_fota_download_chunk()` | 下载数据块 |
| `xy_fota_finish_download()` | 完成下载 |
| `xy_fota_start_update()` | 开始更新 |
| `xy_fota_get_state()` | 获取状态 |
| `xy_fota_get_progress()` | 获取进度 |

---

## 📝 待完成任务

- [ ] 实现 xy_fota.c
- [ ] 添加单元测试
- [ ] 集成 Flash 驱动
- [ ] 添加安全启动支持

---

*最后更新：2026-02-28 | 维护者：XinYi Team*
