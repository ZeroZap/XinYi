# 旧 sensor 框架 → canonical Device 过渡计划（2026-11-01）

## 0. 决策前提

**canonical = `components/device/`**（`xy_device.h` 及其 `src/`）。`components/sensor/` 里与它同名的
类型和函数属于**尚未过渡的旧框架存留**，不是第二套合法实现。

本计划只管**框架层**（类型、函数原型、头文件归属）。各传感器驱动的 owner 化是另一条线，见
`2026-09-20-sensor-canonical-migration-plan.md` 的 M0–M5，本计划不重复、不覆盖、不改写它。

## 1. 度量基线（2026-11-01 实测量，非估算）

| 项 | 数字 | 出处 |
|---|---|---|
| 全仓 typedef 名 | 1162 | `check_type_definition_collisions.py` |
| 同一个名字被两处以上定义 | **23**（17 定义体不同、6 相同） | 同上 |
| 其中属于旧 sensor 框架的 | **3**（`xy_sensor_type_t`、`xy_sensor_device_t`、`xy_sensor_value_t`） | 同上 |
| `components/sensor/inc/` 头文件 | 14（仅 5 个属 sensor 框架本身） | `ls` |
| 旧框架头与别处**重复声明**的函数 | **10**（oled 6、mlx90614 2、sensor.h 2） | 交叉声明扫描 |
| 旧框架消费方（非 inc/ 内部） | **19**（13 产品/示例 + 6 测试） | `grep -l` |
| canonical Device 消费方 | 59 | `grep -l` |
| 总头 `components/xy.h` 指向 | **旧框架**（第 75 行 `sensor/inc/xy_sensor.h`），canonical 完全不在总头里 | `grep` |

三处**硬冲突**（不是措辞差异，实测编译报错）：

- `xy_sensor_type_t`：`TEMP=1`（sensor）vs `TEMP=0`（device），成员名 `ACCEL/GYRO/MAGN` vs
  `ACCELEROMETER/GYROSCOPE/MAGNETOMETER`。同 TU 包含两者任一顺序均报
  `redeclaration of enumerator` + `conflicting types`，并连带 `xy_sensor_value_t`、
  `struct xy_sensor_device`、`xy_sensor_device_t` 冲突、`xy_sensor_sample_fetch` 两份不兼容原型。
- `xy_oled_ssd1306_init`：旧版 `(dev, i2c_handle)` 两参数，canonical 版三参数；
  `xy_oled_ssd1306_clear` 旧版返回 `int`，canonical 返回 `void`。
- `xy_mlx90614_get/set_emissivity`：旧版收 `void *i2c_handle`，canonical 收 `xy_mlx90614_t *dev`。

## 2. 阶段划分

每个阶段都有**独立的退出条件**，任何阶段都不允许"顺手把下一阶段也做了"。

### P0 — 冻结与度量（`DONE` 2026-11-01）

- 已交付：`tests/unit/support/check_type_definition_collisions.py` + 15 例自测，23 处碰撞逐条 pin
  理由，三条反向规则（pin 无理由 / pin 的碰撞已消失 / pin 的名字已无人定义）。
- 退出条件：守卫进 gate，第 24 处碰撞出现即红。**已满足**。
- 本阶段不改任何源码，只让"看不见"变成"有成文理由的清单"。

### P1 — 把重复声明纳入守卫（1 个 slice）

- 缺口：现有守卫只判 **typedef**，不判**函数原型**。上面 10 处重复函数它一个都看不见。
- 动作：把 `check_type_definition_collisions` 扩成"声明冲突"守卫，覆盖同名函数的**原型差异**
  （参数个数/类型、返回类型），继续沿用 pin + 反向规则。
- 退出条件：10 处重复函数全部被点名并 pin 上理由；自测含判别力用例（参数个数差、返回类型差）。
- 依赖：无。

### P2 — 总头切到 canonical（1 个 slice，风险最高，单独做）

