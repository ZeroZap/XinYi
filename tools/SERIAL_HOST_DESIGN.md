# z-serial 简约强大的多串口 GUI 上位机设计书

> 目标：在 XinYi 仓库内建立 `tools/` 上位机工具体系，第一款工具命名为 `z-serial`，聚焦 **GUI 多串口调试终端**。当前阶段不提交 commit，先完成设计书、核心配置/过滤模型和可验证代码骨架。

## 1. 产品定位

`z-serial` 是 ZeroZap / XinYi 面向嵌入式开发、调试、产测的 GUI 多串口上位机工具。

它不是只做“串口助手”，而是作为后续协议调试、日志分析、量产测试、固件升级的公共底座。

## 2. 核心使用场景

1. 同时打开多个串口，每个串口对应一个独立窗口/标签页。
2. 所有新建串口窗口默认继承全局过滤器和全局按钮配置。
3. 每个串口窗口可以追加、覆盖或禁用部分过滤器。
4. 过滤器支持多个规则，每条规则可单独配置：
   - 关键字组合
   - 匹配模式
   - 前景色
   - 背景色
   - 是否隐藏/高亮/置顶/计数
5. 窗口底部有全局快捷按钮区，按钮可配置文本输出或脚本输出。
6. 每个串口窗口也可追加自己的本地按钮。
7. 输入支持普通文本、HEX、转义文本和脚本生成。

## 3. 设计原则

### 3.1 简约

- 默认界面只保留：端口栏、接收区、输入区、按钮区。
- 高级功能收在配置面板，不污染主界面。
- 全局配置能覆盖 80% 日常需求。

### 3.2 强大

- 多串口并发。
- 全局过滤器 + 窗口本地过滤器。
- 每条过滤规则独立颜色和动作。
- 按钮可运行脚本，适合一键复位、进 bootloader、查询版本、跑诊断命令。
- 保存 profile，便于项目复用。

### 3.3 可测试

- 核心逻辑不依赖 GUI。
- 过滤器、按钮脚本、窗口配置继承都用 Python 单元测试验证。
- GUI 层只调用核心模型，不把业务逻辑写死在界面代码里。

## 4. 推荐技术路线

第一阶段采用 **Python core + GUI first** 路线：

- 核心库：纯 Python dataclass + 标准库。
- 串口层：后续接入 `pyserial`。
- CLI：保留为无硬件 demo、profile 生成、自动化验证入口。
- GUI：第一版产品界面优先，推荐先用 PySide6/PyQt6 原型；如果后续需要 Web 技术生态，再评估 Tauri。
- TUI：不作为第一版路线，只保留为未来可选调试界面。

关键原则：**GUI first 不等于 GUI-only**。GUI 是第一交付形态，但底层必须保持可独立调用、可测试、可脚本化，不能把串口读写、过滤、profile、按钮动作等业务逻辑绑死在界面事件里。

### 4.1 分层架构

```text
┌──────────────────────────────────────────────┐
│ GUI Shell / z-serial                          │
│ - 窗口、tab、菜单、设置面板、颜色渲染          │
│ - 只负责用户交互和显示，不直接承载业务规则      │
└──────────────────────┬───────────────────────┘
                       │ calls
┌──────────────────────▼───────────────────────┐
│ Application Service Layer                      │
│ - open/close/send/read/log/session orchestration│
│ - 连接 profile、transport、filter、button action│
│ - GUI、CLI、自动化脚本都调用这一层              │
└──────────────────────┬───────────────────────┘
                       │ uses
┌──────────────────────▼───────────────────────┐
│ Core Model / Engine                            │
│ - SerialWorkspaceProfile / SerialWindowProfile │
│ - FilterRule / apply_filters                   │
│ - ActionButton / render_button_payload         │
│ - JSON profile load/save                       │
└──────────────────────┬───────────────────────┘
                       │ uses
┌──────────────────────▼───────────────────────┐
│ Transport Layer                                │
│ - SerialTransport protocol                     │
│ - MemorySerialTransport for tests              │
│ - PySerialTransport for real hardware          │
└──────────────────────────────────────────────┘
```

依赖方向必须单向：`GUI -> Service -> Core -> Transport protocol`。底层不能 import GUI，也不能依赖 PySide/PyQt 类型；GUI 线程/信号只在 GUI shell 内部处理。

### 4.2 底层可调用要求

