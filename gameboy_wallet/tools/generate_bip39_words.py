#!/usr/bin/env python3
"""Generate compact C data from the official BIP39 English word list."""

import sys
from pathlib import Path


def main() -> None:
    source, c_output, h_output = map(Path, sys.argv[1:4])
    words = source.read_text(encoding="ascii").splitlines()
    if len(words) != 2048 or any(not word.isalpha() or word != word.lower() for word in words):
        raise SystemExit("expected exactly 2048 lowercase ASCII BIP39 words")

    c_output.write_text(
        '#include "bip39_words.h"\n\n'
        'const char bip39_words[] =\n' + ''.join(f'  "{word}\\0"\n' for word in words) + '  ;\n',
        encoding="ascii",
    )
    h_output.write_text(
        '#ifndef BIP39_WORDS_H\n#define BIP39_WORDS_H\n\nextern const char bip39_words[];\n\n#endif\n',
        encoding="ascii",
    )


if __name__ == "__main__":
    main()