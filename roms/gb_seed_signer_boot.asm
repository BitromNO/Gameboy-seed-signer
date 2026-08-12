; Working-style Game Boy ROM boot stub for the BTC seed-signer prototype.
; This follows the same basic pattern used by a known-good hello-world ROM.

INCLUDE "hardware.inc"

SECTION "Header", ROM0[$100]
EntryPoint:
    di
    jp Start

REPT $150 - $104
    db 0
ENDR

SECTION "Game code", ROM0

Start:
    ld sp, $FFFE

.waitVBlank:
    ld a, [rLY]
    cp 144
    jr c, .waitVBlank

    xor a
    ld [rLCDC], a

    ld hl, $9800
    ld de, TitleText
.copyString:
    ld a, [de]
    ld [hli], a
    inc de
    and a
    jr nz, .copyString

    ld a, %11100100
    ld [rBGP], a

    xor a
    ld [rSCY], a
    ld [rSCX], a
    ld [rNR52], a

    ld a, %10000001
    ld [rLCDC], a

.lockup:
    jr .lockup

SECTION "Title text", ROM0
TitleText:
    db "BTCSEED", 0

; For a real ROM build, use rgbfix to set the title and cartridge metadata:
; rgbasm -o gb_seed_signer_boot.obj gb_seed_signer_boot.asm
; rgblink -o gb_seed_signer_boot.gb gb_seed_signer_boot.obj
; rgbfix -v -f lb -t BTCSEED -m 0x00 -p 0xFF gb_seed_signer_boot.gb
