# Pocket BTC Development ROM

> **Development demo only. Never use real funds, a valuable seed phrase, private keys, or production transactions with this project.** It is an unfinished learning project with no secure element, no tamper resistance, no reviewed signing engine, and no secure persistent key storage.

This is a Nintendo Game Boy Pocket ROM project. It does not target the ESP32 sketch in the workspace root.

The ROM shows a rotating Bitcoin boot logo, then provides a D-pad/A/B-operated menu:

- `Up` / `Down`: change the selected entry
- `A`: open the selected screen
- `B`: return to the main menu
- `A`: plays a short original confirmation chime when an item or dice roll is accepted

The screens are development features and placeholders. The ROM does not parse PSBTs, derive Bitcoin addresses, generate a signed transaction, or provide a secure hardware wallet.

`DICE AUDIT` accepts manual D6 results with `Up` / `Down` and records each result with `A`. It requires 99 rolls, the conservative threshold for approximately 256 bits of independently supplied dice entropy. After the final roll, it presents all 32 bytes of raw SHA-256 entropy across four pages for manual transcription. Press `A` to page through it. Pressing `B` wipes the rolls and entropy from RAM before returning to the menu. It does not persist or export seed material to the cartridge `.sav` or SD card.

After 99 rolls, press `A` to open `SEED WORDS`. The 24 BIP39 English words are displayed six at a time. Use `Up` and `Down` to move between the four pages. Write the words down in order; do not photograph or send them electronically. Pressing `B` from either `DICE AUDIT` or `SEED WORDS` wipes the seed session from Game Boy RAM.

## Existing 24-Word Phrase Import

> **Do not use a valuable or production seed phrase.** This exists only to test the user interface and BIP39 validation path.

`SEED WORDS` can also display an existing valid 24-word English BIP39 phrase. The import path is intentionally session-only on the Game Boy, but the transfer `.sav` file is a complete secret while it exists on the PC or SD card.

On an offline computer, put the 24 words in a temporary text file and create the session package:

```sh
python3 tools/make_seed_import.py phrase.txt pocket_seed.sav
```

Run the ROM once so the Pro+ OS creates its save file. With the console powered off, replace that save file with `pocket_seed.sav`, then boot the ROM and select `LOAD SEED`. It verifies the package before `A` loads the phrase into RAM and opens `SEED WORDS`. After confirming the words, remove the `.sav` from the SD card and securely erase the temporary text file and `.sav` on the computer. This experimental route does not provide secure storage and must not be used for funds you cannot afford to lose.

## Pro+ SD-card Inbox

> **Do not load real transactions or any private material into a `.sav` file.** The inbox format is a development transport envelope, not a PSBT parser or secure communication channel.

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

The resulting `pocket_btc.gb` runs in a Game Boy emulator or on a Game Boy Pocket through a compatible flash cartridge. It is a 64 KiB MBC5 image with 8 KiB battery-backed SRAM for the Pro+ save-file experiment.

## Versioned ROM Releases

Every testable ROM is archived in `releases/` with the source, generator scripts, BIP39 word-list asset, and SHA-256 checksum that produced it. The first archived build is `v0.3.0`.

Create a versioned snapshot after a successful test:

```sh
make release VERSION=v0.3.1
```

To roll back, copy the desired `releases/vX.Y.Z/pocket_btc.gb` to the flash cartridge. Do not overwrite a previous release directory.