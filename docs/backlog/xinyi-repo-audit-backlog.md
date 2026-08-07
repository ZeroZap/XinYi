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
| A1 | P0 | Git/vendor hygiene | blocked | Untracked ST CubeMX SDK bundles and modified submodules may be mixed into the worktree. | Re-check `git status --short --ignored`, `.gitignore`, and submodule diffs. Do not clean/reset/update submodules without Eugene confirmation. |
| A2 | P1 | Kconfig/build | open | `cmake/kconfig_parser.py` is a simplified custom parser; `default "..." if ...` and duplicate config handling are unreliable. | First write focused regression tests for current parser behavior. Consider kconfiglib or explicitly documented supported subset. |
| A3 | P1 | Tests/CI | obsolete | `tests/unit/` Unity suite and root `tests/CMakeLists.txt` AT-command tests are separate; default `make test` misses AT path. | Re-checked 2026-08-07: current repo has no root `tests/CMakeLists.txt`; AT client/server are registered in `tests/unit/CMakeLists.txt` as `at_client` / `at_server` and are covered by `make test-unit`. Do not add a stale `test-at` path unless a root BUILD_TESTING tree is reintroduced. |
| A4 | P1 | Headers/clib | done | `components/clib/xy_clib/inc/xy_clib.h` uses `../` includes and omits many xy_clib headers. | Closed by `f5ec3352 fix: make xy_clib aggregate header public`: the public aggregate now uses canonical include-root headers and is included by the active `test_clib_core` CTest. Evidence recorded in that slice: `make test-unit` passed and `git diff --check` passed. |
| A5 | P2 | Docs | done | `CLAUDE.md` build commands/options are stale versus Makefile/Kconfig. | Closed by `407b071e docs: sync Claude build instructions`: `CLAUDE.md` now matches the current Makefile/AGENTS.md command model (`make`, `make test-unit`, QEMU targets, `FOTA=ON`, `BUILD_TESTS=ON`, `build/pc`, active AT CTests). Evidence recorded in that slice: `make test-unit` passed (131/131 tests) and `git diff --check` passed. |
| A6 | P2 | Architecture/Bank | blocked | `projects/Bank/` and root `components/{charger,fuel_gauge,pid}/` contain parallel implementations. | Requires Eugene architecture decision. Safe precursor: inventory overlap and propose migration plan only. |
| A7 | P2 | Kconfig hygiene | ready | Root Kconfig `# Additional Components` block is after `endmenu`, relying on parser/file-order behavior. | Low-risk candidate: move into a proper menu or component Kconfig files; verify generated configs for PC/STM32U5. |
| A8 | P2 | Charger architecture | blocked | `components/charger/` is deprecated but still has Kconfig/build surface; Bank has a separate bq2562x implementation. | Requires Eugene decision: delete deprecated component, migrate Bank driver into component, or keep both intentionally. |
| A9 | P2 | clib/Kconfig | ready | `XY_XY_CLIB_ENABLE` Kconfig switch defaults off but xy_clib is always added by CMake. | Candidate: either remove dead switch or rename/wire `XY_CLIB_ENABLE`; verify build matrix. |
| A10 | P3 | Hygiene | ready | `components/clib/xy_clib/xy_config copy.h` appears duplicated with `xy_config.h`. | Very low-risk after diff confirms identical. Prefer path-limited deletion commit. |
| A11 | P3 | PID organization | open | `components/pid/` has duplicate root/inc headers and confusing src/example organization. | Needs careful include/API audit before file moves. Medium-sized cleanup. |
| A12 | P3 | Fuel gauge API | open | Fuel gauge drivers use static singleton devices, limiting multi-instance tests/use cases. | API-affecting. First write compatibility plan; preserve default singleton wrappers if changing. |
| A13 | P3 | README docs | ready | README repeats device framework description in multiple sections. | Low-risk doc cleanup; verify section numbering and links. |
| A14 | P4 | PC HAL/log validation | ready | Need confirm `xy_log` PC fallback and `hal_*` PC no-op/no-op-with-log behavior. | Analysis/verification candidate. Read PC HAL/log backend and run focused host smoke if available. |
| A15 | P4 | Toolchain UX | open | Cross-toolchain paths default to local Linux hardcoded paths. | Improve only after checking current CMake cache behavior and supported developer workflows. |

## Suggested execution order

1. Safe low-risk execution: A7, A9, A13, A14.
2. Parser/build correctness: A2, after focused regression tests are in place.
3. Documentation sync: A5 is closed; use AGENTS.md/Makefile as command truth if future docs drift appears.
4. Architecture decisions: A1, A6, A8, A12, A15 need re-check and/or Eugene decision before invasive changes.

## Periodic review checklist

When reviewing this backlog:

1. Check current `git status --short` and avoid unrelated dirty files.
2. Re-read the specific files for the candidate item; do not assume the 2026-08-05 report is still current.
3. Classify each candidate as `ready`, `blocked`, `done`, or `obsolete`.
4. If executing, pick one small slice, run the relevant build/test command, and commit only touched paths.
5. Update this backlog with evidence after completion.