- CLI 可以调用同一套 service 做 `list/open/send/log/profile`，不是另写一套逻辑。
- 自动化测试和产测脚本可以直接 import core/service，不启动 GUI。
- `MemorySerialTransport` 必须持续保留，保证无硬件环境也能验证发送、接收、过滤、按钮动作。
- 真实串口实现只作为 `SerialTransport` 的一个适配器，替换 transport 不影响 GUI 和 core。
- JSON profile 是 core 层能力，GUI 只调用 load/save，不直接拼 JSON。
- GUI 控件状态与底层 profile 之间要有明确同步边界，避免界面状态成为唯一真相。

当前先落地：

```text
tools/xy_host_tools/
├── __init__.py
├── serial_config.py      # 配置模型：全局 profile、窗口 profile、按钮、过滤器
├── serial_filter.py      # 过滤规则匹配/颜色决策
├── serial_actions.py     # 文本/HEX/脚本输出动作
├── serial_profile.py     # 标准库 JSON profile 读写
├── serial_transport.py   # 串口 transport 抽象 + 内存 transport 测试替身
├── serial_service.py     # GUI/CLI 共用应用服务层
├── serial_cli.py         # 简单 CLI 骨架
└── gui/
    └── z_serial_app.py   # 后续新增：GUI shell，只调用 service/core

tools/tests/
├── test_serial_filter.py
├── test_serial_profile.py
├── test_serial_actions.py
├── test_serial_transport.py
├── test_serial_service.py
└── test_serial_cli.py
```

## 5. 窗口模型

### 5.1 全局配置 `SerialWorkspaceProfile`

```json
{
  "schema": "xinyi.serial.workspace.v1",
  "name": "XinYi Debug",
  "global_filters": [
    {
      "name": "error",
      "keywords": ["ERROR", "FAIL", "HardFault"],
      "match": "any",
      "foreground": "white",
      "background": "red",
      "action": "highlight",
      "priority": 100
    }
  ],
  "global_buttons": [
    {
      "name": "version",
      "label": "版本",
      "mode": "text",
      "payload": "version\r\n"
    }
  ]
}
```

作用：

- 新建串口窗口时复制这些配置。
- 后续修改全局配置时，可选择：
  - 仅影响新窗口。
  - 同步到所有窗口。
  - 只同步未被窗口本地覆盖的规则。

### 5.2 串口窗口配置 `SerialWindowProfile`

每个串口窗口包含：

- `window_id`
- `title`
- `port`
- `baudrate`
- `local_filters`
- `disabled_filter_names`
- `local_buttons`
- `inherit_global_filters`
- `inherit_global_buttons`

合成规则：

```text
有效过滤器 = 全局过滤器 - disabled_filter_names + 本地过滤器
有效按钮   = 全局按钮 + 本地按钮
```

同名本地过滤器覆盖全局过滤器。

## 6. 过滤器设计

### 6.1 过滤规则字段

```python
FilterRule(
    name="error",
    keywords=("ERROR", "FAIL", "HardFault"),
    match="any",              # any/all/sequence/regex
    case_sensitive=False,
    foreground="white",
    background="red",
    action="highlight",       # highlight/hide/pin/count
    enabled=True,
)
```

### 6.2 关键字组合

| match | 含义 | 示例 |
|---|---|---|
| `any` | 任意关键字命中 | ERROR 或 FAIL |
| `all` | 所有关键字都必须出现 | UART + timeout |
| `sequence` | 按顺序出现 | `boot` 后面出现 `ok` |
| `regex` | 正则匹配 | `temp=\d+\.\d+` |

### 6.3 多规则决策

一行日志可以命中多条规则。建议：

1. `hide` 优先级最高。
2. 多个 `highlight` 命中时，采用优先级最高的规则。
3. 若优先级相同，采用更靠后的规则，便于用户覆盖。
4. 命中结果保留全部规则名，方便统计。

### 6.4 过滤结果

```python
FilterResult(
    visible=True,
    foreground="white",
    background="red",
    matched_rules=("error",),
)
```

## 7. 底部按钮设计

### 7.1 按钮字段

```python
ActionButton(
    name="read_version",
    label="版本",
    mode="text",              # text/hex/script
    payload="version\r\n",
    append_newline=False,
    confirm=False,
)
```

### 7.2 输出模式

| mode | 输入 | 输出 |
|---|---|---|
| `text` | `AT+RST\r\n` | UTF-8 bytes |
| `hex` | `01 03 00 00` | bytes |
| `script` | Python 表达式/函数 | bytes |

### 7.3 脚本模式边界

脚本能力很强，但必须控制边界：

