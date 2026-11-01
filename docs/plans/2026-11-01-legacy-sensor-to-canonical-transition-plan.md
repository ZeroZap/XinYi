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
| 旧框架头与别处**原型不兼容**的函数 | **9**（oled 5、mlx90614 2、sensor.h 2） | `check_function_prototype_collisions.py` |
| 全仓 struct/union/enum **tag** | 153，其中 **3** 个两处定义 | `check_tag_definition_collisions.py` |
| 旧框架消费方（非 inc/ 内部） | **19**（13 产品/示例 + 6 测试） | `grep -l` |
| canonical Device 消费方 | 59 | `grep -l` |
| 总头 `components/xy.h` 指向 | **canonical**（P2 后已切到 `device/inc/xy_device.h`） | `grep` |

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

### P1 — 把重复声明纳入守卫（1 个 slice，`DONE` 2026-11-01）

- 缺口：上一条守卫只判 **typedef**，不判**函数原型**。上面 9 处它一个都看不见。
- 已交付：`check_function_prototype_collisions.py` + 16 例自测。全仓 **3904 个函数名**，其中
  **38 个被两处以上以不兼容原型声明**，逐条 pin 理由，沿用同一套反向规则（pin 无理由 / pin 的
  不兼容已消失 / pin 的文件不存在）。
- 口径：**只判不兼容**。C 允许同名同原型的重复声明（合法且常见），只有不兼容才是缺陷——这是它与
  typedef 守卫的口径差异：typedef 每次重复都非法。
- 退出条件：9 处旧框架函数全部点名并 pin。**已满足**（含在 38 处之内）。
- 自己抓到的三处假阳性（都已修，不是靠放宽阈值）：
  ① 比较时把**参数名**算成差异（`key_bits` vs `key_size`）——C 里是同一个函数，剥掉参数名后
  `xy_aes_init`、`xy_oled_ssd1306_refresh` 两条 pin 自动过期并被守卫点名删除；
  ② 波次数字从「10 处」订正为「9 处」，oled 从 6 订正为 5；
  ③ 首次实现把 `extern "C"` 块当类型体整块丢弃，导致整份头文件一个声明都读不到（自测用例锁住）。
- 依赖：无。

### P2 — 总头切到 canonical（1 个 slice，风险最高，单独做，`DONE` 2026-11-01）

- 现状：`components/xy.h` 第 75 行包含旧框架，canonical 不在总头里 → 所有用总头的地方都在旧轨上。
- 已交付：`xy.h` 改为包含 `device/inc/xy_device.h`，旧 sensor 头移出总头。
- **探针结果：整仓只有 2 个文件包含总头**（一个是 `xy.h` 自己，另一个是
  `examples/pc_all_components/src/main.c`），且都不使用旧 sensor 类型。切之前担心的"19 个消费方全炸"
  没有发生——那 19 个都是直接包含旧头，不经总头。
- **探针炸出的真问题（这次最大的收获）**：`struct xy_device` 在 `components/xy_def.h:153` 和
  `components/device/inc/xy_device.h:194` 各定义一次。总头同时包含两者，所以切换后**任何包含总头的
  TU 都无法编译**（`redefinition of 'struct xy_device'`）。
  这是 **struct tag 碰撞**——不是 typedef，所以此前两个守卫一个都看不见。
  核实 `xy_def.h` 那份是死代码：`struct xy_obj`、`xy_spinlock`、`xy_dlist_t` 及其成员全仓**零引用**，
  来自一个未落地的 RT-Thread 式内核移植。已删除（保留 `xy_base_t`，它外部还有 2 处用）。
- **顺带补上第三个守卫**：`check_tag_definition_collisions.py`（+16 例自测）。全仓 **153 个 tag**，
  **3 个被两处以上定义**，全部 pin 理由。其中两处是新发现：
  `struct at_dev_attr` / `struct at_dev_ops` 在 `atc/core/xy_atc.h` 与 `ats/xy_ats.h` 各一份——
  **正是 Sprint 5 卡住的 AT 两套并存问题**。
