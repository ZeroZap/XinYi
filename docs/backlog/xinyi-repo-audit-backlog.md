# XinYi Repo Audit Backlog

Source: `2026-08-05_repo-audit.md` from Mavis / root session `mvs_2bc6f20c1a174bdbbe321961511ce29e`.
Imported by Zero on 2026-08-06.

Purpose: keep the repository analysis items visible as an execution backlog. Periodic XinYi maintenance agents should review this file, check current repository state, and identify which items are safe/actionable now.

## Operating rules

- Treat this file as backlog, not as proof that the issue still exists. Re-check current files before changing code.
- Prefer small, path-limited, verified commits.
- Do not touch `MCU/` or `third_party/` vendor/submodule trees unless the task explicitly targets them.
- A1, A6, A8, and large API changes require Eugene decision before destructive or architectural action.
- If an item is completed, update its status and add evidence: commit hash, test/build command, and short result.
- If an item is re-checked and no longer applies, mark it `obsolete`, remove it from the suggested execution order, and record the exact file/CTest evidence that made it obsolete.

## Status legend

- `open` — not yet started / still needs verification.
- `ready` — likely safe to execute now after quick re-check.
- `blocked` — needs Eugene decision or external prerequisite.
- `in-progress` — currently being worked.
- `done` — fixed/closed with evidence.
- `obsolete` — no longer applies after re-check.

## Backlog items

