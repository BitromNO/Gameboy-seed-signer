# Game Boy seed signer prototype

This is a small, fun, testnet-only prototype for a Game Boy-style Bitcoin signing workflow. It is not a secure hardware wallet and should not be used with real funds.

## Goal

Create a minimal offline signer concept that can:

- generate an HD wallet seed from BIP39 words
- derive a BIP32 xprv/xpub chain
- load a PSBT-like unsigned transaction
- display the transaction summary
- sign the transaction with the key material
- output the signed transaction in a simple JSON-like payload

## Important warning

This is a prototype for learning and experimentation only.

- use testnet only
- never use a real seed
- never attach a real wallet
- keep it offline
- use tiny amounts only

## Folder layout

- `prototype/` — Python logic that models the wallet signing flow
- `docs/` — design notes and build instructions
- `requirements.txt` — Python dependencies

## Quick start

```bash
cd /home/bitrom/gameboy-seed-signer
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
python3 prototype/demo.py
```

## Prototype behavior

The current prototype does the following:

1. Generates a valid mnemonic phrase
2. Derives a root key from the mnemonic seed
3. Creates a child wallet key for a simple path like `m/84'/1'/0'/0/0`
4. Prints the xpub and child info
5. Produces a sample “transaction” object that would be fed to a real signer

## Security note

This does not provide hardware-backed security, secure storage, tamper resistance, or safe transaction review for real funds.

It is meant as a toy project for un
