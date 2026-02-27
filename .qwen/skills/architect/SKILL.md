# 首席架构师 Skill

**名称**: architect

**角色**: 技术架构决策、系统设计、技术规范制定

**职责**:
- 系统架构设计与评审
- 技术选型与决策
- 代码规范制定
- 架构文档维护
- 技术难点攻关

---

## 使用方式

```bash
# 查看架构状态
/skill architect status

# 查看架构决策
/skill architect decisions

# 查看技术规范
/skill architect specs

# 请求架构评审
/skill architect review <component>

# 生成架构报告
/skill architect report
```

---

## 架构文档

### 系统架构

```
┌─────────────────────────────────────────┐
│           应用层 (Projects)              │
└─────────────────────────────────────────┘
                    ▲
┌─────────────────────────────────────────┐
│           组件层 (Components)            │
│  Crypto | Network | Sensor | Data Mgmt  │
└─────────────────────────────────────────┘
                    ▲
┌─────────────────────────────────────────┐
│        OS 抽象层 (OSAL)                  │
│  FreeRTOS | RT-Thread | Bare-Metal      │
└─────────────────────────────────────────┘
                    ▲
┌─────────────────────────────────────────┐
│       硬件抽象层 (HAL)                   │
│  UART | SPI | I2C | GPIO | Timer | ...  │
└─────────────────────────────────────────┘
                    ▲
┌─────────────────────────────────────────┐
│          平台层 (MCU SDK)                │
│  STM32 | HC32 | WCH | PC Sim            │
└─────────────────────────────────────────┘
```

### 技术栈

| 层次 | 技术选型 | 状态 |
|------|----------|------|
| **RTOS** | FreeRTOS/RT-Thread/CMSIS-RTX | ✅ |
| **HAL** | 自研 xy_hal | ✅ |
| **通信** | MQTT/Modbus/AT | ✅ |
| **加密** | AES/HMAC/SHA | ✅ |
| **构建** | CMake/Kconfig/Make | ✅ |

---

## 架构决策记录 (ADR)

### ADR-001: 选择 CMSIS-RTOS2 作为 OSAL 标准接口

**日期**: 2025-10-27  
**状态**: 已采纳  
**影响**: 
- 统一的 RTOS API
- 支持多后端切换
- 降低应用层耦合

### ADR-002: 采用 third_party 管理第三方库

**日期**: 2026-02-28  
**状态**: 已采纳  
**影响**:
- 清晰分离自有代码和第三方代码
- 便于版本管理和更新
- 许可证合规性更好

### ADR-003: 测试框架统一使用 Unity

**日期**: 2026-02-28  
**状态**: 已采纳  
**影响**:
- 统一测试风格
- 降低维护成本
- 便于 CI/CD 集成

---

## 技术规范

### 代码规范

- 遵循 [xy_code_style.md](docs/rules/100-code_style/xy_code_style.md)
- 使用 clang-format 自动格式化
- 所有函数必须有 Doxygen 注释

### 目录规范

```
components/<component>/
├── include/          # 公共头文件
├── src/              # 源文件
├── tests/            # 单元测试
├── docs/             # 文档
├── CMakeLists.txt    # 构建配置
└── README.md         # 说明
```

### 命名规范

- 头文件：`xy_<module>.h`
- 源文件：`xy_<module>.c`
- 函数：`xy_<module>_<function>()`
- 类型：`xy_<module>_<type>_t`

---

## 架构检查清单

### 新组件接入

- [ ] 目录结构符合规范
- [ ] CMakeLists.txt 配置正确
- [ ] Kconfig 配置选项
- [ ] README.md 文档完整
- [ ] 单元测试覆盖率 >80%
- [ ] Doxygen API 文档

### 架构评审

- [ ] 模块职责单一
- [ ] 接口设计合理
- [ ] 依赖关系清晰
- [ ] 错误处理完善
- [ ] 日志系统健全
- [ ] 性能满足要求

---

## 相关文件

- [组件状态](../../COMPONENTS_STATUS.md)
- [构建系统分析](../../docs/build_system_analysis.md)
- [代码规范](../../docs/rules/100-code_style/xy_code_style.md)

---

**维护者**: 首席架构师  
**更新频率**: 架构变更时
