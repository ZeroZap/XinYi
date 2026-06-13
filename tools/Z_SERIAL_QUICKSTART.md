# z-serial 启动指导书

状态：V1 基线完成 / 可用 GUI + profile 编辑闭环已完成。

当前已具备 GUI 多 tab 骨架、虚拟串口演示、过滤颜色渲染、profile core 读写、profile GUI 打开/保存入口、过滤器/按钮 GUI 编辑入口、端口下拉刷新和手动路径输入、自定义发送、清屏、自动滚动、状态栏、接收日志行数裁剪、host 压力 smoke、Qt offscreen smoke 验证。完整硬件交付仍需要真实 USB 串口闭环和人工长期 soak 验证。

## 1. 从哪里启动

仓库根目录：

```bash
cd /home/eugene/zerozap/XinYi
```

启动 GUI：

```bash
tools/z-serial
```

Windows 启动 GUI：

```bat
tools\z-serial.cmd
```

生成 PyInstaller 桌面包：

```bash
tools/z-serial-package
```

Windows 生成 PyInstaller 桌面包：

```bat
tools\z-serial-package.cmd
```

产物目录：`dist/z-serial/`，Windows 下入口为 `dist\z-serial\z-serial.exe`，Linux/macOS 下入口为 `dist/z-serial/z-serial`。

如果桌面环境缺少 xcb/wayland 依赖，可先跑 offscreen 验证：

```bash
tools/z-serial-smoke
```

当前已验证通过的输出形态：

```text
window=z-serial
tabs=2
open=False
has_error=true
has_second_error=true
has_red=true
has_warn=true
has_editor_button=true
custom_tx=true
cleared=true
```

## 2. 依赖安装

GUI 依赖 PySide6，真实串口访问依赖 pyserial，打包依赖 PyInstaller。首次使用直接运行：

```bash
tools/z-serial-setup
```

Windows 首次使用：

```bat
tools\z-serial-setup.cmd
```

脚本会创建/复用 `tools/.venv/` 并安装 `tools/requirements-z-serial.txt` 中的 `PySide6`、`pyserial`、`pyinstaller`。

当前测试和虚拟串口演示不依赖真实串口硬件。

## 3. GUI 怎么用

启动 GUI 后：

1. 点击 `打开虚拟演示`。
2. GUI 会创建一对 Linux PTY 虚拟串口，并把 host 端路径填到可编辑端口下拉框。
3. 点击 `发送 version`。
4. 点击 `模拟回包`。
5. 接收区应显示模拟设备回包，并对 `ERROR` 行应用红色过滤高亮。
6. 点击 `新建 Tab` 可以创建第二个串口 tab。
7. 每个 tab 有独立的端口、连接、输出区和 view-model 状态。
8. GUI 内部已有 200ms `QTimer` 轮询框架，会轮询所有已打开 tab。
9. 可在发送输入框输入自定义命令，回车或点击 `发送` 输出。
10. 点击 `清屏` 清空当前 tab 的接收区。
11. 点击 `刷新端口` 枚举 pyserial 可见串口，并填充可编辑下拉框；可从下拉选择，也可手动输入虚拟 PTY 或自定义路径。
12. 顶部 `打开 Profile` / `保存 Profile` 可通过文件对话框加载/保存当前 tab 的 JSON profile。
13. 顶部 `编辑过滤器` 可新增/覆盖全局过滤规则，关闭串口后编辑，保存后下次打开立即生效。
14. 顶部 `编辑按钮` 可新增/覆盖全局动作按钮，支持 text/hex/script payload。
15. 接收区默认保留最近 2000 行，长时间运行时会裁剪最旧日志，避免无限增长。

默认过滤器和按钮来自 `serial_cli.DEFAULT_WORKSPACE`，也可以通过 GUI 编辑后保存到 JSON profile。

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

大流量/日志裁剪压力 smoke：

```bash
PYTHONPATH=tools python3 -m xy_host_tools.serial_cli stress-smoke --lines 5000 --retain 2000
```

预期输出包含：

```text
received=5000
retained=2000
retain_limit=2000
error_lines=50
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

## 7. 当前 V1 基线能力

已完成：

- 多 tab GUI shell。
- 每 tab 独立 view-model。
- 200ms 轮询框架。
- 端口刷新可编辑下拉框。
- 自定义发送输入框。
- 清屏、自动滚动、状态栏。
- 虚拟串口一键演示。
- `version` 按钮发送。
- 模拟设备回包。
- 接收区 HTML 富文本颜色渲染。
- JSON profile core 读写。
- GUI profile 打开/保存入口。
- GUI 过滤器编辑入口。
- GUI 动作按钮编辑入口。
- 接收日志默认 2000 行裁剪。
- 大流量/日志裁剪压力 smoke。
- Qt offscreen smoke。
- Linux PTY virtual smoke。
- 完整 host-tool 单测。

未完成，属于硬件交付和人工长期验证剩余项：

- 真实 USB 串口硬件收发验证。
- 人工长期 soak 验证。

## 8. 最近验证结果

最近一次通过：

```text
59 tests OK, 1 skipped
stress-smoke OK, received=5000 retained=2000 error_lines=50
gui-smoke OK, tabs=2
py_compile OK
git diff --check OK
```

最新提交：

```text
feat(tools): add z-serial V1 stress smoke
```
