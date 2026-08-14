GBDK_HOME ?= ../.tools/gbdk
LCC := $(GBDK_HOME)/bin/lcc
VERSION ?= v0.3.0
RELEASE_DIR := releases/$(VERSION)

all: pocket_btc.gb

bip39_words.c bip39_words.h: assets/bip39_english.txt tools/generate_bip39_words.py
	python3 tools/generate_bip39_words.py assets/bip39_english.txt bip39_words.c bip39_words.h

repo_qr.c repo_qr.h: tools/generate_repo_qr.py
	python3 tools/generate_repo_qr.py repo_qr.c repo_qr.h

pocket_btc.gb: main.c bip39_words.c bip39_words.h repo_qr.c repo_qr.h
	$(LCC) -Wm-yt0x1B -Wm-yo4 -Wm-ya1 -o $@ main.c bip39_words.c repo_qr.c

release: pocket_btc.gb
	mkdir -p $(RELEASE_DIR)/assets $(RELEASE_DIR)/tools
	cp pocket_btc.gb $(RELEASE_DIR)/pocket_btc.gb
	cp main.c $(RELEASE_DIR)/main.c
	cp Makefile $(RELEASE_DIR)/Makefile
	cp README.md $(RELEASE_DIR)/README.md
	cp requirements-dev.txt $(RELEASE_DIR)/requirements-dev.txt
	cp bip39_words.c bip39_words.h repo_qr.c repo_qr.h $(RELEASE_DIR)/
	cp assets/bip39_english.txt $(RELEASE_DIR)/assets/
	cp tools/generate_bip39_words.py tools/generate_repo_qr.py tools/make_inbox_save.py tools/make_seed_import.py $(RELEASE_DIR)/tools/
	sha256sum $(RELEASE_DIR)/pocket_btc.gb > $(RELEASE_DIR)/SHA256SUMS

clean:
	rm -f pocket_btc.gb pocket_btc.ihx pocket_btc.map pocket_btc.noi pocket_btc.sym bip39_words.c bip39_words.h repo_qr.c repo_qr.h

.PHONY: all clean release