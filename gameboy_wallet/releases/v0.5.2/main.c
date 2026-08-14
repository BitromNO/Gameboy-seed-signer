#include <gb/gb.h>
#include <gb/hardware.h>
#include <gbdk/console.h>
#include <stdio.h>
#include "bip39_words.h"

typedef enum Screen { SCREEN_MENU, SCREEN_READY, SCREEN_ADDRESS, SCREEN_DICE, SCREEN_WORDS, SCREEN_LOAD_SEED, SCREEN_INBOX, SCREEN_REVIEW, SCREEN_TOOLS, SCREEN_SETTINGS, SCREEN_HODL } Screen;
#define MENU_ITEM_COUNT 10u
#define NOTE_REST 0u
#define SRAM_BASE ((volatile uint8_t *)0xA000u)
#define INBOX_MAX_BYTES 512u
#define DICE_TARGET 99u

static const char *const menu_items[MENU_ITEM_COUNT] = { "READY TO SIGN", "ADDR EXPLORER", "DICE AUDIT", "SEED WORDS", "LOAD SEED", "FILE MGMT", "REVIEW TX", "ADV TOOLS", "SETTINGS", "HODL MODE" };
static const Screen menu_screens[MENU_ITEM_COUNT] = { SCREEN_READY, SCREEN_ADDRESS, SCREEN_DICE, SCREEN_WORDS, SCREEN_LOAD_SEED, SCREEN_INBOX, SCREEN_REVIEW, SCREEN_TOOLS, SCREEN_SETTINGS, SCREEN_HODL };
static const uint16_t coin_frames[][16] = {
  { 0x03C0u, 0x0FF0u, 0x1FF8u, 0x3FFCu, 0x7FFEu, 0x7FFEu, 0x7FFEu, 0x7FFEu, 0x7FFEu, 0x7FFEu, 0x7FFEu, 0x7FFEu, 0x3FFCu, 0x1FF8u, 0x0FF0u, 0x03C0u },
  { 0x0180u, 0x03C0u, 0x03C0u, 0x03C0u, 0x03C0u, 0x07E0u, 0x07E0u, 0x07E0u, 0x07E0u, 0x07E0u, 0x07E0u, 0x03C0u, 0x03C0u, 0x03C0u, 0x03C0u, 0x0180u },
  { 0x03C0u, 0x0FF0u, 0x1FF8u, 0x3FFCu, 0x7FFEu, 0x7E7Eu, 0x7E7Eu, 0x7E7Eu, 0x7E7Eu, 0x7E7Eu, 0x7E7Eu, 0x7FFEu, 0x3FFCu, 0x1FF8u, 0x0FF0u, 0x03C0u },
  { 0x0180u, 0x03C0u, 0x03C0u, 0x03C0u, 0x07E0u, 0x0FF0u, 0x0FF0u, 0x0FF0u, 0x0FF0u, 0x0FF0u, 0x0FF0u, 0x07E0u, 0x03C0u, 0x03C0u, 0x03C0u, 0x0180u }
};
static const unsigned char coin_tiles[] = { 128, 129, 130, 131 };
static const unsigned char blank_line[20] = { 0 };
static unsigned char coin_vram[64];
static uint8_t selected_item;
static uint8_t dice_value = 1u, dice_count, dice_page, mnemonic_page, seed_session_loaded;
static uint16_t dice_audit = 0xA55Au;
static uint8_t dice_rolls[DICE_TARGET];
static uint8_t dice_entropy[32];
static uint16_t mnemonic_words[24];
static Screen active_screen = SCREEN_MENU;

static void print_centered(uint8_t y, const char *text) {
  uint8_t length = 0; const char *cursor = text;
  while (*cursor++) length++;
  gotoxy((20u - length) / 2u, y); printf("%s", text);
}

