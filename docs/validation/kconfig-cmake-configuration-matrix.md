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
| 全关最小配置 | 可选组件、GUI、网络、驱动和额外组件关闭后仍可 configure/build，关闭项不得泄漏 root target | PASS | 显式 all-off override configure/build 通过；target inventory 不含 Device/Charger/Crypto/DM/Sensor/Actuator/Drivers/GUI/Net/FOTA/PM/PID/MUX/SYS/Fuel Gauge |
| 核心组件逐项开启 | Device/Crypto/DM/Sensor/Actuator 各自具备可重复 configure/build 结果 | IN PROGRESS | Device-only 组合已验证生成变量、`xy_device` focused build、target inventory 与归档对象；Crypto/DM/Sensor/Actuator 待逐项验证 |
| Display 子功能 | OLED/SSD1306、LCD SPI/I8080/ST7789、LED/serial RGB 的父子依赖和 source selection 一致 | PASS | 合法组合的生成变量、focused targets 和归档 source inventory 一致；无实现源的 standalone `DRIVER_DISPLAY_RGB` 已从 root/local Kconfig 和 dormant CMake 分支移除，由 parser regression guard 锁定 |
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

cmake -S . -B build/config-matrix-display-lcd \
  -DHAL_PLATFORM=PC \
  -DCMAKE_BUILD_TYPE=Release \
  -DKCONFIG_OVERRIDES='BUILD_TESTING=OFF;FOTA_ENABLED=OFF;DRIVER_DISPLAY=ON;DRIVER_DISPLAY_LCD=ON;DRIVER_DISPLAY_LCD_SPI=ON;DRIVER_DISPLAY_LCD_I8080=ON;DRIVER_DISPLAY_LCD_ST7789=ON'
cmake --build build/config-matrix-display-lcd --target xy_drivers -j$(nproc)
python3 -c "from pathlib import Path; p=Path('build/config-matrix-display-lcd/config.cmake').read_text(); required=['set(CONFIG_DRIVER_DISPLAY ON)','set(CONFIG_DRIVER_DISPLAY_LCD ON)','set(CONFIG_DRIVER_DISPLAY_LCD_SPI ON)','set(CONFIG_DRIVER_DISPLAY_LCD_I8080 ON)','set(CONFIG_DRIVER_DISPLAY_LCD_ST7789 ON)']; missing=[x for x in required if x not in p]; assert not missing, missing"
/home/eugene/Tools/arm-gnu-toolchain/bin/arm-none-eabi-ar t \
  build/config-matrix-display-lcd/components/drivers/libxy_drivers.a

cmake -S . -B build/config-matrix-display-led \
  -DHAL_PLATFORM=PC \
  -DCMAKE_BUILD_TYPE=Release \
  -DKCONFIG_OVERRIDES='BUILD_TESTING=OFF;FOTA_ENABLED=OFF;DRIVER_DISPLAY=ON;DRIVER_DISPLAY_LED=ON;DRIVER_DISPLAY_LED_SERIAL_RGB=ON'
cmake --build build/config-matrix-display-led --target xy_drivers xy_serial_rgb -j$(nproc)
python3 -c "from pathlib import Path; p=Path('build/config-matrix-display-led/config.cmake').read_text(); required=['set(CONFIG_DRIVER_DISPLAY ON)','set(CONFIG_DRIVER_DISPLAY_LED ON)','set(CONFIG_DRIVER_DISPLAY_LED_SERIAL_RGB ON)']; missing=[x for x in required if x not in p]; assert not missing, missing"
/home/eugene/Tools/arm-gnu-toolchain/bin/arm-none-eabi-ar t \
  build/config-matrix-display-led/components/drivers/libxy_drivers.a
/home/eugene/Tools/arm-gnu-toolchain/bin/arm-none-eabi-ar t \
  build/config-matrix-display-led/components/drivers/display/led_drivers/serial_rgb/libxy_serial_rgb.a

cmake -S . -B build/config-matrix-minimal \
  -DHAL_PLATFORM=PC \
  -DCMAKE_BUILD_TYPE=Release \
  -DKCONFIG_OVERRIDES='<all optional component/driver/GUI/network/FS/FOTA/PM symbols OFF>'
cmake --build build/config-matrix-minimal -j$(nproc)
cmake --build build/config-matrix-minimal --target help
# Verify disabled component targets are absent from the generated target inventory.

cmake -S . -B build/config-matrix-device \
  -DHAL_PLATFORM=PC \
  -DCMAKE_BUILD_TYPE=Release \
  -DKCONFIG_OVERRIDES='<all optional components OFF except COMPONENT_DEVICE=ON>'
cmake --build build/config-matrix-device --target xy_device -j$(nproc)
python3 -c "from pathlib import Path; p=Path('build/config-matrix-device/config.cmake').read_text(); required=['set(CONFIG_COMPONENT_DEVICE ON)','set(CONFIG_COMPONENT_CHARGER OFF)','set(CONFIG_COMPONENT_CRYPTO OFF)','set(CONFIG_COMPONENT_DM OFF)','set(CONFIG_COMPONENT_SENSOR OFF)','set(CONFIG_COMPONENT_ACTUATOR OFF)']; missing=[x for x in required if x not in p]; assert not missing, missing"
cmake --build build/config-matrix-device --target help
# Verify xy_device is present and disabled component targets are absent.
/home/eugene/Tools/arm-gnu-toolchain/bin/arm-none-eabi-ar t \
  build/config-matrix-device/components/device/libxy_device.a

make test-unit
cmake -S . -B build/pc -DHAL_PLATFORM=PC -DCMAKE_BUILD_TYPE=Release \
  -DKCONFIG_OVERRIDES='BUILD_TESTING=OFF;FOTA_ENABLED=OFF'
cmake --build build/pc -j$(nproc)
git diff --check
```

结果：parser 5/5、Host CTest 178/178、PC root build 和 whitespace gate 均通过；all-off 最小配置 configure/build 与 target inventory 通过；Device-only 组合仅启用 `COMPONENT_DEVICE`，`xy_device` focused build、target inventory 与归档对象检查通过；OLED/SSD1306、LCD SPI/I8080/ST7789 与 LED/serial RGB 合法组合的生成值、focused target 构建与归档 source inventory 通过。LCD 归档包含 `xy_lcd.c.o`、`xy_lcd_spi.c.o`、`xy_lcd_i8080.c.o`、`xy_lcd_st7789.c.o`；LED 归档包含 `xy_led_driver.c.o`，serial RGB 归档包含 `xy_rgb_matrix.c.o` 与 `xy_ws2812.c.o`。无实现源的 standalone `DRIVER_DISPLAY_RGB` 已移除，避免配置成功但不产生实现对象。

## 本轮发现并修复的配置风险

命令行 override 原先在默认依赖解析之后直接写入 resolved values，因此可以启用父依赖关闭的子符号。此行为会让配置文件声称启用某个 Display 子功能，但 root CMake source selection 仍缺少完整父功能上下文。解析器现在会在 override/select 收敛期间重复关闭依赖不满足的 bool 符号，并由回归测试锁定 fail-closed 行为。

root CMake 原先无条件 auto-discover 大部分组件；即使 all-off 生成值正确，Device、Crypto、Drivers、Net 等 target 仍会进入构建，Crypto 子库尤其会绕过任何组件内 guard。root discovery 现按 Kconfig 生成的 `XY_*` 值跳过关闭组件，all-off target inventory 作为 focused probe 防止“配置关闭、源码仍编译”的假矩阵。
