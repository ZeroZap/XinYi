# 2026-10-29 — Pandora FOTA confirm 分支实板闭环（install → attempt → request → confirm → ack）

## 为什么这条一直没拿到

`pandora_fota_boot_flow.c` 的 confirm 分支从 S6-131 owner 化以来**只有 Host 契约**，2026-10-28/29 的三轮实板尝试都停在
`ATTEMPT_COMMITTED` 或 `ROLLBACK_REQUIRED`。原因不是设备、也不是采集，而是**夹具本身把分支藏起来了**：

1. confirmation-capable application（`PANDORA_FOTA_APPLICATION`）在打印 `PANDORA_FOTA_CONFIRM_ACKNOWLEDGED` 后
   **继续跑压力链**，而压力链第一件事就是把自身镜像重新写入 W25Q128 候选区并 `MCU_RESET_STAGED` → 又一次引导。
2. bootloader 的 attempt 预算只有 3 次（`PANDORA_FOTA_FLOW_MAX_ATTEMPTS`），于是每次 confirm 之后必然跟着一次
   install 周期 → 第三次 attempt 直接 `ROLLBACK_REQUIRED`。任何窗口都是"末尾 ROLLBACK"，而
   `validate_fota_bootloader_capture.py --expect confirm` 要求**最后一帧**是 CONFIRMED，所以永远判不过。
3. 2026-10-29 03:16/03:33 的两次尝试用的还是**修复前**的应用镜像（`e46d5f03`/`4458b9f1`），它在信号量检查处
   就停机（S6-149 归因），连 `W25Q128_MCU_RESET_STAGED` 都到不了——RTC slot 0/1 全程为 0 正是这个原因。

**修法**：让夹具在 ACK 之后**按设计停机**（`PANDORA_FOTA_CONFIRM_FIXTURE_DONE` + `__disable_irq()` 自旋，
同时把停工点写进 `xy_image_last_marker`）。链路的终态于是成为镜像的属性，而不是"捕获窗口什么时候关"的竞态。
`tests/unit/fota/test_pandora_fota_bootloader_layout.py` 钉住该停机点，并且在修复前的源上 RED。

## 校验器同时被实板抓出一个缺陷

第一次判读得到 `chain=INVALID`，失败项是 `unknown boot marker 'BOOTLOADER_COMMIT 5bf5a14f…'`：
**捕获进程启动时 `git rev-parse HEAD` 已被并发会话推进到 `5bf5a14f`，而镜像内嵌身份是 `56ee6030`**。
validator 拒绝把捕获绑到错误 commit 是正确行为（身份绑定生效），改用镜像内嵌身份 `56ee6030` 重判即通过。

重判后又暴露第二个缺陷：`chain=ATTEMPT`——因为旧判据是"窗口里**存在** attempt marker 就标 ATTEMPT"，
而真实的 confirm 必然是 `ATTEMPT → ATTEMPT → CONFIRM` 三帧。现在 `chain` 由**做决定的那一帧**推导
（terminal=confirm ⇒ `CONFIRM`），并加了两条回归用例（attempts→confirm ⇒ CONFIRM；attempts 无 confirm ⇒ ATTEMPT）。
45 例 Host 测试：修复版 OK，回退版 1 例 FAIL。

## 实板链路（Pandora STM32L475，CP2102 `/dev/ttyUSB0`，115200 8N1）

镜像身份：源码 commit `56ee60301d53e7a1c75b787c6caea7cc8e3dffeb`（bootloader 与 application 都在 UART 上自报该值）。

| 工件 | 大小 | SHA-256 |
|---|---|---|
| `pandora_stm32l475_fota_app.bin` | 43768 B | `f8533386…26e89c` |
| candidate envelope（image v2，crc32 `0x1c8c8519`，load `0x08008000`） | 43800 B | `9363a6a5…1b7206` |
| `pandora_stm32l475_fota_bootloader.bin` | 15544 B | `0a884c563c56773fc8a7eb9d7b09ec6b2880fc5f02d9afb50a06e1f52dabfe84` |
| `pandora_stm32l475_fota_candidate_programmer.bin` | 54228 B | `2dfc8f45…f66d5aa` |

写入全部走快照门（`tools/pandora_hil_program.py`，每个写入前都先与当时快照逐字节比对）：

