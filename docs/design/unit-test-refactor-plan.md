# Unit Test Refactor Plan: Unity + CTest + FFF

> **Decision:** Use the current CMake/CTest unit-test framework as the backbone, standardize assertions on Unity, add FFF for fakes/mocks, and defer Ceedling until a later evaluation stage.

## Goal

Build a maintainable host-side unit-test system for XinYi that gives most of Ceedling's practical value without replacing the current CMake/CTest workflow.

## Current Baseline

- Unit tests are driven by `tests/unit/CMakeLists.txt` and run through `make test-unit` / CTest.
- The current suite exposes 72 CTest tests in `build/tests/unit`.
- Unity exists under `tests/unity/` and all tracked `tests/unit/**/*.c` source tests are Unity-style.
- Raw `assert()` and plain compile-smoke source files in `tests/unit/` have been migrated or pruned; build-generated files under `tests/unit/build/` are excluded from the source inventory.
- FFF is vendored under `tests/fff/` and covered by the `fff_smoke` CTest target.
- `tests/Testing_Guideline.md` is still Ceedling-oriented and does not match the preferred near-term direction.

## Target Architecture

```text
tests/
├── unity/                  # Existing Unity framework
├── fff/                    # Add fff.h single-header fake framework
├── support/                # Shared host-test helpers and fake reset helpers
├── unit/                   # Focused CTest targets, grouped by component
│   ├── CMakeLists.txt      # Single source of truth for host unit targets
│   ├── dm/
│   ├── fuel_gauge/
│   ├── display/
│   └── ...
└── Testing_Guideline.md    # Update later to Unity + CTest + FFF guidance
```

## Guiding Rules

1. Keep CTest as the execution/reporting layer.
2. Keep CMake as the compile/link/source-selection layer.
3. Use Unity for all new tests and gradually migrate old raw `assert()` tests.
4. Use FFF for external dependency fakes when call count, argument capture, return sequences, or custom fake behavior are needed.
5. Keep simple local stubs when FFF would add no value.
6. Do not introduce Ruby, Ceedling, CMock, or generated runners in the near-term phases.
7. Make every migration slice independently verifiable with a focused target plus `make test-unit`.

## Phase 0: Freeze The Baseline

**Objective:** Record the current state before changing the test framework shape.

**Tasks:**

1. Run the current full unit suite.
   - Command: `make test-unit`
   - Expected: all existing CTest tests pass, or failures are recorded before framework work starts.
2. Capture the current CTest inventory.
   - Command: `ctest --test-dir build/tests/unit -N`
   - Output to review: test count and test names.
3. Classify current test files.
   - Unity tests: files including `unity.h` or using `TEST_ASSERT_*`.
   - Raw assert tests: files using `assert()` without Unity.
   - Ad-hoc fake-heavy tests: files defining many local dependency stubs.
4. Record the classification in a small tracker file.
   - Suggested path: `docs/design/unit-test-inventory.md`.

**Exit Criteria:**

- Baseline unit-test command is known.
- Test count is known.
- Candidate migration files are categorized.

## Phase 1: Standardize Test Target Helpers

**Objective:** Reduce repeated CMake boilerplate before adding more test targets.

**Status:** Complete. `xy_add_unit_test()` supports an optional `UNITY` flag that appends `${ROOT}/tests/unity/unity.c` and registers the CTest entry in one place. All 72 CTest entries are now registered through the helper, with target names and CTest names kept stable. The only remaining `add_executable()` and `add_test()` calls in `tests/unit/CMakeLists.txt` are inside the helper itself.

**Tasks:**

1. [x] Add a helper function in `tests/unit/CMakeLists.txt`.
   - Suggested name: `xy_add_unit_test`.
   - Responsibilities:
     - Create the executable.
     - Add common include directories.
     - Link Unity when requested by the target.
     - Register the CTest name.
2. [x] Convert existing targets to the helper.
   - Completed across all current unit CTest targets, including targets with include directories, compile definitions, and math library links.
3. [x] Keep target names and CTest names stable.
4. [x] Run focused tests for converted batches.
5. [x] Run `make test-unit`.

**Exit Criteria:**

- New helper exists.
- All current unit CTest targets use it.
- Full unit suite still passes.
- No unrelated test behavior changes.

## Phase 2: Add FFF As Optional Test Dependency

**Objective:** Introduce FFF without changing existing tests.

**Status:** Complete. FFF is vendored at `tests/fff/fff.h`, covered by `tests/unit/framework/test_fff_smoke.c`, and wired into the unit CMake suite as `fff_smoke` through `xy_add_unit_test(... UNITY ...)`. Focused smoke verification and the full 72-test unit suite pass.

**Tasks:**

