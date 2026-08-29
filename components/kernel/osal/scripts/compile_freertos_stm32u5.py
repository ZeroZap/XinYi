#!/usr/bin/env python3
"""Compile the XinYi FreeRTOS adapter/kernel for STM32U5 Cortex-M33.

This is a compile-only integration gate. It does not link startup/HAL code or
provide runtime, ISR, concurrency, or hardware evidence.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[4]
KERNEL = ROOT / "third_party" / "freertos" / "FreeRTOS"
OSAL = ROOT / "components" / "kernel" / "osal"
PORT = OSAL / "portable" / "freertos" / "GCC" / "ARM_CM33_NTZ" / "non_secure"
CONFIG = OSAL / "config" / "freertos"

SOURCES = (
    KERNEL / "tasks.c",
    KERNEL / "queue.c",
    KERNEL / "list.c",
    KERNEL / "timers.c",
    KERNEL / "event_groups.c",
    KERNEL / "portable" / "MemMang" / "heap_4.c",
    PORT / "port.c",
    PORT / "portasm.c",
    OSAL / "backend" / "freertos" / "xy_os_freertos.c",
)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--compiler",
        default="/home/eugene/Tools/arm-gnu-toolchain/bin/arm-none-eabi-gcc",
    )
    args = parser.parse_args()

    compiler = Path(args.compiler)
    if not compiler.is_file() and shutil.which(args.compiler) is None:
        print(f"freertos_stm32u5_compile failed: compiler not found: {args.compiler}")
        return 2

    required = (*SOURCES, KERNEL / "include" / "FreeRTOS.h", CONFIG / "FreeRTOSConfig.h")
    missing = [str(path.relative_to(ROOT)) for path in required if not path.is_file()]
    if missing:
        print("freertos_stm32u5_compile failed: required inputs are missing:")
        for path in missing:
            print(f"- {path}")
        return 2

    common = [
        args.compiler,
        "-std=c99",
        "-mcpu=cortex-m33",
        "-mthumb",
        "-mfpu=fpv5-sp-d16",
        "-mfloat-abi=hard",
        "-ffreestanding",
        "-fno-builtin",
        "-ffunction-sections",
        "-fdata-sections",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-DXY_OS_BACKEND_FREERTOS=1",
        "-DUSE_FREERTOS=1",
        f"-I{CONFIG}",
        f"-I{KERNEL / 'include'}",
        f"-I{PORT}",
        f"-I{OSAL}",
        f"-I{OSAL / 'inc'}",
    ]

    with tempfile.TemporaryDirectory(prefix="xinyi-freertos-stm32u5-") as tmp:
        output = Path(tmp)
        for source in SOURCES:
            obj = output / (source.name + ".o")
            result = subprocess.run(
                [*common, "-c", str(source), "-o", str(obj)],
                cwd=ROOT,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                check=False,
            )
            if result.returncode != 0:
                print(f"freertos_stm32u5_compile failed: {source.relative_to(ROOT)}")
                print(result.stdout, end="")
                return result.returncode

    print(
        "freertos_stm32u5_compile_ok "
        f"objects={len(SOURCES)} cpu=cortex-m33 evidence=compile-only"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
