## 第一部分：Security（安全） - 保护系统免受恶意攻击

### 1. 安全概述

**Security** 关注的是**信息安全和系统保护**，防止**恶意攻击**、**未授权访问**、**数据泄露**和**服务中断**。

### 2. 安全威胁模型

text

```
外部威胁源 → 攻击面 → 漏洞利用 → 资产损害
    │           │           │          │
恶意黑客    网络接口    缓冲区溢出  数据窃取
内部威胁    外设接口    代码注入    系统控制
物理攻击    调试接口    侧信道攻击  服务拒绝
```



### 3. Zephyr 安全架构

#### 3.1 硬件安全特性

c

```
/* 支持的硬件安全特性 */
CONFIG_ARM_TRUSTZONE_M=y           /* ARM TrustZone */
CONFIG_HW_UNIQUE_KEY=y             /* 硬件唯一密钥 */
CONFIG_DEVICE_PROTECTED=y          /* 设备保护 */
CONFIG_SECURE_BOOT=y               /* 安全启动 */
CONFIG_SECURE_FIRMWARE_UPDATE=y    /* 安全固件更新 */
```



#### 3.2 安全启动链

text

```
硬件ROT → Bootloader → Zephyr Kernel → 安全应用
   │          │            │              │
硬件信任根  镜像验签     权限分离     安全服务
   ↓          ↓            ↓              ↓
PUF/OTP    MCUboot      TEE/SE     Crypto服务
```



#### 3.3 关键安全组件

##### 3.3.1 可信执行环境 (TEE)

c

```
/* TF-M (Trusted Firmware-M) 集成 */
CONFIG_TFM=y
CONFIG_TFM_PROFILE_TYPE_MINIMAL=y
CONFIG_TFM_ISOLATION_LEVEL=2

/* 安全服务调用 */
tfm_secure_service_call(TFM_CRYPTO_SID,
                       TFM_CRYPTO_AES_ENCRYPT,
                       &args, sizeof(args));
```



##### 3.3.2 加密子系统

yaml

```
加密服务:
  - 对称加密: AES-128/256, ChaCha20
  - 非对称加密: RSA, ECC, Ed25519
  - 哈希算法: SHA-256, SHA-512
  - 密钥管理: HUK, PSK, 密钥派生
  - 安全存储: 加密文件系统
```



##### 3.3.3 网络安全

kconfig

```
# 网络层安全配置
CONFIG_NET_SOCKETS_SOCKOPT_TLS=y
CONFIG_MBEDTLS=y
CONFIG_MBEDTLS_ENABLE_HEAP=y
CONFIG_MBEDTLS_HEAP_SIZE=8192

# 协议安全
CONFIG_NET_L2_IEEE802154_SECURITY=y
CONFIG_BT_SMP=y
CONFIG_BT_LE_SECURITY_ENABLED=y
```



### 4. 安全功能实现

#### 4.1 内存保护

c

```
/* MPU/MMU 配置示例 */
const struct k_mem_partition app_partitions[] = {
    {
        .start = APP_MEM_START,
        .size = APP_MEM_SIZE,
        .attr = K_MEM_PARTITION_P_RW_U_RW,
    },
    {
        .start = SECURE_DATA_START,
        .size = SECURE_DATA_SIZE,
        .attr = K_MEM_PARTITION_P_RW_U_NA,
    }
};

/* 用户模式进程隔离 */
sys_mem_pagesize_get();  /* 获取内存页大小 */
k_mem_domain_add_thread(); /* 线程添加到保护域 */
```



#### 4.2 安全服务API

c

```
/* 加密API示例 */
#include <zephyr/crypto/crypto.h>

int encrypt_data(const uint8_t *plaintext, size_t len,
                 uint8_t *ciphertext, const uint8_t *key)
{
    struct cipher_ctx ctx = {
        .keylen = 32,
        .key.bit_stream = key,
        .mode_params = {
            .cipher_mode = CRYPTO_CIPHER_MODE_CTR,
            .counter = { .value = 0x12345678 }
        }
    };
    
    return cipher_encrypt(&ctx, plaintext, len,
                         ciphertext, &olen);
}
```



