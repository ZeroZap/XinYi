# AI 代码审查清单

## 审查依据

代码质量、安全、注释、排版和工具检查统一采用 [code_style.md](../../code_style.md) 的规则等级与第 8 节检查要求。
审查结果应区分强制规则违反、建议偏离和允许的替代方案，不再维护另一套数值阈值。
另外确认本次变更的组件文档、行为测试和构建证据是否同步更新。

## 审查输出格式
```json
{
  "file": "filename.c",
  "score": 85,
  "issues": [
    {
      "line": 42,
      "severity": "warning",
      "rule": "6.6",
      "message": "malloc 返回值未检查",
      "suggestion": "添加 NULL 检查"
    }
  ]
}
```


---
