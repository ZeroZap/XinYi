# DM 数据管理组件

DM（Data Management）聚合 XinYi 中与持久化、结构化数据和轻量文件/键值存储相关的实现。当前组件已经具备 root CMake/Kconfig 入口和主线 `tests/unit/dm/` host CTest 护栏；本文档只记录当前已由源码或测试证明的入口，不把历史规划材料当作已完成 API。

## 当前状态

| 子模块 | 主要路径 | 当前状态 | Host CTest |
| --- | --- | --- | --- |
| Base64 | `xy_base64/xy_base64.{h,c}` | RFC 4648 public helper，覆盖基础向量和二进制 roundtrip | `dm_base64` |
| TLV | `xy_tlv/xy_tlv.{h,c}` | TLV encode/decode、iterator/search/count/checksum/stats contract | `dm_tlv` |
| NVM | `xy_nvm/inc/xy_nvm.h`, `xy_nvm/src/xy_nvm.c` | host-simulated KV/NVM lifecycle、set/get/delete/stats/format | `dm_nvm` |
| Factory | `factory/xy_factory.{h,c}` | public factory entry/CRC/status/guard contract | `dm_factory` |
| FEE | `fee/inc/xy_fee.h`, `fee/src/xy_fee.c` | flash-emulated EEPROM lifecycle、read/write/format/guard contract | `dm_fee` |
| coreJSON | `coreJSON/core_json.{h,c}` | imported-but-owned JSON parser guard coverage | `dm_corejson` |
| FS / JSON abstraction | `inc/xy_fs.h`, `src/xy_fs.c`, `inc/xy_json.h`, `src/xy_json.c` | included in the root `xy_dm` library; no dedicated focused CTest yet | backlog |
| FlashDB / NOR glue | `xy_flash/`, `xy_norflash/` | optional root-build glue controlled by generated config; board/backend validation remains separate | backlog / hardware-driven |

## 构建与配置

Root `Kconfig` currently exposes:

```text
config COMPONENT_DM
    bool "Device Management"
    default y
```

The root build auto-discovers `components/dm/CMakeLists.txt` and builds `xy_dm` when the generated CMake variable `XY_COMPONENT_DM` is enabled. The component library currently includes the FS and JSON abstraction sources by default and conditionally adds NOR/FlashDB glue when the generated config enables those features.

Useful probes:

```bash
cmake --build build/pc --target xy_dm -j$(nproc)
make test-unit
ctest --test-dir build/tests/unit -R '^dm_' --output-on-failure
```

`tests/unit/CMakeLists.txt` also links several DM submodules directly into focused tests. Passing focused CTests proves those public contracts; it does not prove board flash endurance, power-loss behavior, NOR timing, or FlashDB hardware integration.

## Public include guide

Prefer including the narrow submodule header that owns the API being used:

```c
#include "xy_base64.h"          /* components/dm/xy_base64 */
#include "xy_tlv.h"             /* components/dm/xy_tlv */
#include "xy_nvm.h"             /* components/dm/xy_nvm/inc */
#include "xy_factory.h"         /* components/dm/factory */
#include "xy_fee.h"             /* components/dm/fee/inc */
#include "core_json.h"          /* components/dm/coreJSON */
#include "xy_fs.h"              /* components/dm/inc */
#include "xy_json.h"            /* components/dm/inc */
```

Do not include stale historical migration notes as API headers. Files such as `DM_INTEGRATION_COMPLETE.md`, `DM_INTEGRATION_PLAN.md`, and `DM_OPTIMIZATION_PLAN.md` are planning/history documents only.

## Verification boundary

Host CTests cover deterministic software contracts and pointer/parameter guards. They intentionally do not claim:

- flash wear-leveling endurance or brown-out recovery on a real board;
- real NOR/FlashDB timing or erase/write failure behavior;
- filesystem backend interoperability beyond the currently compiled abstraction layer;
- migration safety for old project-local code that still expects deleted EEPROM/FEE layouts.

Board-specific flash/NOR/FlashDB work should be introduced as a separate proposal plus focused host seam before any hardware backend is enabled by default.

## Backlog

1. Add a small `dm_fs_json` host CTest if FS/JSON abstraction APIs become active public dependencies.
2. Write a hardware validation record template before claiming NOR/FlashDB board behavior.
3. Reconcile old planning documents (`DM_INTEGRATION_*`, `DM_OPTIMIZATION_PLAN.md`) with current source layout in a docs-only cleanup slice.
4. Avoid large directory reshuffles (`src/` consolidation, legacy note deletion) without a proposal and path-limited migration test.
