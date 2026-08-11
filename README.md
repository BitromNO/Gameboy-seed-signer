# Game Boy Seed Signer

A playful prototype for a Bitcoin seed-signing concept built around the look and feel of a classic Game Boy cartridge.

This project is for experimentation, testnet-only learning, and toy hardware-wallet ideas. It is not a secure wallet, not meant for production use, and should never be used with real funds.

## Highlights

- BIP39 mnemonic generation
- BIP32 derivation using a hardened wallet path
- testnet-oriented signing workflow experiments
- cartridge-style ROM generation concepts
- a clean GitHub-ready project layout for future work

## Warning

> This project is not a secure hardware wallet.
> It is for educational and testnet-only experimentation.
> Never use a real seed or production wallet keys.
> Keep the device offline and use tiny amounts only.

## Project structure

- `prototype/` — Python prototypes for seed generation and signing flow
- `roms/` — Game Boy ROM generation experiments and cartridge packaging
- `docs/` — notes, planning, and design ideas
- `requirements.txt` — Python dependencies

## Quick start

```bash
cd /home/bitrom/gameboy-seed-signer
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
python3 prototype/demo.py
```

## What the prototype does

The current demo flow includes:

1. Generating a BIP39 mnemonic
2. Converting the mnemonic into a seed
3. Deriving a BIP32 root and child keys
4. Creating a path similar to `m/84'/1'/0'/0/0`
5. Printing a transaction-like summary for signing experiments

## ROM experiments

The `roms/` folder contains a simple cartridge generator that creates a Game Boy ROM-style test artifact with a BTC-themed title and valid cartridge header structure.

This is primarily a prototype artifact for display and experimentation, not a real secure signing device.

## Roadmap

- improve the wallet UX for a Game Boy-inspired menu system
- add cleaner transaction review flow
- explore PSBT-style input/output handling
- move from demo logic toward a more realistic offline signer

## License

This repository is provided for experimentation and learning. Use at your own risk.

## Contributing

Feel free to fork this repo and continue development. Keep changes focused on educational and testnet-safe experimentation.

