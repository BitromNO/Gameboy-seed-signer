CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic

BUILD_DIR := build
CORE_SRC := src/psbt_inspector.c src/sha256.c
TEST_BIN := $(BUILD_DIR)/test_psbt_inspector

test: $(TEST_BIN)
	$(TEST_BIN)

$(TEST_BIN): tests/test_psbt_inspector.c $(CORE_SRC) src/psbt_inspector.h
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc -o $@ tests/test_psbt_inspector.c $(CORE_SRC)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: test clean