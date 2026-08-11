# Game Boy seed signer prototype
# Game Boy Seed Signer

This is a small, fun, testnet-only prototype for a Game Boy-style Bitcoin signing workflow. It is not a secure hardware wallet and should not be used with real funds.
A playful prototype for a Bitcoin seed-signing concept built around the look and feel of a classic Game Boy cartridge.

## Goal
This project is for experimentation, testnet-only learning, and toy hardware-wallet ideas. It is not a secure wallet, not meant for production use, and should never be used with real funds.

Create a minimal offline signer concept that can:
## Highlights

- generate an HD wallet seed from BIP39 words
- derive a BIP32 xprv/xpub chain
- load a PSBT-like unsigned transaction
- display the transaction summary
- sign the transaction with the key material
- output the signed transaction in a simple JSON-like payload
- BIP39 mnemonic generation
- BIP32 derivation using a hardened wallet path
- testnet-oriented signing workflow experiments
- cartridge-style ROM generation concepts
- a clean GitHub-ready project layout for future work

## Important warning
## Warning

This is a prototype for learning and experimentation only.
> This project is not a secure hardware wallet.
> It is for educational and testnet-only experimentation.
> Never use a real seed or production wallet keys.
> Keep the device offline and use tiny amounts only.
- use testnet only
- never use a real seed
- never attach a real wallet
- keep it offline
- use tiny amounts only
## Project structure

## Folder layout
- `prototype/` — Python logic that models the wallet signing flow
- `docs/` — design notes and build instructions
- `prototype/` — Python prototypes for seed generation and signing flow
- `roms/` — Game Boy ROM generation experiments and cartridge packaging
- `docs/` — notes, planning, and design ideas
- `requirements.txt` — Python dependencies

## Quick start
@@ -39,18 +36,34 @@ pip install -r requirements.txt
python3 prototype/demo.py
```

## Prototype behavior
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

The current prototype does the following:
## License

1. Generates a valid mnemonic phrase
2. Derives a root key from the mnemonic seed
3. Creates a child wallet key for a simple path like `m/84'/1'/0'/0/0`
4. Prints the xpub and child info
5. Produces a sample “transaction” object that would be fed to a real signer
This repository is provided for experimentation and learning. Use at your own risk.

## Security note
## Contributing

This does not provide hardware-backed security, secure storage, tamper resistance, or safe transaction review for real funds.
Feel free to fork this repo and continue development. Keep changes focused on educational and testnet-safe experimentation.

It is meant as a toy project for un