1. [x] Add FFF single-header dependency.
   - Suggested path: `tests/fff/fff.h`.
   - Source: upstream FFF single header, vendored as test-only code.
2. [x] Add a tiny smoke test.
   - Suggested path: `tests/unit/framework/test_fff_smoke.c`.
3. [x] Wire the smoke test into `tests/unit/CMakeLists.txt`.
4. [x] Verify FFF basics:
   - `FAKE_VALUE_FUNC` compiles.
   - Fake call count increments.
   - Argument history works.
   - `RESET_FAKE` works.
5. [x] Run focused smoke target.
6. [x] Run `make test-unit`.

**Exit Criteria:**

- FFF is available to tests.
- Existing tests remain unchanged.
- Smoke test proves FFF works under current CMake/CTest.

## Phase 3: Define Test Style Contract

**Objective:** Create a clear convention so new tests do not drift.

**Status:** Complete. `tests/Testing_Guideline.md` now defines the active Unity + CTest + CMake helper + FFF convention, including fake-style selection, naming rules, validation commands, and Ceedling deferral. `tests/templates/unity_fff_test_template.c` provides the minimal starting point for new host-side unit tests.

**Tasks:**

1. [x] Update or replace `tests/Testing_Guideline.md`.
2. [x] Make the new guidance explicitly based on:
   - Unity for assertions.
   - CTest for execution.
   - CMake target wiring.
   - FFF for fakes/mocks.
3. [x] Document when to use each fake style:
   - Plain local stub: deterministic simple output only.
   - FFF fake: call count, argument capture, return sequence, or custom fake behavior.
   - Hand-written fixture: complex simulated hardware state.
4. [x] Add naming rules:
   - File: `test_<component>_<feature>.c`.
   - Test function: `test_<condition>_<expected_behavior>`.
   - Fake reset helper: `reset_<module>_fakes()`.
5. [x] Add a minimal test template.
   - Suggested path: `tests/templates/unity_fff_test_template.c`.

**Exit Criteria:**

- New tests have one documented pattern.
- Ceedling-specific commands are removed from the active near-term guide or moved to an appendix.

## Phase 4: Migrate Raw Assert Tests Gradually

**Objective:** Convert existing raw `assert()` tests to Unity without changing tested behavior.

**Status:** Complete by inventory. The current tracked `tests/unit` source tree has 70 C unit-test files, all Unity-style, with 0 raw `assert()` files, 0 mixed Unity/raw files, and 0 plain compile-smoke files. No source migration is required for this phase; keep the guardrail that new tests must use Unity assertions.

**Tasks:**

1. [x] Pick one component group at a time.
   - Suggested order: DM -> IPC -> Storage -> Display -> Fuel Gauge -> Net.
   - No component group currently has raw `assert()` files.
2. [x] For each file:
   - Replace `assert()` with `TEST_ASSERT_*` equivalents.
   - Add `UNITY_BEGIN()` / `RUN_TEST()` / `UNITY_END()` when missing.
   - Keep existing fake/stub behavior intact.
   - Already satisfied across the current tracked unit-test inventory.
3. [x] Run the focused target after each file.
   - No focused migration targets remain.
4. [x] Run `make test-unit` after each component group.
5. [x] Do not mix behavior fixes with assertion-style migration unless the test exposes a real bug.

**Exit Criteria:**

- New failures identify exact Unity assertion location.
- No behavior-only regressions introduced by migration.
- Raw `assert()` usage in `tests/unit` is currently 0.

## Phase 5: Replace Ad-Hoc Fakes With FFF Where Useful

**Objective:** Use FFF to make dependency behavior observable and less error-prone.

**Status:** In progress. First slices converted the storage EEPROM 24xx, BQ25620 charger I2C HAL fakes, analog devices, display OLED/WS2812, display LCD, display LED driver GUI adapter, MUX GPIO/I2C/SPI/UART helpers, CAN component callbacks/OSAL hooks, LTE component callbacks, AT client/server I/O callbacks, FOTA core, SMBus/PMBus net helpers, actuator framework ops, fuel-gauge core, MAX17043 fuel-gauge, BQ27Z746 fuel-gauge, BQ27Z561 fuel-gauge, and BQ40Z50 fuel-gauge host tests: HAL delay/I2C/GPIO/SPI, OSAL tick/delay, display callback adapters, MUX device ops, CAN RX callbacks, LTE URC/receive callbacks, AT client/server read/write/tick/response/command callbacks, FOTA flash-op, SMBus/PMBus lifecycle callbacks, actuator ops, fuel-gauge core API callbacks, and fuel-gauge Sensor-bus boundaries now use FFF for call counts and argument capture, while EEPROM memory, charger register maps, analog readback, OLED I2C logs, WS2812 GPIO callback logs, LCD SPI/GPIO transaction logs, MUX GPIO/I2C/SPI/UART payload/readback/level fixtures, CAN last callback message, actuator last-write state, FOTA flash backing storage, fuel-gauge helper data, and fuel-gauge register maps remain hand-written state fixtures because that keeps behavior clearer.