- MVP 只允许无外部 import 的简单表达式或 `return "..."`。
- 上下文只提供安全变量：`port`、`window_id`、`now`、`last_rx`。
- 脚本输出可以是 `str` 或 `bytes`。
- 不允许文件/网络/系统命令，避免按钮变成任意执行入口。

## 8. GUI 草图

```text
┌─ z-serial ───────────────────────────────────────────────────────────────┐
│ Global: filters=5 buttons=8     +Open Port   Save Profile   Settings   │
├─ Tabs ─────────────────────────────────────────────────────────────────┤
│ [U5-Debug /dev/ttyUSB0] [4G /dev/ttyUSB1] [GPS /dev/ttyACM0] [+]        │
├─ Window: U5-Debug ─────────────────────────────────────────────────────┤
│ Port /dev/ttyUSB0  Baud 115200  8N1  DTR:on RTS:off   ● Open           │
├─ RX ───────────────────────────────────────────────────────────────────┤
│ 12:00:01.001  Boot FW=0.1.0                                             │
│ 12:00:01.102  ERROR sensor timeout                                      │
│ 12:00:02.000  net connected                                             │
├─ Local Filter Bar ─────────────────────────────────────────────────────┤
│ [x] inherit global  [error:red] [boot:cyan] [+local rule]               │
├─ TX ───────────────────────────────────────────────────────────────────┤
│ > version\r\n                                                          │
├─ Buttons ──────────────────────────────────────────────────────────────┤
│ [复位] [版本] [进入 Boot] [读传感器] [清屏] [+]                         │
└────────────────────────────────────────────────────────────────────────┘
```

## 9. Profile 文件格式

第一版明确采用 **JSON**，原因：

- Python 标准库原生支持，当前不增加 YAML 依赖。
- 适合 GUI 直接读写，便于后续做 schema 校验和迁移。
- 已有 `serial_profile.py` 和单元测试验证 JSON roundtrip。
- 后续如果用户编辑体验需要，可再增加 YAML import/export，但内部 canonical format 仍保持 JSON。

```json
{
  "schema": "xinyi.serial.workspace.v1",
  "name": "XinYi U5 Lab",
  "windows": [
    {
      "window_id": "u5",
      "title": "U5 Debug",
      "port": "/dev/ttyUSB0",
      "baudrate": 115200,
      "inherit_global_filters": true,
      "inherit_global_buttons": true,
      "disabled_filter_names": [],
      "local_filters": [],
      "local_buttons": []
    }
  ],
  "global_filters": [
    {
      "name": "error",
      "keywords": ["ERROR", "FAIL", "HardFault"],
      "match": "any",
      "case_sensitive": false,
      "foreground": "white",
      "background": "red",
      "action": "highlight",
      "priority": 100,
      "enabled": true
    }
  ],
  "global_buttons": [
    {
      "name": "reset",
      "label": "复位",
      "mode": "text",
      "payload": "reset\r\n",
      "append_newline": false,
      "confirm": false
    }
  ]
}
```

## 10. MVP 交付边界

第一版已落地：

1. 配置模型。
2. 过滤规则匹配。
3. 全局/窗口过滤器继承与覆盖。
4. 按钮 text/hex/script 输出转换。
5. 标准库 JSON profile 读写。
6. 串口 transport 抽象和内存 transport 测试替身。
7. GUI/CLI 共用 `serial_service.py` 应用服务层。
8. 单元测试。
9. 简单 CLI demo：过滤模拟日志、列出串口、生成 sample profile、通过内存 transport 发送按钮 payload。

当前代码仍暂不做：

- 真串口读写。
- 复杂脚本沙箱。
- 配置编辑器。
- 协议解析。

下一步优先做 GUI 原型，而不是 TUI。

GUI 原型必须调用 `serial_service.py`，不能绕过 service 直接操作过滤器、按钮或 transport。

这样可以先把最关键的“多窗口配置继承 + 强过滤 + 按钮输出”核心打稳。

## 11. 后续迭代

### v0.2

- PySide6/PyQt6 GUI 原型：主窗口、串口 tab、接收区、输入区、按钮区、全局设置入口。
- 接入 pyserial。
- `z-serial` 启动入口。
- JSON profile 打开/保存。

### v0.3

- 多窗口 tabs。
- 实时过滤和颜色显示。
- 过滤器/按钮 GUI 编辑器。

### v0.4

- Modbus/AT/NMEA 插件。
- 日志 JSONL 保存与回放。

### v1.0

- 稳定 GUI。
- 量产 profile。
- 固件升级工具联动。
