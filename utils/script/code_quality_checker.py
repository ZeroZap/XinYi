#!/usr/bin/env python3
"""
XinYi Code Quality Checker
轻量级代码质量检查工具
"""

import os
import re
import sys
from pathlib import Path

class CodeQualityChecker:
    def __init__(self, root_dir):
        self.root_dir = Path(root_dir)
        self.issues = []
        self.stats = {
            'files': 0,
            'lines': 0,
            'functions': 0,
            'warnings': 0,
            'errors': 0,
        }
    
    def check_file(self, filepath):
        """检查单个文件"""
        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                content = f.read()
                lines = content.split('\n')
        except Exception as e:
            self.issues.append({
                'file': str(filepath),
                'line': 0,
                'level': 'error',
                'message': f'无法读取文件：{e}'
            })
            return
        
        self.stats['files'] += 1
        self.stats['lines'] += len(lines)
        
        # 检查项
        self._check_line_length(filepath, lines)
        self._check_function_length(filepath, lines)
        self._check_naming_convention(filepath, content)
        self._check_todo_comments(filepath, lines)
        self._check_includes(filepath, content)
    
    def _check_line_length(self, filepath, lines, max_len=120):
        """检查行长度"""
        for i, line in enumerate(lines, 1):
            if len(line) > max_len:
                self.issues.append({
                    'file': str(filepath),
                    'line': i,
                    'level': 'warning',
                    'message': f'行过长 ({len(line)} > {max_len})'
                })
                self.stats['warnings'] += 1
    
    def _check_function_length(self, filepath, lines, max_lines=50):
        """检查函数长度"""
        func_pattern = re.compile(r'^(\w+)\s+\*?(\w+)\s*\([^)]*\)')
        in_function = False
        func_start = 0
        func_name = ''
        brace_count = 0
        
        for i, line in enumerate(lines, 1):
            match = func_pattern.match(line.strip())
            if match and not in_function:
                in_function = True
                func_start = i
                func_name = match.group(2)
                brace_count = line.count('{') - line.count('}')
                self.stats['functions'] += 1
            elif in_function:
                brace_count += line.count('{') - line.count('}')
                if brace_count <= 0:
                    func_len = i - func_start
                    if func_len > max_lines:
                        self.issues.append({
                            'file': str(filepath),
                            'line': func_start,
                            'level': 'warning',
                            'message': f'函数 {func_name} 过长 ({func_len} > {max_lines} 行)'
                        })
                        self.stats['warnings'] += 1
                    in_function = False
    
    def _check_naming_convention(self, filepath, content):
        """检查命名规范"""
        # 检查驼峰命名 (应该是小写 + 下划线)
        camel_pattern = re.compile(r'\b[a-z]+[A-Z]\w*\b')
        for match in camel_pattern.finditer(content):
            # 忽略类型定义 (typedef)
            line_start = content.rfind('\n', 0, match.start())
            line_end = content.find('\n', match.end())
            line = content[line_start:line_end]
            if 'typedef' not in line and 'struct' not in line:
                self.issues.append({
                    'file': str(filepath),
                    'line': 0,
                    'level': 'info',
                    'message': f'发现驼峰命名：{match.group()} (建议使用小写 + 下划线)'
                })
    
    def _check_todo_comments(self, filepath, lines):
        """检查 TODO 注释"""
        for i, line in enumerate(lines, 1):
            if 'TODO' in line or 'FIXME' in line:
                self.issues.append({
                    'file': str(filepath),
                    'line': i,
                    'level': 'info',
                    'message': line.strip()
                })
    
    def _check_includes(self, filepath, content):
        """检查头文件包含"""
        # 检查是否包含必要的头文件保护
        if filepath.suffix == '.h':
            if '#ifndef' not in content or '#define' not in content or '#endif' not in content:
                self.issues.append({
                    'file': str(filepath),
                    'line': 0,
                    'level': 'error',
                    'message': '缺少头文件保护 (#ifndef/#define/#endif)'
                })
                self.stats['errors'] += 1
    
    def scan_directory(self, patterns=None):
        """扫描目录"""
        if patterns is None:
            patterns = ['*.c', '*.h']
        
        for pattern in patterns:
            for filepath in self.root_dir.rglob(pattern):
                # 跳过第三方目录
                if 'third_party' in str(filepath) or 'build' in str(filepath):
                    continue
                self.check_file(filepath)
    
    def report(self):
        """生成报告"""
        print("=" * 60)
        print("XinYi Code Quality Report")
        print("=" * 60)
        print()
        print("Statistics:")
        print(f"  Files:     {self.stats['files']}")
        print(f"  Lines:     {self.stats['lines']}")
        print(f"  Functions: {self.stats['functions']}")
        print(f"  Errors:    {self.stats['errors']}")
        print(f"  Warnings:  {self.stats['warnings']}")
        print()
        
        if self.issues:
            print("Issues:")
            by_level = {'error': [], 'warning': [], 'info': []}
            for issue in self.issues:
                by_level[issue['level']].append(issue)
            
            for level in ['error', 'warning', 'info']:
                if by_level[level]:
                    print(f"\n{level.upper()}S ({len(by_level[level])}):")
                    for issue in by_level[level][:10]:  # 只显示前 10 个
                        print(f"  {issue['file']}:{issue['line']} - {issue['message']}")
                    if len(by_level[level]) > 10:
                        print(f"  ... and {len(by_level[level]) - 10} more")
        else:
            print("No issues found! ✓")
        
        print()
        print("=" * 60)

def main():
    import argparse
    parser = argparse.ArgumentParser(description='XinYi Code Quality Checker')
    parser.add_argument('root_dir', nargs='?', default='.', help='Root directory to scan')
    parser.add_argument('--output', '-o', help='Output file (default: stdout)')
    args = parser.parse_args()
    
    checker = CodeQualityChecker(args.root_dir)
    checker.scan_directory()
    
    if args.output:
        with open(args.output, 'w') as f:
            old_stdout = sys.stdout
            sys.stdout = f
            checker.report()
            sys.stdout = old_stdout
        print(f"Report saved to {args.output}")
    else:
        checker.report()
    
    # 返回错误码
    sys.exit(0 if checker.stats['errors'] == 0 else 1)

if __name__ == '__main__':
    main()
