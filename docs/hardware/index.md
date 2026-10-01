# 硬件支持

**版本**: 1.2.0
**最后更新**: 2026-10-01
**维护者**: XinYi Team

XinYi 支持的硬件平台和开发板。

---

## 📖 支持的 MCU

### STM32 系列

| 系列 | 状态 | 备注 |
|------|------|------|
| STM32U5 | 🟡 编译兼容 | enhancement compile-only；Board pending |
| STM32F4 | 🟡 部分实现 | 部分外设 unsupported；QEMU partial |
| STM32F1 | 📋 未纳入当前支持矩阵 | 不作为当前 Sprint release scope |
| STM32L4 | 🟡 Pandora reference board | 受限 B1/B2；不得外推为完整 HAL |
| STM32H7 | 🔄 进行中 | - |

### 其他 MCU

| 厂商 | 状态 | 备注 |
|------|------|------|
| HC32 | 📋 占位符 | 可实现 |
| WCH | 📋 占位符 | 可实现 |

---

## 📖 支持的开发板

### Nucleo 系列

| 开发板 | MCU | 状态 |
|--------|-----|------|
| NUCLEO-U575ZI | STM32U575 | compile-only / Board pending |
| NUCLEO-F429ZI | STM32F429 | QEMU partial / Board pending |
| NUCLEO-F103RB | STM32F103 | 不在当前 release scope |
| NUCLEO-L476RG | STM32L476 | Board pending；Pandora L475 是正式 reference |

### 自定义开发板

| 开发板 | 说明 | 状态 |
|--------|------|------|
| XinYi Bridge | USB 桥接器 | 未纳入当前 release scope |
| XinYi Meter | LCR 表 | 未纳入当前 release scope |

---

## 📖 外设支持

### 通信外设

| 外设 | 支持 | 说明 |
|------|------|------|
| UART | ✅ | 异步串口 |
| SPI | ✅ | 同步串口 |
| I2C | ✅ | 两线接口 |
| CAN | ✅ | CAN 总线 |
| USB | ✅ | USB 设备/主机 |

### 模拟外设

| 外设 | 支持 | 说明 |
|------|------|------|
| ADC | ✅ | 模数转换 |
| DAC | ✅ | 数模转换 |

### 定时器

| 外设 | 支持 | 说明 |
|------|------|------|
| Timer | ✅ | 通用定时器 |
| PWM | ✅ | 脉宽调制 |
| RTC | ✅ | 实时时钟 |
| WDG | ✅ | 看门狗 |

---

## 📖 移植指南

### 移植到新 MCU

1. **创建 MCU 目录**
   ```
   components/hal/<mcu_series>/
   ```

2. **实现 HAL 接口**
   - 复制参考实现
   - 实现所有 `xy_hal_*.c` 文件

3. **配置构建系统**
   ```cmake
   # CMakeLists.txt
   add_subdirectory(components/hal/<mcu_series>)
   ```

4. **测试验证**
   - 运行单元测试
   - 硬件在环测试

---

## 📖 设备树

XinYi 支持简化的设备树配置。

```dts
&uart1 {
    status = "okay";
    baudrate = <115200>;
};

&spi1 {
    status = "okay";
    mode = <0>;
    frequency = <1000000>;
};
```

---

*最后更新：2026-02-28 | 维护者：XinYi Team*