#### 4.3 安全审计与监控

c

```
/* 安全事件日志 */
SECURITY_EVENT_DEFINE(unauthorized_access,
                     "Unauthorized memory access detected",
                     CRITICAL);

void security_monitor_init(void)
{
    /* 注册安全事件回调 */
    security_event_register_callback(&event_handler);
    
    /* 启用运行时安全检查 */
    CONFIG_STACK_CANARIES=y
    CONFIG_HW_STACK_PROTECTION=y
    CONFIG_DEBUG_COREDUMP=y
}
```



### 5. 安全认证与合规

text

```
认证标准:
- PSA Certified Level 1-3
- SESIP (IoT安全评估标准)
- FIPS 140-2/3 (加密模块)
- Common Criteria

安全测试:
- 渗透测试
- 模糊测试
- 静态代码分析
- 侧信道分析
```



### 6. 安全最佳实践

markdown

```
1. **最小权限原则**
   - 每个线程只拥有必要权限
   - 用户模式运行应用代码
   - 特权分离

2. **深度防御**
   - 多层安全控制
   - 冗余安全检查
   - 失效安全设计

3. **安全开发生命周期**
   - 威胁建模
   - 安全代码审查
   - 持续安全测试
```



------

## 第二部分：Safety（功能安全） - 确保系统安全可靠运行

### 1. 安全概述

**Safety** 关注的是**功能安全**，确保系统在**发生故障**时不会造成**人员伤害**、**设备损坏**或**环境危害**。

### 2. 安全标准

text

```
主要标准:
- IEC 61508 (工业通用)
- ISO 26262 (汽车)
- IEC 62304 (医疗)
- DO-178C (航空)
- EN 50128 (铁路)
```



### 3. Zephyr 安全架构

#### 3.1 安全等级支持

yaml

```
安全完整性等级:
  - SIL 0: 无安全要求
  - SIL 1: 低风险，单点故障可接受
  - SIL 2: 中等风险，需要诊断覆盖率
  - SIL 3: 高风险，高诊断覆盖率
  - SIL 4: 极高风险，冗余设计
  
Zephyr目标:
  - 支持SIL 2/3应用
  - ASIL B/C (汽车)
```



#### 3.2 安全内核特性

kconfig

```
# 内核安全配置
CONFIG_ASSERT=y                    /* 运行时断言 */
CONFIG_STACK_SENTINEL=y            /* 栈哨兵 */
CONFIG_HW_STACK_PROTECTION=y       /* 硬件栈保护 */
CONFIG_DETECT_STACK_OVERFLOW=y     /* 栈溢出检测 */

# 内存安全
CONFIG_INIT_STACKS=y
CONFIG_MEMORY_PROTECTION=y
CONFIG_MPU_STACK_GUARD=y

# 错误处理
CONFIG_FAULT_DUMP=y
CONFIG_EXCEPTION_DEBUG=y
```



### 4. 安全机制实现

#### 4.1 故障检测与处理

c

```
/* 看门狗配置 */
struct wdt_timeout_cfg wdt_config = {
    .window.min = 0,
    .window.max = WDT_MAX_TIMEOUT,
    .callback = wdt_callback,
    .flags = WDT_FLAG_RESET_SOC
};

wdt_install_timeout(wdt_dev, &wdt_config);
wdt_setup(wdt_dev, WDT_OPT_PAUSE_HALTED_BY_DBG);

/* ECC内存保护 */
CONFIG_ECC=y
CONFIG_ECC_ERROR_INJECTION=y
CONFIG_ECC_ERROR_CORRECTION=y
```



#### 4.2 冗余与多样性

c

