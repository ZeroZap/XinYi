# Pandora 板载传感器 owner 运行时：AP3216C 与 ICM20608（2026-10-29）

## 范围

S6-116 把 U8 AP3216C 与 U6 ICM20608 的板上 owner 从 legacy `sensor_*` wrapper 换成 canonical
`xy_ap3216c` / `xy_icm20608` 驱动（`pandora_ap3216c_owner.*` / `pandora_icm20608_owner.*`），当时
记录明确写着**「板上 owner 切换本身仍无 runtime 证据」**。2026-10-28 的 AHT10 记录把同类缺口在
U7 上补上了，AP3216C 与 ICM20608 两个镜像仍然只有 source/Host/compile 证据。

本记录补的是那一格：把两个 owner 镜像真烧进 Pandora，按各自 validator 做机器判定，全部以
字节级还原收尾。

- 源码 commit：`7ecc522c39f930cc59fc161ce1bdf8efba07b010`（两个镜像内嵌的 `FIRMWARE_COMMIT`
  与 `git rev-parse HEAD` 一致，validator 按内嵌身份判定）
- 板卡：STM32L475VE，ST-Link V2.1 `V2J24S11` / `066AFF313933554D43244015`，chipid `0x415`
- UART：CP2102 `/dev/ttyUSB0`，115200 8N1（ST-Link 的 VCP 未参与任何采集）

## 产物

| 路径 | 内容 |
| --- | --- |
| `ap3216c-uart.log` | AP3216C 采样窗口，4040 B，SHA-256 `6879a68dfb21be7d0c43b68ced2c98d8ce9b163ee784aad42b263e52e2114599` |
| `ap3216c-capture-judgement.json` | 判定 `AP3216C_HW_I2C3_B1` |
| `ap3216c-capture.json` | 采集记录（设备、窗口、字节数、sha256、non-ASCII 计数） |
| `icm20608-uart.log` | ICM20608 采样窗口，6168 B，SHA-256 `9eaa5e7c585eb55794b5e64793f3d3987acd1b36bf699471bae0019653249864` |
| `icm20608-capture-judgement.json` | 判定 `ICM20608_B1_REVIEW_CANDIDATE` |
| `icm20608-capture.json` | 同上，采集记录 |
| `negative-*.json` | 三份**反证**：判定确实会拒绝（见下文） |
| `pandora-state-s0/` | 运行前快照（含 `program.json`：AP3216C 镜像的 gated 写入日志） |
| `pandora-state-s1/` | AP3216C 运行后的快照（含 ICM20608 镜像的 `program.json`） |

镜像（同一 build 目录 `build/pandora-owner-hil`）：

| 镜像 | 大小 | SHA-256 | ELF 符号 |
| --- | --- | --- | --- |
| `pandora_stm32l475_ap3216c.bin` | 11324 B | `e98b781dc79747f0b54ea57b7e2e4e9f27e7d598c4312b664cd7aebbd3fac5b5` | `pandora_ap3216c_owner_{init,read_reg,read_raw,sample}` + `xy_ap3216c_init`，**零** `sensor_*` 符号 |
| `pandora_stm32l475_icm20608.bin` | 10412 B | `be58fb11298ae83de1c36881e36f1101b89c217e6211bde3b0ea41ab107a7bec` | `pandora_icm20608_owner_{init,read_reg,read_raw,sample}` + `xy_icm20608_init_i2c`，**零** `sensor_*` 符号 |

两个镜像由 `make`/CMake 的 `STM32L4_BOARD=pandora_stm32l475` 目标产出；`arm-none-eabi-nm` 的
输出即上表右列（`sensor_` 前缀的 grep 结果为空），也就是「板上跑的是 canonical owner」这件事在
镜像层面先成立，再由 runtime 采集确认它真的跑起来。

## 过程