- 口径：tag 判**存在**不判内容。C 里同一个 tag 定义两次就是错误，即使定义体完全相同；而
  `struct x` 与 `union x` 是不同命名空间，不构成碰撞。
- 自证：7 个变异全拦。最初 8 个变异里 3 个存活，暴露**定义体提取是死代码**（判定只看哪几个文件
  定义，配不配对花括号都改变不了结论）——已整段删除，而不是留一段测不到的代码。
- 退出条件：总头消费方编译通过；旧框架头的显式包含点全部列出。**已满足**。
- 验证：`xy.h` TU 编译 0 错误；`struct xy_device` + `xy_sensor_type_t` 经总头可见（canonical 侧数值）。
- 依赖：无。

### P3 — 旧框架 core 迁移

消费方按依赖从轻到重。**3b 已完成 2026-11-01**；3a 因语义差异被 pin 住（见下），3c 随 P4 一起做。

| 批次 | 文件 | 状态 |
|---|---|---|
| 3a | `components/fuel_gauge/drivers/xy_fg_*.{h,c}`（5） | **DONE 2026-11-01**：MAX17043 先迁（§2.2），BQ27Z746/BQ27Z561/BQ40Z50 同批收尾（§2.3） |
| 3b | `components/drivers/sensor/motion/bmi270/xy_bmi270.h` | **DONE 2026-11-01** |
| 3c | `components/sensor/core/{sensor_core,sensor_trigger,sensor_power}.c`、`sensor/sensor_bus.c` | 待 3a 全部迁完 → **unblocked** |

#### 2.1 3a 为什么不是类型对齐

五个 fuel-gauge 文件用的是旧框架的**总线 helper**（`xy_sensor_bus_t` + `xy_sensor_i2c_read_reg16`），
底层是 `xy_hal_i2c_mem_read`（寄存器写+读在**一次**事务里，重复起始）。canonical 的
`xy_i2c_device_read_reg` 是 `xy_hal_i2c_master_transmit` + `xy_hal_i2c_master_receive` 两次调用，
中间有一个 STOP。对 SMBus 电量计这是**线上行为变化**，而本阶段明文"不为迁移顺手改协议/寄存器行为"，
所以 3a 保持 pin，等 canonical 侧有一个重复起始的寄存器读入口再动。这不是命名问题。

**阻塞已解除（2026-11-01）**：canonical 侧补齐了三个一次性事务入口，走 `xy_hal_i2c_mem_read` /
`xy_hal_i2c_mem_write`，与旧 helper 的线上流量同形：

| 入口 | 对应旧 helper |
|---|---|
| `xy_i2c_device_read_reg_repeated_start()` | `xy_sensor_i2c_read` / `xy_sensor_i2c_read_reg` |
| `xy_i2c_device_read_reg16_repeated_start()` | `xy_sensor_i2c_read_reg16`（SMBus 字，低字节在前） |
| `xy_i2c_device_write_reg_repeated_start()` | `xy_sensor_i2c_write_reg16` |

**两种形状的差别是测出来的，不是写进注释的**：`test_device_bus_repeated_start` 用链接期计数桩替身
HAL，断言 `xy_i2c_device_read_reg` 恰好 1 次 transmit + 1 次 receive + 0 次 mem_read，而新入口恰好
1 次 mem_read + 0 次 transmit/receive。13 例。如果哪天老入口悄悄变成一次事务（或反过来），这个
测试会红——那正是 3a 当初卡住的原因，不该只活在一段散文里。

于是 3a 剩下的**只是驱动迁移本身**：把 `xy_sensor_bus_t` 换成 `xy_i2c_device_t`，四个旧调用换成上表
三个入口。这才是 pin 当时仍然存在的理由（pin 已随 §2.3 一起归零）。

