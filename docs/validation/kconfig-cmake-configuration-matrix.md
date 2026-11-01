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
| 非法 Display 子功能单开 | `DRIVER_DISPLAY_LCD_SPI=ON` 且父符号关闭时，子符号必须保持 `OFF`；`DRIVER_DISPLAY` 关闭时 `if` 块内的直接子符号（`DRIVER_DISPLAY_LCD`/`_OLED`/`_LED`）同样必须保持 `OFF` | PASS | `python3 -m unittest tests.test_kconfig_parser -v`（15/15）；root PC configure 后检查三个生成变量均为 `OFF`；`-DKCONFIG_OVERRIDES='BUILD_TESTING=OFF;FOTA_ENABLED=OFF;DRIVER_DISPLAY=OFF;DRIVER_DISPLAY_LCD=ON'` 真实 configure 生成 `set(CONFIG_DRIVER_DISPLAY OFF)` + `set(CONFIG_DRIVER_DISPLAY_LCD OFF)`（修复前同一命令生成 `_LCD ON`） |
| PC 默认基线 | 默认 Kconfig + FOTA off 可 configure/build | PASS | `cmake -S . -B build/pc ...`；`cmake --build build/pc -j$(nproc)` |
| 全关最小配置 | 可选组件、GUI、网络、驱动和额外组件关闭后仍可 configure/build，关闭项不得泄漏 root target | PASS | 显式 all-off override configure/build 通过；target inventory 不含 Device/Charger/Crypto/DM/Sensor/Actuator/Drivers/GUI/Net/FOTA/PM/PID/MUX/SYS/Fuel Gauge |
| 核心组件逐项开启 | Device/Crypto/DM/Sensor/Actuator 各自具备可重复 configure/build 结果 | PASS | Device-only、Crypto-only、DM-only、Sensor-only 与 Actuator-only 组合已验证生成变量、focused build、target inventory 与归档对象；Sensor/Actuator 的两个兼容入口分别生成相同 root target 对象 |
| Display 子功能 | OLED/SSD1306、LCD SPI/I8080/ST7789、LED/serial RGB 的父子依赖和 source selection 一致 | PASS | 合法组合的生成变量、focused targets 和归档 source inventory 一致；无实现源的 standalone `DRIVER_DISPLAY_RGB` 已从 root/local Kconfig 和 dormant CMake 分支移除，由 parser regression guard 锁定 |
| Sensor 兼容模式 | legacy `XY_SENSOR_ENABLE`、`COMPONENT_SENSOR` 与新 Device driver 路径的 active ownership 明确 | PASS | 两个 framework 符号分别单开均生成同一 `sensor_component` 和相同归档对象；`src/xy_*` 不在该 root target；`DRIVER_SENSOR` 是独立 Device-driver 路径且本 probe 保持关闭。此结果记录现状，不将三轨实现认定为已收敛 |
| STM32U5 默认配置 | 平台条件默认值与 target compile 一致 | PASS | clean STM32U5 configure/build 通过；生成配置确认 STM32/U5、FS/FlashDB 条件默认值，FOTA 显式关闭；`arm-none-eabi-gcc` 完成 root static-library compile。仅记 C1，不构成实板证据 |

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

cmake -S . -B build/config-matrix-crypto \
  -DHAL_PLATFORM=PC \
  -DCMAKE_BUILD_TYPE=Release \
  -DKCONFIG_OVERRIDES='<all optional components OFF except COMPONENT_CRYPTO=ON>'
cmake --build build/config-matrix-crypto --target xy_tiny_crypto -j$(nproc)
python3 -c "from pathlib import Path; p=Path('build/config-matrix-crypto/config.cmake').read_text(); required=['set(CONFIG_COMPONENT_CRYPTO ON)','set(CONFIG_COMPONENT_DEVICE OFF)','set(CONFIG_COMPONENT_DM OFF)','set(CONFIG_COMPONENT_SENSOR OFF)','set(CONFIG_COMPONENT_ACTUATOR OFF)']; missing=[x for x in required if x not in p]; assert not missing, missing"
cmake --build build/config-matrix-crypto --target help
# Verify xy_tiny_crypto/xy_sm2/xy_sm3/xy_sm4 are present and disabled component targets are absent.
ar t build/config-matrix-crypto/components/crypto/libxy_tiny_crypto.a
ar t build/config-matrix-crypto/components/crypto/xy_sm2/libxy_sm2.a
ar t build/config-matrix-crypto/components/crypto/xy_sm3/libxy_sm3.a
ar t build/config-matrix-crypto/components/crypto/xy_sm4/libxy_sm4.a

