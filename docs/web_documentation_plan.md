# XinYi 嵌入式框架 - 网页文档规划方案

**版本**: 1.0  
**日期**: 2026-02-28  
**参考**: Zephyr RTOS 文档结构

---

## 📋 一、文档网站结构

### 1.1 顶部主导航 (Top Navigation)

```
┌─────────────────────────────────────────────────────────────────┐
│  XinYi Docs    首页 | 快速开始 | 组件 | API | 示例 | 工具 | 关于  │
│                                                                 │
│  [搜索框 🔍]  [版本 v1.0 ▼]  [PDF 📄]  [GitHub 🔗]             │
└─────────────────────────────────────────────────────────────────┘
```

### 1.2 导航菜单详细结构

```
主导航
├── 首页 (Home)
├── 快速开始 (Getting Started)
│   ├── 简介
│   ├── 特性一览
│   ├── 快速上手 (5 分钟)
│   └── 开发环境搭建
│
├── 组件文档 (Components)
│   ├── OSAL (OS 抽象层)
│   ├── HAL (硬件抽象层)
│   ├── Crypto (密码学)
│   ├── CLib (C 库)
│   ├── DM (数据管理)
│   ├── NET (网络协议)
│   ├── Sensor (传感器)
│   ├── IPC (进程间通信)
│   ├── PM (电源管理)
│   ├── PID (控制算法)
│   ├── ADDC (ADC/DAC)
│   ├── FOTA (固件升级)
│   └── GUI (图形界面)
│
├── API 参考 (API Reference)
│   ├── 按组件分类
│   ├── 按功能分类
│   └── 索引
│
├── 示例代码 (Samples)
│   ├── Hello World
│   ├── 组件示例
│   └── 综合项目
│
├── 工具链 (Toolchain)
│   ├── 构建系统
│   ├── 测试框架
│   └── CI/CD
│
└── 关于 (About)
    ├── 项目简介
    ├── 贡献指南
    └── 更新日志
```

### 1.3 左侧边栏 (Side Navigation)

```
┌─────────────────────────┐
│ 📚 组件文档             │
├─────────────────────────┤
│ ▼ OSAL                  │
│   ├── 简介              │
│   ├── 快速开始          │
│   ├── 配置指南          │
│   ├── API 参考          │
│   └── 示例代码          │
├─────────────────────────┤
│ ▼ HAL                   │
│   ├── 简介              │
│   ├── 支持平台          │
│   ├── 外设驱动          │
│   ├── API 参考          │
│   └── 移植指南          │
├─────────────────────────┤
│ ▼ Crypto                │
│   ├── 简介              │
│   ├── 算法列表          │
│   ├── API 参考          │
│   └── 使用示例          │
└─────────────────────────┘
```

---

## 🏗️ 二、页面布局设计

### 2.1 首页布局

```
┌──────────────────────────────────────────────────────────────┐
│  [顶部导航栏]                                                │
├────────────┬─────────────────────────────────────────────────┤
│            │                                                 │
│  [左侧边栏]│  [主内容区]                                     │
│            │                                                 │
│  组件导航  │  ┌───────────────────────────────────────────┐ │
│            │  │  🎉 XinYi 嵌入式框架 v1.0                  │ │
│  参考文档  │  │  模块化、生产级的嵌入式 C 框架               │ │
│            │  └───────────────────────────────────────────┘ │
│            │                                                 │
│            │  ┌───────────────────────────────────────────┐ │
│            │  │  📖 快速入口                              │ │
│            │  │  [简介] [快速开始] [组件] [示例]          │ │
│            │  └───────────────────────────────────────────┘ │
│            │                                                 │
│            │  ┌───────────────────────────────────────────┐ │
│            │  │  🧩 核心组件 (12 个完善组件)               │ │
│            │  │  - OSAL: 4 种 RTOS 支持                    │ │
│            │  │  - HAL: STM32 完整实现                    │ │
│            │  │  - Crypto: AES/SHA/CRC 等                 │ │
│            │  │  - ...                                    │ │
│            │  └───────────────────────────────────────────┘ │
│            │                                                 │
│            │  ┌───────────────────────────────────────────┐ │
│            │  │  📊 项目统计                              │ │
│            │  │  228 个测试用例 | 14 个代码文件 | 100% 开源  │ │
│            │  └───────────────────────────────────────────┘ │
│            │                                                 │
│            │  ┌───────────────────────────────────────────┐ │
│            │  │  🚀 选择你的起点                          │ │
│            │  │  [新手入门] [组件开发] [硬件移植] [贡献]  │ │
│            │  └───────────────────────────────────────────┘ │
├────────────┴─────────────────────────────────────────────────┤
│  [页脚]  Apache 2.0 | GitHub | 最后更新：2026-02-28          │
└──────────────────────────────────────────────────────────────┘
```