#### 2.2 3a 第一支：MAX17043（2026-11-01）

MAX17043 是四个电量计里最窄的一支（`read_reg16` + `write_reg16`），所以先做它。改动只有三处形态：

- 头文件：`#include "xy_sensor_device.h"` → `"xy_device.h"`，成员 `xy_sensor_bus_t bus` →
  `xy_i2c_device_t bus`（**成员名不变**，调用点其余部分不用动）。
- 读：`xy_sensor_i2c_read_reg16()` → `xy_i2c_device_read_reg16_repeated_start()`，仍走 3 次重试，
  成功判据从 `== 0` 改为 `== XY_DEVICE_OK`（两者同为 0，比较语义不变）。
- 写：旧 helper 收 `uint16_t`，canonical 的写入口收**字节缓冲**，所以驱动自己按 SMBus 低字节在前
  组 2 字节再调 `xy_i2c_device_write_reg_repeated_start()`。这不是新协议，是把旧 helper 内部做的事
  搬到调用点。
- 注册：`xy_sensor_bus_config_i2c()` → `xy_i2c_device_init(..., MAX17043_IO_TIMEOUT_MS)`。**超时是刻意
  保留的 100 ms**：旧 `sensor_bus.c` 给每次传输 `XY_SENSOR_BUS_TIMEOUT_MS = 100`，而
  `xy_i2c_device_init()` 传 0 会变成 1000 ms——那是一次静默放宽，测试里现在断言 `timeout == 100`
  钉住它。

**形状是被测试钉住的**：`test_fg_max17043` 的 FFF 替身只定义
`xy_i2c_device_read_reg16_repeated_start` / `xy_i2c_device_write_reg_repeated_start` /
`xy_i2c_device_init`。变异验证：把驱动改回调用 `xy_i2c_device_read_reg`（split-STOP 形状），
链接期直接 `undefined reference to 'xy_i2c_device_read_reg'`——迁移回旧形状不是靠 review 拦住，
而是靠替身签名拦住。还原后 22 例全绿。守卫计数同步从 5 → **4**：
`legacy_sensor_includes ok (9 legacy framework headers, 4 consumers and all of them pinned, 4 pins)`。

剩下的三支 BQ 都是同一形态，但各自还多一条读路径（BQ40Z50 读 2/4 字节、BQ27Z561 读 1 字节与
`read_reg`），下一切片继续。

#### 2.3 3a 收尾：三支 BQ（2026-11-01）

BQ27Z746、BQ27Z561、BQ40Z50 同批迁完，形状与 MAX17043 一致，差别只在读路径的宽度：

| 驱动 | 旧调用 | canonical 入口 |
|---|---|---|
| BQ27Z746 | `xy_sensor_i2c_read(…, 2)` | `xy_i2c_device_read_reg_repeated_start(…, 2)` |
| BQ27Z561 | `xy_sensor_i2c_read_reg16()` / `…_read_reg()` | `…_read_reg16_repeated_start()` / `…_read_reg_repeated_start(…, 1)` |
| BQ40Z50 | `xy_sensor_i2c_read(…, 2)` / `(…, 4)` / `read_reg16()` | `…_read_reg_repeated_start(…, 2)` / `(…, 4)` / `…_read_reg16_repeated_start()` |

每条驱动仍保留自己的 3 次重试与 100 ms 单次预算（`BQ27Z746_IO_TIMEOUT_MS` 等），**不是**
`xy_i2c_device_init()` 的 0 → 1000 ms 默认值。BQ40Z50 的 `read_balance_status()` 顺手把
`int ret = …; if (ret == 0)` 的旧整数约定改成直接判 `== XY_DEVICE_OK`，行为不变。

