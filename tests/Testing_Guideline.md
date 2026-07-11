# XinYi Unit Testing Guideline

> **Current direction:** Unity + CTest + FFF is the active unit-test path. Ceedling is deferred to a later evaluation stage and is not required for new tests.

## 1. Scope

This guide applies to host-side unit tests under `tests/unit/`.

The goals are:

- Keep `make test-unit` as the default local unit-test command.
- Use CMake/CTest for building, linking, test discovery, and CI execution.
- Use Unity for assertions and test reporting.
- Use FFF for fakes/mocks when dependency interactions need to be verified.
- Avoid introducing Ruby/Ceedling/CMock until there is a concrete need.

## 2. Test Stack

| Layer | Tool | Responsibility |
| --- | --- | --- |
| Test assertions | Unity | `TEST_ASSERT_*`, `UNITY_BEGIN`, `RUN_TEST`, `UNITY_END` |
| Test execution | CTest | Test selection, output, CI-friendly pass/fail reporting |
| Build/link | CMake | Source selection, include paths, fake/stub linkage |
| Fakes/mocks | FFF | Call counts, argument capture, return values, custom fake behavior |

## 3. Directory Layout

```text
tests/
├── unity/                  # Unity source/header
├── fff/                    # FFF single-header framework
├── unit/                   # CTest unit targets
│   ├── CMakeLists.txt
│   ├── framework/          # Framework smoke tests
│   ├── dm/
│   ├── display/
│   ├── fuel_gauge/
│   └── ...
└── support/ or unit/support/ # Shared test helpers when needed
```

## 4. Naming Rules

- Test file: `test_<component>_<feature>.c`.
- C executable target: `test_<component>_<feature>`.
- CTest name: concise behavior name, for example `dm_tlv` or `fff_smoke`.
- Test function: `test_<condition>_<expected_behavior>`.
- Fake reset helper: `reset_<module>_fakes()`.

## 5. Basic Unity Test Shape

```c
#include "unity.h"

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_init_rejects_null_config(void)
{
    TEST_ASSERT_EQUAL_INT(XY_ERROR_INVALID_PARAM, module_init(NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_rejects_null_config);
    return UNITY_END();
}
```

Prefer specific assertions:

- `TEST_ASSERT_EQUAL_INT(expected, actual)` for status codes.
- `TEST_ASSERT_EQUAL_UINT32(expected, actual)` for counters.
- `TEST_ASSERT_EQUAL_HEX8(expected, actual)` for registers and bytes.
- `TEST_ASSERT_EQUAL_MEMORY(expected, actual, len)` for buffers.
- `TEST_ASSERT_FLOAT_WITHIN(delta, expected, actual)` for float math.
- `TEST_ASSERT_NULL(ptr)` / `TEST_ASSERT_NOT_NULL(ptr)` for pointer contracts.

## 6. FFF Usage

Use FFF when the test needs to verify dependency interaction, not just provide a simple deterministic return value.

Good FFF cases:

- HAL/driver calls where call count matters.
- Argument capture for SPI/I2C/GPIO/UART transactions.
- Return sequences such as transient NACK then success.
- Callback registration and dispatch.
- Custom fake behavior for small simulated state.

Simple local stubs are still fine when the dependency only returns a constant and no interaction needs to be asserted.

### Minimal FFF Pattern

```c
#include "fff.h"
#include "unity.h"

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(int, xy_hal_i2c_write, uint8_t, const uint8_t *, uint16_t)
FAKE_VOID_FUNC(xy_hal_delay_ms, uint32_t)

void setUp(void)
{
    RESET_FAKE(xy_hal_i2c_write);
    RESET_FAKE(xy_hal_delay_ms);
    FFF_RESET_HISTORY();
}

static void test_driver_writes_expected_register(void)
{
    xy_hal_i2c_write_fake.return_val = 0;

    TEST_ASSERT_EQUAL_INT(0, driver_start());
    TEST_ASSERT_EQUAL_UINT(1U, xy_hal_i2c_write_fake.call_count);
    TEST_ASSERT_EQUAL_HEX8(0x40, xy_hal_i2c_write_fake.arg0_val);
}
```

Rules:

- Put `DEFINE_FFF_GLOBALS;` in exactly one C file per test executable.
- Reset every fake used by the test in `setUp()` or a dedicated reset helper.
- Use `FFF_RESET_HISTORY()` when checking global call order/history.
- Prefer `custom_fake` only when return values alone cannot express the behavior.

## 7. Adding A New Unit Test

Use this checklist for every new host-side unit test:

