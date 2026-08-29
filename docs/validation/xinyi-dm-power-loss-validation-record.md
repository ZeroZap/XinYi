# XinYi DM 掉电与布局恢复验证记录

**建立日期**：2026-08-28

**当前状态**：`HOST_INTERRUPTION_GUARDED`

**范围**：DM NVM/Flash 的 erase/write interruption、metadata/CRC corruption、restart recovery 与 layout migration

**证据边界**：Host memory fault injection does not constitute real NOR/Flash power-loss, endurance, timing, or board evidence.

## 1. 当前结论

本记录当前只包含 Host 内存镜像上的 restart/corruption contract。`dm_nvm` focused test 会写入同一 key 的两个完整版本，重新初始化后要求读取最后一个完整版本；随后在 append 尾部注入可识别但 checksum 不完整的 torn record，再次初始化时必须保留最后一个完整值。NVM 现支持 caller-owned storage ops；Host fault backend 对 header 的每个 byte boundary 与 payload 的每个 byte boundary 模拟“部分写入后返回失败”，`xy_nvm_set()` 必须传播错误，重启后保留上一完整值，并能跳过 torn append 后成功重试。format erase 也会在整个 256-byte Host 区域的每个 byte boundary 注入部分擦除后失败；API 必须传播错误，重启只允许看到最后一个仍完整的值或空存储，并可在后续完整 erase 后恢复为空且继续使用。布局迁移契约同时固定为：现版本读取 legacy `0xAA55AA55` additive-checksum record，所有新 append 使用 `0xAA55AA56` CRC-8 record；同 key 的新格式记录在重启后覆盖旧格式值，避免升级时强制擦除已有数据。current record 的 magic、key、enable、length、CRC 与 payload 逐 byte corruption 均被拒绝并回退上一完整值；checksum scan 通过 storage ops 读取，不要求逻辑 `flash_base` 可被 Host 直接解引用。

Current result: `HOST_INTERRUPTION_GUARDED`

尚无真实 Flash 擦写、断电时刻扫描、磨损、写粒度、ECC、cache flush 或板级供电波形证据。因此不得标记 `BOARD_POWER_LOSS_PASSED`。

## 2. 运行身份

| Field | Value |
|---|---|
| Git SHA | recorded in `docs/plans/SPRINT_TRACKER.md` |
| Host command | `ctest --test-dir build/tests/unit -R '^dm_nvm$' --output-on-failure` |
| Full Host gate | `make test-unit`: 188/188 passed |
| Storage backend | Host byte-array simulation |
| Board / Flash part | pending |
| Raw board logs / captures | pending |

## 3. 场景矩阵

| ID | Scenario | Host contract | Board evidence |
|---|---|---|---|
| DM-PL-01 | restart after complete append | newest complete value survives re-init | pending |
| DM-PL-02 | torn header/payload append | incomplete checksum record is ignored; previous complete value remains | pending |
| DM-PL-03 | erase interruption | exhaustive 256-byte Host partial-erase boundary sweep；错误传播、可重启扫描与完整 erase retry guarded | pending |
| DM-PL-04 | write interruption at every supported program granule | header/payload 的 exhaustive byte-boundary partial-write sweep guarded；目标 Flash program granule pending | pending |
| DM-PL-05 | CRC/metadata corruption | current V2 magic/key/enable/length/CRC/payload protected-byte corruption sweep guarded；legacy additive checksum remains readable | pending |
| DM-PL-06 | layout migration | legacy magic 可读、new magic 只写、跨格式 newest-value restart contract guarded | pending |
| DM-PL-07 | repeated reset/endurance | pending | pending |

## 4. Board 验收要求

真实 B2 记录必须固定 board、Flash 型号、供电、电压阈值、固件 SHA、编译参数与写粒度，并在 erase/header/data/commit 各边界切断电源。每次重启需保存原始串口日志、供电波形和 Flash dump，明确允许丢失的是当前未提交更新，而不是最后一个完整记录。

只有真实板卡与目标 Flash 在 DM-PL-03～DM-PL-07 中产生可审查的原始日志/capture 后，才可选择 `BOARD_POWER_LOSS_PASSED`。Host、PC build、QEMU 或 compile-only 结果均不得替代。
