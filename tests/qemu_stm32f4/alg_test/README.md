# XinYi 算法测试框架

**平台**: STM32F405 on QEMU  
**接口**: UART 输出 (半主机)  
**状态**: ✅ 17 个测试全部通过

---

## 📋 测试框架说明

### 设计理念
通过 UART 输出测试结果，支持自动化验证：

```
算法代码 → UART 输出 → QEMU stdout → 脚本解析 → PASS/FAIL 报告
```

### 测试输出格式

#### 人类可读
```
[TEST] CRC-8 Algorithm...
  ✓ PASS: Empty data (0)
  ✓ PASS: Single byte non-zero
  CRC-8(0102030405) = 0xBC
  ✓ PASS: Known vector
  ✓ PASS: Repeatability (188)
```

#### 机器可读
```
[RESULT] PASS=17 FAIL=0
```

---

## 🚀 快速开始

### 编译和运行

```bash
cd tests/qemu_stm32f4/alg_test

# 方式 1: 使用 Bash 脚本
./run_test.sh

# 方式 2: 使用 Python 脚本
python3 verify_test.py

# 方式 3: 手动运行
source /home/eugene/zerozap/scripts/env.sh
arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=softfp \
    -mfpu=fpv4-sp-d16 -O0 -g3 \
    -o alg_test.elf src/main.c src/startup.c \
    -nostdlib -T stm32f405rg.ld

timeout 10 qemu-system-arm -M olimex-stm32-h405 -nographic \
    -kernel alg_test.elf -semihosting
```

---

## 📁 文件结构

```
alg_test/
├── src/
│   ├── main.c              # 测试框架 + 被测算法
│   └── startup.c           # 启动代码
├── stm32f405rg.ld          # 链接脚本
├── alg_test.elf            # 编译产物 (4.7KB)
├── run_test.sh             # Bash 测试脚本
├── verify_test.py          # Python 验证脚本
└── README.md               # 本文档
```

---

## 🧪 测试用例

### 1. CRC-8 算法测试
- ✅ 空数据处理
- ✅ 单字节计算
- ✅ 已知向量验证
- ✅ 重复性测试

### 2. 环形缓冲区测试
- ✅ 初始状态
- ✅ 推入/弹出操作
- ✅ 环形特性
- ✅ 满缓冲区保护

### 3. 移动平均滤波测试
- ✅ 恒定输入
- ✅ 线性增长
- ✅ 部分窗口
- ✅ 零值处理

### 4. 性能基准测试
- ✅ CRC-8 100 字节循环

---

## 🔧 添加新测试

### 1. 实现被测算法

```c
/* 示例：加法算法 */
static uint32_t add(uint32_t a, uint32_t b)
{
    return a + b;
}
```

### 2. 创建测试函数

```c
static void test_add(void)
{
    TEST_START("Addition Algorithm");
    
    TEST_ASSERT_EQ(5, add(2, 3), "2+3=5");
    TEST_ASSERT_EQ(0, add(0, 0), "0+0=0");
    TEST_ASSERT_EQ(100, add(50, 50), "50+50=100");
}
```

### 3. 注册测试

```c
int main(void)
{
    print_str("XinYi Algorithm Test Framework\n");
    
    test_crc8();
    test_ring_buffer();
    test_filter();
    test_add();  /* 添加新测试 */
    
    /* 输出总结... */
}
```

---

## 📊 测试宏说明

### TEST_START(name)
开始一个测试，输出测试名称

```c
TEST_START("CRC-8 Algorithm");
// 输出：[TEST] CRC-8 Algorithm...
```

### TEST_ASSERT(condition, name)
断言条件为真

```c
TEST_ASSERT(crc != 0, "Non-zero result");
// ✓ PASS: Non-zero result
// 或
// ✗ FAIL: Non-zero result
```

### TEST_ASSERT_EQ(expected, actual, name)
断言期望值等于实际值

```c
TEST_ASSERT_EQ(100, add(50, 50), "50+50");
// ✓ PASS: 50+50 (100)
// 或
// ✗ FAIL: 50+50 (expected 100, got 99)
```

---

## 🎯 CI/CD 集成

### GitHub Actions 示例

```yaml
name: Algorithm Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Install QEMU
      run: sudo apt install qemu-system-arm
    
    - name: Run Tests
      run: |
        cd tests/qemu_stm32f4/alg_test
        ./run_test.sh
```

### GitLab CI 示例

```yaml
algorithm_test:
  stage: test
  image: ubuntu:22.04
  script:
    - apt update && apt install -y qemu-system-arm gcc-arm-none-eabi
    - cd tests/qemu_stm32f4/alg_test
    - ./run_test.sh
  artifacts:
    reports:
      junit: test_results.xml
```

---

## 📈 测试结果示例

### 通过
```
╔════════════════════════════════════════╗
║  Test Summary                          ║
╚════════════════════════════════════════╝
  Total: 17
  PASS:  17
  FAIL:   0

>>> ALL TESTS PASSED <<<

[RESULT] PASS=17 FAIL=0
```

### 失败
```
╔════════════════════════════════════════╗
║  Test Summary                          ║
╚════════════════════════════════════════╝
  Total: 17
  PASS:  15
  FAIL:  2

>>> SOME TESTS FAILED <<<

[RESULT] PASS=15 FAIL=2
```

---

## 🔍 调试技巧

### 1. 查看详细输出
```bash
./run_test.sh 2>&1 | tee test.log
```

### 2. 只运行特定测试
修改 `main.c`，注释掉其他测试：
```c
test_crc8();
// test_ring_buffer();  // 暂时跳过
test_filter();
```

### 3. 添加调试信息
```c
print_str("Debug: crc = 0x");
print_hex8(crc);
print_str("\n");
```

---

## 📚 参考资料

- [QEMU ARM Documentation](https://www.qemu.org/docs/master/system/arm/)
- [ARM Semihosting Specification](https://github.com/ARM-software/abi-aa/blob/main/semihosting/semihosting.rst)
- [STM32F405 Reference Manual](https://www.st.com/resource/en/reference_manual/rm0090.pdf)

---

**最后更新**: 2026-03-17  
**测试结果**: ✅ 17/17 通过