### 2.2 组件页面布局

```
┌──────────────────────────────────────────────────────────────┐
│  Components » Crypto » 简介                                  │
├────────────┬─────────────────────────────────────────────────┤
│            │                                                 │
│  Crypto    │  # Crypto 组件                                  │
│  ├── 简介  │                                                 │
│  ├── 快速  │  ## 概述                                        │
│  ├── 配置  │  XinYi Crypto 提供轻量级加密算法...             │
│  ├── API   │                                                 │
│  └── 示例  │  ## 特性                                        │
│            │  - ✅ AES-128/192/256                          │
│  其他组件  │  - ✅ SHA-256/MD5                              │
│  ▼ OSAL    │  - ✅ CRC/Base64                               │
│  ▼ HAL     │  - ✅ HMAC                                     │
│  ▼ DM      │                                                 │
│  ▼ NET     │  ## 快速开始                                    │
│            │  ```c                                           │
│            │  #include "xy_tiny_crypto.h"                   │
│            │  ...                                            │
│            │  ```                                            │
│            │                                                 │
│            │  ## API 参考                                    │
│            │  | 函数 | 说明 |                                │
│            │  |--------|------|                              │
│            │  | xy_aes_init | 初始化 AES |                   │
│            │  | xy_md5_hash | MD5 哈希 |                     │
│            │                                                 │
│            │  [编辑此页] [报告问题]                          │
└────────────┴─────────────────────────────────────────────────┘
```

---

## 📂 三、文档目录结构

### 3.1 推荐目录结构

```
docs/
├── index.md                      # 首页
├── getting-started/              # 快速开始
│   ├── index.md
│   ├── introduction.md
│   ├── features.md
│   ├── quickstart.md
│   └── toolchain.md
│
├── components/                   # 组件文档
│   ├── index.md
│   ├── osal/
│   │   ├── index.md
│   │   ├── introduction.md
│   │   ├── quickstart.md
│   │   ├── configuration.md
│   │   ├── api-reference.md
│   │   └── examples.md
│   ├── hal/
│   ├── crypto/
│   ├── clib/
│   ├── dm/
│   ├── net/
│   ├── sensor/
│   ├── ipc/
│   ├── pm/
│   ├── pid/
│   ├── addc/
│   ├── fota/
│   └── gui/
│
├── api/                          # API 参考
│   ├── index.md
│   ├── by-component.md
│   ├── by-function.md
│   └── glossary.md
│
├── samples/                      # 示例代码
│   ├── index.md
│   ├── hello-world.md
│   ├── component-examples.md
│   └── projects.md
│
├── toolchain/                    # 工具链
│   ├── index.md
│   ├── build-system.md
│   ├── test-framework.md
│   └── cicd.md
│
├── hardware/                     # 硬件支持
│   ├── index.md
│   ├── supported-boards.md
│   ├── porting-guide.md
│   └── device-tree.md
│
├── contribute/                   # 贡献指南
│   ├── index.md
│   ├── code-style.md
│   ├── submit-pr.md
│   └── bug-report.md
│
└── about/                        # 关于
    ├── index.md
    ├── changelog.md
    ├── roadmap.md
    └── team.md
```

---

## 🎨 四、视觉设计规范

### 4.1 配色方案

```css
:root {
  /* 主色调 - 科技蓝 */
  --primary-color: #2563eb;
  --primary-hover: #1d4ed8;
  
  /* 辅助色 */
  --success-color: #10b981;
  --warning-color: #f59e0b;
  --error-color: #ef4444;
  
  /* 中性色 */
  --text-primary: #1f2937;
  --text-secondary: #6b7280;
  --bg-primary: #ffffff;
  --bg-secondary: #f9fafb;
  --border-color: #e5e7eb;
  
  /* 代码块 */
  --code-bg: #1f2937;
  --code-text: #f3f4f6;
}
```

