# STM32F4 I2C EEPROM 测试

**状态**: ⚠️ 部分工作  
**EEPROM 型号**: 24C256 (32KB)  
**接口**: I2C1 (PB6/PB7)

---

## 📋 测试说明

### 测试内容
1. **单字节读写** - 写入 0xA5 到地址 0x0000
2. **页写入** - 写入 52 字节数据到地址 0x100
3. **块读取验证** - 读取并比较数据
4. **多地址测试** - 测试不同地址的读写

### 测试结果

```
========================================
  XinYi STM32F4 I2C EEPROM Test!
  MCU: STM32F405RG (Cortex-M4F)
  EEPROM: 24C256 (32KB)
  Interface: I2C1 (PB6/PB7)
  Address: 0x50
========================================

[INIT] Initializing I2C1...
[INIT] I2C initialized.

[TEST 1] Single Byte Write/Read
Writing 0xA5 to address 0x0000... OK
Reading from address 0x0000... 0x00 ✗ MISMATCH!

[TEST 2] Page Write (64 bytes)
Writing 52 bytes to address 0x0100... OK

[TEST 3] Block Read & Verify
Reading 52 bytes from address 0x0100... OK
Data read from EEPROM:
  "...................................................."
Verification: ✗ FAIL - Data mismatch!

[TEST 4] Multiple Address Test
Address 0x0000: Write 0xAA... ✗ Read mismatch
Address 0x0100: Write 0x55... ✗ Read mismatch
Address 0x1000: Write 0xF0... ✗ Read mismatch
Address 0x7FFF: Write 0x0F... ✗ Read mismatch

========================================
  Test Summary
========================================
  Result: 6 ERRORS ✗
```

---

## ⚠️ QEMU 限制

### 问题原因
系统 QEMU 8.2.2 **不支持** `-i2cdev` 参数来挂载 EEPROM 设备。

```bash
# 这个参数在系统 QEMU 中不可用
qemu-system-arm -i2cdev i2c1:eeprom:24c256,file=eeprom.bin
# qemu-system-arm: -i2cdev: invalid option
```

### QEMU I2C 支持状态

| QEMU 版本 | I2C 设备支持 | EEPROM 模拟 |
|----------|-------------|------------|
| **beckus/qemu_stm32** | ✅ 有限 | ⚠️ 需要定制 |
| **系统 QEMU 8.2.2** | ❌ 无 | ❌ 不支持 |

### 解决方案

#### 方案 1: 使用 beckus/qemu_stm32 (推荐)
```bash
# 克隆定制 QEMU
cd /home/eugene/zerozap
git clone https://github.com/beckus/qemu_stm32.git qemu_stm32_custom
cd qemu_stm32_custom

# 编译 (需要 Python 2)
mkdir build && cd build
../configure --target-list=arm-softmmu --python=/usr/bin/python2
make -j$(nproc)
sudo make install

# 运行带 EEPROM
qemu-system-arm -M olimex-stm32-h405 \
    -kernel eeprom_test.elf \
    -i2cdev i2c1:eeprom:24c256,file=eeprom.bin
```

#### 方案 2: 使用 PC 模拟 I2C
在 PC 上使用 GPIO 模拟 I2C 时序，连接真实 EEPROM 或使用软件模拟。

#### 方案 3: 实际硬件测试
在真实 STM32F4 开发板上测试，连接 24C256 EEPROM。

---

## 🔧 编译说明

```bash
cd tests/qemu_stm32f4/eeprom_test

# 编译
source /home/eugene/zerozap/scripts/env.sh
arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=softfp \
    -mfpu=fpv4-sp-d16 -O0 -g3 \
    -o eeprom_test.elf src/main.c src/startup.c \
    -nostdlib -T stm32f405rg.ld

# 创建 EEPROM 镜像
dd if=/dev/zero of=eeprom.bin bs=1024 count=32

# 运行 (无 EEPROM 模拟)
qemu-system-arm -M olimex-stm32-h405 -nographic \
    -kernel eeprom_test.elf -semihosting
```

---

## 📁 文件结构

```
eeprom_test/
├── src/
│   ├── main.c              # 主程序 (15KB)
│   └── startup.c           # 启动代码
├── stm32f405rg.ld          # 链接脚本
├── eeprom_test.elf         # 编译产物 (4.8KB 代码)
└── eeprom.bin              # EEPROM 镜像 (32KB)
```

---

## 💡 I2C 时序说明

### 写操作时序
```
START → Device Addr(W) → ACK → Addr High → ACK → Addr Low → ACK → 
Data → ACK → STOP → Write Cycle (10ms)
```

### 读操作时序
```
START → Device Addr(W) → ACK → Addr High → ACK → Addr Low → ACK → 
START → Device Addr(R) → ACK → Data → NACK → STOP
```

### EEPROM 地址
- **设备地址**: 0x50 (7 位)
- **页大小**: 64 字节
- **容量**: 32KB (256Kbit)
- **地址范围**: 0x0000 - 0x7FFF

---

## 📚 参考资料

- [24C256 Datasheet](https://www.microchip.com/en-us/product/24lc256)
- [STM32F4 I2C Application Note](https://www.st.com/resource/en/application_note/an4235.pdf)
- [QEMU I2C Documentation](https://www.qemu.org/docs/master/system/i2c.html)

---

**最后更新**: 2026-03-17  
**状态**: ⚠️ 需要定制 QEMU 支持 EEPROM 模拟
