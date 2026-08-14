# Game Boy Seed Signer

> **Development demo only. Never use real funds, a valuable seed phrase, private keys, passphrases, or production transactions with this repository.** It is an unfinished learning project, not a secure hardware wallet.

This repository explores Bitcoin-related workflows on classic Game Boy hardware. It contains early Python experiments, historical ROM-header work, and an actively developed Game Boy Pocket interface for learning about offline workflows.
<img width="483" height="435" alt="Screenshot_2026-08-15_00-11-36" src="https://github.com/user-attachments/assets/1642a4aa-31ec-48a7-9cd7-17b30bff38ae" />
<img width="483" height="435" alt="Screenshot_2026-08-15_00-11-15" src="https://github.com/user-attachments/assets/6fa6a31e-649b-42aa-9691-f7971237affd" />

## Security Status

This project does **not** provide secure key storage, tamper resistance, a secure element, reviewed `secp256k1` signing, PSBT parsing, transaction signing, address derivation, or safe production backup handling.

- Never use real funds.
- Never enter a valuable recovery phrase.
- Never place private material in a `.sav` file, SD card, chat, issue, or screenshot.
- Treat every ROM and helper here as development software only.

## Project Layout

- `gameboy_wallet/` - Current Game Boy Pocket ROM, build scripts, release snapshots, and emulator/hardware test target.
- `gameboy_wallet/releases/` - Versioned, checksummed ROM snapshots for rollback.
- `prototype/` - Python concepts for wallet and signing workflows.
- `roms/` - Earlier ROM-header and cartridge boot experiments.
- `docs/` - Project notes and planning.

## Current Game Boy ROM

The Pocket ROM includes a navigable monochrome interface, dice-entry audit mode, volatile BIP39-word display for UI research, a Game Boy Pro+ SRAM inbox experiment, and a QR link to this repository. None of these features constitute a safe signer or wallet.

Build the current ROM with GBDK-2020:

```sh
cd gameboy_wallet
make
```

The current testable artifact is [gameboy_wallet/releases/v0.6.1/pocket_btc.gb](gameboy_wallet/releases/v0.6.1/pocket_btc.gb). Verify its checksum before testing:

```sh
sha256sum gameboy_wallet/releases/v0.6.1/pocket_btc.gb
```

See [gameboy_wallet/README.md](gameboy_wallet/README.md) for controls, build requirements, the Pro+ save-file experiment, and important limitations.

## Development Workflow

Every successful hardware or emulator build is preserved in `gameboy_wallet/releases/vX.Y.Z/` with its ROM, source, generated assets, scripts, and SHA-256 checksum. Do not overwrite an existing release; create a new version for each testable change.

## Roadmap

- Improve Game Boy UI and hardware behavior through emulator and Pocket testing.
- Keep file transport and seed-session experiments clearly separated from production wallet claims.
- Add cryptographic functionality only with published test vectors and independent review.
- Do not use this project with real Bitcoin funds until a complete security design and audit exist.
