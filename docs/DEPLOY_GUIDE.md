# XinYi 文档部署指南

## 本地预览

```bash
cd docs
pip install mkdocs mkdocs-material
mkdocs serve
# 访问 http://127.0.0.1:8000
```

## GitHub Pages 部署

```bash
# 安装依赖
pip install mkdocs mkdocs-material

# 部署到 GitHub Pages
mkdocs gh-deploy --force
```

## 文档结构

```
docs/
├── index.md                    # 首页
├── getting-started/
│   ├── QUICK_START.md         # 快速入门 ✅
│   └── installation.md
├── components/
│   ├── osal.md
│   ├── hal.md
│   └── device.md
├── COMPONENT_STATUS_REPORT.md  # 组件状态报告 ✅
└── api/                        # API 参考
```

## 自动化部署 (GitHub Actions)

创建 `.github/workflows/docs.yml`:

```yaml
name: Deploy Docs
on:
  push:
    branches: [main]
    paths: ['docs/**']

jobs:
  deploy:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - uses: actions/setup-python@v4
        with:
          python-version: 3.x
      - run: pip install mkdocs mkdocs-material
      - run: mkdocs gh-deploy --force
```

## 文档更新清单

- [x] 快速入门指南 (QUICK_START.md)
- [x] 组件状态报告 (COMPONENT_STATUS_REPORT.md)
- [ ] API 参考文档 (Doxygen 生成)
- [ ] 教程系列
- [ ] 示例代码文档

---

*XinYi Documentation Team*
