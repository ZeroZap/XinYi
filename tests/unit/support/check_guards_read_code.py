#!/usr/bin/env python3
"""Guards that judge C source have to read code, and this says which ones do.

source_scan.read_code() strips comments before a guard looks for a symbol. The defect it
fixes runs in both directions, and only one of them is loud:

- a forbidden symbol named in a comment (usually to explain why it is *not* used) makes a
  guard red, so writing the reason down breaks the build;
- a required symbol named only in a comment satisfies the contract. This is the one that
  matters, because it is silent: the guard prints green and the behaviour was never there.

The fix is per call site, so this checks call sites rather than restating the rule in each
guard's docstring. A guard is registered here when it decides something from C source. For
every registered guard:

  1. it imports from source_scan and calls read_code() at least once;
  2. it does not read a C source file through .read_text(): the call site has to name the
     choice, read_code() for C and read_raw() for anything else.

A file that reads C source but is not registered is reported rather than skipped, so the
next guard cannot quietly opt out; exempting a file requires a stated reason, because "it
does not apply here" and "nobody looked" look the same in a list of numbers.

The C-file test is a name map, not full parsing: a variable is treated as C when the
statement assigning it mentions a .c/.h/.ld literal. A path assembled at runtime (f-strings,
glob results) is not resolved - that is the boundary, and it is why a registration is still
a human decision rather than something this can infer.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
SUPPORT = Path(__file__).resolve().parent

C_SUFFIX = re.compile(r"\.(?:c|h|ld)\b")
C_LITERAL = re.compile(r'"[^"]*\.(?:c|h|ld)"')
ASSIGN = re.compile(r"^\s*([A-Za-z_]\w*)\s*=", re.M)

# Guards that decide something by looking at C source.
REGISTRY = (
    "tests/test_osal_backend_contract.py",
    "tests/unit/display/check_pandora_st7789_board.py",
    "tests/unit/fota/test_pandora_fota_bootloader_layout.py",
    "tests/unit/hal_component/check_board_irq_vectors.py",
    "tests/unit/hal_component/check_error_code_alignment.py",
    "tests/unit/hal_component/check_hal_platform_build_contract.py",
    "tests/unit/hal_component/check_opench_osal_hal_usage.py",
    "tests/unit/hal_component/check_pandora_stm32l475_board.py",
    "tests/unit/hal_component/check_stm32_uart_rx_irq_contract.py",
    "tests/unit/hal_component/check_stm32_hal_timebase_hook.py",
    "tests/unit/hal_component/check_stm32f4_dma_wrapper_contract.py",
    "tests/unit/hal_component/check_stm32f4_i2c_wrapper_contract.py",
    "tests/unit/hal_component/check_stm32f4_spi_wrapper_contract.py",
    "tests/unit/hal_component/check_stm32f4_uart_wrapper_contract.py",
    "tests/unit/hal_component/check_wch_hal_tick_timebase.py",
    "tests/unit/hal_component/check_wch_i2c_lifecycle.py",
    "tests/unit/hal_component/check_wch_spi_lifecycle.py",
    "tests/unit/hal_component/test_pandora_hal_boundary_policy.py",
    "tests/unit/kernel/check_freertos_hal_timebase_binding.py",
    "tests/unit/kernel/check_freertos_osal_thread_flags.py",
    "tests/unit/kernel/check_pandora_freertos_runtime.py",
    "tests/unit/kernel/check_reference_rtos_decision.py",
    "tests/unit/kernel/check_rtthread_osal_context_atomicity.py",
    "tests/unit/kernel/check_rtthread_osal_event_flags.py",
    "tests/unit/kernel/check_rtthread_osal_kernel_lock.py",
    "tests/unit/kernel/check_rtthread_osal_mempool.py",
    "tests/unit/kernel/check_rtthread_osal_msgqueue.py",
    "tests/unit/kernel/check_rtthread_osal_priority.py",
    "tests/unit/kernel/check_rtthread_osal_sync_lifecycle.py",
    "tests/unit/kernel/check_rtthread_osal_thread_flags.py",
    "tests/unit/kernel/check_rtthread_osal_thread_lifecycle.py",
    "tests/unit/kernel/check_rtthread_osal_timeout.py",
    "tests/unit/kernel/check_rtthread_osal_timer.py",
    "tests/unit/net/check_net_product_selection.py",
    "tests/unit/pm/check_charger_ownership.py",
    "tests/unit/pm/check_pm_charger_ownership.py",
    "tests/unit/sensor/check_sensor_active_source_manifest.py",
)

# Files that mention C sources without judging them. Each needs the reason it does not
# read code, because the alternative - leaving them out of every list - is indistinguishable
# from never having looked at them.
EXEMPTIONS = {
    "tests/test_kconfig_parser.py":
        "parses Kconfig text in temp directories; no C source is read",
    "tests/unit/check_pc_artifact_reproducibility.py":
        "compares built artifact hashes and SBOM/licence metadata; the C paths are build inputs",
    "tests/unit/check_public_component_evidence.py":
        "reads documentation only",
    "tests/unit/check_release_input_inventory.py":
        "reads the release-input JSON manifest",
    "tests/unit/clib/mutation_check_frame.py":
        "rewrites xy_frame.c, so it needs the raw text it is going to mutate",
    "tests/unit/crypto/check_crypto_curve25519_root_policy.py":
        "reads CMake, JSON, Makefile and Markdown; the C paths are entries in those files",
    "tests/unit/crypto/check_crypto_review_manifest.py":
        "reads the review manifest and its CMake registration",
    "tests/unit/fota/test_pandora_fota_candidate_tool.py":
        "reads the ASCII header the tool generates, which is payload bytes rather than code",
    "tests/unit/hal_component/check_osal_backend_paths.py":
        "reads the OSAL CMakeLists and checks third-party headers exist",
    "tests/unit/check_pandora_release_sbom.py":
        "reads JSON policy/records and linker .map listings to hash release inputs",
    "tests/unit/hal_component/check_hal_uart_blocking_contract.py":
        "PENDING: new guard from the parallel UART receive-contract slice; it still reads raw "
        "text and registers here when that slice lands",
}


def c_variables(text: str) -> set[str]:
    """Names assigned in a statement that mentions a C-file literal."""
    names: set[str] = set()
    for line in text.splitlines():
        if not C_LITERAL.search(line):
            continue
        match = ASSIGN.match(line)
        if match:
            names.add(match.group(1))
    return names


def c_read_violations(text: str) -> list[str]:
    """Reads that pass a C source through .read_text() instead of read_code()/read_raw()."""
    names = c_variables(text)
    problems: list[str] = []
    for number, line in enumerate(text.splitlines(), start=1):
        if ".read_text(" not in line:
            continue
        inline = re.search(r'"[^"]*\.(?:c|h|ld)"\)\s*\.read_text\(', line)
        receiver = re.search(r"\b([A-Za-z_]\w*)\.read_text\(", line)
        if inline:
            problems.append(f"line {number}: reads a C file literal through .read_text()")
        elif receiver and receiver.group(1) in names:
            problems.append(
                f"line {number}: reads {receiver.group(1)} (a C source) through .read_text()"
            )
        elif receiver and ".c" in line and C_SUFFIX.search(line):
            problems.append(f"line {number}: reads what looks like a C source through .read_text()")
    return problems


def guard_files(root: Path) -> list[Path]:
    files = []
    for path in sorted((root / "tests").rglob("*.py")):
        if "__pycache__" in path.parts or SUPPORT == path.parent:
            continue
        files.append(path)
    return files


def findings(root: Path = ROOT, registry=REGISTRY, exemptions=EXEMPTIONS) -> list[str]:
    problems: list[str] = []
    present = {path.relative_to(root).as_posix(): path for path in guard_files(root)}

    for entry in registry:
        path = present.get(entry)
        if path is None:
            problems.append(f"{entry}: registered as a code guard but the file is gone")
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        if "source_scan" not in text:
            problems.append(f"{entry}: scans C source but does not import source_scan")
        if "read_code(" not in text:
            problems.append(f"{entry}: scans C source but never calls read_code()")
        for violation in c_read_violations(text):
            problems.append(f"{entry}: {violation}")

    for entry, reason in exemptions.items():
        if entry in registry:
            problems.append(f"{entry}: listed as both a registered guard and an exemption")
        if entry not in present:
            problems.append(f"{entry}: exempted from the code-read rule but the file is gone")
        if not reason.strip():
            problems.append(f"{entry}: exemption has no stated reason")

    for name, path in present.items():
        if name in registry or name in exemptions:
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        if C_SUFFIX.search(text) and ".read_text(" in text:
            problems.append(
                f"{name}: reads C source but is neither a registered code guard nor an "
                f"exemption with a reason - register it and read code, or state why not"
            )

    return problems


def main() -> int:
    problems = findings()
    if problems:
        print("guards_read_code failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    print(
        f"guards_read_code ok registered={len(REGISTRY)} exempted={len(EXEMPTIONS)} "
        f"(reads C through read_code, names the choice at every site)"
    )
    for entry in sorted(EXEMPTIONS):
        print(f"  exempt: {entry}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
