# 2026-10-29 — W25Q128 candidate programmer: judged stream + W25Q128 candidate, confirm 分支仍未上板

## 做了什么

1. **新校验器** `boards/pandora_stm32l475/validate_fota_candidate_programmer_capture.py`：`fota_candidate_programmer_main.c`
   的 marker 流此前**没有任何仓库内判据**（bootloader validator 盯的是另一条流），一个打印了 `PROGRAM_ERROR`
   的 programmer 只能靠眼睛判“看着没问题”。契约按 boot frame 建模：`READY → PROGRAMMER_COMMIT <sha> →
   CANDIDATE_SOURCE_COMMIT <sha> → PROGRAMMED_VERIFIED`；**最后一帧必须完整**（截断的帧可能是被打断的写入）、
   两个 commit 身份分别绑定 `--firmware-commit` / `--candidate-source-commit`、任何 `PROGRAM_ERROR` /
   `VERIFY_ERROR` / 未知 `PANDORA_FOTA_CANDIDATE_*` / 帧内夹入其它输出一律 fail-closed；`NO_DATA_TIMEOUT`
   不算通过。Host 19 例、7 种源码变异全部被拦、CTest `pandora_fota_candidate_programmer_capture_validator`。
2. **实板**：候选 envelope = `pandora_stm32l475_fota_app.bin`（42576 B，v2，source commit `800154be…`），
   programmer 镜像 53044 B 链接在 `0x08000000`（`FLASH.ld`），**比 bootloader 段（32 KiB）大**，
   因此 `pandora_hil_program.py` 的一区域规则拒绝整段写入；改为按区域切成两段分别门控写入
   （`prog-part1` 0x08000000/32768 B、`prog-part2` 0x08008000/20276 B），两段各自 `span_identical=true`、
   `unexpected_changes=0`（part2 `erased_tail=200`），合起来与整镜像逐字节等价。
3. **programmer 实际运行**（显式 `st-flash reset` 启动，见下）：211 B、1 个完整帧、
   `PROGRAMMER_CAPTURE_REVIEW_CANDIDATE`，sha256 `a5673302…98cf`；`PROGRAMMED_VERIFIED` 表示每个
   256 B chunk 写完都回读相等。

## 两个工具/流程事实（本轮实测暴露）

- **`st-flash write` 自身那次复位打不断两段式 trampoline**：把 part2 写进 `0x08008000` 时，
  写入触发的复位发生在 part2 尚未落盘之前，programmer 跑到一半就死在旧字节上，CP2102 一个字节都没收到
  （首轮 `NO_DATA_TIMEOUT`）。正确做法是等两段都写完后**再显式复位**一次。
- **programmer 镜像跨区域是常态**，不是特例：任何带 candidate blob 的一次性 programmer 都会超过 32 KiB
  bootloader 段。要么按区域分段门控写入（本轮做法），要么给写入工具加显式的跨区域承认——不能靠退回裸
  `st-flash write`。

## confirm 分支：本轮**仍未有结论**（不是通过，也不是失败）

链路按设计搭好：snapshot `s0` → 分段写 programmer → programmer 把 v2 candidate 写进 W25Q128 →
snapshot `s2` → 门控写 15548 B bootloader（`38108391…`，内嵌 `800154be…`）→ 330 s 窗口。

捕获结果 `BOOTLOADER_CHAIN_VALIDATION_FAILED`（`chain=INVALID`、`boots=2`、`attempts=1`、654 B）：
第 1 帧完整（`CANDIDATE_INSTALLED`→`ATTEMPT_COMMITTED`→`JUMP_APP`），随后被 jump 的 FOTA application 打印
`READY` / `FIRMWARE_COMMIT` / `OSAL_STRESS_READY` + 15 行 task marker 后**再无输出**，随后又出现一帧只有
`READY`+`BOOTLOADER_COMMIT` 的残缺引导；窗口内 journal 前进到 `attempts=2`。RTC slot 0/1 全程为 0，
即 application 从未走到写 `W25Q128_MCU_RESET_MAGIC` 或 `CONFIRM_REQUEST_MAGIC` 的那两行。

**为什么不能据此判应用缺陷**：本轮全程在共享工作树上跑，另一个并发会话在
`00:22:19` 提交了 `149ccfa7`（ICM20608 capture validator，必然要烧录/复位这块同一块 Pandora），
时间正落在 330 s 捕获窗口内；`tools/pandora_hil_state.py verify` 事后也显示四段 flash 已被别人恢复回
原始镜像。因此“应用打印到一半停住”“出现第二次引导”这些观测**无法归属**到我的镜像，也无法排除是
并发烧录/复位插进来造成的。按证据纪律，本轮不主张任何应用 runtime 结论。

顺带拿到的一条**有效**帧：窗口之后的一次普通复位（journal 已 `attempts=2`）得到
`ALREADY_INSTALLED`→`ROLLBACK_REQUIRED`、**无** `JUMP_APP`，`validate_fota_bootloader_capture.py
--expect rollback` 判 `BOOTLOADER_CHAIN_REVIEW_CANDIDATE chain=ROLLBACK terminal=halt_rollback`
（`rollback-frame.log/json`）；journal 两槽 `ROLLED_BACK gen5 attempts=3`、`INSTALLED gen4 attempts=2`
（`journal-after-attempts.json`）。即 owner 化后的 flow owner 在 HEAD 上**仍然**正确地把耗尽 attempt 变成
halt（不 jump）——这条与 S6-138 的结论一致，本轮只是再复现一次。

## 板子收尾

`restore --snapshot s0` → `verify_ok regions=5`（含 RTC backup）。bootloader 回到原驻 `762c4a75…`、
application `d35e2e9a…`、journal 空白。**W25Q128 `0xF00000` 现在留着本轮写入的 v2 candidate**：
快照工具不覆盖片外 SPI flash，而恢复后的原驻 bootloader 不扫 W25Q128 候选，故该候选处于惰性状态，
但下一次用本仓库 bootloader 时会先装它——重跑前应知道这一点。

## 结论 / 边界

- 升级：W25Q128 candidate programmer 的 marker 流现在有机器判据（Host 19 例 + 7 变异）；
  实板一次完整、身份绑定到 `800154be…` 的运行被判通过（211 B）。
- **未**升级：confirm 分支实板证据。它与本轮并发占用强相关，必须在独占板子时重跑。
- 不构成：W25Q128 擦写耐久、掉电/半写、programmer 之外的升级路径、Secure FOTA。
