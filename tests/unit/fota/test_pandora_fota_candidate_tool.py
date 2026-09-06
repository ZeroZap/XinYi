#!/usr/bin/env python3
"""Unit contract for Pandora candidate envelope construction and validation."""

import binascii
from pathlib import Path
import struct
import sys
import tempfile

BOARD = Path(__file__).resolve().parents[3] / "boards" / "pandora_stm32l475"
sys.path.insert(0, str(BOARD))

from make_fota_candidate import (  # noqa: E402
    HEADER,
    build_candidate,
    inspect_candidate,
    validate_source_commit,
    write_c_header,
)


def expect_value_error(callable_) -> None:
    try:
        callable_()
    except ValueError:
        return
    raise AssertionError("expected ValueError")


def main() -> int:
    image = bytearray(b"\xff" * 64)
    struct.pack_into("<II", image, 0, 0x20018000, 0x08008021)
    candidate = build_candidate(bytes(image), 3)
    result = inspect_candidate(candidate)
    assert result["status"] == "PANDORA_FOTA_CANDIDATE_VALID"
    assert result["image_version"] == 3
    assert result["image_size"] == len(image)
    assert result["candidate_size"] == len(image) + HEADER.size
    assert int(result["image_crc32"], 16) == binascii.crc32(image) & 0xFFFFFFFF

    tampered = bytearray(candidate)
    tampered[-1] ^= 1
    expect_value_error(lambda: inspect_candidate(bytes(tampered)))
    expect_value_error(lambda: build_candidate(bytes(image), 0))
    bad_vector = bytearray(image)
    struct.pack_into("<I", bad_vector, 4, 0x08008020)
    expect_value_error(lambda: build_candidate(bytes(bad_vector), 3))
    expect_value_error(lambda: validate_source_commit("abc"))
    expect_value_error(lambda: validate_source_commit("A" * 40))

    with tempfile.TemporaryDirectory() as temporary:
        path = Path(temporary) / "candidate.bin"
        path.write_bytes(candidate)
        assert inspect_candidate(path.read_bytes())["image_version"] == 3
        header = Path(temporary) / "candidate.h"
        source_commit = "0123456789abcdef0123456789abcdef01234567"
        write_c_header(candidate, header, source_commit)
        generated = header.read_text(encoding="ascii")
        assert f'PANDORA_FOTA_CANDIDATE_SOURCE_COMMIT "{source_commit}"' in generated
        assert f"PANDORA_FOTA_CANDIDATE_BLOB_SIZE {len(candidate)}U" in generated
        assert "0x43U, 0x46U, 0x42U, 0x58U" in generated
        result = inspect_candidate(candidate, source_commit)
        assert result["source_commit"] == source_commit
        assert len(result["candidate_sha256"]) == 64
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