- 现状：`components/xy.h` 第 75 行包含旧框架，canonical 不在总头里 → 所有用总头的地方都在旧轨上。
- 动作：`xy.h` 改为包含 `device/inc/xy_device.h`，并**移出**旧 sensor 框架头；旧头改为显式 opt-in。
- 为什么单独做：这一步会让"同时包含两套"从潜伏变成现实，必然暴露 P3 之前的所有冲突——它是一次
  **探针**，量出真实调用面，而不是一次重构。
- 退出条件：`xy.h` 消费方（examples/pc_all_components 等）编译通过；旧框架头的显式包含点全部列出。
- 风险：暴露的冲突数未知。若超过 20 处，停下回到 P1 补判据再继续。

### P3 — 旧框架 core 迁移（2–3 个 slice）

消费方 19 个，按依赖从轻到重：

| 批次 | 文件 | 特点 |
|---|---|---|
| 3a | `components/fuel_gauge/drivers/xy_fg_*.{h,c}`（5）+ 对应 5 个测试 | 只用到 `xy_sensor_device.h` 里的少量类型，改到 canonical 的等价类型 |
| 3b | `components/drivers/sensor/motion/bmi270/xy_bmi270.h` | 已经同时包含 canonical 的 `xy_device_error.h`，只差类型对齐 |
| 3c | `components/sensor/core/{sensor_core,sensor_trigger,sensor_power}.c`、`sensor/sensor_bus.c` | 是旧框架**自身实现**，迁完即无调用者，随 P4 一起删 |

- 退出条件：每批跑完 `make test-unit` 全绿，且该批的旧头包含行数为 0。
- 依赖：P2（顺序由探针结果定）。

### P4 — 删除重复副本（1 个 slice，仅限已无消费方的）

- 候选（**必须 P3 完成后确认零消费方才动手**）：
  - `components/sensor/inc/xy_oled_ssd1306.h`（14 条声明中 6 条与 canonical 重复）
  - `components/sensor/inc/xy_mlxeeprom.h`（4 条中 2 条与 `drivers/sensor/temperature/mlx90614` 重复）
  - `components/sensor/inc/xy_sensor{,_device,_channel,_attr,_trigger}.h` 中的重复函数声明
  - `components/sensor/core/*.c`（P3c 迁完后）
- 不动：`xy_i2c.h`、`xy_spi.h`、`xy_w25qxx.h`、`xy_max17043.h`、`xy_coulomb.h`、`xy_dmp.h`、`xy_ret.h`
  —— 它们全仓只有一份，只是**放错了目录**，属"搬家"而非"删除"，另开 slice 处理。
- 退出条件：碰撞数从 23 降到 20（三条 sensor pin 全部过期并自动报红后删除）。

### P5 — 守卫收紧（1 个 slice）

- 动作：删掉三条 sensor 相关 pin；`check_type_definition_collisions` 的 `PINS` 里 sensor 部分归零。
- 退出条件：守卫仍绿，且 pin 过期规则证明"删得对"（删多了会红）。

## 3. 不做什么

- **不删任何传感器驱动**，不动 Pandora 已闭环的 AHT10 / AP3216C / ICM20608 owner。
- **不改 M0–M5 的驱动路线**，那是另一条轴。
- **不为迁移顺手改协议/寄存器行为**——P3 只做类型与头文件对齐，行为由现有 Host 测试守住。
- **`components/sensor/inc/` 里那 7 个"放错目录"的头不在本计划内**，它们是搬家问题。

## 4. 进度记账

沿用迁移计划既有口径，每个 slice 后记录三个数：

1. **旧框架消费方数**（基线 19）——只能减；
2. **同名碰撞数**（基线 23）——P5 后应为 20，删完 sensor 副本后继续降；
3. **总头指向**（基线：旧框架）——P2 后应为 canonical。

一个 slice 只有在这三项之一真的动了才算进展；「建了新的 canonical 副本」不计分。
