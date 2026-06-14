# XinYi 测试目录

## 目录布局

- `unit/`：PC 单元测试主套件，由 `make test-unit` 构建和运行；组件级测试按 `unit/<component>/` 归类。
- `actuator/`、`mux_test/`：仍保留独立 CMake 测试入口的组件/组合测试。
- `hal/`：HAL 平台或仿真相关测试。
- `qemu_*`：QEMU 平台测试。
- `unity/`：Unity 测试框架。

根目录不再放散落的 `.c` 测试文件；新增单组件测试应放入 `unit/<component>/`，组件组合测试放入 `integration/<domain>/`，并接入对应 CMake 入口。

## AT Client 测试

## 测试用例

| # | 测试名称 | 描述 | 状态 |
|---|---------|------|------|
| 1 | Init/Register | 初始化和设备注册 | ✅ |
| 2 | NULL command | 空命令拒绝 | ✅ |
| 3 | Busy state | 忙碌状态处理 | ✅ |
| 4 | OK response | OK 响应处理 | ✅ |
| 5 | ERROR response | ERROR 响应处理 | ✅ |
| 6 | Command with args | 带参数命令 | ✅ |
| 7 | Statistics | 统计信息更新 | ✅ |

## 运行测试

```bash
# 构建
cd build_pc_test
cmake .. -DBUILD_TESTING=ON
make test_at_client

# 运行
./tests/test_at_client
```

## 测试结果

```
╔══════════════════════════════════════════╗
║       AT Client Test Suite              ║
╚══════════════════════════════════════════╝

✅ Passed: 7
❌ Failed: 0
📊 Total:  7
```
