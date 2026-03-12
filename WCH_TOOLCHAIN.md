# WCH 沁恒工具链配置

## MRS IDE 安装位置

```
/usr/share/MRS2/
```

## 工具链目录

### ARM 工具链 (STM32/CH32F/CH32V30x)

**位置**: 
```
/usr/share/MRS2/MRS-linux-x64/resources/app/resources/linux/components/WCH/Toolchain/arm-none-eabi-gcc/
```

**版本**: `arm-none-eabi-gcc 9.3.1` (GNU Arm Embedded Toolchain 9-2020-q4-major)

**工具列表**:
- `arm-none-eabi-gcc` - C 编译器
- `arm-none-eabi-g++` - C++ 编译器
- `arm-none-eabi-ar` - 归档工具
- `arm-none-eabi-objcopy` - 目标文件转换
- `arm-none-eabi-objdump` - 反汇编
- `arm-none-eabi-size` - 代码大小分析
- `arm-none-eabi-gdb` - 调试器

**环境变量配置**:
```bash
export PATH="/usr/share/MRS2/MRS-linux-x64/resources/app/resources/linux/components/WCH/Toolchain/arm-none-eabi-gcc/bin:$PATH"
```

---

### RISC-V 工具链 (CH32V03x/CH32X03x)

**位置**: 
```
/usr/share/MRS2/MRS-linux-x64/resources/app/resources/linux/components/WCH/Toolchain/
├── RISC-V Embedded GCC/    # 默认版本
├── RISC-V Embedded GCC12/  # GCC 12
└── RISC-V Embedded GCC15/  # GCC 15 (最新)
```

**工具列表** (以 GCC12 为例):
- `riscv32-wch-elf-gcc` - C 编译器
- `riscv32-wch-elf-g++` - C++ 编译器
- `riscv32-wch-elf-objcopy` - 目标文件转换
- `riscv32-wch-elf-objdump` - 反汇编
- `riscv32-wch-elf-size` - 代码大小分析
- `riscv32-wch-elf-gdb` - 调试器

**环境变量配置**:
```bash
# 使用 GCC12 (推荐)
export PATH="/usr/share/MRS2/MRS-linux-x64/resources/app/resources/linux/components/WCH/Toolchain/RISC-V Embedded GCC12/bin:$PATH"

# 或使用 GCC15 (最新)
export PATH="/usr/share/MRS2/MRS-linux-x64/resources/app/resources/linux/components/WCH/Toolchain/RISC-V Embedded GCC15/bin:$PATH"
```

---

## 快速加载脚本

使用 XinYi 仓库提供的环境配置脚本:

```bash
# ARM 工具链
source /home/eugene/zerozap/XinYi/scripts/env.sh

# 或手动配置
export ARM_TOOLCHAIN_ROOT="/usr/share/MRS2/MRS-linux-x64/resources/app/resources/linux/components/WCH/Toolchain/arm-none-eabi-gcc"
export PATH="$ARM_TOOLCHAIN_ROOT/bin:$PATH"
```

---

## CMake 工具链文件

XinYi 项目已提供 CMake 工具链文件:

```
/home/eugene/zerozap/XinYi/cmake/arm-gcc.cmake
```

**使用方式**:
```bash
cd /home/eugene/zerozap/XinYi
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-gcc.cmake ..
make
```

---

## 其他 WCH 组件

### OpenOCD (调试器)

```
/usr/share/MRS2/MRS-linux-x64/resources/app/resources/linux/components/WCH/OpenOCD/OpenOCD/bin/
```

**配置文件**:
- `wch-arm.cfg` - ARM 芯片调试配置
- `wch-riscv.cfg` - RISC-V 芯片调试配置
- `wch-dual-core.cfg` - 双核调试配置

### 固件烧录工具

```
/usr/share/MRS2/MRS-linux-x64/resources/app/resources/linux/components/WCH/Others/Firmware_Link/
```

**烧录器支持**:
- WCH-Link (ARM/RISC-V)
- WCH-LinkE
- WCH-LinkW
- DAPLink

---

## 验证安装

```bash
# ARM 工具链验证
arm-none-eabi-gcc --version
# 输出：arm-none-eabi-gcc (GNU Arm Embedded Toolchain 9-2020-q4-major) 9.3.1

# RISC-V 工具链验证 (GCC12)
riscv32-wch-elf-gcc --version
# 输出：riscv32-wch-elf-gcc (WCH RISC-V Embedded GCC12) x.x.x
```

---

**最后更新**: 2026-03-12  
**维护者**: ese (嵌入式系统工程师)
