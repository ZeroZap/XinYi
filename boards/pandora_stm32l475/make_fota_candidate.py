#!/usr/bin/env python3
"""Build and validate the Pandora W25Q128 FOTA candidate envelope."""

import argparse
import binascii
import hashlib
import json
from pathlib import Path
import re
import struct

MAGIC = 0x58424643
FORMAT_VERSION = 1
HEADER = struct.Struct("<IHHIIIIII")
APP_BASE = 0x08008000
APP_LIMIT = 0x0807E000
SRAM_RANGES = ((0x20000000, 0x20018000), (0x10000000, 0x10008000))
SLOT_SIZE = 0x00080000


def validate_source_commit(source_commit: str) -> str:
    if re.fullmatch(r"[0-9a-f]{40}", source_commit) is None:
        raise ValueError("source commit must be an exact lowercase 40-character Git SHA")
    return source_commit


def validate_image(image: bytes, load_address: int) -> tuple[int, int]:
    if len(image) < 8 or len(image) > SLOT_SIZE - HEADER.size:
        raise ValueError("image size is outside the candidate slot")
    initial_sp, reset_handler = struct.unpack_from("<II", image)
    if initial_sp & 7 or not any(low < initial_sp <= high for low, high in SRAM_RANGES):
        raise ValueError("invalid Cortex-M initial stack pointer")
    reset_address = reset_handler & ~1
    if not reset_handler & 1 or not load_address <= reset_address < APP_LIMIT:
        raise ValueError("invalid Cortex-M Thumb reset vector")
    if load_address != APP_BASE or load_address + len(image) > APP_LIMIT:
        raise ValueError("image does not fit the Pandora execution slot")
    return initial_sp, reset_handler


def build_candidate(image: bytes, image_version: int, load_address: int = APP_BASE) -> bytes:
    if not 0 < image_version <= 0xFFFFFFFF:
        raise ValueError("image version must be a non-zero uint32")
    validate_image(image, load_address)
    crc32 = binascii.crc32(image) & 0xFFFFFFFF
    header = HEADER.pack(
        MAGIC,
        FORMAT_VERSION,
        HEADER.size,
        HEADER.size,
        len(image),
        image_version,
        load_address,
        crc32,
        0,
    )
    return header + image


def inspect_candidate(candidate: bytes, source_commit: str | None = None) -> dict:
    if len(candidate) < HEADER.size:
        raise ValueError("candidate is shorter than its header")
    fields = HEADER.unpack_from(candidate)
    magic, fmt, header_size, image_offset, image_size, version, load_address, expected_crc, flags = fields
    if magic != MAGIC or fmt != FORMAT_VERSION or header_size != HEADER.size:
        raise ValueError("candidate header identity mismatch")
    if image_offset != HEADER.size or flags != 0 or image_size != len(candidate) - image_offset:
        raise ValueError("candidate header layout mismatch")
    image = candidate[image_offset:]
    initial_sp, reset_handler = validate_image(image, load_address)
    actual_crc = binascii.crc32(image) & 0xFFFFFFFF
    if actual_crc != expected_crc:
        raise ValueError("candidate image CRC32 mismatch")
    result = {
        "status": "PANDORA_FOTA_CANDIDATE_VALID",
        "format_version": fmt,
        "header_size": header_size,
        "image_offset": image_offset,
        "image_size": image_size,
        "image_version": version,
        "load_address": f"0x{load_address:08x}",
        "image_crc32": f"0x{actual_crc:08x}",
        "initial_sp": f"0x{initial_sp:08x}",
        "reset_handler": f"0x{reset_handler:08x}",
        "candidate_size": len(candidate),
        "candidate_sha256": hashlib.sha256(candidate).hexdigest(),
    }
    if source_commit is not None:
        result["source_commit"] = validate_source_commit(source_commit)
    return result


def write_c_header(candidate: bytes, output: Path, source_commit: str) -> None:
    validate_source_commit(source_commit)
    rows = []
    for offset in range(0, len(candidate), 12):
        rows.append("    " + ", ".join(f"0x{byte:02x}U" for byte in candidate[offset : offset + 12]))
    output.write_text(
        "#ifndef PANDORA_FOTA_CANDIDATE_BLOB_H\n"
        "#define PANDORA_FOTA_CANDIDATE_BLOB_H\n\n"
        "#include <stdint.h>\n\n"
        f'#define PANDORA_FOTA_CANDIDATE_SOURCE_COMMIT "{source_commit}"\n'
        "static const uint8_t pandora_fota_candidate_blob[] = {\n"
        + ",\n".join(rows)
        + "\n};\n"
        f"#define PANDORA_FOTA_CANDIDATE_BLOB_SIZE {len(candidate)}U\n\n"
        "#endif\n",
        encoding="ascii",
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--image", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--version", type=int)
    parser.add_argument("--candidate", type=Path)
    parser.add_argument("--metadata", type=Path)
    parser.add_argument("--source-commit")
    parser.add_argument("--c-header", type=Path)
    args = parser.parse_args()
    if args.candidate:
        candidate = args.candidate.read_bytes()
    elif args.image and args.output and args.version is not None:
        candidate = build_candidate(args.image.read_bytes(), args.version)
        args.output.write_bytes(candidate)
    else:
        parser.error("use --candidate, or --image/--output/--version")
    if args.c_header and args.source_commit is None:
        parser.error("--c-header requires --source-commit")
    if args.c_header:
        write_c_header(candidate, args.c_header, args.source_commit)
    result = inspect_candidate(candidate, args.source_commit)
    text = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.metadata:
        args.metadata.write_text(text, encoding="utf-8")
    print(text, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