```
/* 双核锁步 (DCLS) 支持 */
#ifdef CONFIG_SAFETY_DCLS
    /* 主核运行 */
    primary_core_function();
    
    /* 从核验证 */
    #ifdef CONFIG_DCLS_CHECKER_CORE
    verify_primary_results();
    #endif
    
    /* 比较结果 */
    if (results_differ()) {
        safety_fault_handler(FAULT_DCLS_MISMATCH);
    }
#endif

/* 软件冗余示例 */
uint32_t calculate_crc_redundant(const uint8_t *data, size_t len)
{
    /* 使用不同算法计算两次 */
    uint32_t crc1 = crc32_ieee(data, len);
    uint32_t crc2 = crc16_ccitt(data, len);
    
    /* 比较结果 */
    if (!validate_crc_pair(crc1, crc2)) {
        safety_fault_detected(FAULT_CRC_MISMATCH);
        return SAFE_DEFAULT_VALUE;
    }
    
    return crc1;
}
```



#### 4.3 安全状态机

c

```
/* IEC 61508 安全状态机 */
enum safety_state {
    SAFETY_STATE_NORMAL,
    SAFETY_STATE_DEGRADED,
    SAFETY_STATE_SAFE,
    SAFETY_STATE_FAULT,
    SAFETY_STATE_EMERGENCY_STOP
};

struct safety_supervisor {
    enum safety_state current_state;
    uint32_t fault_bits;
    struct k_timer self_test_timer;
    struct k_mutex state_lock;
};

void safety_state_transition(enum safety_state new_state)
{
    /* 状态转换必须通过验证 */
    if (validate_state_transition(current_state, new_state)) {
        enter_safe_state();
        current_state = new_state;
        log_safety_event(SAFETY_EVENT_STATE_CHANGE);
    } else {
        /* 非法状态转换，进入安全状态 */
        emergency_shutdown();
    }
}
```



### 5. 安全认证支持

#### 5.1 认证包配置

kconfig

```
# 功能安全认证包
CONFIG_SAFETY_CERTIFICATION=y
CONFIG_SAFETY_STANDARD="IEC 61508"
CONFIG_SAFETY_SIL_LEVEL=2

# 文档生成
CONFIG_SAFETY_DOCS=y
CONFIG_SAFETY_REQUIREMENTS_TRACE=y
CONFIG_SAFETY_TEST_COVERAGE_REPORT=y
```



#### 5.2 安全案例文档

markdown

```
安全案例要素:
1. 安全需求规范
2. 安全架构设计
3. 故障模式与影响分析 (FMEA)
4. 故障树分析 (FTA)
5. 诊断覆盖率分析
6. 测试验证报告
7. 安全手册
```



### 6. 安全测试与验证

#### 6.1 自动化安全测试

python

```
# safety_test.py - 安全测试框架示例
class SafetyTestSuite:
    def test_fault_injection(self):
        """故障注入测试"""
        inject_fault(FAULT_TYPE_STACK_OVERFLOW)
        assert system_enters_safe_state()
        
    def test_diagnostic_coverage(self):
        """诊断覆盖率测试"""
        coverage = calculate_diagnostic_coverage()
        assert coverage >= REQUIRED_DC_SIL2
        
    def test_safe_state_recovery(self):
        """安全状态恢复测试"""
        simulate_critical_failure()
        assert recovery_time() < MAX_RECOVERY_TIME
```



#### 6.2 运行时监控

c

```
/* 运行时安全监控 */
struct safety_monitor_metrics {
    uint32_t cpu_usage;
    uint32_t stack_usage[MAX_THREADS];
    uint32_t heap_fragmentation;
    uint32_t fault_counters[FAULT_TYPE_COUNT];
    uint64_t uptime;
    bool self_test_passed;
};

void safety_health_check(void)
{
    struct safety_monitor_metrics metrics;
    
    collect_safety_metrics(&metrics);
    
    if (metrics.cpu_usage > CPU_USAGE_THRESHOLD) {
        safety_warning(WARNING_CPU_OVERLOAD);
    }
    
    if (any_stack_near_overflow(metrics.stack_usage)) {
        safety_fault(FAULT_STACK_OVERFLOW_IMMINENT);
    }
    
    /* 定期自检 */
    if (self_test_due()) {
        run_safety_self_test();
    }
}
```



### 7. 安全集成指南

#### 7.1 安全关键应用开发

c