cmake -S . -B build/config-matrix-dm \
  -DHAL_PLATFORM=PC \
  -DCMAKE_BUILD_TYPE=Release \
  -DKCONFIG_OVERRIDES='<all optional components OFF except COMPONENT_DM=ON>'
cmake --build build/config-matrix-dm --target xy_dm -j$(nproc)
python3 -c "from pathlib import Path; p=Path('build/config-matrix-dm/config.cmake').read_text(); required=['set(CONFIG_COMPONENT_DM ON)','set(CONFIG_COMPONENT_DEVICE OFF)','set(CONFIG_COMPONENT_CRYPTO OFF)','set(CONFIG_COMPONENT_SENSOR OFF)','set(CONFIG_COMPONENT_ACTUATOR OFF)','set(CONFIG_FS_ENABLED OFF)','set(CONFIG_FS_FLASHDB OFF)','set(CONFIG_NOR_FLASH_ENABLED OFF)']; missing=[x for x in required if x not in p]; assert not missing, missing"
cmake --build build/config-matrix-dm --target help
# Verify xy_dm is present and disabled component targets are absent.
ar t build/config-matrix-dm/components/dm/libxy_dm.a

cmake -S . -B build/config-matrix-sensor \
  -DHAL_PLATFORM=PC \
  -DCMAKE_BUILD_TYPE=Release \
  -DKCONFIG_OVERRIDES='<all optional components OFF except COMPONENT_SENSOR=ON>'
cmake --build build/config-matrix-sensor --target sensor_component -j$(nproc)
python3 -c "from pathlib import Path; p=Path('build/config-matrix-sensor/config.cmake').read_text(); required=['set(CONFIG_COMPONENT_SENSOR ON)','set(CONFIG_XY_SENSOR_ENABLE OFF)','set(CONFIG_DRIVER_SENSOR OFF)','set(CONFIG_COMPONENT_DEVICE OFF)']; missing=[x for x in required if x not in p]; assert not missing, missing"
cmake --build build/config-matrix-sensor --target help
ar t build/config-matrix-sensor/components/sensor/libsensor_component.a

cmake -S . -B build/config-matrix-sensor-legacy \
  -DHAL_PLATFORM=PC \
  -DCMAKE_BUILD_TYPE=Release \
  -DKCONFIG_OVERRIDES='<all optional components OFF except XY_SENSOR_ENABLE=ON>'
cmake --build build/config-matrix-sensor-legacy --target sensor_component -j$(nproc)
python3 -c "from pathlib import Path; p=Path('build/config-matrix-sensor-legacy/config.cmake').read_text(); required=['set(CONFIG_COMPONENT_SENSOR OFF)','set(CONFIG_XY_SENSOR_ENABLE ON)','set(CONFIG_DRIVER_SENSOR OFF)','set(CONFIG_COMPONENT_DEVICE OFF)']; missing=[x for x in required if x not in p]; assert not missing, missing"
cmp <(ar t build/config-matrix-sensor/components/sensor/libsensor_component.a) \
    <(ar t build/config-matrix-sensor-legacy/components/sensor/libsensor_component.a)

cmake -S . -B build/config-matrix-actuator \
  -DHAL_PLATFORM=PC \
  -DCMAKE_BUILD_TYPE=Release \
  -DKCONFIG_OVERRIDES='<all optional components OFF except COMPONENT_ACTUATOR=ON>'
cmake --build build/config-matrix-actuator --target xy_actuator -j$(nproc)
python3 -c "from pathlib import Path; p=Path('build/config-matrix-actuator/config.cmake').read_text(); required=['set(CONFIG_COMPONENT_ACTUATOR ON)','set(CONFIG_XY_ACTUATOR_ENABLE OFF)','set(CONFIG_COMPONENT_DEVICE OFF)','set(CONFIG_COMPONENT_SENSOR OFF)']; missing=[x for x in required if x not in p]; assert not missing, missing"
cmake --build build/config-matrix-actuator --target help
ar t build/config-matrix-actuator/components/actuator/libxy_actuator.a

cmake -S . -B build/config-matrix-actuator-legacy \
  -DHAL_PLATFORM=PC \
  -DCMAKE_BUILD_TYPE=Release \
  -DKCONFIG_OVERRIDES='<all optional components OFF except XY_ACTUATOR_ENABLE=ON>'
