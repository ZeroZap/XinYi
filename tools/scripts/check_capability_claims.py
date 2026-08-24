#!/usr/bin/env python3
"""Fail when current public docs make unqualified product-evidence claims."""

from __future__ import annotations

import re
import sys
from pathlib import Path


PUBLIC_DOCS = (
    "README.md",
    "components/README.md",
    "components/hal/README.md",
    "components/net/README.md",
    "components/net/modbus/README.md",
    "components/net/at/atc/xy_lte/STATUS.md",
    "components/kernel/osal/freertos/README.md",
    "components/crypto/xy_25519/README.md",
    "components/crypto/xy_25519/README_RISCV.md",
    "components/crypto/xy_chacha/README.md",
)

FORBIDDEN = (
    re.compile(r"\bproduction[- ]ready\b", re.IGNORECASE),
    re.compile(r"\bproduction[- ]grade\b", re.IGNORECASE),
    re.compile(r"生产就绪"),
    re.compile(r"完整测试覆盖"),
)


def validate(repo: Path) -> list[str]:
    failures: list[str] = []
    for relative in PUBLIC_DOCS:
        path = repo / relative
        if not path.is_file():
            failures.append(f"missing public capability document: {relative}")
            continue
        for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            for pattern in FORBIDDEN:
                if pattern.search(line):
                    failures.append(f"{relative}:{line_number}: unqualified claim: {line.strip()}")
    return failures


def main() -> int:
    repo = Path(__file__).resolve().parents[2]
    failures = validate(repo)
    if failures:
        print("capability claim validation failed:", file=sys.stderr)
        for failure in failures:
            print(f"- {failure}", file=sys.stderr)
        return 1
    print(f"capability claims valid: {len(PUBLIC_DOCS)} public documents checked")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())