1. `tools/pandora_hil_state.py snapshot --out pandora-state-s0`：bootloader `762c4a75…`、
   application `d35e2e9a…`、`fota_metadata` `f47a8ec3…`、`board_reserved` `118f9c6f…`、RTC backup
   全零 —— 与 2026-10-28 AHT10 记录的基态**逐项相同**，说明板子自那以后没有被悄悄改动。
2. `tools/pandora_hil_program.py run --image …ap3216c.bin --address 0x08000000 --snapshot
   pandora-state-s0 --yes`：`span_identical=true`、`erased_tail=964`、`unexpected_changes=0`。
   写 `0x08000000` 而不是 `0x08008000`，是 AHT10 记录留下的规矩：这些验证镜像按
   `STM32L475VETX_FLASH.ld` 链接在 `0x08000000`，写进应用槽会被常驻 bootloader 用 W25Q128
   候选覆盖掉。
3. `boards/pandora_stm32l475/capture_window.py --seconds 6 --reset-command "st-flash reset"`：
   **先开串口并 flush，再复位**，因此 banner 不会丢（这正是两个 validator 只做 analyze、没有
   capture 能力的原因）。
4. `validate_ap3216c_hw_i2c_capture.py … --firmware-commit 7ecc522c…` → `AP3216C_HW_I2C3_B1`。
5. 对 ICM20608 重复 2–4（快照 `s1` 作为门禁基线）→ `ICM20608_B1_REVIEW_CANDIDATE`。
6. `verify --snapshot pandora-state-s1`：只有 `bootloader` 区域变了（就是两次写入本身），
   `application` / `fota_metadata` / `board_reserved` / `rtc_backup` **全部 identical** —— 两个
   验证镜像没有碰 FOTA 状态，也没有碰 RTC 握手槽。
7. `restore --snapshot pandora-state-s0 --yes` 后 `verify` → `verify_ok regions=5`。

## 结果

**AP3216C**（硬件 I2C3，SCL=PC0 / SDA=PC1，7 位地址 `0x1E`）：

- 有序 marker 全中：`PANDORA AP3216C SENSOR READY` → `FIRMWARE_COMMIT 7ecc522c…` →
  `AP3216C_BUS=HW_I2C3 SCL=PC0 SDA=PC1 ADDR=0x1E` → `AP3216C_HW_I2C_READY` →
  `AP3216C_NACK_OBSERVED ADDR=0x7F` → `AP3216C_NACK_RECOVERED ADDR=0x1E` →
  `AP3216C_CONFIG=0x03 MODE=ALS_PS` → `AP3216C_DIAG INT=0x00 ALS_CONF=0x00 PS_CONF=0x05 PS_LED=0x13`
- 53 条样本、41 条互不相同、无畸形行、无 error marker、non-ASCII 0 字节
- `PS_raw` 135..163（中位 151）、`IR_raw` 0..512（中位 257），全部落在驱动公开范围内
- `ALS_lux` 全程 0：窗口内没有光照激励（无人值守，未做任何遮蔽/照射），因此**不主张响应**；
  这一格的 B1 只覆盖「canonical owner 在板上把 init/诊断寄存器/三通道采样跑通」
- `0x7F` 的 NACK 是镜像自身的反证步骤：先证明总线会如实报告无应答，再证明 `0x1E` 能被正常寻址

**ICM20608**（同一 I2C3，地址 `0x68`）：

- 有序 marker 全中，`ICM20608_WHO_AM_I_RAW=174`（`0xAE`）与 `ICM20608_ADDR=0x68 WHO_AM_I=0xAE`
  一致；`ICM20608_INIT_PATH=ACCEL_ONLY RESET_COUNT=1`
- 43 条样本 + 44 行寄存器块 + 43 行原始块（validator 要求 raw 行与 sample 行对齐）
- 加速度合量 **1001.3–1004.3 mg**（板子静止，重力轴），陀螺 **|dps| ≤ 1**，无 saturate 标记
- 总线扫描 `I2C3_ACK=0x10 / 0x1E / 0x68`，`count=3`：`0x1E` 是同一块板上的 AP3216C，`0x10`
  本记录**不做归属**（未在任何 owner 的范围内，也不影响 ICM 的判定）