cmake --build build/config-matrix-actuator-legacy --target xy_actuator -j$(nproc)
python3 -c "from pathlib import Path; p=Path('build/config-matrix-actuator-legacy/config.cmake').read_text(); required=['set(CONFIG_COMPONENT_ACTUATOR OFF)','set(CONFIG_XY_ACTUATOR_ENABLE ON)','set(CONFIG_COMPONENT_DEVICE OFF)','set(CONFIG_COMPONENT_SENSOR OFF)']; missing=[x for x in required if x not in p]; assert not missing, missing"
cmp <(ar t build/config-matrix-actuator/components/actuator/libxy_actuator.a) \
    <(ar t build/config-matrix-actuator-legacy/components/actuator/libxy_actuator.a)

cmake -S . -B build/config-matrix-stm32u5-default-20260824 \
  -DHAL_PLATFORM=STM32U5 \
  -DCMAKE_BUILD_TYPE=Release \
  -DKCONFIG_OVERRIDES='BUILD_TESTING=OFF;FOTA_ENABLED=OFF'
cmake --build build/config-matrix-stm32u5-default-20260824 -j$(nproc)
python3 -c "from pathlib import Path; p=Path('build/config-matrix-stm32u5-default-20260824/config.cmake').read_text(); required=['set(CONFIG_PLATFORM_PC OFF)','set(CONFIG_PLATFORM_STM32 ON)','set(CONFIG_PLATFORM_STM32U5 ON)','set(CONFIG_FS_ENABLED ON)','set(CONFIG_FS_FLASHDB ON)','set(CONFIG_FOTA_ENABLED OFF)']; missing=[x for x in required if x not in p]; assert not missing, missing; cache=Path('build/config-matrix-stm32u5-default-20260824/CMakeCache.txt').read_text(); assert 'CMAKE_C_COMPILER:STRING=/home/eugene/Tools/arm-gnu-toolchain/bin/arm-none-eabi-gcc' in cache"

make test-unit
cmake -S . -B build/pc -DHAL_PLATFORM=PC -DCMAKE_BUILD_TYPE=Release \
  -DKCONFIG_OVERRIDES='BUILD_TESTING=OFF;FOTA_ENABLED=OFF'
