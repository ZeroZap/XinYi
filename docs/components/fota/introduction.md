# FOTA 组件 - 固件无线升级

> 历史材料 / 非当前安全结论：本页保留早期 FOTA 功能介绍，不能作为当前
> secure boot、ECDSA 签名验证、安全审计、来源审查或真实硬件验证记录。
> 当前 FOTA 事实源请以 `components/fota/README.md` 为准；Crypto/ECDSA 安全边界请以
> `components/crypto/crypto_review_manifest.json` 与
> `docs/validation/xinyi-crypto-ecdsa-placeholder-security-review-2026-08-13.md` 为准。
> 不得用本页的历史“安全启动/ECDSA”表述替代真实安全/来源审查或板级验证证据。

**状态**: ✅ 完整 | **测试**: 0 用例 | **版本**: 1.1.0

---

## 📖 简介

XinYi FOTA 组件提供固件 Over-The-Air 升级框架，支持多种升级模式。

### 核心特性

- ✅ **双槽升级 (Dual Bank)** - A/B 双镜像无缝切换
- ✅ **单槽升级 (Single Slot)** - 外部 Flash 备份区方案
- ✅ **增量升级 (Delta)** - 支持 bsdiff 差分升级
- ✅ **回滚机制 (Rollback)** - 版本号校验 + 备份恢复
- ✅ **CRC32 校验** - 完整性验证
- ✅ **进度回调** - 升级进度跟踪
- ⚠️ **安全启动（历史设想）** - 早期文档曾写作“可选 ECDSA 签名验证”；当前
  `components/crypto/src/xy_ecdsa.c` 是 format-only placeholder，已标记
  `security-rejected`，不得用于 production secure boot / firmware authenticity。

---

## 🚀 快速开始

### 1. 双槽模式 (默认)

```c
#include "xy_fota.h"

int main(void) {
    xy_fota_t fota;
    xy_fota_config_t config = {
        .mode = XY_FOTA_MODE_DUAL_BANK,
        .flash_base_addr = 0x08010000,
        .slot_size = 128 * 1024,
        .slot_count = 2,
    };
    
    xy_fota_init(&fota, &config);
    xy_fota_start_download(&fota, version, size, false);
    // ... 下载数据 ...
    xy_fota_finish_download(&fota);
    xy_fota_start_update(&fota);
    
    return 0;
}
```

### 2. 单槽模式 (外部 Flash 备份)

```c
xy_fota_config_t config = {
    .mode = XY_FOTA_MODE_SINGLE_SLOT,
    .flash_base_addr = 0x08010000,  // 内部 Flash
    .slot_size = 128 * 1024,
    .slot_count = 1,
    .backup_addr = 0x00000000,      // 外部 Flash 基地址
    .backup_size = 128 * 1024,
    .enable_rollback = true,
    .min_version = 100,              // 最低版本防回滚
};
```

### 3. 增量升级模式

```c
xy_fota_config_t config = {
    .mode = XY_FOTA_MODE_DELTA,
    .flash_base_addr = 0x08010000,
    .slot_size = 128 * 1024,
    .slot_count = 2,
};
// 增量包下载和打补丁流程...
```

---

## 🏗️ 系统架构

```
┌─────────────────────────────────────────────────────────┐
│                    FOTA Manager                         │
├─────────────────────────────────────────────────────────┤
│  Dual Bank  │  Single Slot  │  Delta Patch  │ Rollback │
├─────────────────────────────────────────────────────────┤
│                 Flash Abstraction Layer                │
├──────────────────────┬──────────────────────────────────┤
│   Internal Flash     │      External Flash (SPI/I2C)   │
└──────────────────────┴──────────────────────────────────┘
```

### 内存布局

**双槽模式:**
```
0x08000000 +─────────────┐
        │   Bootloader  │
        ├───────────────┤
        │   Slot 0      │ ← 运行区
        │  (128KB)     │
        ├───────────────┤
        │   Slot 1     │ ← 升级区
        │  (128KB)     │
        └───────────────┘
```

**单槽模式 (外部 Flash):**
```
内部 Flash:          外部 Flash:
0x08010000 +──────┐  0x00000000 +──────┐
        │  App   │          │  Backup  │
        │(128KB)│          │ (128KB)  │
        └───────┘          └──────────┘
```

---

## 📋 API 参考

### 核心函数

| 函数 | 说明 |
|------|------|
| `xy_fota_init()` | 初始化 FOTA |
| `xy_fota_deinit()` | 反初始化 |
| `xy_fota_start_download()` | 开始下载 |
| `xy_fota_download_chunk()` | 下载数据块 |
| `xy_fota_finish_download()` | 完成下载并验证 |
| `xy_fota_start_update()` | 开始更新 |
| `xy_fota_rollback()` | 回滚到上一版本 |

### 辅助函数

| 函数 | 说明 |
|------|------|
| `xy_fota_get_state()` | 获取状态 |
| `xy_fota_get_progress()` | 获取进度 |
| `xy_fota_cancel()` | 取消更新 |
| `xy_fota_reset()` | 重置 FOTA |
| `xy_fota_validate_version()` | 版本号校验 (防回滚) |
| `xy_fota_set_flash_ops()` | 设置 Flash 接口 |
| `xy_fota_set_backup_flash_ops()` | 设置备份区 Flash 接口 |

### 配置项

```c
typedef struct {
    xy_fota_mode_t mode;         // 模式: DUAL_BANK / SINGLE_SLOT / DELTA
    uint32_t flash_base_addr;    // Flash 基地址
    uint32_t slot_size;          // 槽位大小
    uint8_t slot_count;          // 槽位数量 (1 或 2)
    uint32_t backup_addr;        // 备份区地址 (单槽模式必须)
    uint32_t backup_size;        // 备份区大小
    bool enable_rollback;        // 启用回滚
    uint32_t min_version;        // 最低版本号 (防回滚)
} xy_fota_config_t;
```

---

## 📝 待完成任务

- [x] 实现 xy_fota.c
- [x] 添加单元测试 (tests/test_fota.c)
- [x] 集成 Flash 驱动 (src/xy_fota_flash.c)
- [x] 添加安全启动支持 (src/xy_fota_secure.c)
- [x] 单槽模式 (外部 Flash 备份)
- [x] 增量升级 (Delta)
- [x] 回滚机制

---

## 🔧 错误码

| 错误码 | 说明 |
|--------|------|
| XY_FOTA_OK | 成功 |
| XY_FOTA_ERROR | 通用错误 |
| XY_FOTA_INVALID_PARAM | 参数无效 |
| XY_FOTA_FLASH_ERROR | Flash 错误 |
| XY_FOTA_CRC_ERROR | CRC 校验失败 |
| XY_FOTA_AUTH_ERROR | 认证失败 |
| XY_FOTA_VERSION_ERROR | 版本过旧 |
| XY_FOTA_NO_BACKUP | 无备份 |
| XY_FOTA_DELTA_ERROR | 增量升级失败 |

---

*最后更新：2026-04-02 | 维护者：XinYi Team*
