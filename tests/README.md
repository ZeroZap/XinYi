# XinYi 测试目录

## 目录布局

- `unit/`：PC 单元测试主套件，由 `make test-unit` 构建和运行；组件级测试按 `unit/<component>/` 归类。
- `actuator/`、`mux_test/`：仍保留独立 CMake 测试入口的组件/组合测试。
- `hal/`：HAL 平台或仿真相关测试。
- `qemu_*`：QEMU 平台测试。
- `unity/`：Unity 测试框架。

根目录不再放散落的 `.c` 测试文件；新增单组件测试应放入 `unit/<component>/`，组件组合测试放入 `integration/<domain>/`，并接入对应 CMake 入口。

## 运行测试

```bash
make test-unit
```

AT Client / AT Server 已并入 `tests/unit/net/*_core.c` 和 `tests/unit/CMakeLists.txt`，CTest 名称为 `at_client` / `at_server`。