1. Pick the component group under `tests/unit/<group>/`.
2. Create `test_<component>_<feature>.c` from `tests/templates/unity_fff_test_template.c`.
3. Include the public header under test first, then `unity.h`, and add `fff.h` only when dependency interaction must be observed.
4. Write one behavior per `test_<condition>_<expected_behavior>()` function.
5. Prefer real code and small local fixtures; use FFF for HAL/OSAL/driver dependencies when you need call counts, captured arguments, ordered calls, return sequences, or `custom_fake` behavior.
6. Reset fixture state and all fakes in `setUp()`; release owned state in `tearDown()`.
7. Wire the executable in `tests/unit/CMakeLists.txt` with `xy_add_unit_test(... UNITY ...)` for tests that include `unity.h`.
8. Keep the executable target name and CTest name stable if converting an existing test.
9. Run the focused target and CTest entry before broader validation.
10. Run `make test-unit` before committing a component group.
11. Run `git diff --check` before committing.

### Test Selection Rules

- **Pure utility logic:** Unity only; no fake framework.
- **Simple dependency return:** small local stub is acceptable.
- **Observable dependency contract:** use FFF and assert `call_count`, `arg*_val`, `arg*_history`, return sequences, or call history.
- **Complex simulated hardware state:** keep a readable hand-written fixture; use FFF only at the boundary calls.

### Required Validation Sequence

```bash
cmake -S tests/unit -B build/tests/unit
cmake --build build/tests/unit --target test_<component>_<feature> -j"$(nproc)"
ctest --test-dir build/tests/unit -R '^<ctest_name>$' --output-on-failure
make test-unit
git diff --check
```

## 8. CMake/CTest Wiring

Add tests in `tests/unit/CMakeLists.txt`.

Preferred helper form for new Unity tests:

```cmake
xy_add_unit_test(test_foo_bar foo_bar UNITY
    ${UNIT_FOO}/test_foo_bar.c
    ${FOO}/xy_foo.c
)
```

`UNITY` appends `${ROOT}/tests/unity/unity.c` and the helper registers the CTest entry. Add target-specific configuration immediately after the helper call when needed:

```cmake
target_include_directories(test_foo_bar PRIVATE ${FOO}/inc)
target_link_libraries(test_foo_bar m)
target_compile_definitions(test_foo_bar PRIVATE XY_FOO_TEST=1)
```

Use explicit CMake only when the helper cannot express a target requirement. If explicit CMake is necessary, keep one CTest entry per executable and document why the helper was not used.

## 9. Execution Commands

Run one focused test:

```bash
cmake -S tests/unit -B build/tests/unit
cmake --build build/tests/unit --target test_fff_smoke -j"$(nproc)"
ctest --test-dir build/tests/unit -R fff_smoke --output-on-failure
```

Run the full host unit suite:

```bash
make test-unit
```

Check CTest inventory:

```bash
ctest --test-dir build/tests/unit -N
```

Check patch hygiene before commit:

```bash
git diff --check
```

## 10. Maintenance Rules

The raw-`assert()` migration is complete for tracked `tests/unit` sources. Keep the inventory healthy:

1. New tests must use Unity assertions from the start.
2. Keep raw `assert()` and unwired-source scans at zero for tracked `tests/unit` source files.
3. Treat first-party-looking files outside `tests/unit` as triage items before migrating them.
4. Keep behavior unchanged during assertion-style cleanup.
5. Run the focused target for touched tests, then `make test-unit` and `git diff --check`.
6. Only introduce FFF when it makes dependency behavior clearer.

## 11. Coverage And CI

Optional coverage is available through the CMake/CTest path, not Ceedling:

```bash
cmake -S tests/unit -B build/tests/unit_coverage -DXY_ENABLE_UNIT_COVERAGE=ON
cmake --build build/tests/unit_coverage --target unit_coverage -j"$(nproc)"
```

Rules:

- Coverage is disabled by default.
- Coverage reports exclude vendor, build directories, Unity, and FFF.
- The HTML report is generated under `build/tests/unit_coverage/coverage/index.html`.
- CI publishes the optional unit coverage directory as an artifact.
- The unit CI gate also runs `git diff --check`, blocks new raw `assert()` in `tests/unit`, and checks clang-format only for touched C/C++ files.
- Do not introduce Ceedling only for coverage unless CMake/CTest coverage proves insufficient.

## 12. Ceedling Status

Ceedling remains deferred after the Phase 8 re-evaluation. The current stack already covers the near-term needs with Unity assertions, CTest execution, FFF interaction fakes, CI quality gates, and gcovr reporting.

Re-evaluate it only if one of these becomes true:

- Automatic CMock generation would clearly save significant maintenance effort.
- FFF/manual fake maintenance becomes a bottleneck.
- Required reporting/plugins are hard to reproduce with CMake/CTest.
- The team accepts the Ruby toolchain and `project.yml` workflow cost.

If evaluated, use a separate pilot module and keep the current CTest target as the reference baseline.
