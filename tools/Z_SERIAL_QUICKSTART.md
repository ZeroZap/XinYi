# z-serial 启动指导书

状态：V1 Alpha / 主骨架完成，尚不是完整 V1。

当前已具备 GUI 多 tab 骨架、虚拟串口演示、过滤颜色渲染、profile core 读写、profile GUI 打开/保存入口、端口刷新、自定义发送、清屏、自动滚动、状态栏、Qt offscreen smoke 验证。完整 V1 还需要真实串口硬件闭环、过滤器/按钮 GUI 编辑器、大日志性能和长期运行稳定性。

## 1. 从哪里启动

仓库根目录：

```bash
cd /home/eugene/zerozap/XinYi
```

启动 GUI：

```bash
PYTHONPATH=tools python3 -m xy_host_tools.serial_cli gui
```

如果桌面环境缺少 xcb/wayland 依赖，可先跑 offscreen 验证：

```bash
QT_QPA_PLATFORM=offscreen PYTHONPATH=tools python3 -m xy_host_tools.serial_cli gui-smoke
```

当前已验证通过的输出形态：

```text
window=z-serial
tabs=2
open=False
has_error=true
has_second_error=true
has_red=true
```

## 2. 依赖安装

GUI 依赖 PySide6。当前环境已安装 `PySide6 6.11.1`。

如果换机器或环境，需要安装：

```bash
python3 -m pip install -i https://pypi.tuna.tsinghua.edu.cn/simple --trusted-host pypi.tuna.tsinghua.edu.cn --timeout 120 PySide6
```

真实串口访问未来需要 `pyserial`：

```bash
python3 -m pip install -i https://pypi.tuna.tsinghua.edu.cn/simple --trusted-host pypi.tuna.tsinghua.edu.cn --timeout 120 pyserial
```

当前测试和虚拟串口演示不依赖真实串口硬件。

## 3. GUI 怎么用

启动 GUI 后：

1. 点击 `打开虚拟演示`。
2. GUI 会创建一对 Linux PTY 虚拟串口，并把 host 端路径填到端口输入框。
3. 点击 `发送 version`。
4. 点击 `模拟回包`。
5. 接收区应显示模拟设备回包，并对 `ERROR` 行应用红色过滤高亮。
6. 点击 `新建 Tab` 可以创建第二个串口 tab。
7. 每个 tab 有独立的端口、连接、输出区和 view-model 状态。
8. GUI 内部已有 200ms `QTimer` 轮询框架，会轮询所有已打开 tab。
9. 可在发送输入框输入自定义命令，回车或点击 `发送` 输出。
10. 点击 `清屏` 清空当前 tab 的接收区。
11. 点击 `刷新端口` 枚举 pyserial 可见串口；无串口时会显示状态提示。
12. 顶部 `打开 Profile` / `保存 Profile` 可通过文件对话框加载/保存当前 tab 的 JSON profile。

当前 GUI 仍未提供过滤器/按钮编辑器；默认过滤器和按钮来自 `serial_cli.DEFAULT_WORKSPACE`。

## 4. CLI 验证命令

过滤器 demo：

```bash
PYTHONPATH=tools python3 -m xy_host_tools.serial_cli demo-filter
```

按钮发送 demo：

```bash
PYTHONPATH=tools python3 -m xy_host_tools.serial_cli send-demo version
```

预期输出：

```text
76657273696f6e0d0a
```

生成 sample profile：

```bash
PYTHONPATH=tools python3 -m xy_host_tools.serial_cli sample-profile /tmp/z-serial-profile.json
```

列出串口：

```bash
PYTHONPATH=tools python3 -m xy_host_tools.serial_cli list
```

无硬件虚拟串口闭环：

```bash
PYTHONPATH=tools python3 -m xy_host_tools.serial_cli virtual-smoke
```

GUI offscreen smoke：

```bash
QT_QPA_PLATFORM=offscreen PYTHONPATH=tools python3 -m xy_host_tools.serial_cli gui-smoke
```

完整单测：

```bash
QT_QPA_PLATFORM=offscreen PYTHONPATH=tools python3 -m unittest discover -s tools/tests -v
```

## 5. 从哪个源码开始看

推荐阅读顺序：

1. `tools/xy_host_tools/serial_cli.py`
   - CLI 总入口。
   - `gui` / `gui-smoke` / `virtual-smoke` 都从这里进。
   - 默认 workspace、过滤器、按钮也在这里定义。

2. `tools/xy_host_tools/gui/z_serial_app.py`
   - PySide6 GUI shell。
   - 负责 `QMainWindow`、`QTabWidget`、按钮、接收区、`QTimer`。
   - 只绑定 view-model，不直接处理串口业务。

3. `tools/xy_host_tools/gui/z_serial_tabs.py`
   - 多 tab 管理器。
   - 每个 tab 持有一个独立 `ZSerialWindowViewModel`。
   - 聚合轮询所有打开的 tab。

4. `tools/xy_host_tools/gui/z_serial_view_model.py`
   - GUI 状态/动作适配层。
   - 负责打开/关闭、发送按钮、轮询接收、虚拟演示、profile 保存/加载。
   - GUI 主要调用这里。

5. `tools/xy_host_tools/serial_service.py`
   - GUI/CLI 共用服务层。
   - 管理 serial window session、发送、接收、过滤结果。

6. `tools/xy_host_tools/serial_transport.py`
   - 串口抽象层。
   - 包含 `MemorySerialTransport`、可选 `PySerialTransport`、端口枚举。

7. `tools/xy_host_tools/serial_virtual.py`
   - Linux PTY 虚拟串口 harness。
   - 无硬件验证 host/device 收发闭环。

8. `tools/xy_host_tools/serial_filter.py`
   - 过滤器匹配和颜色决策。

9. `tools/xy_host_tools/serial_actions.py`
   - 按钮 payload 渲染。
   - 支持 text/hex/script。

10. `tools/xy_host_tools/serial_profile.py`
    - JSON profile 读写。

## 6. 架构分层

```text
CLI / GUI Shell
  -> GUI ViewModel / Tab Manager
  -> SerialWorkspaceService
  -> Core: config/filter/actions/profile
  -> Transport: memory / pyserial / virtual PTY
```

约束：

- GUI 层只做控件、tab、timer、信号绑定、展示。
- view-model 做 GUI 状态和用户动作适配。
- service 做串口 session 编排。
- core 做纯逻辑。
- transport 做真实/虚拟串口适配。
- bottom layers 不 import PySide6。

## 7. 当前 V1 Alpha 能力

已完成：

- 多 tab GUI shell。
- 每 tab 独立 view-model。
- 200ms 轮询框架。
- 端口刷新入口。
- 自定义发送输入框。
- 清屏、自动滚动、状态栏。
- 虚拟串口一键演示。
- `version` 按钮发送。
- 模拟设备回包。
- 接收区 HTML 富文本颜色渲染。
- JSON profile core 读写。
- GUI profile 打开/保存入口。
- Qt offscreen smoke。
- Linux PTY virtual smoke。
- 完整 host-tool 单测。

未完成，属于完整 V1 剩余项：

- 真实 USB 串口硬件收发验证。
- 端口刷新下拉框自动选择细化。
- 过滤器/按钮 GUI 编辑器。
- 大日志性能和长期运行稳定性。

## 8. 最近验证结果

最近一次通过：

```text
50 tests OK, 1 skipped
virtual-smoke OK
gui-smoke OK, tabs=2
compileall OK
git diff --check OK
```

最新提交：

```text
b70d5be feat(tools): add z-serial multi-tab GUI shell
```
