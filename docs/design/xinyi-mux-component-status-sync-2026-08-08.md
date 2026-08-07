# XinYi MUX Component Status Sync (2026-08-08)

## Slice

同步 MUX 组件状态，关闭旧完整度报告中“无示例/无测试/需补基线”的过期描述。

## 现状证据

- `components/mux/CMakeLists.txt` 在 `COMPONENT_MUX` 默认启用时创建 `xy_mux` 静态库，并提供 `mux_component` alias。
- `components/mux/README.md` 已记录 GPIO/I2C/SPI/UART typed API、完整示例、完成度和 host 验证契约。
- `components/mux/examples/example_mux_basic.c` 已作为 `mux_example_basic` build-guarded smoke 纳入 active unit suite。
- `tests/unit/mux/` 已包含并注册：
  - `mux_core`
  - `mux_gpio`
  - `mux_i2c`
  - `mux_spi`
  - `mux_uart`
  - `mux_example_basic`
- `docs/design/unit-test-inventory.md` 当前统计显示 `mux` 组件 5 个 Unity 测试文件、0 个 raw assert、0 个 unwired/legacy 残留。

## 本次同步结果

`components/COMPONENT_COMPLETENESS_ANALYSIS.md` 中 MUX 从旧的 `45% / 需补充 / 示例❌ / 测试❌` 更新为 `100% / 主线完善 / 示例✅ / 测试✅`。

## 后续维护口径

MUX 不再作为“补 README/Kconfig/CMake/示例/测试基线”的 backlog 候选。后续只在以下情况选择 MUX slice：

1. focused CTest 或根构建暴露真实 packet/typed ops 回归；
2. 上位机/硬件集成提出新的可验证协议契约；
3. public API 示例与实现再次漂移，需要小范围 smoke/README 同步。

## 验证命令

```bash
cmake --build build/tests/unit --target test_mux_core test_mux_gpio test_mux_i2c test_mux_spi test_mux_uart test_mux_example_basic -j$(nproc)
cd build/tests/unit && ctest -R '^mux_(core|gpio|i2c|spi|uart|example_basic)$' --output-on-failure
make test-unit
git diff --check
```
