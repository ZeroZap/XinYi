# YOLO 待开发任务清单

**扫描时间**: 2026-03-02  
**模式**: YOLO 通宵

---

## 📊 扫描结果

| 类别 | TODO 数 | 优先级 |
|------|--------|--------|
| **FOTA 安全** | 7 个 | 🔴 高 |
| **Flash 驱动** | 4 个 | 🔴 高 |
| **网络协议** | 8 个 | 🟡 中 |
| **GUI** | 1 个 | 🟢 低 |
| **HAL** | 1 个 | 🟢 低 |
| **传感器** | 3 个 | 🟢 低 |
| **其他** | 10+ 个 | 🟢 低 |

---

## 🔴 高优先级任务

### 1. FOTA 安全加密实现 (7 个 TODO)

**位置**: `components/fota/src/xy_fota_secure.c`

**待实现**:
- [ ] ECDSA P-256 签名验证
- [ ] ChaCha20 块函数
- [ ] ChaCha20-Poly1305 解密
- [ ] Poly1305 tag 验证
- [ ] 双 Bank 交换逻辑
- [ ] Slot 有效性标记
- [ ] Slot 有效性读取

**影响**: Secure FOTA 核心功能

---

### 2. Flash 驱动实现 (4 个 TODO)

**位置**: `components/fota/src/xy_fota_flash.c`

**待实现**:
- [ ] STM32 HAL Flash Unlock
- [ ] STM32 HAL Flash Lock
- [ ] STM32 Flash Erase
- [ ] STM32 Flash Program

**影响**: FOTA 底层支持

---

## 🟡 中优先级任务

### 3. CAN 协议栈完善 (5 个 TODO)

**位置**: `components/net/src/xy_can.c`

**待实现**:
- [ ] 硬件 CAN 初始化
- [ ] 硬件 CAN 停止
- [ ] 硬件发送触发
- [ ] 回调注册
- [ ] 硬件接收处理

**影响**: CAN 总线功能

---

### 4. Modbus 协议完善 (3 个 TODO)

**位置**: `components/net/src/nano_modbus.c`

**待实现**:
- [ ] 发送响应
- [ ] 发送错误响应
- [ ] 读线圈功能

**影响**: Modbus 通信

---

## 🟢 低优先级任务

### 5. GUI 字体缓存 (1 个 TODO)

**位置**: `components/gui/src/xy_font.c`

**待实现**:
- [ ] 字符缓存机制

**影响**: 字体渲染性能

---

### 6. HAL ADC DMA (1 个 TODO)

**位置**: `components/hal/wch/src/xy_hal_adc.c`

**待实现**:
- [ ] DMA 读取模式

**影响**: ADC 高速采集

---

### 7. 传感器完善 (3 个 TODO)

**位置**: `components/sensor/src/xy_mlx90614.c`

**待实现**:
- [ ] PEC 校验
- [ ] EEPROM 读取发射率
- [ ] EEPROM 写入发射率

**影响**: MLX90614 高级功能

---

## 📋 执行计划

### 阶段 1: FOTA 安全核心 (2 小时)
1. 实现 ECDSA 验证 (使用 mbedTLS)
2. 实现 ChaCha20-Poly1305
3. 完善 Bank 交换逻辑

### 阶段 2: Flash 驱动 (1 小时)
1. 实现 STM32 Flash 操作
2. 添加 Flash 抽象层

### 阶段 3: 协议栈完善 (2 小时)
1. CAN 硬件抽象
2. Modbus 响应处理

### 阶段 4: 优化与修复 (1 小时)
1. GUI 字体缓存
2. 传感器 PEC 校验
3. HAL DMA 支持

---

**总计**: 约 6 小时完成所有高优任务

---

**YOLO 模式执行中！** 🚀