**Good candidates:**

- HAL functions: I2C, SPI, GPIO, UART, delay.
- OSAL functions: tick, delay, mutex/semaphore wrappers when simple enough.
- Storage/flash hooks.
- Callback registration and dispatch paths.
- Driver ops where tests assert exact call counts or parameters.

**Tasks:**

1. Choose one fake-heavy test file.
2. Replace only the fakes that need observability.
3. Add `DEFINE_FFF_GLOBALS` in exactly one C file per test executable.
4. Add `RESET_FAKE(...)` and `FFF_RESET_HISTORY()` in `setUp()` or a local reset helper.
5. Use FFF assertions for:
   - `fake.call_count`.
   - `fake.arg0_val`, `fake.arg1_val`, or argument history.
   - `fake.return_val` / return sequences.
   - `custom_fake` for simulated hardware state.
6. Keep stateful hardware simulations hand-written if FFF would make them harder to read.
7. Run focused target and `make test-unit`.

**Exit Criteria:**

- Repeated dependency fake boilerplate is reduced.
- Tests can assert dependency interactions clearly.
- Fixture readability improves rather than worsens.

## Phase 6: Add Coverage And Reporting Without Ceedling

**Objective:** Get the useful reporting pieces usually associated with Ceedling while staying in CMake/CTest.

**Tasks:**

1. Add optional coverage flags to the unit-test CMake flow.
   - Suggested option: `XY_ENABLE_UNIT_COVERAGE`.
2. Add a coverage command using `gcovr` or `lcov`.
3. Keep coverage disabled by default.
4. Exclude vendor, build, and test framework directories.
5. Generate HTML and summary reports.
6. Add CI artifact support later if needed.

**Exit Criteria:**

- Coverage can be generated locally without Ruby/Ceedling.
- Normal `make test-unit` remains fast and unchanged.

## Phase 7: CI And Quality Gates

**Objective:** Make the new framework enforceable.

**Tasks:**

1. Add CI steps for:
   - Configure/build unit tests.
   - Run `ctest --output-on-failure`.
   - Run `git diff --check`.
2. Add an optional style check for touched C/H files.
3. Add a simple guard that blocks new raw `assert()` in `tests/unit` unless explicitly justified.
4. Publish test logs as artifacts when CI fails.

**Exit Criteria:**

- Every PR can run the same host unit-test gate locally and in CI.
- New tests follow Unity + CTest + FFF conventions.

## Phase 8: Ceedling Re-Evaluation Gate

**Objective:** Reconsider Ceedling only after the lightweight framework reaches maturity.

**Evaluate Ceedling only if at least one condition becomes true:**

- Header-driven automatic mock generation would save significant time across many modules.
- FFF/manual fake maintenance becomes a bottleneck.
- The team needs Ceedling plugins that are hard to replicate with CMake/CTest.
- CI/reporting requirements exceed what CTest + gcovr/lcov provide.

**Pilot scope if evaluated:**

1. Choose one isolated module with clear dependencies.
2. Keep existing CTest target as the reference baseline.
3. Add a separate Ceedling experiment outside the main unit-test path.
4. Compare:
   - Setup complexity.
   - Mock maintenance effort.
   - Build speed.
   - CI integration cost.
   - Developer workflow friction.
5. Decide whether to keep, expand, or delete the pilot.

**Exit Criteria:**

- Ceedling decision is evidence-based, not framework-driven.
- Mainline Unity + CTest + FFF workflow remains stable during evaluation.

## Recommended First Implementation Slice

1. Create `docs/design/unit-test-inventory.md` with current classification.
2. Add `xy_add_unit_test` helper and migrate 2-3 simple CMake targets.
3. Vendor `tests/fff/fff.h`.
4. Add `tests/unit/framework/test_fff_smoke.c`.
5. Update `tests/Testing_Guideline.md` to stop presenting Ceedling as the active path.
6. Run:
   - `cmake --build build/tests/unit --target test_fff_smoke -j$(nproc)`
   - `ctest --test-dir build/tests/unit -R fff_smoke --output-on-failure`
   - `make test-unit`
   - `git diff --check`

## Success Metrics

- New unit tests use Unity consistently.
- New fake-heavy tests prefer FFF where interaction checks matter.
- Existing raw `assert()` tests are reduced phase by phase.
- `make test-unit` remains the primary local command.
- CTest test count increases only when meaningful coverage is added.
- Ceedling remains deferred until a concrete need appears.
