# Pocket BTC Demo ROM

This is a Nintendo Game Boy ROM project. It does not target the ESP32 sketch in the workspace root.

The ROM shows a rotating Bitcoin boot logo, then provides a D-pad/A/B-operated menu:

- `Up` / `Down`: change the selected entry
- `A`: open the selected screen
- `B`: return to the main menu
- `A`: plays a short original confirmation chime when an item or dice roll is accepted

The screens are intentionally non-custodial placeholders. This ROM has no seed phrase, private key, signing implementation, or real receiving address.

`DICE AUDIT` accepts manual D6 results with `Up` / `Down` and records each result with `A`. It requires 99 rolls, the conservative threshold for approximately 256 bits of independently supplied dice entropy. It intentionally does not generate a mnemonic or persist rolls: that conversion is security-critical and remains unimplemented until it has test vectors and an independently reproducible derivation.

## Pro+ SD-card Inbox

The Game Boy Pro+ / EDGB V4 does not expose its SD-card filesystem directly to a running ROM. It does persist battery-backed cartridge SRAM as the game save file, so this ROM uses its 8 KiB `.sav` as an offline inbox.

Create an inbox save file from a prepared package of at most 512 bytes:

```sh
python3 tools/make_inbox_save.py prepared-package.bin pocket_btc.sav
```

First run `pocket_btc.gb` on the cartridge so the Pro+ OS creates its matching save file and note the save directory and filename it uses. With the Game Boy powered off, replace that save file with `pocket_btc.sav`, then start the ROM and open `TX INBOX`.

The ROM verifies only the package envelope (`PBT1`, version, length, and checksum). It does not parse PSBTs or sign anything yet. Do not place private keys, a seed phrase, or real transaction data in this experimental save file.

## Build

The workspace includes GBDK-2020 locally in `.tools/gbdk`. Build the ROM with:

```sh
cd gameboy_wallet
make
```

For a different GBDK installation location:

```sh
make GBDK_HOME=/path/to/gbdk
```

The resulting `pocket_btc.gb` runs in a Game Boy emulator or on a Game Boy Pocket through a compatible flash cartridge. It is a 32 KiB ROM-only cartridge image, which is broadly compatible with Game Boy Pocket flash cartridges.

## Versioned ROM Releases

Every testable ROM is archived in `releases/` with the source, build notes, and SHA-256 checksum that produced it. The first archived build is `v0.3.0`.

Create a versioned snapshot after a successful test:

```sh
make release VERSION=v0.3.1
```

To roll back, copy the desired `releases/vX.Y.Z/pocket_btc.gb` to the flash cartridge. Do not overwrite a previous release directory.