static void sound_init(void) { NR52_REG = 0x80u; NR50_REG = 0x77u; NR51_REG = 0x11u; NR10_REG = 0x00u; NR11_REG = 0x80u; NR12_REG = 0x00u; }
static void play_confirm_sound(void) {
  NR12_REG = 0x61u;
  NR13_REG = 0xC0u;
  NR14_REG = 0x87u;
}
static void print_u8(uint8_t value) {
  if (value >= 100u) { printf("%c", (char)('0' + value / 100u)); value %= 100u; printf("%c", (char)('0' + value / 10u)); }
  else if (value >= 10u) printf("%c", (char)('0' + value / 10u));
  if (value >= 10u) value %= 10u;
  printf("%c", (char)('0' + value));
}
static void print_hex16(uint16_t value) {
  static const char digits[] = "0123456789ABCDEF";
  printf("%c", digits[(value >> 12) & 0x0Fu]);
  printf("%c", digits[(value >> 8) & 0x0Fu]);
  printf("%c", digits[(value >> 4) & 0x0Fu]);
  printf("%c", digits[value & 0x0Fu]);
}
static void print_hex8(uint8_t value) {
  static const char digits[] = "0123456789ABCDEF";
  printf("%c", digits[value >> 4]);
  printf("%c", digits[value & 0x0Fu]);
}
static void wipe_dice_material(void) {
  uint8_t index;
  for (index = 0; index != DICE_TARGET; index++) dice_rolls[index] = 0u;
  for (index = 0; index != 32u; index++) dice_entropy[index] = 0u;
  dice_count = 0u;
  dice_page = 0u;
  mnemonic_page = 0u;
  seed_session_loaded = 0u;
  dice_audit = 0xA55Au;
}
static void draw_coin_frame(uint8_t frame, uint8_t x, uint8_t y) {
  uint8_t row;
  uint16_t bitmap;

  for (row = 0; row != 16u; row++) {
    bitmap = coin_frames[frame][row];
    coin_vram[(row < 8u ? 0u : 2u) * 16u + (row & 7u) * 2u] = (uint8_t)(bitmap >> 8);
    coin_vram[(row < 8u ? 1u : 3u) * 16u + (row & 7u) * 2u] = (uint8_t)bitmap;
  }
  set_bkg_data(128, 4, coin_vram);
  set_bkg_tiles(x, y, 2, 2, coin_tiles);
}
static uint32_t rotr(uint32_t value, uint8_t count) { return (value >> count) | (value << (32u - count)); }
static void sha256_hash(const uint8_t *input, uint8_t input_length, uint8_t *digest) {
  static const uint32_t initial_state[8] = { 0x6A09E667UL, 0xBB67AE85UL, 0x3C6EF372UL, 0xA54FF53AUL, 0x510E527FUL, 0x9B05688CUL, 0x1F83D9ABUL, 0x5BE0CD19UL };
  static const uint32_t round_constants[64] = {
    0x428A2F98UL,0x71374491UL,0xB5C0FBCFUL,0xE9B5DBA5UL,0x3956C25BUL,0x59F111F1UL,0x923F82A4UL,0xAB1C5ED5UL,0xD807AA98UL,0x12835B01UL,0x243185BEUL,0x550C7DC3UL,0x72BE5D74UL,0x80DEB1FEUL,0x9BDC06A7UL,0xC19BF174UL,
    0xE49B69C1UL,0xEFBE4786UL,0x0FC19DC6UL,0x240CA1CCUL,0x2DE92C6FUL,0x4A7484AAUL,0x5CB0A9DCUL,0x76F988DAUL,0x983E5152UL,0xA831C66DUL,0xB00327C8UL,0xBF597FC7UL,0xC6E00BF3UL,0xD5A79147UL,0x06CA6351UL,0x14292967UL,
    0x27B70A85UL,0x2E1B2138UL,0x4D2C6DFCUL,0x53380D13UL,0x650A7354UL,0x766A0ABBUL,0x81C2C92EUL,0x92722C85UL,0xA2BFE8A1UL,0xA81A664BUL,0xC24B8B70UL,0xC76C51A3UL,0xD192E819UL,0xD6990624UL,0xF40E3585UL,0x106AA070UL,
    0x19A4C116UL,0x1E376C08UL,0x2748774CUL,0x34B0BCB5UL,0x391C0CB3UL,0x4ED8AA4AUL,0x5B9CCA4FUL,0x682E6FF3UL,0x748F82EEUL,0x78A5636FUL,0x84C87814UL,0x8CC70208UL,0x90BEFFFAUL,0xA4506CEBUL,0xBEF9A3F7UL,0xC67178F2UL
  };
  uint8_t block[128];
  uint32_t state[8], working[8], words[64];
  uint8_t index, round, block_index, block_count;
  uint16_t pad_index, block_length;

  block_count = input_length > 55u ? 2u : 1u;
  block_length = (uint16_t)block_count * 64u;
  for (pad_index = 0; pad_index != block_length; pad_index++) block[pad_index] = 0u;
  for (index = 0; index != input_length; index++) block[index] = input[index];
  block[input_length] = 0x80u;
  block[(uint16_t)(block_length - 2u)] = (uint8_t)((uint16_t)input_length * 8u >> 8);
  block[(uint16_t)(block_length - 1u)] = (uint8_t)((uint16_t)input_length * 8u);
  for (index = 0; index != 8u; index++) state[index] = initial_state[index];
  for (block_index = 0; block_index != block_count; block_index++) {
    for (index = 0; index != 16u; index++) {
      uint8_t offset = (uint8_t)(block_index * 64u + index * 4u);
      words[index] = ((uint32_t)block[offset] << 24) | ((uint32_t)block[(uint8_t)(offset + 1u)] << 16) | ((uint32_t)block[(uint8_t)(offset + 2u)] << 8) | block[(uint8_t)(offset + 3u)];
    }
    for (index = 16u; index != 64u; index++) words[index] = (rotr(words[index - 2u], 17u) ^ rotr(words[index - 2u], 19u) ^ (words[index - 2u] >> 10u)) + words[index - 7u] + (rotr(words[index - 15u], 7u) ^ rotr(words[index - 15u], 18u) ^ (words[index - 15u] >> 3u)) + words[index - 16u];
    for (index = 0; index != 8u; index++) working[index] = state[index];
    for (round = 0; round != 64u; round++) {
      uint32_t choice = (working[4] & working[5]) ^ ((~working[4]) & working[6]);
      uint32_t majority = (working[0] & working[1]) ^ (working[0] & working[2]) ^ (working[1] & working[2]);
      uint32_t next_one = working[7] + (rotr(working[4], 6u) ^ rotr(working[4], 11u) ^ rotr(working[4], 25u)) + choice + round_constants[round] + words[round];
      uint32_t next_two = (rotr(working[0], 2u) ^ rotr(working[0], 13u) ^ rotr(working[0], 22u)) + majority;
      working[7] = working[6]; working[6] = working[5]; working[5] = working[4]; working[4] = working[3] + next_one; working[3] = working[2]; working[2] = working[1]; working[1] = working[0]; working[0] = next_one + next_two;
    }
    for (index = 0; index != 8u; index++) state[index] += working[index];
  }
  for (index = 0; index != 8u; index++) { digest[index * 4u] = (uint8_t)(state[index] >> 24); digest[index * 4u + 1u] = (uint8_t)(state[index] >> 16); digest[index * 4u + 2u] = (uint8_t)(state[index] >> 8); digest[index * 4u + 3u] = (uint8_t)state[index]; }
}
static const char *bip39_word_at(uint16_t word_index) {
  const char *word = bip39_words;
  while (word_index != 0u) { while (*word++) { } word_index--; }
  return word;
}
static void make_mnemonic_words(void) {
  uint8_t checksum[32];
  uint8_t word_index, bit_index;
  uint16_t all_bit_index, value;

  sha256_hash(dice_entropy, 32u, checksum);
  for (word_index = 0; word_index != 24u; word_index++) {
    value = 0u;
    for (bit_index = 0; bit_index != 11u; bit_index++) {
      all_bit_index = (uint16_t)word_index * 11u + bit_index;
      value <<= 1;
      if (all_bit_index < 256u) value |= (uint16_t)((dice_entropy[all_bit_index >> 3] >> (7u - (all_bit_index & 7u))) & 1u);
      else value |= (uint16_t)((checksum[0] >> (7u - (all_bit_index & 7u))) & 1u);
    }
    mnemonic_words[word_index] = value;
  }
}
static uint16_t inbox_checksum(void) {
  uint16_t checksum = 0xFFFFu;
  uint16_t index;
  uint16_t length;

  ENABLE_RAM_MBC5;
  SWITCH_RAM_MBC5(0);
  length = (uint16_t)SRAM_BASE[6] | ((uint16_t)SRAM_BASE[7] << 8);
  if (length > INBOX_MAX_BYTES) return 0;
  for (index = 0; index != 8u; index++) checksum = (uint16_t)(checksum ^ SRAM_BASE[index]);
  for (index = 0; index != length; index++) checksum = (uint16_t)(checksum ^ SRAM_BASE[10u + index]);
  return checksum;
}
static uint8_t inbox_is_valid(void) {
  uint16_t stored;
  uint16_t calculated;

  ENABLE_RAM_MBC5;
  SWITCH_RAM_MBC5(0);
  if (SRAM_BASE[0] != 'P' || SRAM_BASE[1] != 'B' || SRAM_BASE[2] != 'T' || SRAM_BASE[3] != '1' || SRAM_BASE[4] != 1u || SRAM_BASE[5] != 1u) return 0;
  stored = (uint16_t)SRAM_BASE[8] | ((uint16_t)SRAM_BASE[9] << 8);
  calculated = inbox_checksum();
  return stored == calculated;
}
static uint8_t seed_import_is_valid(void) {
  uint16_t checksum = 0xFFFFu;
  uint16_t stored;
  uint8_t index;

  ENABLE_RAM_MBC5;
  SWITCH_RAM_MBC5(0);
  if (SRAM_BASE[0] != 'P' || SRAM_BASE[1] != 'B' || SRAM_BASE[2] != 'S' || SRAM_BASE[3] != '1' || SRAM_BASE[4] != 1u || SRAM_BASE[5] != 2u || SRAM_BASE[6] != 32u || SRAM_BASE[7] != 0u) return 0;
  for (index = 0; index != 8u; index++) checksum = (uint16_t)(checksum ^ SRAM_BASE[index]);
  for (index = 0; index != 32u; index++) checksum = (uint16_t)(checksum ^ SRAM_BASE[10u + index]);
  stored = (uint16_t)SRAM_BASE[8] | ((uint16_t)SRAM_BASE[9] << 8);
  return checksum == stored;
}
static void load_seed_session(void) {
  uint8_t index;
  if (!seed_import_is_valid()) return;
  for (index = 0; index != 32u; index++) dice_entropy[index] = SRAM_BASE[10u + index];
  make_mnemonic_words();
  seed_session_loaded = 1u;
}
static void clear_screen(void) {
  uint8_t row;
  for (row = 0; row != 18u; row++) set_bkg_tiles(0, row, 20, 1, blank_line);
}
static void draw_frame(const char *title) { clear_screen(); gotoxy(0, 0); printf("+------------------+"); print_centered(1, title); gotoxy(0, 2); printf("+------------------+"); }
static void draw_menu_header(void) {
  draw_coin_frame(0, 1, 1);
  gotoxy(5, 1); printf("POCKET BTC");
  gotoxy(4, 2); printf("MAINNET OFFLINE");
  gotoxy(0, 3); printf("====================");
}
static void draw_menu(void) {
  uint8_t index;
  clear_screen(); draw_menu_header();
  for (index = 0; index != MENU_ITEM_COUNT; index++) {
    uint8_t column; const char *label = menu_items[index];
    gotoxy(0, (uint8_t)(5u + index)); printf("%c ", index == selected_item ? '>' : ' ');
    for (column = 0; label[column] && column != 15u; column++) printf("%c", label[column]);
    while (column++ != 15u) printf(" ");
    printf("%c", index == selected_item ? '<' : ' ');
  }
  gotoxy(0, 15); printf("--------------------");
  gotoxy(1, 16); printf("D-PAD MOVE  A OPEN");
  gotoxy(1, 17); printf("LOAD SEED: .SAV");
}
static void draw_active_screen(void) {
  if (active_screen == SCREEN_MENU) { draw_menu(); return; }
  if (active_screen == SCREEN_READY) {
    draw_frame("READY TO SIGN");
    if (inbox_is_valid()) { print_centered(4, "PACKAGE LOADED"); print_centered(6, "REVIEW REQUIRED"); print_centered(8, "SIGNER NOT BUILT"); }
    else { print_centered(4, "NO PACKAGE LOADED"); print_centered(6, "USE FILE MANAGEMENT"); }
  }
  else if (active_screen == SCREEN_ADDRESS) { draw_frame("ADDRESS EXPLORER"); print_centered(4, "NO WALLET LOADED"); print_centered(6, "ADDRESS DERIVATION"); print_centered(8, "NOT BUILT YET"); print_centered(10, "NO KEYS STORED"); }
  else if (active_screen == SCREEN_DICE) {
    draw_frame("DICE AUDIT");
    gotoxy(3, 4); printf("ROLL: "); print_u8(dice_value);
    gotoxy(3, 6); printf("COUNT: "); print_u8(dice_count); printf("/99");
    if (dice_count >= DICE_TARGET) {
      uint8_t offset = (uint8_t)(dice_page * 8u);
      gotoxy(3, 8); printf("BACKUP PAGE "); print_u8((uint8_t)(dice_page + 1u)); printf("/4");
      gotoxy(3, 10); print_hex8(dice_entropy[offset]); printf(" "); print_hex8(dice_entropy[(uint8_t)(offset + 1u)]); printf(" "); print_hex8(dice_entropy[(uint8_t)(offset + 2u)]); printf(" "); print_hex8(dice_entropy[(uint8_t)(offset + 3u)]);
      gotoxy(3, 12); print_hex8(dice_entropy[(uint8_t)(offset + 4u)]); printf(" "); print_hex8(dice_entropy[(uint8_t)(offset + 5u)]); printf(" "); print_hex8(dice_entropy[(uint8_t)(offset + 6u)]); printf(" "); print_hex8(dice_entropy[(uint8_t)(offset + 7u)]);
      print_centered(14, "A:VIEW WORDS"); print_centered(16, "B:WIPE AND EXIT");
    } else {
      gotoxy(3, 8); printf("AUDIT: "); print_hex16(dice_audit);
      print_centered(11, "PRIVATE D6 ROLLS ONLY"); print_centered(14, "UP/DN:VALUE A:ADD"); print_centered(16, "B:BACK NO SEED YET");
    }
    return;
  }
  else if (active_screen == SCREEN_WORDS) {
    uint8_t index, word_number = (uint8_t)(mnemonic_page * 6u);
    draw_frame("SEED WORDS");
    if (!seed_session_loaded) { print_centered(5, "NO SEED SESSION"); print_centered(7, "GENERATE OR IMPORT"); print_centered(9, "A VALID 24-WORD"); print_centered(11, "BIP39 PHRASE"); }
    else {
      gotoxy(1, 3); printf("PAGE "); print_u8((uint8_t)(mnemonic_page + 1u)); printf("/4");
      for (index = 0; index != 6u; index++) {
        const char *word = bip39_word_at(mnemonic_words[(uint8_t)(word_number + index)]);
        gotoxy(1, (uint8_t)(5u + index * 2u)); print_u8((uint8_t)(word_number + index + 1u)); printf(": "); printf("%s", word);
      }
      print_centered(17, "UP/DN PAGE B:EXIT");
    }
    return;
  }
  else if (active_screen == SCREEN_LOAD_SEED) {
    draw_frame("LOAD SEED");
    if (seed_import_is_valid()) {
      print_centered(4, "VALID 24-WORD SEED");
      print_centered(6, "FOUND IN .SAV");
      print_centered(9, "A:LOAD TO RAM");
      print_centered(11, "B:BACK");
    } else {
      print_centered(4, "NO VALID SEED FILE");
      print_centered(6, "CREATE POCKET_SEED");
      print_centered(8, "WITH PC HELPER");
      print_centered(11, "B:BACK");
    }
    return;
  }
  else if (active_screen == SCREEN_INBOX) {
    draw_frame("FILE MANAGEMENT");
    if (inbox_is_valid()) { uint16_t payload_length = (uint16_t)SRAM_BASE[6] | ((uint16_t)SRAM_BASE[7] << 8); print_centered(4, "PACKAGE VERIFIED"); print_centered(6, "SRAM INBOX READY"); print_centered(8, "PAYLOAD BYTES:"); gotoxy(9, 10); print_u8((uint8_t)payload_length); }
    else { print_centered(4, "NO VALID PACKAGE"); print_centered(6, "COPY .SAV FROM PC"); print_centered(8, "THEN RESTART ROM"); }
  }
  else if (active_screen == SCREEN_REVIEW) { draw_frame("REVIEW TX"); print_centered(4, "PARSER NOT BUILT"); print_centered(6, "INBOX IS CHECKED"); print_centered(8, "PSBT REVIEW NEXT"); print_centered(10, "NO SIGNING YET"); }
  else if (active_screen == SCREEN_TOOLS) { draw_frame("ADVANCED TOOLS"); print_centered(4, "SELFTEST: PASS"); print_centered(6, "SRAM: AVAILABLE"); print_centered(8, "ROM: MBC5 + BATT"); print_centered(10, "FW: POCKET BTC 0.2"); }
  else if (active_screen == SCREEN_HODL) { draw_frame("HODL MODE"); print_centered(4, "VOLATILITY ALERT"); print_centered(6, "KEEP CALM"); print_centered(8, "HOLD THE LINE"); print_centered(10, "TICK TOCK NEXT"); print_centered(13, "[##########] 100%"); }
  else { draw_frame("SETTINGS"); print_centered(4, "NETWORK: MAINNET"); print_centered(6, "SIGNER: DISABLED"); print_centered(8, "SOUND: ACTION PING"); print_centered(10, "DEMO BUILD 0.3"); print_centered(16, "B:BACK"); return; }
  print_centered(16, "B:BACK");
}
static void show_boot_logo(void) {
  uint8_t tick;
  clear_screen(); print_centered(2, "POCKET BTC"); print_centered(4, "OFFLINE MAINNET"); print_centered(12, "LOADING BLOCKS...");
  for (tick = 0; tick != 20u; tick++) { draw_coin_frame((uint8_t)((tick / 5u) & 0x03u), 9, 7); wait_vbl_done(); delay(250u); }
}
void main(void) {
  uint8_t previous_keys = 0, keys;
  SHOW_BKG; sound_init(); show_boot_logo(); draw_menu();
  while (1) {
    keys = joypad();
    if (!(previous_keys & J_UP) && (keys & J_UP) && active_screen == SCREEN_MENU) { selected_item = selected_item == 0 ? MENU_ITEM_COUNT - 1u : selected_item - 1u; draw_menu(); }
    if (!(previous_keys & J_DOWN) && (keys & J_DOWN) && active_screen == SCREEN_MENU) { selected_item = selected_item == MENU_ITEM_COUNT - 1u ? 0 : selected_item + 1u; draw_menu(); }
    if (!(previous_keys & J_UP) && (keys & J_UP) && active_screen == SCREEN_DICE) { dice_value = dice_value == 6u ? 1u : dice_value + 1u; draw_active_screen(); }
    if (!(previous_keys & J_DOWN) && (keys & J_DOWN) && active_screen == SCREEN_DICE) { dice_value = dice_value == 1u ? 6u : dice_value - 1u; draw_active_screen(); }
    if (!(previous_keys & J_UP) && (keys & J_UP) && active_screen == SCREEN_WORDS && seed_session_loaded) { mnemonic_page = mnemonic_page == 0u ? 3u : mnemonic_page - 1u; draw_active_screen(); }
    if (!(previous_keys & J_DOWN) && (keys & J_DOWN) && active_screen == SCREEN_WORDS && seed_session_loaded) { mnemonic_page = mnemonic_page == 3u ? 0u : mnemonic_page + 1u; draw_active_screen(); }
    if (!(previous_keys & J_A) && (keys & J_A) && active_screen == SCREEN_MENU) { play_confirm_sound(); active_screen = menu_screens[selected_item]; draw_active_screen(); }
    if (!(previous_keys & J_A) && (keys & J_A) && active_screen == SCREEN_DICE && dice_count != DICE_TARGET) { play_confirm_sound(); dice_rolls[dice_count] = (uint8_t)('0' + dice_value); dice_audit = (uint16_t)(((dice_audit << 5) | (dice_audit >> 11)) ^ dice_value); dice_count++; if (dice_count == DICE_TARGET) { sha256_hash(dice_rolls, DICE_TARGET, dice_entropy); make_mnemonic_words(); seed_session_loaded = 1u; } draw_active_screen(); }
    if (!(previous_keys & J_A) && (keys & J_A) && active_screen == SCREEN_DICE && dice_count == DICE_TARGET) { play_confirm_sound(); active_screen = SCREEN_WORDS; draw_active_screen(); }
    if (!(previous_keys & J_A) && (keys & J_A) && active_screen == SCREEN_LOAD_SEED && seed_import_is_valid()) { play_confirm_sound(); load_seed_session(); active_screen = SCREEN_WORDS; draw_active_screen(); }
    if (!(previous_keys & J_B) && (keys & J_B) && (active_screen == SCREEN_DICE || active_screen == SCREEN_WORDS)) { wipe_dice_material(); active_screen = SCREEN_MENU; draw_menu(); }
    else if (!(previous_keys & J_B) && (keys & J_B) && active_screen != SCREEN_MENU) { active_screen = SCREEN_MENU; draw_menu(); }
    previous_keys = keys; wait_vbl_done();
  }
}