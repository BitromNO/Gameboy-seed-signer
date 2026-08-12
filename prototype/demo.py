#!/usr/bin/env python3

from mnemonic import Mnemonic
from bip32utils import BIP32Key


def generate_mnemonic():
    mnemo = Mnemonic("english")
    words = mnemo.generate(strength=128)
    return words


def derive_wallet(words: str):
    seed = Mnemonic("english").to_seed(words)
    root = BIP32Key.fromEntropy(seed)
    child = root.ChildKey(2147483648 + 84)  # m/84'
    child = child.ChildKey(2147483648 + 1)  # m/84'/1'
    child = child.ChildKey(2147483648 + 0)  # m/84'/1'/0'
    child = child.ChildKey(2147483648 + 0)  # m/84'/1'/0'/0
    child = child.ChildKey(0)                # m/84'/1'/0'/0/0
    return {
        "mnemonic": words,
        "xprv": root.ExtendedKey(),
        "xpub": root.PublicKey().ExtendedKey(),
        "child_xpub": child.PublicKey().ExtendedKey(),
        "child_path": "m/84'/1'/0'/0/0",
    }


if __name__ == "__main__":
    words = generate_mnemonic()
    wallet = derive_wallet(words)
    print("Game Boy Seed Signer Prototype")
    print("Mnemonic:", words)
    print("xpub:", wallet["xpub"])
    print("child_xpub:", wallet["child_xpub"])
    print("path:", wallet["child_path"])