```
/* 安全关键线程配置 */
K_THREAD_DEFINE(safety_thread, SAFETY_STACK_SIZE,
                safety_thread_entry, NULL, NULL, NULL,
                SAFETY_THREAD_PRIORITY,
                K_ESSENTIAL | K_FP_REGS, K_NO_WAIT);

/* 安全关键内存区域 */
K_APPMEM_PARTITION_DEFINE(safety_partition);
K_APP_BMEM(safety_partition) uint32_t safety_data[1024];

/* 安全互斥锁 - 带优先级继承 */
struct k_mutex safety_mutex;
k_mutex_init(&safety_mutex);
```



#### 7.2 安全配置文件

cmake

```
# CMakeLists.txt - 安全构建配置
set(SAFETY_FLAGS
    -fsanitize=safe-stack
    -fstack-protector-strong
    -fstack-clash-protection
    -fcf-protection=full
)

if(SAFETY_CERTIFICATION)
    add_compile_options(
        ${SAFETY_FLAGS}
        -DSAFETY_CHECKS_ENABLED
        -DFAULT_INJECTION_ENABLED
    )
    
    # 启用所有断言
    set(ASSERT_LEVEL 2)
endif()
```



## 第三部分：Security vs Safety 对比总结

### 对比表格

| 方面         | Security (安全)                | Safety (功能安全)        |
| :----------- | :----------------------------- | :----------------------- |
| **核心目标** | 保护系统免受恶意攻击           | 防止系统故障造成伤害     |
| **关注点**   | 机密性、完整性、可用性         | 可靠性、可用性、可维护性 |
| **威胁来源** | 恶意攻击者、恶意软件           | 随机硬件故障、系统错误   |
| **主要标准** | PSA Certified, Common Criteria | IEC 61508, ISO 26262     |
| **实现重点** | 加密、认证、访问控制           | 冗余、诊断、故障处理     |
| **故障模型** | 智能对手，主动攻击             | 随机故障，系统性失效     |
| **时间特性** | 实时性要求中等                 | 硬实时，确定性响应       |
| **典型技术** | TEE、加密、安全启动            | 看门狗、ECC、DCLS        |

### 协同工作示例

c

```
/* 安全与安全协同设计 */
void safety_critical_secure_operation(void)
{
    /* 1. 安全检查 - Security */
    if (!authenticate_operator()) {
        security_event_log(UNAUTHORIZED_ACCESS_ATTEMPT);
        return;
    }
    
    /* 2. 安全状态检查 - Safety */
    if (!safety_system_ready()) {
        enter_safe_state();
        return;
    }
    
    /* 3. 安全关键操作 - Safety */
    safety_state_transition(SAFETY_STATE_OPERATIONAL);
    
    /* 4. 安全通信 - Security */
    secure_transmit_command(encrypt_command(CMD_START));
    
    /* 5. 持续监控 - Both */
    while (operation_active) {
        safety_health_check();      // 安全检查
        security_intrusion_check(); // 安全监控
    }
}
```



### 集成建议

1. **分层设计**
   - 底层：安全机制（硬件保护）
   - 中间层：安全服务（故障处理）
   - 应用层：业务逻辑
2. **独立认证**
   - 安全认证和安全认证可分开进行
   - 考虑复合认证（如ISO 21434 + ISO 26262）
3. **工具链支持**
   - 使用经认证的编译器（如Qualified GCC）
   - 静态分析工具（Coverity, Klocwork）
   - 形式化验证工具

### 资源与参考

markdown

```
官方文档:
- Zephyr Security Documentation: /doc/security
- Zephyr Safety Documentation: /doc/safety
- PSA Certified Resources: https://www.psacertified.org/

认证指南:
- Zephyr Safety Manual
- Security Best Practices Guide
- Certification Cookbook

社区资源:
- Safety Working Group
- Security Working Group
- 认证用户案例
```



## 总结

Zephyr RTOS 为**Security**和**Safety**提供了全面的支持框架。**Security**专注于防御外部威胁和保护数据，而**Safety**确保系统在故障时安全运行。两者在现代嵌入式系统中往往需要协同设计，特别是在汽车、医疗、工业控制等关键领域。通过Zephyr的模块化架构，开发者可以根据应用需求灵活配置安全和安全特性。