| ID | Priority | Area | Status | Summary | Execution notes |
|---|---:|---|---|---|---|
| A1 | P0 | Git/vendor hygiene | open | Decision recorded 2026-09-03: vendor SDKs and third-party dependencies shall use Git submodules pinned to reviewed commits. | Inventory tracked vendor bundles versus `.gitmodules`, then migrate in bounded slices. Never reset/clean/update unrelated submodules; preserve build reproducibility and provenance during each migration. |
| A2 | P1 | Kconfig/build | open | `cmake/kconfig_parser.py` is a simplified custom parser; `default "..." if ...` and duplicate config handling are unreliable. | First write focused regression tests for current parser behavior. Consider kconfiglib or explicitly documented supported subset. |
| A3 | P1 | Tests/CI | obsolete | `tests/unit/` Unity suite and root `tests/CMakeLists.txt` AT-command tests are separate; default `make test` misses AT path. | Re-checked 2026-08-07: current repo has no root `tests/CMakeLists.txt`; AT client/server are registered in `tests/unit/CMakeLists.txt` as `at_client` / `at_server` and are covered by `make test-unit`. Do not add a stale `test-at` path unless a root BUILD_TESTING tree is reintroduced. |
| A4 | P1 | Headers/clib | done | `components/clib/xy_clib/inc/xy_clib.h` uses `../` includes and omits many xy_clib headers. | Closed by `f5ec3352 fix: make xy_clib aggregate header public`: the public aggregate now uses canonical include-root headers and is included by the active `test_clib_core` CTest. Evidence recorded in that slice: `make test-unit` passed and `git diff --check` passed. |
| A5 | P2 | Docs | done | `CLAUDE.md` build commands/options are stale versus Makefile/Kconfig. | Closed by `407b071e docs: sync Claude build instructions`: `CLAUDE.md` now matches the current Makefile/AGENTS.md command model (`make`, `make test-unit`, QEMU targets, `FOTA=ON`, `BUILD_TESTS=ON`, `build/pc`, active AT CTests). Evidence recorded in that slice: `make test-unit` passed (131/131 tests) and `git diff --check` passed. |
| A6 | P2 | Architecture/Bank | open | Decision recorded 2026-09-03: root `components/` is the sole owner of reusable implementations; Bank retains board composition, configuration, and product-specific policy only. | Inventory overlap, migrate reusable Bank code into canonical components with compatibility shims, then remove parallel implementations after callers and tests move. |
| A7 | P2 | Kconfig hygiene | done | Root Kconfig `# Additional Components` block is after `endmenu`, relying on parser/file-order behavior. | Closed by this Kconfig hygiene slice: additional component symbols now live inside a proper `menu "Additional Components"` before the root `endmenu`, and the lightweight parser was verified for PC/STM32U5 generated config output. Evidence: `PYTHONPATH=. python3 tests/test_kconfig_parser.py`, direct `cmake/kconfig_parser.py --platform PC/STM32U5` probes, `make test-unit`, and `git diff --check` passed. |
| A8 | P2 | Charger architecture | open | Decision recorded 2026-09-03: retain the canonical public charger owner, migrate required Bank BQ2562x capability into it, then remove deprecated `components/charger/`. | Preserve Bank compatibility while migrating; delete deprecated build/Kconfig surface only after focused and caller coverage passes. |
| A9 | P2 | clib/Kconfig | obsolete | `XY_XY_CLIB_ENABLE` Kconfig switch defaults off but xy_clib is always added by CMake. | Re-checked 2026-08-08: root `Kconfig` does not source `components/clib/xy_clib/Kconfig`, generated PC config contains no `XY_XY_CLIB_ENABLE`/`COMPONENT_CLIB` symbols, and root `CMakeLists.txt` intentionally adds `components/clib/xy_clib` unconditionally because `xy_clib` is a core runtime dependency used by other components. Do not wire this stale nested switch as a user-disableable option unless a larger dependency model is designed first. Evidence for this backlog-sync slice: `python3 cmake/kconfig_parser.py --kconfig Kconfig --platform PC ...` plus `make test-unit` and `git diff --check` passed before commit. |
| A10 | P3 | Hygiene | done | `components/clib/xy_clib/xy_config copy.h` appears duplicated with `xy_config.h`. | Closed by `784e4b66 test: prune stale clib config copy`: current path scan finds no `*copy*` files under `components/clib/xy_clib`, and `git show --stat 784e4b66 -- components/clib/xy_clib` confirms the stale duplicate was deleted. Evidence for this backlog-sync slice: `make test-unit` and `git diff --check` passed before `docs/backlog/xinyi-repo-audit-backlog.md` was committed. |
| A11 | P3 | PID organization | open | `components/pid/` has duplicate root/inc headers and confusing src/example organization. | Needs careful include/API audit before file moves. Medium-sized cleanup. |
| A12 | P3 | Fuel gauge API | open | Decision recorded 2026-09-03: add context-based multi-instance APIs while retaining singleton wrappers for compatibility. | Migrate one gauge family at a time with test-first instance isolation; keep existing public wrappers delegating to a default context until callers can migrate. |
| A13 | P3 | README docs | done | README repeats device framework description in multiple sections. | Closed by this README cleanup slice: duplicate `components/device` narrative sections (`### 5. Device Component` and `### 11. Device Framework (New)`) were removed while keeping the canonical earlier `### 3. Device Framework Component` plus the Additional Components table reference. Evidence: `make test-unit` and `git diff --check` passed before commit. |
| A14 | P4 | PC HAL/log validation | done | Need confirm `xy_log` PC fallback and `hal_*` PC no-op/no-op-with-log behavior. | Closed by this Trace/PC HAL validation slice: existing `hal_pc` and `trace_component` CTests already guard PC HAL clock/GPIO/UART/I2C/SPI no-op receive behavior and trace output/dynamic-level contracts; this slice added a defensive `xy_log_raw(NULL, len)` guard plus focused regression coverage. Evidence: focused `test_trace` build + `ctest -R '^trace_component$'`, full `make test-unit` (132/132), and `git diff --check` passed before commit. |
| A15 | P4 | Toolchain UX | open | Cross-toolchain paths default to local Linux hardcoded paths. | Improve only after checking current CMake cache behavior and supported developer workflows. |

## Suggested execution order

1. Safe low-risk execution: A14 is closed. A7, A9, and A13 are closed/obsolete.
2. Parser/build correctness: A2, after focused regression tests are in place.
3. Documentation sync: A5 and A10 are closed; use AGENTS.md/Makefile as command truth if future docs drift appears.
4. Architecture execution: A1, A6, A8, and A12 are approved for bounded migration; A15 still needs re-check before invasive changes.

## Periodic review checklist

When reviewing this backlog:

1. Check current `git status --short` and avoid unrelated dirty files.
2. Re-read the specific files for the candidate item; do not assume the 2026-08-05 report is still current.
3. Classify each candidate as `ready`, `blocked`, `done`, or `obsolete`.
4. If executing, pick one small slice, run the relevant build/test command, and commit only touched paths.
5. Update this backlog with evidence after completion.
