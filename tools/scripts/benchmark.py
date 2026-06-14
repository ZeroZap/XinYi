#!/usr/bin/env python3
"""
XinYi Performance Benchmark Tool
性能基准测试工具
"""

import time
import statistics
import sys

class Benchmark:
    def __init__(self, name):
        self.name = name
        self.results = []
    
    def run(self, func, iterations=10):
        """运行测试"""
        print(f"Running {self.name}...")
        
        times = []
        for i in range(iterations):
            start = time.perf_counter()
            func()
            end = time.perf_counter()
            times.append((end - start) * 1000)  # ms
        
        self.results = times
        self.report()
    
    def report(self):
        """生成报告"""
        if not self.results:
            return
        
        avg = statistics.mean(self.results)
        median = statistics.median(self.results)
        std_dev = statistics.stdev(self.results) if len(self.results) > 1 else 0
        min_val = min(self.results)
        max_val = max(self.results)
        
        print(f"\n{self.name} Results:")
        print(f"  Iterations: {len(self.results)}")
        print(f"  Avg:    {avg:.3f} ms")
        print(f"  Median: {median:.3f} ms")
        print(f"  StdDev: {std_dev:.3f} ms")
        print(f"  Min:    {min_val:.3f} ms")
        print(f"  Max:    {max_val:.3f} ms")
        print()

def benchmark_memcpy(size, iterations=100):
    """测试内存拷贝性能"""
    import ctypes
    
    buf1 = (ctypes.c_ubyte * size)()
    buf2 = (ctypes.c_ubyte * size)()
    
    def test():
        ctypes.memmove(buf2, buf1, size)
    
    return test

def benchmark_pid_calc(iterations=1000):
    """测试 PID 计算性能"""
    # 模拟 PID 计算
    kp, ki, kd = 1.0, 0.5, 0.2
    error_sum = 0
    last_error = 0
    
    def test():
        nonlocal error_sum, last_error
        error = 1.0
        error_sum += error
        output = kp * error + ki * error_sum + kd * (error - last_error)
        last_error = error
    
    return test

def benchmark_json_parse(iterations=100):
    """测试 JSON 解析性能"""
    import json
    
    test_json = '{"sensor": "temp", "value": 25.5, "unit": "C"}'
    
    def test():
        data = json.loads(test_json)
        _ = data['value']
    
    return test

def main():
    print("=" * 60)
    print("XinYi Performance Benchmark")
    print("=" * 60)
    print()
    
    # 内存拷贝测试
    bench = Benchmark("memcpy (1KB)")
    bench.run(benchmark_memcpy(1024, 100), iterations=100)
    
    bench = Benchmark("memcpy (10KB)")
    bench.run(benchmark_memcpy(10240, 100), iterations=100)
    
    # PID 计算测试
    bench = Benchmark("PID Calculation")
    bench.run(benchmark_pid_calc(1000), iterations=1000)
    
    # JSON 解析测试
    bench = Benchmark("JSON Parse")
    bench.run(benchmark_json_parse(100), iterations=100)
    
    print("=" * 60)

if __name__ == '__main__':
    main()
