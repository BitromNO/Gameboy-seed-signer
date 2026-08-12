; Minimal Game Boy ROM boot stub for the BTC seed-signer prototype.
; This is a template intended for RGBDS compilation.

INCLUDE "hardware.inc"

SECTION "Header", ROM0[$0100]
    nop
    jp start

SECTION "Program", ROM0[$0150]
start:
    di
    ld sp, $FFFE
    xor a

loop:
    jr loop

; For a real ROM build, use rgbfix to set the title, cartridge type, and checksum:
; rgbasm -o gb_seed_signer_boot.obj gb_seed_signer_boot.asm
; rgblink -o gb_seed_signer_boot.gb gb_seed_signer_boot.obj
; rgbfix -v -f lb -t BTCSEED -m 0x00 -p 0xFF gb_seed_signer_boot.gb
