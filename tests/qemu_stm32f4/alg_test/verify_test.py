#!/usr/bin/env python3
"""
算法测试验证脚本
捕获 QEMU 输出并验证测试结果
"""

import subprocess
import sys
import re

def run_test():
    """运行 QEMU 测试并捕获输出"""
    cmd = [
        'qemu-system-arm',
        '-M', 'olimex-stm32-h405',
        '-nographic',
        '-kernel', 'alg_test.elf',
        '-semihosting'
    ]
    
    print("Running QEMU test...")
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
    
    return result.stdout + result.stderr

def parse_results(output):
    """解析测试结果"""
    # 查找 [RESULT] PASS=x FAIL=y
    match = re.search(r'\[RESULT\] PASS=(\d+) FAIL=(\d+)', output)
    
    if not match:
        print("❌ Failed to parse test results")
        return None
    
    pass_count = int(match.group(1))
    fail_count = int(match.group(2))
    
    return {
        'pass': pass_count,
        'fail': fail_count,
        'total': pass_count + fail_count
    }

def main():
    print("=" * 50)
    print("XinYi Algorithm Test Validator")
    print("=" * 50)
    print()
    
    # 运行测试
    output = run_test()
    
    # 显示输出
    print("Test Output:")
    print("-" * 50)
    print(output)
    print("-" * 50)
    print()
    
    # 解析结果
    results = parse_results(output)
    
    if results is None:
        sys.exit(1)
    
    # 打印总结
    print(f"Total Tests: {results['total']}")
    print(f"PASSED:      {results['pass']}")
    print(f"FAILED:      {results['fail']}")
    print()
    
    if results['fail'] == 0:
        print("✅ ALL TESTS PASSED")
        sys.exit(0)
    else:
        print("❌ SOME TESTS FAILED")
        sys.exit(1)

if __name__ == '__main__':
    main()