- programmer part1 `0x08000000` 32768 B → `span_identical=true erased_tail=0 unexpected_changes=0`
- programmer part2 `0x08008000` 21460 B → `span_identical=true erased_tail=1064 unexpected_changes=0`
- bootloader `0x08000000` 15544 B → `span_identical=true erased_tail=840 unexpected_changes=0`

programmer 运行（显式 `st-flash reset` 启动；**两段写完后必须再显式复位一次**，S6-142 的结论复现）：
211 B、1 个完整帧、`PROGRAMMER_CAPTURE_REVIEW_CANDIDATE`，两个 commit 身份都等于 `56ee6030…`
（捕获 sha256 `072cf5ea…d45967`）。

引导链（一次连续 90 s 捕获，2892 B / 93 行应用输出，捕获 sha256 `781af52a…f2bf78`）：

```
boot 1  READY / BOOTLOADER_COMMIT 56ee6030… / CANDIDATE_INSTALLED / ATTEMPT_COMMITTED / JUMP_APP
        app: … PANDORA_W25Q128_FOTA_FULL_IMAGE_OK / PANDORA_FOTA_BOOTABLE_CANDIDATE_VALIDATED
             / PANDORA_W25Q128_MCU_RESET_STAGED  → 软件复位
boot 2  READY / COMMIT / CANDIDATE_ALREADY_INSTALLED / ATTEMPT_COMMITTED / JUMP_APP
        app: PANDORA_FOTA_CONFIRM_REQUESTED → 软件复位
boot 3  READY / COMMIT / CANDIDATE_ALREADY_INSTALLED / CANDIDATE_CONFIRMED / JUMP_APP
        app: PANDORA_FOTA_CONFIRM_ACKNOWLEDGED / PANDORA_FOTA_CONFIRM_FIXTURE_DONE → 停机
```

判定：`BOOTLOADER_CHAIN_REVIEW_CANDIDATE chain=CONFIRM terminal=jump boots=3 attempts=2 confirms=1`。

**journal（`0x0807E000` 4096 B 实读 + CRC 解码）**：`JOURNAL_VALID active_slot=0`，
slot 0 `state=CONFIRMED generation=5 attempts=0 image_crc32=0x1c8c8519 image_size=43768 record_crc_ok=True`，
slot 1 `INSTALLED generation=4 attempts=2`（同一 candidate）。即**确认是持久状态**，并且绑定到本轮的候选
（crc/size 一致），不是只打了一行 marker。

**停工点证明**（RAM 回读，应用 ELF 的符号地址）：`xy_image_failed=0`、`xy_image_failed_line=0`、
`xy_image_last_marker=0x080125e4` → 该地址处字符串 `"PANDORA_FOTA_CONFIRM_FIXTURE_DONE\r\n"`。
即这次停机是夹具的**设计终态**，不是 `fail()` 或栈溢出那种"静默停机"。

**收尾**：`restore --snapshot s0` → `restore_ok regions=4`；`verify` → `verify_ok regions=5`
（bootloader `762c4a75…`、application `d35e2e9a…`、journal `f47a8ec3…`、board_reserved `118f9c6f…`、
rtc_backup `38723a2e…` 全部逐字节相同）。

## 边界

- 单板、单窗口、单个 candidate（image v2）；不构成 W25Q128 擦写耐久、掉电/半写中断、RTC backup 掉电保持证据。
- 不构成 Secure FOTA：envelope 无签名，`source_commit` 只做人审绑定。
- 不主张被 jump 之后压力链的 runtime 结论；本轮应用在 ACK 处**按设计停机**，OSAL 链只作为到达 W25Q128 段的路径。
  该链的 runtime 结论仍在 S6-149（`2026-10-29-runtime-timeline`）。
- `chain=CONFIRM` 证明决策与 journal 状态，不证明 bootloader 之后的 vector handoff 正确性（那一跳由 `JUMP_APP` +
  应用自身打印 `READY`/`FIRMWARE_COMMIT` 间接支持）。
- 中间快照 `s1`/`s2` 只保留 `manifest.json` 与 `program.json`（region 二进制已删除），恢复源是 `s0`。
- **W25Q128 `0xF00000` 现在留着本轮写入的 candidate**（43800 B，source commit `56ee6030…`）：快照工具只覆盖 MCU
  Flash 四段 + RTC backup，不覆盖片外 SPI flash，因此 `restore` 不会把它清掉。board 复位后运行的是原驻
  `762c4a75…` bootloader（不扫 W25Q128 候选），该候选处于惰性状态；下次用本仓库 bootloader 时会先装它。
