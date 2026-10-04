#!/usr/bin/env python3
from __future__ import annotations

import argparse
import select
import termios
import time
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("port")
    parser.add_argument("--samples", type=int, default=10)
    parser.add_argument("--interval-ms", type=int, default=100)
    parser.add_argument("--duration-ms", type=int, default=3000)
    args = parser.parse_args()
    fd = open(args.port, "rb", buffering=0)
    try:
        attrs = termios.tcgetattr(fd.fileno())
        attrs[0] = 0
        attrs[1] = 0
        attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
        attrs[3] = 0
        attrs[4] = termios.B115200
        attrs[5] = termios.B115200
        attrs[6][termios.VMIN] = 0
        attrs[6][termios.VTIME] = 0
        termios.tcsetattr(fd.fileno(), termios.TCSANOW, attrs)
        end = time.monotonic() + args.duration_ms / 1000.0
        data = bytearray()
        while time.monotonic() < end:
            ready, _, _ = select.select([fd], [], [], max(0.0, end - time.monotonic()))
            if ready:
                data.extend(fd.read(4096))
        text = data.decode("ascii", errors="replace")
        print(f"bytes={len(data)}")
        print(text, end="")
        required = (
            "XINYI OPENCH CH32V307 UART1 READY",
            "PA9=UART1_TX PA10=UART1_RX WCHLINK=CH549F",
            "OPENCH_UART1_ALIVE",
        )
        missing = [marker for marker in required if marker not in text]
        if missing:
            print(f"missing_markers={missing}")
            return 1
        print("OPENCH_UART1_RUNTIME_OK")
        return 0
    finally:
        fd.close()


if __name__ == "__main__":
    raise SystemExit(main())
