# 模块开发工程师 Skill

**名称**: developer

**角色**: 模块代码开发、单元测试编写、文档维护

**职责**:
- 模块功能开发
- 单元测试编写
- 代码自审
- 文档编写
- Bug 修复

---

## 使用方式

```bash
# 查看开发任务
/skill developer tasks

# 查看编码规范
/skill developer specs

# 请求代码审查
/skill developer review <module>

# 生成开发报告
/skill developer report
```

---

## 开发流程

### 1. 接收任务

从项目管理查看分配的任务：
```bash
/skill project-manager tasks
```

### 2. 开发前准备

- [ ] 阅读相关文档
- [ ] 确认接口定义
- [ ] 准备测试环境

### 3. 编码实现

遵循 [代码规范](../../docs/rules/100-code_style/xy_code_style.md):
- 4 空格缩进
- 小写 + 下划线命名
- 所有函数有 Doxygen 注释

### 4. 自测

- [ ] 编译无警告
- [ ] 单元测试通过
- [ ] 代码格式化

### 5. 提交审查

- [ ] 代码自审完成
- [ ] 测试覆盖率达标
- [ ] 文档更新

---

## 开发模板

### 头文件模板

```c
/**
 * @file xy_<module>.h
 * @brief <Module> Hardware Abstraction Layer
 * @version 1.0.0
 * @date YYYY-MM-DD
 */

#ifndef XY_<MODULE>_H
#define XY_<MODULE>_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_hal.h"
#include <stdint.h>

/**
 * @brief <Module> configuration structure
 */
typedef struct {
    uint32_t param1;
    uint32_t param2;
} xy_<module>_config_t;

/**
 * @brief Initialize <module>
 * @param <module> <Module> instance
 * @param config Configuration structure
 * @return XY_HAL_OK on success, negative on error
 */
xy_hal_error_t xy_<module>_init(void *<module>, 
                                 const xy_<module>_config_t *config);

#ifdef __cplusplus
}
#endif

#endif /* XY_<MODULE>_H */
```

### 源文件模板

```c
/**
 * @file xy_<module>.c
 * @brief <Module> HAL Implementation
 * @version 1.0.0
 * @date YYYY-MM-DD
 */

#include "xy_<module>.h"

xy_hal_error_t xy_<module>_init(void *<module>, 
                                 const xy_<module>_config_t *config)
{
    if (!<module> || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* Implementation */
    
    return XY_HAL_OK;
}
```

### 测试文件模板

```c
/**
 * @file test_<module>.c
 * @brief <Module> Unit Tests
 */

#include "unity.h"
#include "xy_<module>.h"

void setUp(void) { }
void tearDown(void) { }

void test_<module>_init_null_param(void)
{
    xy_hal_error_t ret;
    
    ret = xy_<module>_init(NULL, NULL);
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, ret);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_<module>_init_null_param);
    return UNITY_END();
}
```

---

## 检查清单

### 代码提交前

- [ ] 遵循代码规范
- [ ] 使用 clang-format 格式化
- [ ] 所有函数有 Doxygen 文档
- [ ] 使用 xy_log_*() 进行日志记录
- [ ] 实现错误处理
- [ ] 包含单元测试
- [ ] 无编译器警告
- [ ] 在目标平台测试

### 单元测试要求

- [ ] 测试覆盖率 > 80%
- [ ] 边界条件测试
- [ ] 错误路径测试
- [ ] 正常路径测试

---

## 常用命令

```bash
# 编译模块
make <module>

# 运行测试
make test_<module>

# 代码格式化
clang-format -i src/xy_<module>.c

# 生成文档
doxygen docs/doxygen.config
```

---

## 相关文件

- [代码规范](../../docs/rules/100-code_style/xy_code_style.md)
- [组件状态](../../COMPONENTS_STATUS.md)
- [构建指南](../../docs/build_system_analysis.md)

---

**维护者**: 开发团队  
**更新频率**: 开发流程变更时
