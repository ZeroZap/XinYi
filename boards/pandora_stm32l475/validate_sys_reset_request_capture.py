#!/usr/bin/env python3
"""Machine-check the Pandora SYS reset-request contract from a bounded UART log.

The smoke image records the reason handed to `xy_sys_reset()` in a retained
backup register, reports it on the next boot as `SYS_RESET_REQUEST <n>`, and
consumes the record one-shot. The reset-kind dispatch happens before the boot
banner, so the log is walked as `SYS_RESET_CSR` records rather than by banner.
This validator asserts the observed chain shows exactly the intended contract:
a pin-reset boot with no retained record, the software-reset boot that reports
the retained request, and the following watchdog boot with no stale record.
"""

import argparse
import json
import re
from pathlib import Path

FIRMWARE_COMMIT_PREFIX = b"FIRMWARE_COMMIT "
RECORD_START = b"SYS_RESET_CSR 0x"
CSR = re.compile(rb"\A([0-9A-Fa-f]{8})")
RESET_KIND = re.compile(rb"SYS_RESET_KIND ([A-Z_]+)")
RESET_REQUEST = re.compile(rb"SYS_RESET_REQUEST ([0-9]+)")

# Observed boot chain: (accepted reset kinds, retained request reason, meaning)
# The first post-program boot may report POWER_ON or EXTERNAL_PIN depending on the
# probe's reset method (a debug-port core reset does not set RCC reset flags), so
# position 0 accepts both non-SYS kinds while still requiring "no retained record".
EXPECTED_CHAIN = (
    (
        ("POWER_ON", "EXTERNAL_PIN"),
        0,
        "no record before the first SYS-requested reset",
    ),
    (("SOFTWARE",), 1, "reason recorded before the previous software reset"),
    (
        ("WATCHDOG",),
        0,
        "record consumed one-shot, not carried into the watchdog boot",
    ),
)


def split_records(payload: bytes) -> list[bytes]:
    parts = payload.split(RECORD_START)
    return parts[1:]


def parse_record(body: bytes) -> dict | None:
    kind_match = RESET_KIND.search(body)
    request_match = RESET_REQUEST.search(body)
    if kind_match is None or request_match is None:
        return None
    csr_match = CSR.search(body)
    return {
        "csr": csr_match.group(1).decode("ascii") if csr_match else None,
        "kind": kind_match.group(1).decode("ascii"),
        "request": int(request_match.group(1)),
    }


def find_chain(observed: list[tuple[str, int]]) -> int | None:
    def matches(start: int) -> bool:
        for offset, (kinds, request, _) in enumerate(EXPECTED_CHAIN):
            kind, seen_request = observed[start + offset]
            if kind not in kinds or seen_request != request:
                return False
        return True

    for start in range(len(observed) - len(EXPECTED_CHAIN) + 1):
        if matches(start):
            return start
    return None


def validate(payload: bytes, firmware_commit: str) -> tuple[bool, dict]:
    commit_marker = FIRMWARE_COMMIT_PREFIX + firmware_commit.encode("ascii")
    records = [parsed for parsed in (parse_record(body) for body in split_records(payload)) if parsed]
    observed = [(record["kind"], record["request"]) for record in records]

    problems = []
    if commit_marker not in payload:
        problems.append(f"capture is not bound to FIRMWARE_COMMIT {firmware_commit}")
    if len(records) < len(EXPECTED_CHAIN):
        problems.append(
            f"observed {len(records)} SYS reset record(s), expected at least {len(EXPECTED_CHAIN)}"
        )
    elif find_chain(observed) is None:
        expected_desc = [
            {"kinds": list(kinds), "request": request, "meaning": meaning}
            for kinds, request, meaning in EXPECTED_CHAIN
        ]
        problems.append(f"reset chain {observed} does not satisfy {expected_desc}")

    record = {
        "observed_chain": records,
        "expected_chain": [
            {"kinds": list(kinds), "request": request, "meaning": meaning}
            for kinds, request, meaning in EXPECTED_CHAIN
        ],
        "firmware_commit": firmware_commit,
        "bytes_examined": len(payload),
        "problems": problems,
    }
    return not problems, record


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--capture", type=Path, required=True)
    parser.add_argument("--firmware-commit", required=True)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    payload = args.capture.read_bytes()
    ok, record = validate(payload, args.firmware_commit)
    record["capture"] = str(args.capture)
    record["status"] = "PASS" if ok else "FAIL"
    if args.output is not None:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")

    if not ok:
        for problem in record["problems"]:
            print(f"SYS reset-request contract: FAIL: {problem}")
        return 1
    print("SYS reset-request contract: PASS")
    for kinds, request, meaning in EXPECTED_CHAIN:
        print(f"  {'/'.join(kinds)} -> SYS_RESET_REQUEST {request} ({meaning})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
