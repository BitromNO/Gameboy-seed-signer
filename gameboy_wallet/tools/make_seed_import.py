#!/usr/bin/env python3
"""Validate a 24-word BIP39 phrase and package its entropy for Pocket BTC."""

import argparse
import hashlib
from pathlib import Path

SRAM_SIZE = 8 * 1024
WORDLIST = Path(__file__).parent.parent / "assets" / "bip39_english.txt"


def xor_checksum(data: bytes) -> int:
    checksum = 0xFFFF
    for value in data:
        checksum ^= value
    return checksum


def mnemonic_entropy(phrase: str) -> bytes:
    words = phrase.lower().split()
    wordlist = WORDLIST.read_text(encoding="ascii").splitlines()
    lookup = {word: index for index, word in enumerate(wordlist)}
    if len(words) != 24:
        raise ValueError("only a 24-word BIP39 phrase is supported")
    try:
        indexes = [lookup[word] for word in words]
    except KeyError as error:
        raise ValueError(f"unknown BIP39 word: {error.args[0]}") from error
    bits = "".join(f"{index:011b}" for index in indexes)
    entropy = int(bits[:256], 2).to_bytes(32, "big")
    if int(bits[256:], 2) != hashlib.sha256(entropy).digest()[0]:
        raise ValueError("BIP39 checksum does not match; phrase may contain a typo")
    return entropy


def main() -> None:
    parser = argparse.ArgumentParser(description="Create a secret Pocket BTC seed-session .sav file")
    parser.add_argument("phrase_file", type=Path, help="Text file containing exactly 24 BIP39 words")
    parser.add_argument("output", type=Path, help="Output .sav file")
    args = parser.parse_args()

    entropy = mnemonic_entropy(args.phrase_file.read_text(encoding="utf-8"))
    save = bytearray(SRAM_SIZE)
    save[0:4] = b"PBS1"
    save[4] = 1
    save[5] = 2
    save[6:8] = (32).to_bytes(2, "little")
    checksum = xor_checksum(save[0:8] + entropy)
    save[8:10] = checksum.to_bytes(2, "little")
    save[10:42] = entropy
    args.output.write_bytes(save)
    print(f"Wrote secret session file {args.output} ({checksum:04X})")


if __name__ == "__main__":
    main()