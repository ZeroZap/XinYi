# 更新日志

XinYi 嵌入式框架的版本更新历史。

---

## [Unreleased] - 2026-02-28

### Added
- 添加网页文档规划方案
- 创建 MkDocs 文档框架
- 添加组件文档模板（OSAL/HAL/Crypto）
- 添加贡献指南和代码风格文档
- 添加关于页面和常见问题

### Changed
- 更新 COMPONENTS_STATUS.md 添加 pid/addc 状态
- 完善文档目录结构

### Fixed
- 修复文档链接和格式问题

---

## [1.0.0] - 2026-02-28

### Added - 第一阶段

#### 测试系统
- **crypto 组件测试** (28 用例)
  - CRC/MD5/SHA256/AES/Base64/Hex/HMAC/随机数
- **clib 组件测试** (21 用例)
  - 滤波/排序/数学/字符串操作
- **trace 组件测试** (10 用例)
  - 日志级别/函数/宏

#### 代码实现
- 创建 `xy_tiny_crypto.h` 统一头文件

#### 工具脚本
- `format_code.sh` - 代码格式化
- `check_style.sh` - 风格检查
- `run_tests.sh` - 运行测试
- `coverage.sh` - 覆盖率报告
- `build.bat` / `test.bat` - Windows 脚本

#### 文档
- `TEST_IMPROVEMENTS_2026-02-28.md` - 第一阶段总结
- `COMPONENTS_STATUS.md` - 组件状态表

---

### Added - 第二阶段

#### 测试系统
- **dm 组件测试** (24 用例)
  - TLV 编码/解码/迭代器/查找/验证
- **net 组件测试** (22 用例)
  - ISO7816 协议/Modbus RTU
- **sensor 组件测试** (18 用例)
  - 传感器类型/数据结构/配置
- **ipc 组件测试** (14 用例)
  - Pipe 管道/Observer 观察者
- **pm 组件测试** (19 用例)
  - Charger/Fuel Gauge

#### 代码实现
- `components/ipc/pipe/xy_pipe.c/h` - 管道通信
- `components/ipc/observer/xy_observer.c/h` - 观察者模式
- `components/pm/charger/xy_charger.h` - 充电管理
- `components/pm/fuel-gauge/xy_fuel_gauge.h` - 电量计量

#### 文档
- `WORK_SUMMARY_PHASE2_2026-02-28.md` - 第二阶段总结

---

### Added - 第三阶段

#### 测试系统
- **hal 组件测试** (11 用例)
  - 版本宏/错误码/Handle 结构/子模块

#### CI/CD
- `.github/workflows/ci-cd.yml` - GitHub Actions 工作流
  - 多平台构建 (Ubuntu/Windows/macOS)
  - 自动化测试
  - 代码质量检查
  - API 文档生成

#### 文档
- `WORK_SUMMARY_FINAL_2026-02-28.md` - 最终总结

---

### Added - 第四阶段

#### 测试系统
- **pid 组件测试** (20 用例)
  - PID 控制器初始化/增益/计算/重置
- **addc 组件测试** (24 用例)
  - ADC/DAC 初始化/采样/转换

#### 代码实现
- `components/pid/xy_pid.c/h` - PID 控制器
- `components/addc/xy_adc.c/h` - ADC/DAC 辅助库
- `components/fota/xy_fota.h` - FOTA 框架
- `components/gui/xy_gui.h` - GUI 框架

#### 文档
- `WORK_SUMMARY_COMPLETE_2026-02-28.md` - 完整总结

---

### Added - 第五阶段

#### 工具脚本
- `build.sh` - Linux/macOS 构建脚本

#### 文档系统
- `docs/web_documentation_plan.md` - 网页文档规划
- `docs/mkdocs.yml` - MkDocs 配置
- `docs/index.md` - 网站首页
- `docs/components/index.md` - 组件状态
- `docs/getting-started/quickstart.md` - 快速开始
- `docs/components/osal/introduction.md` - OSAL 简介
- `docs/components/hal/introduction.md` - HAL 简介
- `docs/components/crypto/introduction.md` - Crypto 简介
- `docs/contribute/index.md` - 贡献指南
- `docs/contribute/code-style.md` - 代码风格
- `docs/about/index.md` - 关于页面
- `docs/about/faq.md` - 常见问题
- `docs/about/changelog.md` - 更新日志（本文档）

#### CI/CD
- `.github/workflows/deploy-docs.yml` - 文档部署工作流

#### 总结文档
- `README_ONE_DAY.md` - 一天工作成果汇总
- `WORK_SUMMARY_ULTIMATE_2026-02-28.md` - 最终极总结

---

## [0.9.0] - 2026-02-27

### Added
- 完整的项目管理和自动化系统
- STM32U5 子模块

---

## 版本说明

### 版本号规则

采用语义化版本号：`MAJOR.MINOR.PATCH`

- **MAJOR**: 不兼容的 API 变更
- **MINOR**: 向后兼容的功能新增
- **PATCH**: 向后兼容的问题修复

### 状态标识

| 标识 | 说明 |
|------|------|
| 🟢 完善 | 代码 + 测试 + 文档完整 |
| 🟡 进行中 | 开发中 |
| 🟠 基础 | 基础框架 |
| 🔴 缺失 | 待开发 |

---

## 📊 统计趋势

| 版本 | 测试用例 | 代码文件 | 文档文件 | 提交数 |
|------|---------|---------|---------|--------|
| 0.9.0 | 17 | 0 | 1 | 2 |
| 1.0.0 | 228 | 14 | 15+ | 17+ |

---

## 🔗 相关链接

- [GitHub Releases](https://github.com/ZeroZap/XinYi/releases)
- [贡献指南](../contribute/index.md)
- [快速开始](../getting-started/quickstart.md)

---

*最后更新：2026-02-28 | 维护者：XinYi Team*