cmake --build build/pc -j$(nproc)
git diff --check
```

结果：parser 5/5、Host CTest 178/178、PC root build 和 whitespace gate 均通过；all-off 最小配置 configure/build 与 target inventory 通过；Device-only 组合仅启用 `COMPONENT_DEVICE`，`xy_device` focused build、target inventory 与归档对象检查通过；Crypto-only 组合仅启用 `COMPONENT_CRYPTO`，`xy_tiny_crypto` 及其 `xy_sm2`/`xy_sm3`/`xy_sm4` 依赖构建、target inventory 与归档对象检查通过（现有 SM2/PHOTON warning 保留为已知源码风险，本矩阵不将 compile 结果升级为安全证据）；DM-only 组合仅启用 `COMPONENT_DM`，`xy_dm` focused build、target inventory 与归档对象检查通过，归档仅含 `xy_json.c.o` 与 `xy_fs.c.o`，FlashDB/NOR 均保持关闭（现有 CLIB unused-variable warning 保留为已知源码风险）；Sensor-only 的 `COMPONENT_SENSOR` 与兼容 `XY_SENSOR_ENABLE` 两种单开配置均构建同一 `sensor_component`，归档对象列表完全相同，且独立 `DRIVER_SENSOR`/Device 路径保持关闭；Actuator-only 的 `COMPONENT_ACTUATOR` 与兼容 `XY_ACTUATOR_ENABLE` 两种单开配置均构建同一 `xy_actuator`，归档均仅含 `xy_actuator.c.o`，Device/Sensor 等关闭组件未泄漏 target。现有 Sensor 源码 warning 保留为已知风险，本矩阵只确认配置 ownership 现状。OLED/SSD1306、LCD SPI/I8080/ST7789 与 LED/serial RGB 合法组合的生成值、focused target 构建与归档 source inventory 通过。LCD 归档包含 `xy_lcd.c.o`、`xy_lcd_spi.c.o`、`xy_lcd_i8080.c.o`、`xy_lcd_st7789.c.o`；LED 归档包含 `xy_led_driver.c.o`，serial RGB 归档包含 `xy_rgb_matrix.c.o` 与 `xy_ws2812.c.o`。无实现源的 standalone `DRIVER_DISPLAY_RGB` 已移除，避免配置成功但不产生实现对象。STM32U5 默认配置使用 `arm-none-eabi-gcc` 完成 clean root static-library compile，生成值确认 `PLATFORM_STM32=ON`、`PLATFORM_STM32U5=ON`、`FS_ENABLED=ON`、`FS_FLASHDB=ON`，且本 probe 显式保持 `FOTA_ENABLED=OFF`；编译日志中的既有 warning 未被升级为硬件、安全或产品证据。

## 本轮发现并修复的配置风险

命令行 override 原先在默认依赖解析之后直接写入 resolved values，因此可以启用父依赖关闭的子符号。此行为会让配置文件声称启用某个 Display 子功能，但 root CMake source selection 仍缺少完整父功能上下文。解析器现在会在 override/select 收敛期间重复关闭依赖不满足的 bool 符号，并由回归测试锁定 fail-closed 行为。

root CMake 原先无条件 auto-discover 大部分组件；即使 all-off 生成值正确，Device、Crypto、Drivers、Net 等 target 仍会进入构建，Crypto 子库尤其会绕过任何组件内 guard。root discovery 现按 Kconfig 生成的 `XY_*` 值跳过关闭组件，all-off target inventory 作为 focused probe 防止“配置关闭、源码仍编译”的假矩阵。

### 2026-11-02 组件间依赖：fuel gauge 必须和 Device 一起出现

四支 fuel-gauge 驱动走 canonical Device 总线入口后，`COMPONENT_FUEL_GAUGE` 与
`COMPONENT_DEVICE` 之间存在**硬依赖**，而 Kconfig 里没有任何表达。实测：默认 PC 配置
（`COMPONENT_DEVICE=n`、`COMPONENT_FUEL_GAUGE=y`）生成的 `libxy_fuel_gauge.a` 带 4 个
`xy_i2c_device_*` 未定义符号，全归档交叉对账显示建置内**零个归档**定义它们。

现在 `COMPONENT_FUEL_GAUGE` 在 root `Kconfig` 里 `select COMPONENT_DEVICE`，并且
`components/fuel_gauge/CMakeLists.txt` 在 `xy_device` 不存在时 `FATAL_ERROR`（原先静默跳过链接）。
矩阵含义的变化只有一处，需按新口径读：**只要 fuel gauge 保持默认 `y`，`COMPONENT_DEVICE` 就会被
选上**；显式把 `COMPONENT_FUEL_GAUGE` 也关掉的组合（本文件里的 all-off / crypto-only 等 probe，
它们的 Sensor 符号同样是关的）生成值不变，仍为 `CONFIG_COMPONENT_DEVICE=n`（实测 `n/n`）。
**sensor-only probe 从下一刻起不再属于这一类**，见下一节。

复现命令（默认配置下的缺陷形状）：

```bash
cmake -B <dir> -S . -DHAL_PLATFORM=PC -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF
cmake --build <dir> --target xy_fuel_gauge -j$(nproc)
nm -u <dir>/components/fuel_gauge/libxy_fuel_gauge.a | grep xy_i2c_device
# 修复前：4 个 U，建置内无定义；修复后：CONFIG_COMPONENT_DEVICE=y，xy_device 提供全部 4 个符号
```

`check_kconfig_fragment_agreement.py`（CTest `kconfig_fragment_agreement` / `_unit`）防止这条
`select` 只被写进构建不读的 `components/fuel_gauge/Kconfig`。

**连带观察（未修）**：默认 L4/U5 配置现在也会编 `components/device`，把两处**既存**warning 一并带进
默认构建日志——`components/device/src/xy_device_pm.c` 的 `-Wtype-limits`（"comparison is always true
due to limited range of data type"），每平台 2 条。它们不是本轮引入的代码，是本轮让该文件第一次进入
默认构建；记录在此以免下次被误判为新回归。

### 2026-11-02 同一形态的第二处：driver/Sensor 归档也必须有 Device

fuel gauge 那一处收口后，同一形态在别处还在。这次先按配置逐个量（七组：sensor-only、六个
`DRIVER_*` 单开），建完整棵并做「归档 `nm -u` ∩ 全建置 `nm --defined-only`」对账：

| 配置 | `CONFIG_COMPONENT_DEVICE`（修复前） | 未满足符号 |
|---|---|---|
| sensor-only（`XY_SENSOR_ENABLE=ON`、fuel gauge 关） | `n` | `libxy_sensor_drivers_core.a` **5** 个 `xy_i2c_device_*` |
| `DRIVER_SENSOR` / `_DISPLAY` / `_POWER` / `_WIRELESS` / `_SYSTEM` 单开 | `n` | `libxy_drivers.a` **7** 个（5 `xy_i2c_device_*` + `xy_spi_device_init/transfer`），`libxy_sensor_drivers_core.a` **5** 个 |
| all-off / crypto-only | `n` | 无（drivers 目录不进构建） |

关键是**缺陷形状与 fuel gauge 完全同源**：静态归档照编，`rc=0`，没有任何归档定义这些符号，
失败被推迟到第一个真正链接它的可执行文件——本仓库默认建置里没有这样一个可执行文件，所以长期不可见。

根因有两层，都在 `components/drivers/CMakeLists.txt`：①进入该目录就无条件 `add_library(xy_sensor_drivers_core ...)`；
②聚合 `xy_drivers` 的 source 只对 display/storage 做了 family 过滤，sensor/power/wireless/system
的驱动**始终在聚合里**。于是任何一个 `DRIVER_*` 单开都需要 Device，而 Kconfig 一个 `select` 都没有。

修法：root `Kconfig` 里 **`DRIVER_SENSOR` / `DRIVER_DISPLAY` / `DRIVER_STORAGE` / `DRIVER_POWER` /
`DRIVER_WIRELESS` / `DRIVER_SYSTEM` / `COMPONENT_SENSOR` / `XY_SENSOR_ENABLE` 全部 `select COMPONENT_DEVICE`**；
`components/drivers/CMakeLists.txt` 在 `xy_device` 目标缺失时 `FATAL_ERROR`（并把 `xy_device` 变成
`xy_sensor_drivers_core` / `xy_drivers` 的 PUBLIC 依赖，让"谁提供这些符号"写在构建里），
`components/sensor/CMakeLists.txt` 同样 fail-closed——它原先把 `xy_device` 当成库名传给
`target_link_libraries`，目标不存在时 CMake 会静默降级成 `-lxy_device`。

验证（全部真跑）：
- 同一批七个配置修复后 **`CONFIG_COMPONENT_DEVICE=ON`**，且 `libxy_device.a` 正好提供归档需要的
  10 个符号；逐个归档复查为「refs satisfied」，没有配置再产出不可链接的归档。
- 两次真 RED：从 root 删掉 `XY_SENSOR_ENABLE` 的 select → configure **rc=1** 并点名
  `target 'xy_device' does not exist`；从 root 删掉 `DRIVER_SENSOR` 的 select（同一次 configure 里
  Sensor 全关）→ 同样 rc=1。两次按 `sha256 4cb5ad42…` 字节还原。

**矩阵口径的变化（必须按新口径读）**：sensor-only 与任一 `DRIVER_*` 单开 probe 现在生成
`CONFIG_COMPONENT_DEVICE=y`。「Sensor-only PASS」这一行此前只检查生成值、focused target 与归档对象
列表——三项都对，但归档根本链不起来，属**假 PASS**；现在它至少是一个能链接的配置。
`check_kconfig_fragment_agreement.py` 仍守着「这条 select 只能写在 root」，`components/sensor/Kconfig`
里的同名 `select` 只是文档同步。

新守卫 `tests/unit/support/check_device_bus_dependency.py`（CTest `device_bus_dependency` / `_unit`，
15 例自测）：凡源文件引用 `xy_(i2c|spi|uart|adc|gpio)_device_*` 的组件树，必须在显式表里映射到
root Kconfig 符号，且这些符号必须 `select COMPONENT_DEVICE`；表还会反向报三类腐烂——目录已无引用
（映射过期）、符号被改名/删除、出现未映射的新消费方。变异验证：删掉 `DRIVER_STORAGE` 的 select →
守卫 rc=1 并点名该符号，恢复后 rc=0。

**本轮明确未修（独立 slice）**：`DRIVER_STORAGE=ON` 单开时构建仍 **rc=2**，与本轮无关——
`components/drivers/storage/littlefs/xy_littlefs_w25q128.h` 无条件 `#include "lfs.h"`，而 littlefs
的 include root 只在 `CONFIG_FS_LITTLEFS=ON` 时才加到 `xy_drivers` 上，于是这个「合法配置」
编不过。它属于 storage 驱动的条件编译边界，不与 Device 依赖混在一刀里。
