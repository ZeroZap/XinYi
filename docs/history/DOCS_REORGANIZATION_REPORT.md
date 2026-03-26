# Docs 目录整理报告

**整理日期**: 2026-02-28  
**整理状态**: 第一阶段完成

---

## 📊 当前状态

### 目录结构

```
docs/
├── README.md                    ✅ 文档索引（新增）
├── about/                       ⚠️ 部分完成
│   ├── index.md                 ✅
│   ├── faq.md                   ✅
│   └── changelog.md             ✅
├── api/                         ❌ 空白
│   └── index.md                 ⚠️ 仅有索引，无实际 API 文档
├── components/                  ✅ 完成 (15 个组件)
├── contribute/                  ✅ 完成
├── design/                      ✅ 完成 (6 个设计文档)
├── doxygen/                     ⚠️ 仅有配置
├── getting-started/             ⚠️ 部分完成
│   ├── quickstart.md            ✅
│   ├── introduction.md          ❌ 缺失（但被引用）
│   └── toolchain.md             ❌ 缺失（但被引用）
├── hardware/                    ⚠️ 部分完成
│   ├── index.md                 ✅
│   └── rtos_selection_guide.md  ✅
├── misc/                        ⚠️ 杂项堆积
├── rules/                       ✅ 完成
├── samples/                     ⚠️ 有示例但分散
└── toolchain/                   ⚠️ 部分完成
```

---

## 🔍 发现的问题

### 1. 重复内容

| 重复文档 | 位置 1 | 位置 2 | 建议 |
|---------|-------|-------|------|
| 首页欢迎页 | `docs/misc/index.md` | `docs/README.md` | 保留 `docs/README.md`，删除 `docs/misc/index.md` |
| 项目概述 | `docs/about/index.md` | `docs/design/XinYi_Project_Overview.md` | 合并到 `docs/about/index.md` |
| 示例代码 | `docs/samples/index.md` | `projects/Demo/main.c` | 保持独立，samples 为通用示例 |

### 2. 缺失文档（但被引用）

| 缺失文档 | 被谁引用 | 优先级 | 建议操作 |
|---------|---------|--------|---------|
| `getting-started/introduction.md` | misc/index.md, samples/index.md | 🔴 高 | 创建或修改引用 |
| `getting-started/toolchain.md` | misc/index.md | 🟡 中 | 创建或修改引用 |
| `components/*/api-reference.md` | api/index.md (所有组件) | 🔴 高 | 创建或修改引用 |
| `components/hal/drivers.md` | misc/index.md | 🟡 中 | 创建或修改引用 |
| `hardware/boards.md` | misc/index.md | 🟢 低 | 创建或修改引用 |
| `hardware/porting.md` | misc/index.md | 🟢 低 | 创建或修改引用 |
| `hardware/device-tree.md` | misc/index.md | 🟢 低 | 创建或修改引用 |
| `contribute/submit-pr.md` | misc/index.md | 🟡 中 | 创建或修改引用 |

### 3. 空白/未实现目录

| 目录 | 状态 | 问题 | 建议 |
|------|------|------|------|
| `docs/api/` | ❌ 空白 | 只有索引，无实际 API 文档 | 使用 Doxygen 生成或手动创建 |
| `docs/misc/` | ⚠️ 堆积 | 杂项文档过多 | 进一步分类或移动到 design/ |

### 4. 引用链接检查

**失效链接**:
- `[文档索引](index.md)` → 应为 `[文档索引](README.md)`
- `[项目简介](getting-started/introduction.md)` → 文件不存在
- `[详细 API](../components/osal/api-reference.md)` → 文件不存在

---

## ✅ 已完成的整理

### 第一阶段（已完成）

- [x] 移动杂项文件到 `docs/misc/`
- [x] 移动 AI 相关到 `docs/misc/ai-prompts/` 和 `docs/misc/ai-skills/`
- [x] 移动设计文档到 `docs/design/`
- [x] 移动工具链文档到 `docs/toolchain/`
- [x] 移动硬件文档到 `docs/hardware/`
- [x] 清理空目录 `requirements/`
- [x] 创建 `docs/README.md` 索引

### 第二阶段（待完成）

- [ ] 删除重复的 `docs/misc/index.md`
- [ ] 合并 `docs/about/index.md` 和 `docs/design/XinYi_Project_Overview.md`
- [ ] 创建缺失的 `getting-started/introduction.md`
- [ ] 修复所有失效链接
- [ ] 生成或创建 API 文档
- [ ] 进一步整理 `docs/misc/`

---

## 📋 建议的下一步行动

### 高优先级

1. **修复失效链接**
   - 修改 `misc/index.md` 中的引用
   - 统一使用 `README.md` 作为索引

2. **创建缺失文档**
   - `getting-started/introduction.md` - 项目简介
   - 各组件 `api-reference.md` - API 参考

3. **删除重复文档**
   - 删除 `docs/misc/index.md`（内容与 README.md 重复）
   - 合并项目概述文档

### 中优先级

4. **整理杂项目录**
   - 移动 `doc_struture.md` 到 `design/`
   - 移动 `嵌入式产品日志设计.md` 到 `design/`

5. **创建必要文档**
   - `hardware/boards.md` - 支持的开发板
   - `contribute/submit-pr.md` - 提交 PR 指南

### 低优先级

6. **生成 API 文档**
   - 配置 Doxygen 自动生成
   - 或手动创建关键 API 文档

---

## 📊 文档统计

| 类别 | 已有 | 缺失 | 总计 |
|------|------|------|------|
| 设计文档 | 6 | 0 | 6 |
| 组件文档 | 15 | 0 | 15 |
| API 文档 | 0 | 15 | 15 |
| 快速开始 | 1 | 2 | 3 |
| 硬件文档 | 1 | 3 | 4 |
| 杂项文档 | 6 | 0 | 6 |
| **总计** | **29** | **20** | **49** |

---

**维护者**: XinYi Team  
**整理日期**: 2026-02-28
