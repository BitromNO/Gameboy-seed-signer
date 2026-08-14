# Pocket BTC Releases

Each subdirectory is an immutable, testable ROM snapshot. It contains:

- `pocket_btc.gb`: ROM copied to the Game Boy Pro+ SD card
- `main.c`: exact source used for the build
- `README.md`: project documentation at the release point
- `SHA256SUMS`: SHA-256 checksum of the ROM

Release directories use semantic version numbers: `vMAJOR.MINOR.PATCH`.

- Increase `PATCH` for visual or local behavior fixes.
- Increase `MINOR` when adding a user-visible feature.
- Increase `MAJOR` only for incompatible data/protocol changes.