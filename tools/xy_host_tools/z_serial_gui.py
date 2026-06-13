from __future__ import annotations

from xy_host_tools.serial_cli import main as cli_main


def main() -> int:
    return cli_main(["gui"])


if __name__ == "__main__":
    raise SystemExit(main())
