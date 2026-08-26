# STM32U5 FOTA 集成骨架

本目录是 XinYi FOTA core 的 **compile-only 集成入口**，不是可直接烧录的产品工程，也不是
bootloader、Secure FOTA、掉电恢复或实板验证证据。

## 当前边界

`main.c` 只证明公开 FOTA API 可由 Cortex-M33/STM32U5 项目消费：

- 双槽地址与 anti-rollback 最低版本配置；
- 显式注册 boot handoff 与 image confirmation callback；
- 默认 board callback 返回 `XY_FOTA_NOT_SUPPORTED`，不会伪造持久化成功。

以下能力仍必须由具体参考板/bootloader 提供后才能执行升级：

- 内部 Flash erase/write/read backend；
- durable candidate/active/rollback/anti-rollback metadata；
- bootloader handoff、启动尝试计数与掉电恢复；
- reviewed signature provider 与 key provisioning；
- reset、串口日志、下载 transport 与 B1/B2 实板记录。

## Focused compile gate

从仓库根目录执行：

```bash
/home/eugene/Tools/arm-gnu-toolchain/bin/arm-none-eabi-gcc \
  -std=c99 -Wall -Wextra -Werror -mcpu=cortex-m33 -mthumb -ffreestanding \
  -fsyntax-only projects/stm32u5_fota/main.c -Icomponents/fota/inc
```

该命令只提供 STM32U5/Cortex-M33 **源级 compile evidence**，不链接启动文件、链接脚本、HAL、
bootloader 或固件镜像，因此不得标记为可烧录或硬件通过。

完整 FOTA component 的 PC compile gate 仍通过 root CMake/Kconfig 执行；Host contract 由
`fota_core`、`fota_smoke_example` 和 `fota_secure_provider` CTest 维护。

## 文件

- `main.c`：fail-closed 项目入口。
- `STM32U5x8_FLASH_NOR.ld`：历史内存布局草案；使用前须由目标芯片/bootloader owner 审核。
- `template.config.cmake`：已弃用的历史配置示例，不再作为 canonical 构建入口。
