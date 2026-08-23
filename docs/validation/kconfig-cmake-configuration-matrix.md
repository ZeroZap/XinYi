# Kconfig/CMake 配置组合矩阵

**建立日期**：2026-08-23  
**适用范围**：XinYi root Kconfig generator 与 root CMake configure/build  
**证据边界**：本矩阵只记录配置解析、Host 构建或 target compile；不构成实板、安全或产品证据。

## 状态定义

- `PASS`：本仓库、所列命令与当前提交上实际通过。
- `PENDING`：尚未执行或尚未形成可重复门禁。
- `BLOCKED`：已执行但被明确依赖或环境阻塞。

## 配置矩阵

| 组合 | 目标契约 | 状态 | 当前证据/下一 probe |
|---|---|---|---|
| 非法 Display 子功能单开 | `DRIVER_DISPLAY_LCD_SPI=ON` 且父符号关闭时，子符号必须保持 `OFF` | PASS | `python3 -m unittest tests.test_kconfig_parser -v`（4/4）；root PC configure 后检查三个生成变量均为 `OFF` |
| PC 默认基线 | 默认 Kconfig + FOTA off 可 configure/build | PASS | `cmake -S . -B build/pc ...`；`cmake --build build/pc -j$(nproc)` |
| 全关最小配置 | 可选组件、GUI、网络、驱动和额外组件关闭后仍可 configure/build | PENDING | 下一 slice 建立显式 override 集并检查 target inventory |
| 核心组件逐项开启 | Device/Crypto/DM/Sensor/Actuator 各自具备可重复 configure/build 结果 | PENDING | 逐项验证生成变量与 root target |
| Display 子功能 | OLED/SSD1306、LCD SPI/I8080/ST7789、LED/serial RGB 的父子依赖和 source selection 一致 | IN_PROGRESS | OLED/SSD1306 合法组合 PASS：三个生成变量均为 `ON`，`xy_drivers` 构建成功且归档包含 `xy_oled_ssd1306.c.o`；LCD/LED/RGB 组合待验证 |
| Sensor 兼容模式 | legacy `XY_SENSOR_ENABLE`、`COMPONENT_SENSOR` 与新 Device driver 路径的 active ownership 明确 | PENDING | 先记录现有双入口行为，再定义迁移期组合 |
| STM32U5 默认配置 | 平台条件默认值与 target compile 一致 | PENDING | clean STM32U5 configure/build；仅记 C1 compile evidence |

## 已验证命令

```bash
python3 -m unittest tests.test_kconfig_parser -v

cmake -S . -B build/config-matrix-invalid-display \
  -DHAL_PLATFORM=PC \
  -DCMAKE_BUILD_TYPE=Release \
  -DKCONFIG_OVERRIDES='BUILD_TESTING=OFF;FOTA_ENABLED=OFF;DRIVER_DISPLAY_LCD_SPI=ON'

python3 -c "from pathlib import Path; p=Path('build/config-matrix-invalid-display/config.cmake').read_text(); assert 'set(CONFIG_DRIVER_DISPLAY OFF)' in p; assert 'set(CONFIG_DRIVER_DISPLAY_LCD OFF)' in p; assert 'set(CONFIG_DRIVER_DISPLAY_LCD_SPI OFF)' in p"

cmake -S . -B build/config-matrix-display-ssd1306 \
  -DHAL_PLATFORM=PC \
  -DCMAKE_BUILD_TYPE=Release \
  -DKCONFIG_OVERRIDES='BUILD_TESTING=OFF;FOTA_ENABLED=OFF;DRIVER_DISPLAY=ON;DRIVER_DISPLAY_OLED=ON;DRIVER_DISPLAY_SSD1306=ON'
cmake --build build/config-matrix-display-ssd1306 --target xy_drivers -j$(nproc)
python3 -c "from pathlib import Path; p=Path('build/config-matrix-display-ssd1306/config.cmake').read_text(); assert 'set(CONFIG_DRIVER_DISPLAY ON)' in p; assert 'set(CONFIG_DRIVER_DISPLAY_OLED ON)' in p; assert 'set(CONFIG_DRIVER_DISPLAY_SSD1306 ON)' in p"
/home/eugene/Tools/arm-gnu-toolchain/bin/arm-none-eabi-ar t \
  build/config-matrix-display-ssd1306/components/drivers/libxy_drivers.a

make test-unit
cmake -S . -B build/pc -DHAL_PLATFORM=PC -DCMAKE_BUILD_TYPE=Release \
  -DKCONFIG_OVERRIDES='BUILD_TESTING=OFF;FOTA_ENABLED=OFF'
cmake --build build/pc -j$(nproc)
git diff --check
```

结果：parser 4/4、Host CTest 178/178、PC root build 和 whitespace gate均通过；OLED/SSD1306 合法组合生成值、focused `xy_drivers` 构建与归档 source inventory 通过。

## 本轮发现并修复的配置风险

命令行 override 原先在默认依赖解析之后直接写入 resolved values，因此可以启用父依赖关闭的子符号。此行为会让配置文件声称启用某个 Display 子功能，但 root CMake source selection 仍缺少完整父功能上下文。解析器现在会在 override/select 收敛期间重复关闭依赖不满足的 bool 符号，并由回归测试锁定 fail-closed 行为。
