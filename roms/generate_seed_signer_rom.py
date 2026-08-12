#!/usr/bin/env python3
"""Create a prototype Game Boy ROM for the seed-signer concept.

This script writes a 32 KiB .gb image with a Nintendo-compatible header and a
small boot payload so it can be tested on a flash cartridge. It is intended for
fun, prototype validation, and offline testnet experiments only.
"""

from __future__ import annotations

from pathlib import Path

TITLE = "BTCSEED"
OUTPUT = Path(__file__).with_name("gb_seed_signer_test.gb")

NINTENDO_LOGO = bytes([
    0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83,
    0x00, 0x0C, 0x00, 0x0D, 0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
    0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63,
    0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E,
])


def header_checksum(data: bytes) -> int:
    total = 0
    for index in range(0x0134, 0x014D):
        total = (total - data[index] - 1) & 0xFF
    return total & 0xFF


def make_rom() -> bytes:
    rom = bytearray(32 * 1024)

    # Standard Nintendo logo must be present for a valid-looking GB ROM header.
    rom[0x0104:0x0134] = NINTENDO_LOGO

    # Title and basic cartridge metadata.
    title_bytes = TITLE.encode("ascii")
    rom[0x0134:0x0134 + len(title_bytes)] = title_bytes
    rom[0x0143] = 0x00
    rom[0x0144] = 0x00
    rom[0x0145] = 0x00
    rom[0x0146] = 0x00
    rom[0x0147] = 0x00  # ROM only, no banking
    rom[0x0148] = 0x00  # 32 KiB ROM
    rom[0x0149] = 0x00  # no RAM
    rom[0x014A] = 0x00  # non-Japanese
    rom[0x014B] = 0x00
    rom[0x014C] = 0x00

    # Minimal ROM payload to make the cartridge look like a custom demo ROM.
    payload = (
        "BTC SEED SIGNER\n"
        "TESTNET DEMO\n"
        "FOR FUN ONLY\n"
        "LOAD ROM TO TEST\n"
    ).encode("ascii")
    for offset, byte in enumerate(payload):
        if 0x0150 + offset < len(rom):
            rom[0x0150 + offset] = byte

    # Boot sequence: NOP + JP 0x0150.
    rom[0x0100:0x0104] = bytes([0x00, 0xC3, 0x50, 0x01])

    # Header checksum and simple global checksum values.
    rom[0x014D] = header_checksum(bytes(rom))
    rom[0x014E] = 0x00
    rom[0x014F] = 0x00

    return bytes(rom)


def main() -> None:
    rom = make_rom()
    OUTPUT.write_bytes(rom)
    print(f"Created {OUTPUT} ({len(rom)} bytes)")
    print(f"Title: {TITLE}")
    print("This is a prototype ROM test image for cartridge validation only.")


if __name__ == "__main__":
    main()
