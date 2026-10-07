#!/usr/bin/env python3
from __future__ import annotations

import argparse
import select
import termios
import time


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("port")
    parser.add_argument("--samples", type=int, default=10)
    parser.add_argument("--interval-ms", type=int, default=100)
    parser.add_argument("--duration-ms", type=int, default=3000)
    args = parser.parse_args()
    with open(args.port, "rb", buffering=0) as stream:
        fd = stream.fileno()
        attrs = termios.tcgetattr(fd)
        attrs[0] = 0
        attrs[1] = 0
        attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
        attrs[3] = 0
        attrs[4] = termios.B115200
        attrs[5] = termios.B115200
        attrs[6][termios.VMIN] = 0
        attrs[6][termios.VTIME] = 0
        termios.tcsetattr(fd, termios.TCSANOW, attrs)
        end = time.monotonic() + args.duration_ms / 1000.0
        data = bytearray()
        while time.monotonic() < end:
            ready, _, _ = select.select([stream], [], [], end - time.monotonic())
            if ready:
                data.extend(stream.read(4096))
        text = data.decode("ascii", errors="replace")
        print(f"bytes={len(data)}")
        print(text, end="")
        if "OPENCH_UART1_ALIVE" not in text:
            print("OPENCH_UART1_RUNTIME_NOT_OBSERVED")
            return 1
        print("OPENCH_UART1_RUNTIME_OK")
        return 0


if __name__ == "__main__":
    raise SystemExit(main())
