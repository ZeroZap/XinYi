#!/usr/bin/env python3
"""Guard public HAL/FOTA status wording against the evidence matrix."""

from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
INDEX = ROOT / "docs" / "components" / "index.md"
EVIDENCE = ROOT / "docs" / "validation" / "component-evidence-matrix.md"
HAL_MATRIX = ROOT / "docs" / "validation" / "hal-platform-evidence-matrix.md"


def require(condition: bool, message: str, errors: list[str]) -> None:
    if not condition:
        errors.append(message)


def validate() -> list[str]:
    errors: list[str] = []
    index = INDEX.read_text(encoding="utf-8")
    evidence = EVIDENCE.read_text(encoding="utf-8")
    hal_matrix = HAL_MATRIX.read_text(encoding="utf-8")

    for token in (
        "HAL | H1（PC）",
        "FOTA | H1（状态机 + signature-provider/boot-handoff/delta/boot-confirm fail-closed",
    ):
        require(token in evidence, f"canonical evidence matrix is missing token: {token}", errors)
    require("BOARD_PENDING" in hal_matrix, "HAL matrix must retain BOARD_PENDING", errors)

    for forbidden in (
        "| **HAL** | ✅ | ✅ | ✅ | ✅ | 11 | 🟢 完善 |",
        "| **FOTA** | ✅ | ✅ | ✅ | ✅ | 🟢 主线可用",
        "- STM32U5 (完整实现)",
        "- ✅ HAL 统一工程 100% 完成",
        "🟢 完善",
        "🟢 tail host coverage 已收口",
        "### 🟡 主线可用",
        "总计：234 个测试用例",
        "Host-guarded / 分层证据 | 13 | 81%",
        "硬件、安全或人工证据待补 | 3 | 19%",
    ):
        require(forbidden not in index, f"public component index overclaims evidence: {forbidden}", errors)

    for required in (
        "HAL：PC Host contract、部分 QEMU；STM32U5/WCH/HC32 实板证据 pending",
        "FOTA：Host fail-closed contract；board Flash、bootloader、secure provider 与实板 pending",
        "STM32U5（source/compile 前置；Board pending）",
        "Host/PC/QEMU/compile-only 不构成实板、安全或 production-ready 证据",
        "测试数量以 canonical CTest 实际发现结果为准",
        "公开入口分类不是产品完成度或 maturity 百分比",
    ):
        require(required in index, f"public component index is missing evidence boundary: {required}", errors)

    return errors


def main() -> int:
    errors = validate()
    if errors:
        print("public_component_evidence failed:")
        for error in errors:
            print(f"- {error}")
        return 1
    print("public_component_evidence_ok hal=host-qemu-partial fota=host-fail-closed board=pending")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
