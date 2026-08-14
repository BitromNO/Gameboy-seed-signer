#!/usr/bin/env python3
"""Generate a fixed QR matrix for the Pocket BTC repository URL."""

import sys
from pathlib import Path

import qrcode

URL = "https://github.com/BitromNO/Gameboy-seed-signer"


def main() -> None:
    c_output, h_output = map(Path, sys.argv[1:3])
    code = qrcode.QRCode(error_correction=qrcode.constants.ERROR_CORRECT_L, border=2)
    code.add_data(URL)
    code.make(fit=True)
    matrix = code.get_matrix()
    size = len(matrix)
    if size > 36:
        raise SystemExit(f"QR matrix is too large for the Game Boy layout: {size}")
    values = ", ".join("1" if cell else "0" for row in matrix for cell in row)
    c_output.write_text(
        '#include "repo_qr.h"\n\n'
        f'const uint8_t repo_qr_size = {size}u;\n'
        f'const uint8_t repo_qr_matrix[{size * size}] = {{ {values} }};\n',
        encoding="ascii",
    )
    h_output.write_text(
        '#ifndef REPO_QR_H\n#define REPO_QR_H\n\n#include <stdint.h>\n\n'
        'extern const uint8_t repo_qr_size;\nextern const uint8_t repo_qr_matrix[];\n\n#endif\n',
        encoding="ascii",
    )


if __name__ == "__main__":
    main()