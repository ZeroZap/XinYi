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

## 7. CMake/CTest Wiring

Add tests in `tests/unit/CMakeLists.txt`.

Preferred helper form for new simple tests:

```cmake
xy_add_unit_test(test_fff_smoke fff_smoke
    ${UNIT_FRAMEWORK}/test_fff_smoke.c
    ${ROOT}/tests/unity/unity.c
)
```

For complex tests, it is still acceptable to use explicit CMake commands when the target needs special include directories, compile definitions, link libraries, or source lists.

Always register one CTest entry per executable unless there is a specific reason not to.

## 8. Execution Commands

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

## 9. Migration Rules

When migrating old tests:

1. Convert one file or one component group at a time.
2. Replace raw `assert()` with Unity assertions.
3. Keep behavior unchanged during assertion-style migration.
4. Run the focused target after each file.
5. Run `make test-unit` after each component group.
6. Only introduce FFF when it makes dependency behavior clearer.

Recommended order:

1. DM
2. IPC
3. Storage
4. Display
5. Fuel Gauge
6. Net
7. Remaining component groups

## 10. Coverage

Coverage should be added later through optional CMake/CTest integration, for example `gcovr` or `lcov`.

Rules:

- Coverage must be disabled by default.
- Exclude vendor, build directories, Unity, and FFF from coverage reports.
- Do not introduce Ceedling only for coverage unless CMake/CTest coverage proves insufficient.

## 11. Ceedling Status

Ceedling is deferred.

Re-evaluate it only if one of these becomes true:

- Automatic CMock generation would clearly save significant maintenance effort.
- FFF/manual fake maintenance becomes a bottleneck.
- Required reporting/plugins are hard to reproduce with CMake/CTest.
- The team accepts the Ruby toolchain and `project.yml` workflow cost.

If evaluated, use a separate pilot module and keep the current CTest target as the reference baseline.
