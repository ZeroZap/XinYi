"""Put each shipped defect back into the framer and check the tests notice.

A test that never fails is not a test. Each mutation below is a bug that actually shipped
in the openCH probe, so the set says something specific: these tests would have caught
the three defects that cost a full debugging round on the board.

The source is restored in a finally block - leaving a mutated framer behind in a checkout
would be a far worse outcome than a failed test.
"""

import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

SRC = Path(__file__).resolve().parents[3] / "components" / "clib" / "xy_clib" / "xy_frame.c"
BUILD_DIR = Path(__file__).resolve().parents[3] / "build" / "tests" / "unit"

MUTATIONS = {
    "0x02 mid-payload restarts the frame": (
        """    if (decoder->want == 0U) {
        if (byte == XY_FRAME_STX) {""",
        """    if (byte == XY_FRAME_STX) {
        if (byte == XY_FRAME_STX) {""",
    ),
    "length accounting waits one byte too long": (
        """        decoder->want = (uint8_t)(byte + 1U); /* what still has to arrive */""",
        """        decoder->want = (uint8_t)(byte + 2U); /* what still has to arrive */""",
    ),
    "an impossible length is accepted": (
        """        if (byte < 2U || byte > XY_FRAME_PAYLOAD_MAX + 1U) {""",
        """        if (0) {""",
    ),
    "the checksum is a xor sum again": (
        """        crc ^= bytes[i];
        for (bit = 0U; bit < 8U; ++bit) {
            if ((crc & 0x80U) != 0U) {
                crc = (uint8_t)((uint8_t)(crc << 1) ^ 0x07U);
            } else {
                crc = (uint8_t)(crc << 1);
            }
        }""",
        """        (void)bit;
        crc ^= bytes[i];""",
    ),
    "the polynomial is not the standard one": (
        """                crc = (uint8_t)((uint8_t)(crc << 1) ^ 0x07U);""",
        """                crc = (uint8_t)((uint8_t)(crc << 1) ^ 0x1DU);""",
    ),
    "the checksum is not verified": (
        """        if (expected != actual) {""",
        """        if (0) {""",
    ),
}


def build_and_run() -> tuple[bool, str]:
    build = subprocess.run(
        ["cmake", "--build", str(BUILD_DIR), "--target", "test_xy_frame"],
        capture_output=True, text=True,
    )
    if build.returncode != 0:
        return False, "build failed"
    result = subprocess.run([str(BUILD_DIR / "test_xy_frame")], capture_output=True, text=True)
    return result.returncode == 0, result.stdout


def main() -> int:
    if not SRC.is_file():
        print(f"mutation check needs {SRC}")
        return 1
    original = SRC.read_text()
    backup = Path(tempfile.mkstemp(suffix=".c")[1])
    shutil.copyfile(SRC, backup)

    caught = 0
    try:
        for name, (old, new) in MUTATIONS.items():
            if old not in original:
                print(f"!! anchor missing for: {name}")
                continue
            SRC.write_text(original.replace(old, new, 1))
            passed, output = build_and_run()
            failures = [line.split(":")[1] for line in output.splitlines() if ":FAIL:" in line]
            if passed:
                print(f"MISSED  {name}")
            else:
                caught += 1
                print(f"caught  {name} -> {', '.join(failures[:3])}")
    finally:
        SRC.write_text(original)
        backup.unlink(missing_ok=True)

    passed, _ = build_and_run()
    print(f"\n{caught}/{len(MUTATIONS)} mutations caught; restored source passes: {passed}")
    return 0 if caught == len(MUTATIONS) and passed else 1


if __name__ == "__main__":
    sys.exit(main())
