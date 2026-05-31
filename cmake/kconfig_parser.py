#!/usr/bin/env python3
"""
Kconfig Parser for XinYi Framework
Similar to Linux kernel's Kconfig system

Generates:
1. .config - Build configuration (like kernel .config)
2. autoconf.h - C header with CONFIG_* macros
3. config.cmake - CMake variables for build system
"""

import argparse
import os
import re
import sys
from pathlib import Path
from typing import Dict, List, Optional, Tuple

class KconfigParser:
    def __init__(self):
        self.config: Dict[str, dict] = {}
        self.defaults: Dict[str, str] = {}
        self.dependencies: Dict[str, List[str]] = {}
        self.selects: Dict[str, List[str]] = {}
        
    def parse(self, kconfig_file: str) -> None:
        """Parse Kconfig file"""
        with open(kconfig_file, 'r') as f:
            content = f.read()
        
        # Parse config entries
        config_pattern = r'config\s+(\w+)'
        for match in re.finditer(config_pattern, content):
            config_name = match.group(1)
            start = match.end()
            
            # Find the block
            block_end = self._find_block_end(content, start)
            block = content[start:block_end]
            
            # Parse type
            type_match = re.search(r'\b(bool|string|int|hex)\b', block)
            config_type = type_match.group(1) if type_match else 'bool'
            
            # Parse default
            default_match = re.search(r'default\s+(.+?)(?:\n|$)', block)
            default_value = default_match.group(1).strip() if default_match else None
            default_if = None
            if default_value is not None:
                default_value, default_if = self._split_default(default_value)
            
            # Parse depends on
            depends_match = re.search(r'depends on\s+(.+?)(?:\n|$)', block)
            depends_on = depends_match.group(1).strip() if depends_match else None
            
            # Parse select
            selects = re.findall(r'select\s+(\w+)', block)
            
            # Parse help
            help_match = re.search(r'help\s*\n((?:\s+.+\n)+)', block)
            help_text = help_match.group(1) if help_match else ""
            
            self.config[config_name] = {
                'type': config_type,
                'default': default_value,
                'default_if': default_if,
                'depends_on': depends_on,
                'selects': selects,
                'help': help_text.strip()
            }
    
    def _find_block_end(self, content: str, start: int) -> int:
        """Find end of config block"""
        lines = content[start:].split('\n')
        end = start
        for i, line in enumerate(lines):
            if line.strip() and not line.startswith(' ') and not line.startswith('\t'):
                if not line.startswith('#'):
                    break
            end += len(line) + 1
        return end
    
    def resolve_dependencies(self) -> None:
        """Resolve config dependencies"""
        # Simple dependency resolution
        for name, cfg in self.config.items():
            if cfg['default'] and 'if' in cfg['default']:
                # Handle conditional defaults
                parts = cfg['default'].split('if')
                cfg['default'] = parts[0].strip()
                if len(parts) > 1:
                    cfg['depends_on'] = parts[1].strip()
    
    def _format_string_value(self, value: str) -> str:
        """Return a CMake/C-string friendly value without Kconfig quotes."""
        if value is None:
            return ""
        value = value.strip()
        if len(value) >= 2 and value[0] == '"' and value[-1] == '"':
            value = value[1:-1]
        return value
    
    def _split_default(self, default: str) -> Tuple[str, Optional[str]]:
        """Split a Kconfig default value from its optional `if <expr>` guard."""
        match = re.match(r'(.+?)\s+if\s+(.+)$', default.strip())
        if not match:
            return default.strip(), None
        return match.group(1).strip(), match.group(2).strip()
    
    def _default_matches_context(self, condition: Optional[str], context: Optional[Dict[str, bool]]) -> bool:
        """Return whether a simple default condition is satisfied by context."""
        if not condition:
            return True
        if context is None:
            return False
        return self._eval_expr(condition, context)
    
    def _eval_expr(self, expr: str, context: Dict[str, bool]) -> bool:
        """Evaluate the small Kconfig boolean subset used by this repository."""
        expr = expr.strip()
        if not expr:
            return True

        # Strip one pair of balanced outer parentheses.
        while expr.startswith('(') and expr.endswith(')'):
            depth = 0
            balanced = True
            for i, ch in enumerate(expr):
                if ch == '(':
                    depth += 1
                elif ch == ')':
                    depth -= 1
                    if depth == 0 and i != len(expr) - 1:
                        balanced = False
                        break
            if not balanced:
                break
            expr = expr[1:-1].strip()

        for op, reducer in (('||', any), ('&&', all)):
            parts = self._split_top_level(expr, op)
            if len(parts) > 1:
                return reducer(self._eval_expr(part, context) for part in parts)

        if expr.startswith('!'):
            return not self._eval_expr(expr[1:], context)

        if expr in ('y', 'Y', '1'):
            return True
        if expr in ('n', 'N', '0'):
            return False
        return bool(context.get(expr, False))
    
    def _split_top_level(self, expr: str, op: str) -> List[str]:
        """Split expr by op, ignoring operators inside parentheses."""
        parts = []
        depth = 0
        start = 0
        i = 0
        while i < len(expr):
            ch = expr[i]
            if ch == '(':
                depth += 1
            elif ch == ')':
                depth -= 1
            elif depth == 0 and expr.startswith(op, i):
                parts.append(expr[start:i].strip())
                i += len(op)
                start = i
                continue
            i += 1
        if parts:
            parts.append(expr[start:].strip())
        return parts
    
    def _context_for_platform(self, platform: Optional[str]) -> Dict[str, bool]:
        """Create a minimal symbol context for platform-specific defaults."""
        context = {name: False for name in self.config}
        if not platform:
            return context
        platform = platform.upper()
        symbol = f"PLATFORM_{platform}"
        context[symbol] = True
        if platform.startswith('STM32'):
            context['PLATFORM_STM32'] = True
        return context
    
    def _selects_for_enabled_defaults(self, values: Dict[str, str]) -> None:
        """Propagate simple bool selects from currently enabled bool symbols."""
        changed = True
        while changed:
            changed = False
            context = {name: value == 'y' for name, value in values.items()}
            for name, cfg in self.config.items():
                if cfg['type'] != 'bool' or values.get(name) != 'y':
                    continue
                for selected in cfg['selects']:
                    selected_cfg = self.config.get(selected)
                    if not selected_cfg or selected_cfg['type'] != 'bool' or values.get(selected) == 'y':
                        continue
                    depends_on = selected_cfg.get('depends_on')
                    if depends_on and not self._eval_expr(depends_on, context):
                        continue
                    values[selected] = 'y'
                    changed = True
    
    def _platform_symbol_value(self, name: str, platform: Optional[str]) -> Optional[bool]:
        """Return forced value for platform selector symbols when platform is set."""
        if not platform or not name.startswith('PLATFORM_'):
            return None
        platform = platform.upper()
        if name == f'PLATFORM_{platform}':
            return True
        if name == 'PLATFORM_STM32':
            return platform.startswith('STM32')
        return False
    
    def resolve_values(self, platform: Optional[str] = None) -> Dict[str, str]:
        """Resolve default values for generation, honoring simple platform guards."""
        context = self._context_for_platform(platform)
        values: Dict[str, str] = {}

        for name, cfg in self.config.items():
            forced_platform_value = self._platform_symbol_value(name, platform)
            if forced_platform_value is not None:
                value = 'y' if forced_platform_value else 'n'
            else:
                depends_on = cfg.get('depends_on')
                default = cfg['default']
                condition = cfg.get('default_if')
                if depends_on and not self._eval_expr(depends_on, context):
                    value = 'n'
                elif default is None or not self._default_matches_context(condition, context):
                    value = 'n'
                else:
                    value = default

            if cfg['type'] == 'bool':
                values[name] = 'y' if value and value != 'n' else 'n'
                context[name] = values[name] == 'y'
            else:
                values[name] = value

        self._selects_for_enabled_defaults(values)
        return values
    
    def generate_config(self, output_file: str, platform: Optional[str] = None) -> None:
        """Generate .config file"""
        values = self.resolve_values(platform)
        with open(output_file, 'w') as f:
            f.write("# XinYi Framework Configuration\n")
            f.write("# Auto-generated - DO NOT EDIT\n\n")
            
            for name, cfg in sorted(self.config.items()):
                value = values[name]
                if cfg['type'] == 'string':
                    value = f'"{self._format_string_value(value)}"'
                f.write(f"CONFIG_{name}={value}\n")
    
    def generate_autoconf(self, output_file: str, platform: Optional[str] = None) -> None:
        """Generate autoconf.h"""
        values = self.resolve_values(platform)
        os.makedirs(os.path.dirname(output_file), exist_ok=True)
        
        with open(output_file, 'w') as f:
            f.write("/**\n")
            f.write(" * @file autoconf.h\n")
            f.write(" * @brief Auto-generated configuration header\n")
            f.write(" * @note DO NOT EDIT - Generated by CMake\n")
            f.write(" */\n\n")
            f.write("#ifndef XY_AUTOCONF_H\n")
            f.write("#define XY_AUTOCONF_H\n\n")
            
            for name, cfg in sorted(self.config.items()):
                value = values[name]
                if cfg['type'] == 'bool':
                    enabled = value == 'y'
                    f.write(f"#define CONFIG_{name} {'1' if enabled else '0'}\n")
                elif cfg['type'] == 'string':
                    f.write(f'#define CONFIG_{name} "{self._format_string_value(value)}"\n')
                elif cfg['type'] == 'int':
                    try:
                        f.write(f"#define CONFIG_{name} {int(value)}\n")
                    except (ValueError, TypeError):
                        f.write(f"#define CONFIG_{name} 0\n")
                elif cfg['type'] == 'hex':
                    f.write(f"#define CONFIG_{name} 0x{value}\n")
            
            f.write("\n#endif /* XY_AUTOCONF_H */\n")
    
    def generate_cmake(self, output_file: str, platform: Optional[str] = None) -> None:
        """Generate config.cmake for CMake"""
        values = self.resolve_values(platform)
        with open(output_file, 'w') as f:
            f.write("# XinYi Framework CMake Configuration\n")
            f.write("# Auto-generated - DO NOT EDIT\n\n")
            
            for name, cfg in sorted(self.config.items()):
                value = values[name]
                var_name = f"XY_{name}"
                
                if cfg['type'] == 'bool':
                    enabled = value == 'y'
                    f.write(f"set({var_name} {'ON' if enabled else 'OFF'})\n")
                elif cfg['type'] == 'string':
                    f.write(f"set({var_name} \"{self._format_string_value(value)}\")\n")
                else:
                    f.write(f"set({var_name} \"{value}\")\n")
                
                # Set environment variable for message()
                f.write(f"set(ENV{{{var_name}}} \"${{{var_name}}}\")\n")


def main():
    parser = argparse.ArgumentParser(description='Kconfig Parser for XinYi')
    parser.add_argument('--kconfig', required=True, help='Kconfig file path')
    parser.add_argument('--output', required=True, help='Output .config file')
    parser.add_argument('--autoconf', required=True, help='Output autoconf.h file')
    parser.add_argument('--cmake', required=True, help='Output config.cmake file')
    parser.add_argument('--platform', help='Target platform for conditional defaults (PC, STM32U5, STM32F4, ...)')
    
    args = parser.parse_args()
    
    # Parse Kconfig
    kconfig = KconfigParser()
    kconfig.parse(args.kconfig)
    kconfig.resolve_dependencies()
    
    # Generate output files
    kconfig.generate_config(args.output, platform=args.platform)
    kconfig.generate_autoconf(args.autoconf, platform=args.platform)
    kconfig.generate_cmake(args.cmake, platform=args.platform)
    
    print(f"Generated: {args.output}")
    print(f"Generated: {args.autoconf}")
    print(f"Generated: {args.cmake}")


if __name__ == '__main__':
    main()
