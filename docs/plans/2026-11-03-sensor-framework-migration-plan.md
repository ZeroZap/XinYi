# Sensor 框架迁移计划：旧框架 → canonical Device

> 状态：2026-11-03 起草。目标读者：接手这条线的人（含未来的我自己）。
> 原则：**每删一个旧驱动、每搬一个独有驱动，都必须有对应的验证证明行为没丢**——不是"挪代码"，是"用测试证明等价"。

## 为什么现在做

`components/sensor/`（旧框架）和 `components/device/` + `components/drivers/sensor/`（canonical）**两套都编译、都有测试**：

- `tests/unit/CMakeLists.txt` 里有 **89 处**引用 `sensor_*.c`（旧驱动被编译进测试树）。
- canonical 侧已有 `test_<name>_device` 一套完整测试（aht10/bmp280/ak09918/as5048b/ina219/...）。

两套并存意味着：同一颗传感器有两个驱动、两个测试、两份维护。旧框架的 `sensor_device_t`/`sensor_ops_t`/`sensor_data_t` 是未迁移存留，canonical 的 `xy_<name>_t`/`xy_error_t`/`xy_hal_*` 是权威。

## 现状盘点（2026-11-03 实测）

旧框架 `components/sensor/sensors/` 下 **39 个驱动**，按器件名与 canonical 比对：

- **27 个重复**（canonical 已有等价物）→ 验证覆盖后删旧的：adxl362, aht10, aht20, ak09918, ap3216c,
  apds9960, as5600, bh1750, bma400, bmp280, ccs811, icm20608, ist8310, kx023, lis2dh12, lis2dw12,
  lsm6dsl, lsm6dso, lsm6dsr, lsm9ds1, max44009, mpu6050, qma6100p, qmc5883l, sht30, vcnl4040,
  vl53l0x, **as5048**（对应 canonical `as5048b`）。
- **10 个独有**（canonical 没有，须用 `xy_device` 框架重写迁移）：acs712, cmm905, fsr, gps,
  guvas12sd, mg811, mq135, mq3, mq7, sc7a20。其中 7 个是模拟量读出（acs712/fsr/guvas12sd/
  mg811/mq135/mq3/mq7），gps 是 UART/NMEA，cmm905 待查，sc7a20 是加速度计。
  （另有 `silan_sc7a20`，疑似 sc7a20 的变体，迁移时一并判断去留。）

旧框架的**框架层**（不是驱动，是支撑全部旧驱动的底座），全部搬完后才能删：
`core`(sensor_core.c), `bus`, `fifo`, `filter`, `calibration`, `fusion`, `interrupt`, `power`,
`threshold`, `mem_pool`, `dma`, `self_test`, `moton`, `devices`, `framework`。

## 验证策略（每条 slice 的"做好验证"具体化）

1. **删重复驱动前**：必须证明 canonical 驱动的测试覆盖了旧驱动的行为。
   - 做法：对照旧驱动的测试断言（`test_<name>_sensor.c` 或 `test_<name>_legacy_sensor.c`）与
     canonical 的 `test_<name>_device.c`。旧测试里每一条**有独特断言**的（寄存器、换算公式、
     边界），canonical 测试必须已有等价断言，或补上，否则不删。
2. **搬独有驱动时**：新 `xy_<name>.c` + `xy_<name>.h` + `test_<name>_device.c`，测试要**复现旧驱动
   的行为**（同样的换算、同样的返回码语义），并做一次变异证明测试真能红。
3. **删旧框架层前**：`check_legacy_sensor_includes` 必须报"0 消费方、0 pin"（它现在就是干这个的），
   且 `check_type/function/tag_definition_collisions` 里 sensor 相关的 pin 必须已经自然过期。
4. 每一步跑**隔离 worktree 门禁**（`dd9f5188` 起的 `make test-unit`，全绿才提交），提交后三方核对。

## 顺序（先易后难、先删重复后搬独有）

- **Slice A**：删 as5048（重复、canonical as5048b 已测）——最小的端到端验证样板。
- **Slice B（进行中）**：删其余 26 个重复，分批（按 test 覆盖逐一核对）。
  - 批 1（2026-11-03）：删 `sht30`、`bh1750` 两个重复（canonical `xy_sht30.c` / `xy_bh1750.c` 的 focused Device 测试已覆盖 reset/measure 命令、CRC8、双地址、raw→lux 换算、输出保持与生命周期委托；旧 wrapper 仅是 `sensor_device_t` 兼容壳）。剩余 24 个。
  - 批 2（2026-11-03）：删 `ccs811`、`vl53l0x` 两个重复（canonical `xy_ccs811.c` / `xy_vl53l0x.c` 的 focused Device 测试已覆盖 HW_ID/MODEL_ID 身份读、APP_START/SYSRANGE_START 启动写、100/50 ms settle、STATUS data-ready 门、ALG_RESULT/RANGE_STATUS 解码与输出保持；旧 wrapper 是独立重复实现，非委托壳，但协议逐条等价）。剩余 22 个。
  - 批 3（2026-11-03）：删 `qma6100p`、`apds9960` 两个重复（`sensor_qma6100p.c` 是纯委托壳，`sensor_apds9960.c` 是 `hal_i2c_mem_read/write` 上的独立重复实现）。删除前先把**只存在于旧测试**的契约补进 canonical focused 测试：`test_qma6100p.c` 补非法地址拒绝、嵌套 transport 丢失后全部公开操作零总线拒绝且 owner 保留、active-low 中断 profile 双 data-ready 引脚映射；`test_apds9960_device.c` 补两个合法 ID、未知 ID 拒绝且不写 enable、enable/disable 寄存器档位、四通道 RGB 解码、gesture FIFO 先读 level 再读 payload、超 32 上限拒绝、level 读失败传播、NULL 参数零总线拒绝。两次变异证明断言真能红（FIFO 上限 32→128、QMA 地址校验移除），按 sha256 字节还原。剩余 20 个。
  - 批 3 收尾（2026-11-03）：删 `aht10`、`vcnl4040`、`lsm9ds1` 三个重复（canonical `test_aht10_device.c` / `test_vcnl4040_device.c` / `test_lsm9ds1_device.c` 已覆盖身份/配置/解码/失败保输出）。剩余 17 个。
  - 批 4（2026-11-04）：删 `bmp280`、`ap3216c` 两个委托壳 + 退役 `components/sensor/example/` 三份 legacy 样张（与并发 GPS 迁移合并提交 `e4f78a6a`）。剩余 15 个。
  - 批 5（2026-11-04）：删 `mpu6050`、`aht20` 两个委托壳（canonical `test_mpu6050.c` / `test_aht20.c` 已覆盖 WHO_AM_I 身份读、PWR_MGMT_1 等配置写、raw→g / raw→centi 换算、失败输出保持、NULL 守卫与 deinit/range 写失败状态保持）。剩余 13 个，其中 `icm20608` 需先补 canonical 独立 focused 测试再退役。
- **Slice C**：搬 7 个模拟量独有驱动（acs712 起，建立"模拟量驱动"canonical 模板）。
- **Slice D**：搬 gps（UART/NMEA）、sc7a20（加速度）、cmm905。
- **Slice E**：删旧框架层，收尾 `check_legacy_sensor_includes` 与碰撞 pin 全清。

## 成功标志

- `components/sensor/` 整体消失或只剩空目录占位。
- 三个碰撞守卫里 sensor 相关 pin 全部过期（当前剩 tag 1 条 `struct xy_sensor_device` + typedef 7 条）。
- `make test-unit` 全绿，测试总数不因"删旧"而下降（等价测试已迁到 canonical）。
