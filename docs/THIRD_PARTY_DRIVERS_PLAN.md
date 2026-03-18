# Third Party 驱动引入计划

**任务来源**: 微信文章 - 100+ 芯片驱动开源  
**时间**: 2026-03-18  
**状态**: 🟡 执行中

---

## 📋 任务目标

从 GitHub 下载开源驱动到 `third_party/` 文件夹，学习完整驱动实现，适配到 XinYi 框架。

---

## 🔍 待确认信息

**微信文章链接**: https://mp.weixin.qq.com/s/vn4mQsC4KmvN9yQyQ0bopA

**文章内容**: 100+ 芯片驱动开源，涵盖：
- 温湿度传感器
- 5G 模组
- 其他嵌入式常用驱动

**需要确认**:
1. GitHub 仓库地址
2. 驱动清单
3. 许可证信息

---

## 📁 third_party 目录结构

```
third_party/
├── freertos/         # ✅ 已有
├── rt-thread/        # ✅ 已有
├── CMSIS-RTX/        # ✅ 已有
├── unity/            # ✅ 已有
├── sensor_drivers/   # 🆕 待添加
│   ├── dht11/
│   ├── bme280/
│   └── ...
├── connectivity/     # 🆕 待添加
│   ├── 5g_modem/
│   └── wifi/
└── display/          # 🆕 待添加
    ├── oled/
    └── lcd/
```

---

## 🚀 执行计划

### 阶段 1: 确认驱动仓库 (30m)
- [ ] 获取 GitHub 仓库地址
- [ ] 确认驱动清单
- [ ] 检查许可证兼容性

### 阶段 2: 下载驱动 (1h)
- [ ] Clone 或下载驱动代码
- [ ] 整理到 third_party 目录
- [ ] 添加 README 说明

### 阶段 3: 学习适配 (4h)
- [ ] 分析驱动架构
- [ ] 适配 XinYi HAL 接口
- [ ] 编写移植文档

### 阶段 4: 集成测试 (2h)
- [ ] 编译测试
- [ ] 功能验证
- [ ] 性能基准

---

## 📊 预计工作量

| 阶段 | 工时 | 产出 |
|------|------|------|
| 确认仓库 | 0.5h | 仓库地址 + 清单 |
| 下载驱动 | 1h | third_party 代码 |
| 学习适配 | 4h | 适配文档 + 代码 |
| 集成测试 | 2h | 测试报告 |
| **总计** | **7.5h** | 完整驱动集成 |

---

**下一步**: 获取 GitHub 仓库地址后立即执行！⚡

---

## 🔗 常见开源驱动库参考

### 1. Arduino Core Libraries
- **GitHub**: https://github.com/arduino/ArduinoCore-libraries
- **驱动数**: 100+
- **类型**: 传感器/显示/通信

### 2. Adafruit Libraries
- **GitHub**: https://github.com/adafruit
- **驱动数**: 200+
- **质量**: 高（文档完善）

### 3. SparkFun Libraries
- **GitHub**: https://github.com/sparkfun
- **驱动数**: 150+
- **类型**: 传感器/模块

### 4. M5Stack Libraries
- **GitHub**: https://github.com/m5stack
- **驱动数**: 100+
- **类型**: 物联网模块

### 5. Zephyr Drivers
- **GitHub**: https://github.com/zephyrproject-rtos/zephyr/tree/main/drivers
- **驱动数**: 300+
- **质量**: 工业级

---

## 📝 建议优先引入

### 传感器类
1. **BME280** - 温湿度气压
2. **MPU6050** - 6 轴 IMU
3. **MAX30102** - 心率血氧
4. **VL53L1X** - ToF 测距

### 通信类
1. **ESP-AT** - WiFi 模组
2. **EC20** - 4G/5G 模组
3. **RC522** - RFID

### 显示类
1. **SSD1306** - OLED
2. **ST7735** - TFT LCD

---

**等待用户提供具体 GitHub 地址后继续执行！** ⚡