**两条变异都是真 RED**（BQ27Z746 上做的，其余同构）：把 `IO_TIMEOUT_MS` 改成 `0U` →
`test_bq27z746_registers_default_i2c_bus` 报 `Expected 100 Was 0`；把读改回 split-STOP 的
`xy_i2c_device_read_reg` → 链接期 `undefined reference to 'xy_i2c_device_read_reg'`。两处都按
`sha256` 校验字节还原后全绿。

**守卫计数 4 → 0**：`legacy_sensor_includes ok (9 legacy framework headers, 0 consumers, 0 pins)`，
`PINS` 表保留为空表——它不是被删掉的规则，而是"当前确实没有消费方"的量测结果。
`test_fuel_gauge_smbus_hardware_smoke_example` 的替身同步换成三个 canonical 入口，并且**故意不定义
SPI 入口**：驱动若在总线上改配 SPI，链接期就红，而不是被一个空替身静默放过。

**边界**：这几支驱动在写方向上不产生字节级证据（BQ27Z561/BQ27Z746/BQ40Z50 只读不写，MAX17043 只写
`0x0000`），字节序仍由 `test_device_bus_repeated_start` 的 16 位读侧覆盖；无实板运行时。

**新增的依赖尚未写进 Kconfig**：四支驱动现在硬依赖 `components/device`，而
`COMPONENT_FUEL_GAUGE` 没有 `depends on COMPONENT_DEVICE`；`build/pc` 恰好是 `DEVICE=n` +
`FUEL_GAUGE=y`，静态库照编、`nm -u` 里留四个 `xy_i2c_device_*` 未定义符号。也就是说缺的是
"final link 才红"而不是"配置时就红"。补依赖会让 `build/pc` 丢掉 fuel gauge（那本身也是一个
产品决定），所以留给下一轮显式决策。

#### 3b 交付

`xy_bmi270.h` 是 `components/drivers/` 下**最后一个**包含旧框架的驱动头。它包含 `xy_sensor.h`
只为结构体成员 `xy_sensor_device_t base`，而该成员全仓**零读取**（驱动自己用 `bus_handle`/`bus_addr`）。
改为包含 canonical 的 `xy_device.h`：成员名不变、类型换成 canonical 的 `xy_sensor_device_t`
（`{xy_device_t base; const struct xy_sensor_api *; void *}`）。行为零变化——15 例
`test_bmi270` 全绿，且该成员本来就没人读。

**计数变成机器判据**：新守卫 `check_legacy_sensor_includes.py`（+16 例自测，CTest
`legacy_sensor_includes` / `_unit`）把"谁还在包含旧框架"变成可对账的清单——
`components/sensor/` 自身与 `tests/` 之外，基线 **6 → 5**，5 条 pin 逐条写理由（3a 的语义原因）。
三条反向规则：未 pin 的消费者即红 / pin 无理由即红 / pin 的 include 消失即过期报红。
**自证**：改动前直接跑守卫，点名 `xy_bmi270.h` 这一条（真 RED，不是构造的）；
自测再从 git 历史取**修复前**的该文件要求判红。假阳性处理：`examples/pc_all_components/src/main.c`
里是 `/* #include "xy_sensor.h" */` 注释（守卫按 C 注释剥离后不算消费者）、
`xy_i2c.h`/`xy_spi.h` 是**放错目录的唯一副本**（不是重复，属搬家问题，规则不管）。

- 退出条件：每批跑完 `make test-unit` 全绿，且该批的旧头包含行数为 0。3b **已满足**。
- 依赖：P2（顺序由探针结果定）。

### P4 — 删除重复副本（2026-11-02 第一刀已完成）

