# libdriver 驱动集成计划

**时间**: 2026-03-18 15:40  
**来源**: https://github.com/libdriver  
**状态**: 🟡 执行中

---

## 📊 驱动统计

| 类别 | 驱动数 | 优先级 |
|------|--------|--------|
| **传感器** | 40+ | 🔴 高 |
| **显示** | 20+ | 🟡 中 |
| **通信** | 20+ | 🟡 中 |
| **存储** | 15+ | 🟢 低 |
| **其他** | 10+ | 🟢 低 |
| **总计** | **100+** | - |

---

## 🎯 第一阶段 (今日 2h)

### 1. DHT11/DHT22 温湿度
**源文件**: libdriver/dht11/  
**适配**:
- [ ] 复制到 `components/sensor/drivers/`
- [ ] 适配 XinYi HAL 接口
- [ ] 添加 Kconfig
- [ ] 测试验证

### 2. SSD1306 OLED
**源文件**: libdriver/ssd1306/  
**适配**:
- [ ] 复制到 `components/drivers/display/`
- [ ] 适配 I2C/SPI HAL
- [ ] 集成 GUI 框架
- [ ] 测试验证

### 3. MPU6050 IMU
**源文件**: libdriver/mpu6050/  
**适配**:
- [ ] 复制到 `components/sensor/drivers/motion/`
- [ ] 适配 I2C HAL
- [ ] 添加 DMP 支持
- [ ] 测试验证

---

## 📁 目录结构

```
third_party/libdriver/          # 原始驱动
├── dht11/
├── ssd1306/
├── mpu6050/
└── ...

components/sensor/drivers/      # 适配后
├── temperature/
│   └── xy_sensor_dht11.c
├── motion/
│   └── xy_sensor_mpu6050.c
└── ...

components/drivers/display/
└── xy_oled_ssd1306.c
```

---

## 🔧 适配步骤

### 1. 复制驱动
```bash
cp -r third_party/libdriver/dht11 components/sensor/drivers/temperature/
```

### 2. 重命名文件
```
dht11.c → xy_sensor_dht11.c
dht11.h → xy_sensor_dht11.h
```

### 3. 适配接口
```c
// libdriver API
dht11_read(&dev, &temp, &humid);

// XinYi API
xy_sensor_read(&dev, XY_SENSOR_TEMP, &value);
```

### 4. 添加配置
```kconfig
config SENSOR_DHT11
    bool "DHT11/DHT22 Sensor"
    default y
```

---

## 📋 检查清单

- [ ] 下载 libdriver 仓库
- [ ] 选择 3 个优先驱动
- [ ] 复制到 components
- [ ] 适配 XinYi 接口
- [ ] 添加 CMake/Kconfig
- [ ] 编译测试
- [ ] 功能验证
- [ ] 编写文档

---

**预计完成**: 2 小时 ⚡