### 4.2 组件状态标识

| 状态 | 图标 | 颜色 | 说明 |
|------|------|------|------|
| ✅ 完善 | 🟢 | 绿色 | 代码 + 测试 + 文档完整 |
| 🔄 进行中 | 🟡 | 黄色 | 开发中 |
| 📋 基础 | 🟠 | 橙色 | 基础框架 |
| ❌ 缺失 | 🔴 | 红色 | 待开发 |

---

## 🛠️ 五、技术选型建议

### 5.1 推荐方案 (基于 MkDocs)

```yaml
# mkdocs.yml
site_name: XinYi Docs
site_url: https://xinyi.zerovoid.com/
repo_url: https://github.com/ZeroZap/XinYi

theme:
  name: material
  features:
    - navigation.tabs
    - navigation.sections
    - navigation.expand
    - search.suggest
    - search.highlight
    - content.code.copy

plugins:
  - search
  - minify
  - git-revision-date

markdown_extensions:
  - admonition
  - codehilite
  - toc:
      permalink: true
```

### 5.2 备选方案

| 方案 | 工具 | 优点 | 缺点 |
|------|------|------|------|
| **方案 A** | MkDocs + Material | 简单快速 | 自定义有限 |
| **方案 B** | Docusaurus | React 生态 | 学习曲线 |
| **方案 C** | VuePress | Vue 生态 | 社区较小 |
| **方案 D** | Sphinx | Python 生态 | 配置复杂 |

---

## 📊 六、内容优先级

### 6.1 第一阶段 (1 周)

- [ ] 首页设计
- [ ] 快速开始文档
- [ ] 12 个完善组件文档
- [ ] API 参考框架

### 6.2 第二阶段 (1 周)

- [ ] 示例代码文档
- [ ] 工具链文档
- [ ] 硬件支持文档
- [ ] 贡献指南

### 6.3 第三阶段 (1 周)

- [ ] 搜索功能
- [ ] 版本管理
- [ ] PDF 导出
- [ ] 多语言支持

---

## 🎯 七、关键页面示例

### 7.1 组件状态总览页

```markdown
# 组件状态总览

| 组件 | 代码 | 测试 | 文档 | 构建 | 状态 |
|------|------|------|------|------|------|
| OSAL | ✅ | ✅ | ✅ | ✅ | 🟢 完善 |
| HAL | ✅ | ✅ | ✅ | ✅ | 🟢 完善 |
| Crypto | ✅ | ✅ | ✅ | ✅ | 🟢 完善 |
| ... | ... | ... | ... | ... | ... |

## 统计
- 🟢 完善：12 个
- 🟡 进行中：0 个
- 🟠 基础：2 个
- 🔴 缺失：0 个
```

### 7.2 API 参考页

```markdown
# API 参考

## 按组件分类

### OSAL
- `xy_os_kernel_init()` - 初始化内核
- `xy_os_delay()` - 延时函数
- ...

### Crypto
- `xy_aes_init()` - 初始化 AES
- `xy_md5_hash()` - MD5 哈希
- ...

## 索引
[A](#) [B](#) [C](#) ... [X](#) [Y](#) [Z](#)
```

---

## 📈 八、SEO 与可访问性

### 8.1 SEO 优化

- 语义化 HTML 标签
- Meta 描述和关键词
- Open Graph 标签
- sitemap.xml
- robots.txt

### 8.2 可访问性

- ARIA 标签
- 键盘导航
- 高对比度模式
- 字体大小调整

---

## 🚀 九、部署方案

### 9.1 GitHub Pages

```yaml
# .github/workflows/deploy-docs.yml
name: Deploy Docs
on:
  push:
    branches: [main]
jobs:
  deploy:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/setup-python@v5
      - run: pip install mkdocs-material
      - run: mkdocs gh-deploy --force
```

### 9.2 自定义域名

```
docs.xinyi.com → GitHub Pages
或
xinyi.zerovoid.com → Vercel/Netlify
```

---

## 📋 十、检查清单

### 上线前检查

- [ ] 所有组件文档完整
- [ ] API 参考生成
- [ ] 搜索功能正常
- [ ] 移动端适配
- [ ] 404 页面配置
- [ ] 统计代码集成
- [ ] 域名解析配置

---

**维护者**: XinYi Team  
**日期**: 2026-02-28  
**许可证**: Apache License 2.0