- 候选（**必须 P3 完成后确认零消费方才动手**）：
  - `components/sensor/inc/xy_oled_ssd1306.h`（14 条声明中 5 条与 canonical 原型不兼容）——**仍 blocked**：
    CTest `sensor_oled_ssd1306`（`tests/unit/CMakeLists.txt:1168`）正按 `${SENSOR}/inc` 编译
    `components/sensor/src/xy_oled_ssd1306.c`。删这个头等于先决定"废弃 sensor 侧 SSD1306 驱动及其
    CTest"，属产品决定（SSD1306 当前整体搁置）。
  - `components/sensor/inc/xy_mlxeeprom.h`（4 条中 2 条与 `drivers/sensor/temperature/mlx90614` 重复）
    ——**DONE 2026-11-02**：全仓 `#include "xy_mlxeeprom.h"` **0 处**（只有它自己那行 `@file` 注释提到这个名字），
    文件本身还有一处坏宏（`#define MLX90614_EE_ADDR emissivity 0x20` 展开成两个 token）。删除后
    `check_function_prototype_collisions.py` 的过期规则**当场报红**并点名两条 pin
    （`xy_mlx90614_get_emissivity` / `_set_emissivity: pin has expired`），删掉这两条后守卫从
    **38 → 36** 不兼容声明，自测 16/16。
  - `components/sensor/inc/xy_sensor{,_device,_channel,_attr,_trigger}.h` 中的重复函数声明——未动；
    `xy_sensor.h` 在 `tests/` 与 `components/sensor/` 之外仍有一个被注释掉的 example 引用，
    框架总头属 P2/P5 范围。
  - `components/sensor/core/*.c`（P3c 迁完后）——**DONE 2026-11-02**：`sensor_core.c`/`sensor_power.c`/
    `sensor_trigger.c` 三个文件**没有任何构建文件引用**（1487 个 CMake/Makefile 里零命中，
    `components/sensor/CMakeLists.txt` 只列根目录那批），`build/` 下也找不到由它们编出的 `.o`。
    它们实现的是旧框架 `xy_sensor_*` API（与根目录同名文件是**两套 API**，不是两份副本），随 P4 一并删除。
- 不动：`xy_i2c.h`、`xy_spi.h`、`xy_w25qxx.h`、`xy_max17043.h`、`xy_coulomb.h`、`xy_dmp.h`、`xy_ret.h`
  —— 它们全仓只有一份，只是**放错了目录**，属"搬家"而非"删除"，另开 slice 处理。
- 退出条件：碰撞数从 23 降到 20（三条 sensor pin 全部过期并自动报红后删除）。**原型侧已 38 → 36**；
  类型侧仍 23（`xy_mlxeeprom.h` 只带宏不带类型，所以今天这一刀动不到这个数）。

#### 行为中性的证明方式（2026-11-02）

删完以后 `make`（增量，build/pc）重编，**19 个静态归档 sha256 与删除前逐个相同**——包括
`libsensor_component.a`。也就是说这四个文件从来没有进过任何产物：它们不是"少编了一次"，
而是**一直没有编**，`nm`/归档顺序从此不必再替它们回答两份定义该听谁的。


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

1. **旧框架消费方数**（基线 19：13 产品/示例 + 6 测试）——只能减。**P3b 后 18**，
   **P3a 第一支（MAX17043）后 17**，**P3a 收尾（三支 BQ）后 13**。
   其中 `components/`、`boards/`、`projects/`、`examples/` 下（即不含 `tests/` 与
   `components/sensor/` 自身）的消费方由 `check_legacy_sensor_includes.py` 逐条对账：
   基线 6 → P3b 后 **5** → MAX17043 迁移后 **4** → 三支 BQ 迁移后 **0**。这个数从此不再靠人记。
2. **同名碰撞数**（基线 23）——P5 后应为 20，删完 sensor 副本后继续降；
   **原型侧另有口径**：`check_function_prototype_collisions.py` 基线 38 条不兼容声明，P4 第一刀
   （删 `xy_mlxeeprom.h` 并移除两条过期 pin）后 **36**，其中 sensor 系从 9 条降到 7 条；
3. **总头指向**（基线：旧框架）——P2 后应为 canonical。

一个 slice 只有在这三项之一真的动了才算进展；「建了新的 canonical 副本」不计分。
