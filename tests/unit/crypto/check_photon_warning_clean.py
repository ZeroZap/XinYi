#!/usr/bin/env python3
"""Compile the active Photon-Beetle source with warnings as errors."""

from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / "components/crypto/xy_photon_beetle/xy_photon_beetle.c"
INCLUDE = ROOT / "components/crypto/xy_photon_beetle"

with tempfile.TemporaryDirectory(prefix="xinyi-photon-warning-") as temp:
    output = Path(temp) / "xy_photon_beetle.o"
    subprocess.run(
        [
            "cc",
            "-std=c99",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-I",
            str(INCLUDE),
            "-c",
            str(SOURCE),
            "-o",
            str(output),
        ],
        check=True,
    )
    if not output.is_file() or output.stat().st_size == 0:
        raise SystemExit("Photon-Beetle warning-clean object was not produced")

print("crypto_photon_warning_clean_ok")
