# ROM Notes

This folder contains cartridge-oriented test artifacts for the Game Boy seed-signer concept.

The goal is not to produce a secure wallet, but to validate the idea on a flash cartridge and keep the project testable and fun.

## Current artifact

- `gb_seed_signer_test.gb` — prototype ROM file generated for cartridge testing
- `generate_seed_signer_rom.py` — script that builds the ROM image

## How to test it

1. Copy the generated `.gb` file to the SD/TF card used by your Game Boy flash cartridge.
2. Insert the cartridge into the Game Boy.
3. Boot the cartridge and check that it loads the ROM from the flash menu.
4. Confirm the boot screen and title text appear before moving to more elaborate UI work.

## Important warning

This is a prototype only. It is meant for hardware validation and offline fun experiments, not for real seed handling or real funds.
