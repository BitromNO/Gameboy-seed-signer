#!/usr/bin/env python3
"""Create an 8 KiB Pocket BTC inbox save file from an arbitrary package."""

import argparse
from pathlib import Path

SRAM_SIZE = 8 * 1024
MAX_PAYLOAD = 512


def xor_checksum(data: bytes) -> int:
    checksum = 0xFFFF
    for value in data:
        checksum ^= value
    return checksum


def main() -> None:
    parser = argparse.ArgumentParser(description="Create a Pocket BTC prepared-package .sav file")
    parser.add_argument("input", type=Path, help="Package to place in the Game Boy SRAM inbox")
    parser.add_argument("output", type=Path, help="Output .sav file")
    args = parser.parse_args()

    payload = args.input.read_bytes()
    if not payload:
        parser.error("input package is empty")
    if len(payload) > MAX_PAYLOAD:
        parser.error(f"input package exceeds {MAX_PAYLOAD} bytes")

    save = bytearray(SRAM_SIZE)
    save[0:4] = b"PBT1"
    save[4] = 1
    save[5] = 1
    save[6:8] = len(payload).to_bytes(2, "little")
    checksum = xor_checksum(save[0:8] + payload)
    save[8:10] = checksum.to_bytes(2, "little")
    save[10:10 + len(payload)] = payload
    args.output.write_bytes(save)
    print(f"Wrote {args.output} with {len(payload)} payload bytes and checksum {checksum:04X}")


if __name__ == "__main__":
    main()