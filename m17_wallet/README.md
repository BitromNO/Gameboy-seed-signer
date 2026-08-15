# M17 Pocket Signer (Development)

> **Development-only experiment. Never use real funds, production seeds, private keys, passphrases, or real transactions with this project.** The M17 has no verified boot, secure element, tamper resistance, or secure key storage.

This project targets the SJGAM M17 running MinUI. It is separate from `gameboy_wallet/`.

## Current State

The current host build validates the PSBT magic prefix and structurally parses the global key-value map using bounded canonical CompactSize decoding. It detects declared PSBT v0/v2 global fields and rejects duplicate or inconsistent version declarations. For v2, it requires and bounds the declared input/output counts. For v0, it parses the unsigned transaction enough to count inputs/outputs and safely total outputs in sats. It classifies recognized output scripts as P2PKH, P2SH, P2WPKH, P2WSH, P2TR, or OP_RETURN. Mainnet P2WPKH, P2WSH, and Taproot scripts can be rendered as tested Bech32/Bech32m addresses. It does not yet render Base58 legacy addresses, calculate fees or change, derive keys, decrypt a vault, sign, export a signed PSBT, or provide an M17 graphical interface.

The first M17 deployment target is a MinUI package:

```text
/sdcard/.system/m17/paks/PocketSigner.pak/
```

The package launcher is included, but the ARM hard-float M17 executable is not built until the M17 cross toolchain and SDL2 headers/libraries are collected from a compatible firmware sysroot.

## Host Test

```sh
cd m17_wallet
make test
```

## Intended Workflow

```text
PC wallet -> unsigned PSBT on SD -> M17 review -> explicit confirmation
-> signed PSBT on SD -> PC wallet broadcast
```

No signing workflow exists yet. The implementation order is intentionally conservative:

1. Strict PSBT v0/v2 parser and read-only transaction review.
2. Full output, fee, change, and network verification UI.
3. Independent test vectors for every parser and hash primitive.
4. Key session and signing design.
5. Signed PSBT export.

## M17 Runtime Facts

- CPU ABI: 32-bit ARM EABI hard-float; dynamic loader `/lib/ld-linux-armhf.so.3`.
- MinUI system path: `/sdcard/.system/m17`.
- Package launcher path: `/sdcard/.system/m17/paks/PocketSigner.pak/launch.sh`.
- The M17 internal boot image is writable; do not treat this device as a secure key vault.