## 反证：这三个判定不是装饰

| 文件 | 输入 | 输出 | 拒绝理由 |
| --- | --- | --- | --- |
| `negative-icm-as-ap3216c.json` | ICM20608 的真实捕获 | `AP3216C_HW_I2C3_VALIDATION_FAILED`（8 条失败） | `missing or out-of-order marker: PANDORA AP3216C SENSOR READY` 等 |
| `negative-ap3216c-as-icm.json` | AP3216C 的真实捕获 | `CONTENT_MISMATCH`（7 条失败） | `missing or out-of-order marker: PANDORA ICM20608 SENSOR READY` 等 |
| `negative-wrong-commit.json` | AP3216C 的真实捕获 + 错的 commit | `AP3216C_HW_I2C3_VALIDATION_FAILED`（1 条失败） | `missing or out-of-order marker: FIRMWARE_COMMIT 0718c5bb…` |

第三条是身份绑定那一格：换一个 commit 去判同一份字节，判定就红，说明「这份捕获来自哪个镜像」
不是靠旁白，而是靠字节里的 `FIRMWARE_COMMIT`。

## 新增的采集工具

`boards/pandora_stm32l475/capture_window.py`：

- 这两个 validator 只 analyze，采集逻辑此前在 `capture_uart.py` / `validate_aht10_capture.py` /
  `validate_fota_bootloader_capture.py` 里各长了一份，且 `capture_uart.py` 还焊死在 smoke 镜像的
  banner 上。本 slice 不再加第四份判定逻辑，只加一个**不做判定**的通用采集窗口：开串口 → flush →
  （可选）复位 → 读 N 秒 → 落盘原始字节 + 元数据（sha256、字节数、non-ASCII、时间戳）。
- 失败面向传输而非内容：`0` 表示「采到了字节」，`2` 表示窗口内没有字节，`3` 表示串口打不开；
  内容好坏交给各自 validator。`RESET_FAILED` 也算失败，不允许「复位没跑但当作成功」。
- 覆盖在 `tests/unit/sensor/test_pandora_capture_window.py`（CTest `pandora_capture_window`，
  6 例，pty 对）：**reset 之前先把端口 flush**（复位前残留字节不得进入窗口）、open 先于 reset、
  无数据 → exit 2 且仍写证据记录、设备不存在 → exit 3 且**不写**捕获文件、复位命令失败 →
  `RESET_FAILED`、成功路径的 sha256 与文件字节一致。

## 边界

- 一块板、两个各 6 秒的有界窗口、两次 gated 写入。**不主张**长时运行、soak、温漂、精度或标定；
  ICM 的 1001.3–1004.3 mg 是「静止房间里的重力读数」，不是校准过的参考。
- AP3216C **无受控激励**（无遮挡、无光照阶跃），因此不主张响应时间、照度绝对精度或 ALS 量程
  行为；ALS 恒 0 只说明窗口内没有光。
- 只证明这两个 owner 镜像的 init / 诊断 / 采样链在板上跑通，以及它们没有副作用到 FOTA/RTC；
  **不**证明 I2C3 时序裕量、上拉、总线恢复，也**不**证明 U6/U8 的长期可靠性。
- 常驻 bootloader 被验证镜像临时替换，随后**逐字节**还原（`verify_ok regions=5`）；这是 flash
  字节级陈述，不是关于本轮 bootloader 自身行为的证据。
- 与前一条记录的关系：S6-116 之后的「板上 owner 切换无 runtime 证据」在 AP3216C 与 ICM20608
  这两格上已关闭；AHT10 那格由 `2026-10-28-aht10-owner` 关闭。
