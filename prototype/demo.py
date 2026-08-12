#!/usr/bin/env python3

import os
import sys
import time

try:
    from mnemonic import Mnemonic
    from bip32utils import BIP32Key
except ImportError:  # pragma: no cover
    Mnemonic = None
    BIP32Key = None


def clear_screen():
    os.system("cls" if os.name == "nt" else "clear")


def spin_btc_logo(seconds=2.5):
    frames = [
        """
        .--.   .--.
       /    \\ /    \\
      |  B  |  T  |
       \\____/ \\____/
          ||  ||
       .-''--''-. 
      /  BTC  \\
      \\______/ 
        """,
        """
         .--.   .--.
        /    \\ /    \\
       |  T  |  C  |
        \\____/ \\____/
           ||  ||
        .-''--''-. 
       /  BTC  \\
       \\______/ 
        """,
        """
         .--.   .--.
        /    \\ /    \\
       |  C  |  B  |
        \\____/ \\____/
           ||  ||
        .-''--''-. 
       /  BTC  \\
       \\______/ 
        """,
    ]

    end = time.time() + seconds
    idx = 0
    while time.time() < end:
        clear_screen()
        print("\n\n")
        print(frames[idx % len(frames)])
        print("\n  LOADING BITCOIN WORLD...")
        time.sleep(0.22)
        idx += 1


def mario_menu():
    print("""
    +-------------------------------+
    |  MUSHROOM WORLD - 1-1         |
    |  [1] START SIGNER            |
    |  [2] NEW SEED                |
    |  [3] WALLET SUMMARY          |
    |  [4] EXIT                    |
    +-------------------------------+
      ^  ^    1-UP  COIN  COIN
     / \\ /\\
    ( o o )
    /|_ _|\\
      / \\
    """)


def generate_mnemonic():
    if Mnemonic is None:
        return "coin block world demo seed only"
    mnemo = Mnemonic("english")
    words = mnemo.generate(strength=128)
    return words


def derive_wallet(words: str):
    if BIP32Key is None:
        return {
            "mnemonic": words,
            "xpub": "demo-xpub-not-derived",
            "child_xpub": "demo-child-xpub",
            "child_path": "m/84'/1'/0'/0/0",
            "note": "Install requirements.txt to enable real BIP32 signing.",
        }

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
        "note": "Key derivation enabled.",
    }


def show_wallet_summary(words: str):
    wallet = derive_wallet(words)
    clear_screen()
    print("\n  BTC SIGNER - STATUS SCREEN")
    print("=" * 34)
    print("Mnemonic:", words)
    print("xpub:", wallet["xpub"])
    print("child_xpub:", wallet["child_xpub"])
    print("path:", wallet["child_path"])
    print("note:", wallet.get("note", ""))
    print("\nPress Enter to return to the menu...")
    input()


def main():
    spin_btc_logo(2.5)
    while True:
        clear_screen()
        mario_menu()
        choice = input("\nSelect your move: ").strip()

        if choice == "1":
            clear_screen()
            words = generate_mnemonic()
            wallet = derive_wallet(words)
            print("\n=== LEVEL CLEAR! ===")
            print("Mnemonic:", words)
            print("child_path:", wallet["child_path"])
            print("xpub:", wallet["xpub"])
            print("\nPress Enter to continue...")
            input()
        elif choice == "2":
            clear_screen()
            words = generate_mnemonic()
            print("\nNEW SEED GENERATED!")
            print(words)
            print("\nPress Enter to continue...")
            input()
        elif choice == "3":
            words = generate_mnemonic()
            show_wallet_summary(words)
        elif choice in {"4", "q", "Q", "quit", "exit"}:
            clear_screen()
            print("\nThanks for playing!\n")
            sys.exit(0)
        else:
            clear_screen()
            print("\nInvalid move! Try 1, 2, 3, or 4.")
            time.sleep(1)


if __name__ == "__main__":
    main()
